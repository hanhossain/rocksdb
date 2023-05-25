//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "options/cf_options.h"

#include <cassert>
#include <cinttypes>
#include <limits>
#include <string>

#include "logging/logging.h"
#include "options/configurable_helper.h"
#include "options/db_options.h"
#include "options/options_helper.h"
#include "options/options_parser.h"
#include "port/port.h"
#include "rocksdb/advanced_cache.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/concurrent_task_limiter.h"
#include "rocksdb/configurable.h"
#include "rocksdb/convenience.h"
#include "rocksdb/env.h"
#include "rocksdb/file_system.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"
#include "rocksdb/utilities/object_registry.h"
#include "rocksdb/utilities/options_type.h"
#include "util/cast_util.h"

// NOTE: in this file, many option flags that were deprecated
// and removed from the rest of the code have to be kept here
// and marked as kDeprecated in order to be able to read old
// OPTIONS files.

namespace rocksdb {

static Status ParseCompressionOptions(const std::string& value,
                                      const std::string& name,
                                      CompressionOptions& compression_opts) {
  const char kDelimiter = ':';
  std::istringstream field_stream(value);
  std::string field;

  if (!std::getline(field_stream, field, kDelimiter)) {
    return Status::InvalidArgument("unable to parse the specified CF option " +
                                   name);
  }
  compression_opts.window_bits = ParseInt(field);

  if (!std::getline(field_stream, field, kDelimiter)) {
    return Status::InvalidArgument("unable to parse the specified CF option " +
                                   name);
  }
  compression_opts.level = ParseInt(field);

  if (!std::getline(field_stream, field, kDelimiter)) {
    return Status::InvalidArgument("unable to parse the specified CF option " +
                                   name);
  }
  compression_opts.strategy = ParseInt(field);

  // max_dict_bytes is optional for backwards compatibility
  if (!field_stream.eof()) {
    if (!std::getline(field_stream, field, kDelimiter)) {
      return Status::InvalidArgument(
          "unable to parse the specified CF option " + name);
    }
    compression_opts.max_dict_bytes = ParseInt(field);
  }

  // zstd_max_train_bytes is optional for backwards compatibility
  if (!field_stream.eof()) {
    if (!std::getline(field_stream, field, kDelimiter)) {
      return Status::InvalidArgument(
          "unable to parse the specified CF option " + name);
    }
    compression_opts.zstd_max_train_bytes = ParseInt(field);
  }

  // parallel_threads is optional for backwards compatibility
  if (!field_stream.eof()) {
    if (!std::getline(field_stream, field, kDelimiter)) {
      return Status::InvalidArgument(
          "unable to parse the specified CF option " + name);
    }
    // Since parallel_threads comes before enabled but was added optionally
    // later, we need to check if this is the final token (meaning it is the
    // enabled bit), or if there are more tokens (meaning this one is
    // parallel_threads).
    if (!field_stream.eof()) {
      compression_opts.parallel_threads = ParseInt(field);
    } else {
      // parallel_threads is not serialized with this format, but enabled is
      compression_opts.enabled = ParseBoolean("", field);
    }
  }

  // enabled is optional for backwards compatibility
  if (!field_stream.eof()) {
    if (!std::getline(field_stream, field, kDelimiter)) {
      return Status::InvalidArgument(
          "unable to parse the specified CF option " + name);
    }
    compression_opts.enabled = ParseBoolean("", field);
  }

  // max_dict_buffer_bytes is optional for backwards compatibility
  if (!field_stream.eof()) {
    if (!std::getline(field_stream, field, kDelimiter)) {
      return Status::InvalidArgument(
          "unable to parse the specified CF option " + name);
    }
    compression_opts.max_dict_buffer_bytes = ParseUint64(field);
  }

  // use_zstd_dict_trainer is optional for backwards compatibility
  if (!field_stream.eof()) {
    if (!std::getline(field_stream, field, kDelimiter)) {
      return Status::InvalidArgument(
          "unable to parse the specified CF option " + name);
    }
    compression_opts.use_zstd_dict_trainer = ParseBoolean("", field);
  }

  if (!field_stream.eof()) {
    return Status::InvalidArgument("unable to parse the specified CF option " +
                                   name);
  }
  return Status::OK();
}

const std::string kOptNameBMCompOpts = "bottommost_compression_opts";
const std::string kOptNameCompOpts = "compression_opts";

// OptionTypeInfo map for CompressionOptions
static std::unordered_map<std::string, OptionTypeInfo>
    compression_options_type_info = {
        {"window_bits",
         {offsetof(struct CompressionOptions, window_bits), rs::options_type::OptionType::Int,
          rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable}},
        {"level",
         {offsetof(struct CompressionOptions, level), rs::options_type::OptionType::Int,
          rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable}},
        {"strategy",
         {offsetof(struct CompressionOptions, strategy), rs::options_type::OptionType::Int,
          rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable}},
        {"max_compressed_bytes_per_kb",
         {offsetof(struct CompressionOptions, max_compressed_bytes_per_kb),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_dict_bytes",
         {offsetof(struct CompressionOptions, max_dict_bytes), rs::options_type::OptionType::Int,
          rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable}},
        {"zstd_max_train_bytes",
         {offsetof(struct CompressionOptions, zstd_max_train_bytes),
          rs::options_type::OptionType::UInt32T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"parallel_threads",
         {offsetof(struct CompressionOptions, parallel_threads),
          rs::options_type::OptionType::UInt32T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"enabled",
         {offsetof(struct CompressionOptions, enabled), rs::options_type::OptionType::Boolean,
          rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable}},
        {"max_dict_buffer_bytes",
         {offsetof(struct CompressionOptions, max_dict_buffer_bytes),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"use_zstd_dict_trainer",
         {offsetof(struct CompressionOptions, use_zstd_dict_trainer),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
};

static std::unordered_map<std::string, OptionTypeInfo>
    fifo_compaction_options_type_info = {
        {"max_table_files_size",
         {offsetof(struct rs::advanced_options::CompactionOptionsFIFO, max_table_files_size),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"age_for_warm",
         {offsetof(struct rs::advanced_options::CompactionOptionsFIFO, age_for_warm),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"ttl",
         {0, rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::None}},
        {"allow_compaction",
         {offsetof(struct rs::advanced_options::CompactionOptionsFIFO, allow_compaction),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
};

static std::unordered_map<std::string, OptionTypeInfo>
    universal_compaction_options_type_info = {
        {"size_ratio",
         {offsetof(struct rs::universal_compaction::CompactionOptionsUniversal, size_ratio),
          rs::options_type::OptionType::UInt, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"min_merge_width",
         {offsetof(struct rs::universal_compaction::CompactionOptionsUniversal, min_merge_width),
          rs::options_type::OptionType::UInt, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_merge_width",
         {offsetof(struct rs::universal_compaction::CompactionOptionsUniversal, max_merge_width),
          rs::options_type::OptionType::UInt, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_size_amplification_percent",
         {offsetof(struct rs::universal_compaction::CompactionOptionsUniversal,
                   max_size_amplification_percent),
          rs::options_type::OptionType::UInt, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"compression_size_percent",
         {offsetof(struct rs::universal_compaction::CompactionOptionsUniversal, compression_size_percent),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"stop_style",
         {offsetof(struct rs::universal_compaction::CompactionOptionsUniversal, stop_style),
          rs::options_type::OptionType::CompactionStopStyle, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"incremental",
         {offsetof(struct rs::universal_compaction::CompactionOptionsUniversal, incremental),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"allow_trivial_move",
         {offsetof(struct rs::universal_compaction::CompactionOptionsUniversal, allow_trivial_move),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}}};

static std::unordered_map<std::string, OptionTypeInfo>
    cf_mutable_options_type_info = {
        {"report_bg_io_stats",
         {offsetof(struct MutableCFOptions, report_bg_io_stats),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"disable_auto_compactions",
         {offsetof(struct MutableCFOptions, disable_auto_compactions),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"filter_deletes",
         {0, rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"check_flush_compaction_key_order",
         {offsetof(struct MutableCFOptions, check_flush_compaction_key_order),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"paranoid_file_checks",
         {offsetof(struct MutableCFOptions, paranoid_file_checks),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"verify_checksums_in_compaction",
         {0, rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"soft_pending_compaction_bytes_limit",
         {offsetof(struct MutableCFOptions,
                   soft_pending_compaction_bytes_limit),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"hard_pending_compaction_bytes_limit",
         {offsetof(struct MutableCFOptions,
                   hard_pending_compaction_bytes_limit),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"hard_rate_limit",
         {0, rs::options_type::OptionType::Double, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"soft_rate_limit",
         {0, rs::options_type::OptionType::Double, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_compaction_bytes",
         {offsetof(struct MutableCFOptions, max_compaction_bytes),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"ignore_max_compaction_bytes_for_input",
         {offsetof(struct MutableCFOptions,
                   ignore_max_compaction_bytes_for_input),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"expanded_compaction_factor",
         {0, rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"level0_file_num_compaction_trigger",
         {offsetof(struct MutableCFOptions, level0_file_num_compaction_trigger),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"level0_slowdown_writes_trigger",
         {offsetof(struct MutableCFOptions, level0_slowdown_writes_trigger),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"level0_stop_writes_trigger",
         {offsetof(struct MutableCFOptions, level0_stop_writes_trigger),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_grandparent_overlap_factor",
         {0, rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_write_buffer_number",
         {offsetof(struct MutableCFOptions, max_write_buffer_number),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"source_compaction_factor",
         {0, rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"target_file_size_multiplier",
         {offsetof(struct MutableCFOptions, target_file_size_multiplier),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"arena_block_size",
         {offsetof(struct MutableCFOptions, arena_block_size),
          rs::options_type::OptionType::SizeT, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"inplace_update_num_locks",
         {offsetof(struct MutableCFOptions, inplace_update_num_locks),
          rs::options_type::OptionType::SizeT, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_successive_merges",
         {offsetof(struct MutableCFOptions, max_successive_merges),
          rs::options_type::OptionType::SizeT, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"memtable_huge_page_size",
         {offsetof(struct MutableCFOptions, memtable_huge_page_size),
          rs::options_type::OptionType::SizeT, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"memtable_prefix_bloom_huge_page_tlb_size",
         {0, rs::options_type::OptionType::SizeT, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"write_buffer_size",
         {offsetof(struct MutableCFOptions, write_buffer_size),
          rs::options_type::OptionType::SizeT, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"memtable_prefix_bloom_bits",
         {0, rs::options_type::OptionType::UInt32T, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"memtable_prefix_bloom_size_ratio",
         {offsetof(struct MutableCFOptions, memtable_prefix_bloom_size_ratio),
          rs::options_type::OptionType::Double, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"memtable_prefix_bloom_probes",
         {0, rs::options_type::OptionType::UInt32T, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"memtable_whole_key_filtering",
         {offsetof(struct MutableCFOptions, memtable_whole_key_filtering),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"min_partial_merge_operands",
         {0, rs::options_type::OptionType::UInt32T, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_bytes_for_level_base",
         {offsetof(struct MutableCFOptions, max_bytes_for_level_base),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"snap_refresh_nanos",
         {0, rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_bytes_for_level_multiplier",
         {offsetof(struct MutableCFOptions, max_bytes_for_level_multiplier),
          rs::options_type::OptionType::Double, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"max_bytes_for_level_multiplier_additional",
         OptionTypeInfo::Vector<int>(
             offsetof(struct MutableCFOptions,
                      max_bytes_for_level_multiplier_additional),
             rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable,
             {0, rs::options_type::OptionType::Int})},
        {"max_sequential_skip_in_iterations",
         {offsetof(struct MutableCFOptions, max_sequential_skip_in_iterations),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"target_file_size_base",
         {offsetof(struct MutableCFOptions, target_file_size_base),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"compression",
         {offsetof(struct MutableCFOptions, compression),
          rs::options_type::OptionType::CompressionType, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"prefix_extractor",
         OptionTypeInfo::AsCustomSharedPtr<const SliceTransform>(
             offsetof(struct MutableCFOptions, prefix_extractor),
             rs::options_type::OptionVerificationType::ByNameAllowNull,
             (rs::options_type::OptionTypeFlags::Mutable | rs::options_type::OptionTypeFlags::AllowNull))},
        {"compaction_options_fifo",
         OptionTypeInfo::Struct(
             "compaction_options_fifo", &fifo_compaction_options_type_info,
             offsetof(struct MutableCFOptions, compaction_options_fifo),
             rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable)
             .SetParseFunc([](const ConfigOptions& opts,
                              const std::string& name, const std::string& value,
                              void* addr) {
               // This is to handle backward compatibility, where
               // compaction_options_fifo could be assigned a single scalar
               // value, say, like "23", which would be assigned to
               // max_table_files_size.
               if (name == "compaction_options_fifo" &&
                   value.find("=") == std::string::npos) {
                 // Old format. Parse just a single uint64_t value.
                 auto options = static_cast<rs::advanced_options::CompactionOptionsFIFO*>(addr);
                 options->max_table_files_size = ParseUint64(value);
                 return Status::OK();
               } else {
                 return OptionTypeInfo::ParseStruct(
                     opts, "compaction_options_fifo",
                     &fifo_compaction_options_type_info, name, value, addr);
               }
             })},
        {"compaction_options_universal",
         OptionTypeInfo::Struct(
             "compaction_options_universal",
             &universal_compaction_options_type_info,
             offsetof(struct MutableCFOptions, compaction_options_universal),
             rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable)},
        {"ttl",
         {offsetof(struct MutableCFOptions, ttl), rs::options_type::OptionType::UInt64T,
          rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable}},
        {"periodic_compaction_seconds",
         {offsetof(struct MutableCFOptions, periodic_compaction_seconds),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"bottommost_temperature",
         {0, rs::options_type::OptionType::Temperature, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"last_level_temperature",
         {offsetof(struct MutableCFOptions, last_level_temperature),
          rs::options_type::OptionType::Temperature, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"enable_blob_files",
         {offsetof(struct MutableCFOptions, enable_blob_files),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"min_blob_size",
         {offsetof(struct MutableCFOptions, min_blob_size),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"blob_file_size",
         {offsetof(struct MutableCFOptions, blob_file_size),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"blob_compression_type",
         {offsetof(struct MutableCFOptions, blob_compression_type),
          rs::options_type::OptionType::CompressionType, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"enable_blob_garbage_collection",
         {offsetof(struct MutableCFOptions, enable_blob_garbage_collection),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"blob_garbage_collection_age_cutoff",
         {offsetof(struct MutableCFOptions, blob_garbage_collection_age_cutoff),
          rs::options_type::OptionType::Double, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"blob_garbage_collection_force_threshold",
         {offsetof(struct MutableCFOptions,
                   blob_garbage_collection_force_threshold),
          rs::options_type::OptionType::Double, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"blob_compaction_readahead_size",
         {offsetof(struct MutableCFOptions, blob_compaction_readahead_size),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"blob_file_starting_level",
         {offsetof(struct MutableCFOptions, blob_file_starting_level),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"prepopulate_blob_cache",
         OptionTypeInfo::Enum<rs::advanced_options::PrepopulateBlobCache>(
             offsetof(struct MutableCFOptions, prepopulate_blob_cache),
             &prepopulate_blob_cache_string_map, rs::options_type::OptionTypeFlags::Mutable)},
        {"sample_for_compression",
         {offsetof(struct MutableCFOptions, sample_for_compression),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"bottommost_compression",
         {offsetof(struct MutableCFOptions, bottommost_compression),
          rs::options_type::OptionType::CompressionType, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"compression_per_level",
         OptionTypeInfo::Vector<CompressionType>(
             offsetof(struct MutableCFOptions, compression_per_level),
             rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::Mutable,
             {0, rs::options_type::OptionType::CompressionType})},
        {"experimental_mempurge_threshold",
         {offsetof(struct MutableCFOptions, experimental_mempurge_threshold),
          rs::options_type::OptionType::Double, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"memtable_protection_bytes_per_key",
         {offsetof(struct MutableCFOptions, memtable_protection_bytes_per_key),
          rs::options_type::OptionType::UInt32T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {"block_protection_bytes_per_key",
         {offsetof(struct MutableCFOptions, block_protection_bytes_per_key),
          rs::options_type::OptionType::UInt8T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::Mutable}},
        {kOptNameCompOpts,
         OptionTypeInfo::Struct(
             kOptNameCompOpts, &compression_options_type_info,
             offsetof(struct MutableCFOptions, compression_opts),
             rs::options_type::OptionVerificationType::Normal,
             (rs::options_type::OptionTypeFlags::Mutable | rs::options_type::OptionTypeFlags::CompareNever),
             [](const ConfigOptions& opts, const std::string& name,
                const std::string& value, void* addr) {
               // This is to handle backward compatibility, where
               // compression_options was a ":" separated list.
               if (name == kOptNameCompOpts &&
                   value.find("=") == std::string::npos) {
                 auto* compression = static_cast<CompressionOptions*>(addr);
                 return ParseCompressionOptions(value, name, *compression);
               } else {
                 return OptionTypeInfo::ParseStruct(
                     opts, kOptNameCompOpts, &compression_options_type_info,
                     name, value, addr);
               }
             })},
        {kOptNameBMCompOpts,
         OptionTypeInfo::Struct(
             kOptNameBMCompOpts, &compression_options_type_info,
             offsetof(struct MutableCFOptions, bottommost_compression_opts),
             rs::options_type::OptionVerificationType::Normal,
             (rs::options_type::OptionTypeFlags::Mutable | rs::options_type::OptionTypeFlags::CompareNever),
             [](const ConfigOptions& opts, const std::string& name,
                const std::string& value, void* addr) {
               // This is to handle backward compatibility, where
               // compression_options was a ":" separated list.
               if (name == kOptNameBMCompOpts &&
                   value.find("=") == std::string::npos) {
                 auto* compression = static_cast<CompressionOptions*>(addr);
                 return ParseCompressionOptions(value, name, *compression);
               } else {
                 return OptionTypeInfo::ParseStruct(
                     opts, kOptNameBMCompOpts, &compression_options_type_info,
                     name, value, addr);
               }
             })},
        // End special case properties
};

static std::unordered_map<std::string, OptionTypeInfo>
    cf_immutable_options_type_info = {
        /* not yet supported
        CompressionOptions compression_opts;
        TablePropertiesCollectorFactories table_properties_collector_factories;
        using TablePropertiesCollectorFactories =
            std::vector<std::shared_ptr<TablePropertiesCollectorFactory>>;
        rs::advanced_options::UpdateStatus (*inplace_callback)(char* existing_value,
                                         uint34_t* existing_value_size,
                                         Slice delta_value,
                                         std::string* merged_value);
        std::vector<DbPath> cf_paths;
         */
        {"compaction_measure_io_stats",
         {0, rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::None}},
        {"purge_redundant_kvs_while_flush",
         {0, rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::None}},
        {"inplace_update_support",
         {offsetof(struct ImmutableCFOptions, inplace_update_support),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"level_compaction_dynamic_level_bytes",
         {offsetof(struct ImmutableCFOptions,
                   level_compaction_dynamic_level_bytes),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"level_compaction_dynamic_file_size",
         {offsetof(struct ImmutableCFOptions,
                   level_compaction_dynamic_file_size),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"optimize_filters_for_hits",
         {offsetof(struct ImmutableCFOptions, optimize_filters_for_hits),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"force_consistency_checks",
         {offsetof(struct ImmutableCFOptions, force_consistency_checks),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"preclude_last_level_data_seconds",
         {offsetof(struct ImmutableCFOptions, preclude_last_level_data_seconds),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"preserve_internal_time_seconds",
         {offsetof(struct ImmutableCFOptions, preserve_internal_time_seconds),
          rs::options_type::OptionType::UInt64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        // Need to keep this around to be able to read old OPTIONS files.
        {"max_mem_compaction_level",
         {0, rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::None}},
        {"max_write_buffer_number_to_maintain",
         {offsetof(struct ImmutableCFOptions,
                   max_write_buffer_number_to_maintain),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None, 0}},
        {"max_write_buffer_size_to_maintain",
         {offsetof(struct ImmutableCFOptions,
                   max_write_buffer_size_to_maintain),
          rs::options_type::OptionType::Int64T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"min_write_buffer_number_to_merge",
         {offsetof(struct ImmutableCFOptions, min_write_buffer_number_to_merge),
          rs::options_type::OptionType::Int, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None, 0}},
        {"num_levels",
         {offsetof(struct ImmutableCFOptions, num_levels), rs::options_type::OptionType::Int,
          rs::options_type::OptionVerificationType::Normal, rs::options_type::OptionTypeFlags::None}},
        {"bloom_locality",
         {offsetof(struct ImmutableCFOptions, bloom_locality),
          rs::options_type::OptionType::UInt32T, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"rate_limit_delay_max_milliseconds",
         {0, rs::options_type::OptionType::UInt, rs::options_type::OptionVerificationType::Deprecated,
          rs::options_type::OptionTypeFlags::None}},
        {"comparator",
         OptionTypeInfo::AsCustomRawPtr<const Comparator>(
             offsetof(struct ImmutableCFOptions, user_comparator),
             rs::options_type::OptionVerificationType::ByName, rs::options_type::OptionTypeFlags::CompareLoose)
             .SetSerializeFunc(
                 // Serializes a Comparator
                 [](const ConfigOptions& opts, const std::string&,
                    const void* addr, std::string* value) {
                   // it's a const pointer of const Comparator*
                   const auto* ptr =
                       static_cast<const Comparator* const*>(addr);
                   // Since the user-specified comparator will be wrapped by
                   // InternalKeyComparator, we should persist the
                   // user-specified one instead of InternalKeyComparator.
                   if (*ptr == nullptr) {
                     *value = kNullptrString;
                   } else if (opts.mutable_options_only) {
                     *value = "";
                   } else {
                     const Comparator* root_comp = (*ptr)->GetRootComparator();
                     if (root_comp == nullptr) {
                       root_comp = (*ptr);
                     }
                     *value = root_comp->ToString(opts);
                   }
                   return Status::OK();
                 })},
        {"memtable_insert_with_hint_prefix_extractor",
         OptionTypeInfo::AsCustomSharedPtr<const SliceTransform>(
             offsetof(struct ImmutableCFOptions,
                      memtable_insert_with_hint_prefix_extractor),
             rs::options_type::OptionVerificationType::ByNameAllowNull, rs::options_type::OptionTypeFlags::None)},
        {"memtable_factory",
         {offsetof(struct ImmutableCFOptions, memtable_factory),
          rs::options_type::OptionType::Customizable, rs::options_type::OptionVerificationType::ByName,
          rs::options_type::OptionTypeFlags::Shared,
          [](const ConfigOptions& opts, const std::string&,
             const std::string& value, void* addr) {
            std::unique_ptr<MemTableRepFactory> factory;
            auto* shared =
                static_cast<std::shared_ptr<MemTableRepFactory>*>(addr);
            Status s =
                MemTableRepFactory::CreateFromString(opts, value, shared);
            return s;
          }}},
        {"memtable",
         {offsetof(struct ImmutableCFOptions, memtable_factory),
          rs::options_type::OptionType::Customizable, rs::options_type::OptionVerificationType::Alias,
          rs::options_type::OptionTypeFlags::Shared,
          [](const ConfigOptions& opts, const std::string&,
             const std::string& value, void* addr) {
            std::unique_ptr<MemTableRepFactory> factory;
            auto* shared =
                static_cast<std::shared_ptr<MemTableRepFactory>*>(addr);
            Status s =
                MemTableRepFactory::CreateFromString(opts, value, shared);
            return s;
          }}},
        {"table_factory",
         OptionTypeInfo::AsCustomSharedPtr<TableFactory>(
             offsetof(struct ImmutableCFOptions, table_factory),
             rs::options_type::OptionVerificationType::ByName,
             (rs::options_type::OptionTypeFlags::CompareLoose |
              rs::options_type::OptionTypeFlags::StringNameOnly |
              rs::options_type::OptionTypeFlags::DontPrepare))},
        {"block_based_table_factory",
         {offsetof(struct ImmutableCFOptions, table_factory),
          rs::options_type::OptionType::Customizable, rs::options_type::OptionVerificationType::Alias,
          rs::options_type::OptionTypeFlags::Shared | rs::options_type::OptionTypeFlags::CompareLoose,
          // Parses the input value and creates a BlockBasedTableFactory
          [](const ConfigOptions& opts, const std::string& name,
             const std::string& value, void* addr) {
            BlockBasedTableOptions* old_opts = nullptr;
            auto table_factory =
                static_cast<std::shared_ptr<TableFactory>*>(addr);
            if (table_factory->get() != nullptr) {
              old_opts =
                  table_factory->get()->GetOptions<BlockBasedTableOptions>();
            }
            if (name == "block_based_table_factory") {
              std::unique_ptr<TableFactory> new_factory;
              if (old_opts != nullptr) {
                new_factory.reset(NewBlockBasedTableFactory(*old_opts));
              } else {
                new_factory.reset(NewBlockBasedTableFactory());
              }
              Status s = new_factory->ConfigureFromString(opts, value);
              if (s.ok()) {
                table_factory->reset(new_factory.release());
              }
              return s;
            } else if (old_opts != nullptr) {
              return table_factory->get()->ConfigureOption(opts, name, value);
            } else {
              return Status::NotFound("Mismatched table option: ", name);
            }
          }}},
        {"plain_table_factory",
         {offsetof(struct ImmutableCFOptions, table_factory),
          rs::options_type::OptionType::Customizable, rs::options_type::OptionVerificationType::Alias,
          rs::options_type::OptionTypeFlags::Shared | rs::options_type::OptionTypeFlags::CompareLoose,
          // Parses the input value and creates a PlainTableFactory
          [](const ConfigOptions& opts, const std::string& name,
             const std::string& value, void* addr) {
            PlainTableOptions* old_opts = nullptr;
            auto table_factory =
                static_cast<std::shared_ptr<TableFactory>*>(addr);
            if (table_factory->get() != nullptr) {
              old_opts = table_factory->get()->GetOptions<PlainTableOptions>();
            }
            if (name == "plain_table_factory") {
              std::unique_ptr<TableFactory> new_factory;
              if (old_opts != nullptr) {
                new_factory.reset(NewPlainTableFactory(*old_opts));
              } else {
                new_factory.reset(NewPlainTableFactory());
              }
              Status s = new_factory->ConfigureFromString(opts, value);
              if (s.ok()) {
                table_factory->reset(new_factory.release());
              }
              return s;
            } else if (old_opts != nullptr) {
              return table_factory->get()->ConfigureOption(opts, name, value);
            } else {
              return Status::NotFound("Mismatched table option: ", name);
            }
          }}},
        {"table_properties_collectors",
         OptionTypeInfo::Vector<
             std::shared_ptr<TablePropertiesCollectorFactory>>(
             offsetof(struct ImmutableCFOptions,
                      table_properties_collector_factories),
             rs::options_type::OptionVerificationType::ByName, rs::options_type::OptionTypeFlags::None,
             OptionTypeInfo::AsCustomSharedPtr<TablePropertiesCollectorFactory>(
                 0, rs::options_type::OptionVerificationType::ByName, rs::options_type::OptionTypeFlags::None))},
        {"compaction_filter",
         OptionTypeInfo::AsCustomRawPtr<const CompactionFilter>(
             offsetof(struct ImmutableCFOptions, compaction_filter),
             rs::options_type::OptionVerificationType::ByName, rs::options_type::OptionTypeFlags::AllowNull)},
        {"compaction_filter_factory",
         OptionTypeInfo::AsCustomSharedPtr<CompactionFilterFactory>(
             offsetof(struct ImmutableCFOptions, compaction_filter_factory),
             rs::options_type::OptionVerificationType::ByName, rs::options_type::OptionTypeFlags::AllowNull)},
        {"merge_operator",
         OptionTypeInfo::AsCustomSharedPtr<MergeOperator>(
             offsetof(struct ImmutableCFOptions, merge_operator),
             rs::options_type::OptionVerificationType::ByNameAllowFromNull,
             rs::options_type::OptionTypeFlags::CompareLoose | rs::options_type::OptionTypeFlags::AllowNull)},
        {"compaction_style",
         {offsetof(struct ImmutableCFOptions, compaction_style),
          rs::options_type::OptionType::CompactionStyle, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"compaction_pri",
         {offsetof(struct ImmutableCFOptions, compaction_pri),
          rs::options_type::OptionType::CompactionPri, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::None}},
        {"sst_partitioner_factory",
         OptionTypeInfo::AsCustomSharedPtr<SstPartitionerFactory>(
             offsetof(struct ImmutableCFOptions, sst_partitioner_factory),
             rs::options_type::OptionVerificationType::ByName, rs::options_type::OptionTypeFlags::AllowNull)},
        {"blob_cache",
         {offsetof(struct ImmutableCFOptions, blob_cache), rs::options_type::OptionType::Unknown,
          rs::options_type::OptionVerificationType::Normal,
          (rs::options_type::OptionTypeFlags::CompareNever | rs::options_type::OptionTypeFlags::DontSerialize),
          // Parses the input value as a Cache
          [](const ConfigOptions& opts, const std::string&,
             const std::string& value, void* addr) {
            auto* cache = static_cast<std::shared_ptr<Cache>*>(addr);
            return Cache::CreateFromString(opts, value, cache);
          }}},
        {"persist_user_defined_timestamps",
         {offsetof(struct ImmutableCFOptions, persist_user_defined_timestamps),
          rs::options_type::OptionType::Boolean, rs::options_type::OptionVerificationType::Normal,
          rs::options_type::OptionTypeFlags::CompareLoose}},
};

const std::string OptionsHelper::kCFOptionsName = "ColumnFamilyOptions";

class ConfigurableMutableCFOptions : public Configurable {
 public:
  explicit ConfigurableMutableCFOptions(const MutableCFOptions& mcf) {
    mutable_ = mcf;
    RegisterOptions(&mutable_, &cf_mutable_options_type_info);
  }

 protected:
  MutableCFOptions mutable_;
};

class ConfigurableCFOptions : public ConfigurableMutableCFOptions {
 public:
  ConfigurableCFOptions(const ColumnFamilyOptions& opts,
                        const std::unordered_map<std::string, std::string>* map)
      : ConfigurableMutableCFOptions(MutableCFOptions(opts)),
        immutable_(opts),
        cf_options_(opts),
        opt_map_(map) {
    RegisterOptions(&immutable_, &cf_immutable_options_type_info);
  }

 protected:
  Status ConfigureOptions(
      const ConfigOptions& config_options,
      const std::unordered_map<std::string, std::string>& opts_map,
      std::unordered_map<std::string, std::string>* unused) override {
    Status s = Configurable::ConfigureOptions(config_options, opts_map, unused);
    if (s.ok()) {
      UpdateColumnFamilyOptions(mutable_, &cf_options_);
      UpdateColumnFamilyOptions(immutable_, &cf_options_);
      s = PrepareOptions(config_options);
    }
    return s;
  }

  virtual const void* GetOptionsPtr(const std::string& name) const override {
    if (name == OptionsHelper::kCFOptionsName) {
      return &cf_options_;
    } else {
      return ConfigurableMutableCFOptions::GetOptionsPtr(name);
    }
  }

  bool OptionsAreEqual(const ConfigOptions& config_options,
                       const OptionTypeInfo& opt_info,
                       const std::string& opt_name, const void* const this_ptr,
                       const void* const that_ptr,
                       std::string* mismatch) const override {
    bool equals = opt_info.AreEqual(config_options, opt_name, this_ptr,
                                    that_ptr, mismatch);
    if (!equals && opt_info.IsByName()) {
      if (opt_map_ == nullptr) {
        equals = true;
      } else {
        const auto& iter = opt_map_->find(opt_name);
        if (iter == opt_map_->end()) {
          equals = true;
        } else {
          equals = opt_info.AreEqualByName(config_options, opt_name, this_ptr,
                                           iter->second);
        }
      }
      if (equals) {  // False alarm, clear mismatch
        *mismatch = "";
      }
    }
    if (equals && opt_info.IsConfigurable() && opt_map_ != nullptr) {
      const auto* this_config = opt_info.AsRawPointer<Configurable>(this_ptr);
      if (this_config == nullptr) {
        const auto& iter = opt_map_->find(opt_name);
        // If the name exists in the map and is not empty/null,
        // then the this_config should be set.
        if (iter != opt_map_->end() && !iter->second.empty() &&
            iter->second != kNullptrString) {
          *mismatch = opt_name;
          equals = false;
        }
      }
    }
    return equals;
  }

 private:
  ImmutableCFOptions immutable_;
  ColumnFamilyOptions cf_options_;
  const std::unordered_map<std::string, std::string>* opt_map_;
};

std::unique_ptr<Configurable> CFOptionsAsConfigurable(
    const MutableCFOptions& opts) {
  std::unique_ptr<Configurable> ptr(new ConfigurableMutableCFOptions(opts));
  return ptr;
}
std::unique_ptr<Configurable> CFOptionsAsConfigurable(
    const ColumnFamilyOptions& opts,
    const std::unordered_map<std::string, std::string>* opt_map) {
  std::unique_ptr<Configurable> ptr(new ConfigurableCFOptions(opts, opt_map));
  return ptr;
}

ImmutableCFOptions::ImmutableCFOptions() : ImmutableCFOptions(Options()) {}

ImmutableCFOptions::ImmutableCFOptions(const ColumnFamilyOptions& cf_options)
    : compaction_style(cf_options.compaction_style),
      compaction_pri(cf_options.compaction_pri),
      user_comparator(cf_options.comparator),
      internal_comparator(InternalKeyComparator(cf_options.comparator)),
      merge_operator(cf_options.merge_operator),
      compaction_filter(cf_options.compaction_filter),
      compaction_filter_factory(cf_options.compaction_filter_factory),
      min_write_buffer_number_to_merge(
          cf_options.min_write_buffer_number_to_merge),
      max_write_buffer_number_to_maintain(
          cf_options.max_write_buffer_number_to_maintain),
      max_write_buffer_size_to_maintain(
          cf_options.max_write_buffer_size_to_maintain),
      inplace_update_support(cf_options.inplace_update_support),
      inplace_callback(cf_options.inplace_callback),
      memtable_factory(cf_options.memtable_factory),
      table_factory(cf_options.table_factory),
      table_properties_collector_factories(
          cf_options.table_properties_collector_factories),
      bloom_locality(cf_options.bloom_locality),
      level_compaction_dynamic_level_bytes(
          cf_options.level_compaction_dynamic_level_bytes),
      level_compaction_dynamic_file_size(
          cf_options.level_compaction_dynamic_file_size),
      num_levels(cf_options.num_levels),
      optimize_filters_for_hits(cf_options.optimize_filters_for_hits),
      force_consistency_checks(cf_options.force_consistency_checks),
      preclude_last_level_data_seconds(
          cf_options.preclude_last_level_data_seconds),
      preserve_internal_time_seconds(cf_options.preserve_internal_time_seconds),
      memtable_insert_with_hint_prefix_extractor(
          cf_options.memtable_insert_with_hint_prefix_extractor),
      cf_paths(cf_options.cf_paths),
      compaction_thread_limiter(cf_options.compaction_thread_limiter),
      sst_partitioner_factory(cf_options.sst_partitioner_factory),
      blob_cache(cf_options.blob_cache),
      persist_user_defined_timestamps(
          cf_options.persist_user_defined_timestamps) {}

ImmutableOptions::ImmutableOptions() : ImmutableOptions(Options()) {}

ImmutableOptions::ImmutableOptions(const Options& options)
    : ImmutableOptions(options, options) {}

ImmutableOptions::ImmutableOptions(const DBOptions& db_options,
                                   const ColumnFamilyOptions& cf_options)
    : ImmutableDBOptions(db_options), ImmutableCFOptions(cf_options) {}

ImmutableOptions::ImmutableOptions(const DBOptions& db_options,
                                   const ImmutableCFOptions& cf_options)
    : ImmutableDBOptions(db_options), ImmutableCFOptions(cf_options) {}

ImmutableOptions::ImmutableOptions(const ImmutableDBOptions& db_options,
                                   const ColumnFamilyOptions& cf_options)
    : ImmutableDBOptions(db_options), ImmutableCFOptions(cf_options) {}

ImmutableOptions::ImmutableOptions(const ImmutableDBOptions& db_options,
                                   const ImmutableCFOptions& cf_options)
    : ImmutableDBOptions(db_options), ImmutableCFOptions(cf_options) {}

// Multiple two operands. If they overflow, return op1.
uint64_t MultiplyCheckOverflow(uint64_t op1, double op2) {
  if (op1 == 0 || op2 <= 0) {
    return 0;
  }
  if (std::numeric_limits<uint64_t>::max() / op1 < op2) {
    return op1;
  }
  return static_cast<uint64_t>(op1 * op2);
}

// when level_compaction_dynamic_level_bytes is true and leveled compaction
// is used, the base level is not always L1, so precomupted max_file_size can
// no longer be used. Recompute file_size_for_level from base level.
uint64_t MaxFileSizeForLevel(const MutableCFOptions& cf_options,
    int level, rs::advanced_options::CompactionStyle compaction_style, int base_level,
    bool level_compaction_dynamic_level_bytes) {
  if (!level_compaction_dynamic_level_bytes || level < base_level ||
      compaction_style != rs::advanced_options::CompactionStyle::Level) {
    assert(level >= 0);
    assert(level < (int)cf_options.max_file_size.size());
    return cf_options.max_file_size[level];
  } else {
    assert(level >= 0 && base_level >= 0);
    assert(level - base_level < (int)cf_options.max_file_size.size());
    return cf_options.max_file_size[level - base_level];
  }
}

size_t MaxFileSizeForL0MetaPin(const MutableCFOptions& cf_options) {
  // We do not want to pin meta-blocks that almost certainly came from intra-L0
  // or a former larger `write_buffer_size` value to avoid surprising users with
  // pinned memory usage. We use a factor of 1.5 to account for overhead
  // introduced during flush in most cases.
  if (std::numeric_limits<size_t>::max() / 3 <
      cf_options.write_buffer_size / 2) {
    return std::numeric_limits<size_t>::max();
  }
  return cf_options.write_buffer_size / 2 * 3;
}

void MutableCFOptions::RefreshDerivedOptions(int num_levels,
                                             rs::advanced_options::CompactionStyle compaction_style) {
  max_file_size.resize(num_levels);
  for (int i = 0; i < num_levels; ++i) {
    if (i == 0 && compaction_style == rs::advanced_options::CompactionStyle::Universal) {
      max_file_size[i] = ULLONG_MAX;
    } else if (i > 1) {
      max_file_size[i] = MultiplyCheckOverflow(max_file_size[i - 1],
                                               target_file_size_multiplier);
    } else {
      max_file_size[i] = target_file_size_base;
    }
  }
}

void MutableCFOptions::Dump(Logger* log) const {
  // Memtable related options
  ROCKS_LOG_INFO(log,
                 "                        write_buffer_size: %" ROCKSDB_PRIszt,
                 write_buffer_size);
  ROCKS_LOG_INFO(log, "                  max_write_buffer_number: %d",
                 max_write_buffer_number);
  ROCKS_LOG_INFO(log,
                 "                         arena_block_size: %" ROCKSDB_PRIszt,
                 arena_block_size);
  ROCKS_LOG_INFO(log, "              memtable_prefix_bloom_ratio: %f",
                 memtable_prefix_bloom_size_ratio);
  ROCKS_LOG_INFO(log, "              memtable_whole_key_filtering: %d",
                 memtable_whole_key_filtering);
  ROCKS_LOG_INFO(log,
                 "                  memtable_huge_page_size: %" ROCKSDB_PRIszt,
                 memtable_huge_page_size);
  ROCKS_LOG_INFO(log,
                 "                    max_successive_merges: %" ROCKSDB_PRIszt,
                 max_successive_merges);
  ROCKS_LOG_INFO(log,
                 "                 inplace_update_num_locks: %" ROCKSDB_PRIszt,
                 inplace_update_num_locks);
  ROCKS_LOG_INFO(log, "                         prefix_extractor: %s",
                 prefix_extractor == nullptr
                     ? "nullptr"
                     : prefix_extractor->GetId().c_str());
  ROCKS_LOG_INFO(log, "                 disable_auto_compactions: %d",
                 disable_auto_compactions);
  ROCKS_LOG_INFO(log, "      soft_pending_compaction_bytes_limit: %" PRIu64,
                 soft_pending_compaction_bytes_limit);
  ROCKS_LOG_INFO(log, "      hard_pending_compaction_bytes_limit: %" PRIu64,
                 hard_pending_compaction_bytes_limit);
  ROCKS_LOG_INFO(log, "       level0_file_num_compaction_trigger: %d",
                 level0_file_num_compaction_trigger);
  ROCKS_LOG_INFO(log, "           level0_slowdown_writes_trigger: %d",
                 level0_slowdown_writes_trigger);
  ROCKS_LOG_INFO(log, "               level0_stop_writes_trigger: %d",
                 level0_stop_writes_trigger);
  ROCKS_LOG_INFO(log, "                     max_compaction_bytes: %" PRIu64,
                 max_compaction_bytes);
  ROCKS_LOG_INFO(log, "    ignore_max_compaction_bytes_for_input: %s",
                 ignore_max_compaction_bytes_for_input ? "true" : "false");
  ROCKS_LOG_INFO(log, "                    target_file_size_base: %" PRIu64,
                 target_file_size_base);
  ROCKS_LOG_INFO(log, "              target_file_size_multiplier: %d",
                 target_file_size_multiplier);
  ROCKS_LOG_INFO(log, "                 max_bytes_for_level_base: %" PRIu64,
                 max_bytes_for_level_base);
  ROCKS_LOG_INFO(log, "           max_bytes_for_level_multiplier: %f",
                 max_bytes_for_level_multiplier);
  ROCKS_LOG_INFO(log, "                                      ttl: %" PRIu64,
                 ttl);
  ROCKS_LOG_INFO(log, "              periodic_compaction_seconds: %" PRIu64,
                 periodic_compaction_seconds);
  std::string result;
  char buf[10];
  for (const auto m : max_bytes_for_level_multiplier_additional) {
    snprintf(buf, sizeof(buf), "%d, ", m);
    result += buf;
  }
  if (result.size() >= 2) {
    result.resize(result.size() - 2);
  } else {
    result = "";
  }

  ROCKS_LOG_INFO(log, "max_bytes_for_level_multiplier_additional: %s",
                 result.c_str());
  ROCKS_LOG_INFO(log, "        max_sequential_skip_in_iterations: %" PRIu64,
                 max_sequential_skip_in_iterations);
  ROCKS_LOG_INFO(log, "         check_flush_compaction_key_order: %d",
                 check_flush_compaction_key_order);
  ROCKS_LOG_INFO(log, "                     paranoid_file_checks: %d",
                 paranoid_file_checks);
  ROCKS_LOG_INFO(log, "                       report_bg_io_stats: %d",
                 report_bg_io_stats);
  ROCKS_LOG_INFO(log, "                              compression: %d",
                 static_cast<int>(compression));
  ROCKS_LOG_INFO(log,
                 "                       experimental_mempurge_threshold: %f",
                 experimental_mempurge_threshold);

  // Universal Compaction Options
  ROCKS_LOG_INFO(log, "compaction_options_universal.size_ratio : %d",
                 compaction_options_universal.size_ratio);
  ROCKS_LOG_INFO(log, "compaction_options_universal.min_merge_width : %d",
                 compaction_options_universal.min_merge_width);
  ROCKS_LOG_INFO(log, "compaction_options_universal.max_merge_width : %d",
                 compaction_options_universal.max_merge_width);
  ROCKS_LOG_INFO(
      log, "compaction_options_universal.max_size_amplification_percent : %d",
      compaction_options_universal.max_size_amplification_percent);
  ROCKS_LOG_INFO(log,
                 "compaction_options_universal.compression_size_percent : %d",
                 compaction_options_universal.compression_size_percent);
  ROCKS_LOG_INFO(log, "compaction_options_universal.stop_style : %d",
                 (int)compaction_options_universal.stop_style);
  ROCKS_LOG_INFO(
      log, "compaction_options_universal.allow_trivial_move : %d",
      static_cast<int>(compaction_options_universal.allow_trivial_move));
  ROCKS_LOG_INFO(log, "compaction_options_universal.incremental        : %d",
                 static_cast<int>(compaction_options_universal.incremental));

  // FIFO Compaction Options
  ROCKS_LOG_INFO(log, "compaction_options_fifo.max_table_files_size : %" PRIu64,
                 compaction_options_fifo.max_table_files_size);
  ROCKS_LOG_INFO(log, "compaction_options_fifo.allow_compaction : %d",
                 compaction_options_fifo.allow_compaction);

  // Blob file related options
  ROCKS_LOG_INFO(log, "                        enable_blob_files: %s",
                 enable_blob_files ? "true" : "false");
  ROCKS_LOG_INFO(log, "                            min_blob_size: %" PRIu64,
                 min_blob_size);
  ROCKS_LOG_INFO(log, "                           blob_file_size: %" PRIu64,
                 blob_file_size);
  ROCKS_LOG_INFO(log, "                    blob_compression_type: %s",
                 CompressionTypeToString(blob_compression_type).c_str());
  ROCKS_LOG_INFO(log, "           enable_blob_garbage_collection: %s",
                 enable_blob_garbage_collection ? "true" : "false");
  ROCKS_LOG_INFO(log, "       blob_garbage_collection_age_cutoff: %f",
                 blob_garbage_collection_age_cutoff);
  ROCKS_LOG_INFO(log, "  blob_garbage_collection_force_threshold: %f",
                 blob_garbage_collection_force_threshold);
  ROCKS_LOG_INFO(log, "           blob_compaction_readahead_size: %" PRIu64,
                 blob_compaction_readahead_size);
  ROCKS_LOG_INFO(log, "                 blob_file_starting_level: %d",
                 blob_file_starting_level);
  ROCKS_LOG_INFO(log, "                   prepopulate_blob_cache: %s",
                 prepopulate_blob_cache == rs::advanced_options::PrepopulateBlobCache::FlushOnly
                     ? "flush only"
                     : "disable");
  ROCKS_LOG_INFO(log, "                   last_level_temperature: %d",
                 static_cast<int>(last_level_temperature));
}

MutableCFOptions::MutableCFOptions(const Options& options)
    : MutableCFOptions(ColumnFamilyOptions(options)) {}

Status GetMutableOptionsFromStrings(
    const MutableCFOptions& base_options,
    const std::unordered_map<std::string, std::string>& options_map,
    Logger* /*info_log*/, MutableCFOptions* new_options) {
  assert(new_options);
  *new_options = base_options;
  ConfigOptions config_options;
  Status s = OptionTypeInfo::ParseType(
      config_options, options_map, cf_mutable_options_type_info, new_options);
  if (!s.ok()) {
    *new_options = base_options;
  }
  return s;
}

Status GetStringFromMutableCFOptions(const ConfigOptions& config_options,
                                     const MutableCFOptions& mutable_opts,
                                     std::string* opt_string) {
  assert(opt_string);
  opt_string->clear();
  return OptionTypeInfo::SerializeType(
      config_options, cf_mutable_options_type_info, &mutable_opts, opt_string);
}
}  // namespace rocksdb
