//  Copyright (c) 2017-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include "db/dbformat.h"
#include "rocksdb/db.h"
#include "rocksdb/types.h"

namespace ROCKSDB_NAMESPACE {

// Returns listing of all versions of keys in the provided user key range.
// The range is inclusive-inclusive, i.e., [`begin_key`, `end_key`], or
// `max_num_ikeys` has been reached. Since all those keys returned will be
// copied to memory, if the range covers too many keys, the memory usage
// may be huge. `max_num_ikeys` can be used to cap the memory usage.
// The result is inserted into the provided vector, `key_versions`.
Status GetAllKeyVersions(DB* db, Slice begin_key, Slice end_key,
                         size_t max_num_ikeys,
                         std::vector<rs::debug::KeyVersion>* key_versions);

Status GetAllKeyVersions(DB* db, ColumnFamilyHandle* cfh, Slice begin_key,
                         Slice end_key, size_t max_num_ikeys,
                         std::vector<rs::debug::KeyVersion>* key_versions);

}  // namespace ROCKSDB_NAMESPACE

