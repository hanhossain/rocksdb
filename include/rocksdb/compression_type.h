// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include "rocksdb/rocksdb_namespace.h"

namespace ROCKSDB_NAMESPACE {

// DB contents are stored in a set of blocks, each of which holds a
// sequence of key,value pairs.  Each block may be compressed before
// being stored in a file.  The following enum describes which
// compression method (if any) is used to compress a block.

enum CompressionType : unsigned char {
  // NOTE: do not change the values of existing entries, as these are
  // part of the persistent format on disk.
  NoCompression = 0x0,
  SnappyCompression = 0x1,
  ZlibCompression = 0x2,
  BZip2Compression = 0x3,
  LZ4Compression = 0x4,
  LZ4HCCompression = 0x5,
  XpressCompression = 0x6,
  ZSTD = 0x7,

  // Only use CompressionType::ZSTDNotFinalCompression if you have to use ZSTD lib older than
  // 0.8.0 or consider a possibility of downgrading the service or copying
  // the database files to another service running with an older version of
  // RocksDB that doesn't have CompressionType::ZSTD. Otherwise, you should use CompressionType::ZSTD. We will
  // eventually remove the option from the public API.
  ZSTDNotFinalCompression = 0x40,

  // CompressionType::DisableCompressionOption is used to disable some compression options.
  DisableCompressionOption = 0xff,
};

}  // namespace ROCKSDB_NAMESPACE
