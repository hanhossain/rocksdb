//  Copyright (c) Meta Platforms, Inc. and affiliates.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "rocksdb-rs-cxx/lib.h"
#include "rocksdb/data_structure.h"


namespace ROCKSDB_NAMESPACE {
namespace detail {

int CountTrailingZeroBitsForSmallEnumSet(uint64_t v) {
  return rs::math::CountTrailingZeroBits(v);
}

}  // namespace detail
}  // namespace ROCKSDB_NAMESPACE
