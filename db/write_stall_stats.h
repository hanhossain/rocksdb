//  Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <array>

#include "db/internal_stats.h"
#include "rocksdb/types.h"

namespace rocksdb {
extern const std::string& InvalidWriteStallHyphenString();

extern const std::string& WriteStallCauseToHyphenString(rs::types::WriteStallCause cause);

extern const std::string& WriteStallConditionToHyphenString(
    rs::types::WriteStallCondition condition);

// REQUIRES:
// cause` is CF-scope `rs::types::WriteStallCause`, see `rs::types::WriteStallCause` for more
//
// REQUIRES:
// `condition` != `rs::types::WriteStallCondition::kNormal`
extern InternalStats::InternalCFStatsType InternalCFStat(
    rs::types::WriteStallCause cause, rs::types::WriteStallCondition condition);

// REQUIRES:
// cause` is DB-scope `rs::types::WriteStallCause`, see `rs::types::WriteStallCause` for more
//
// REQUIRES:
// `condition` != `rs::types::WriteStallCondition::kNormal`
extern InternalStats::InternalDBStatsType InternalDBStat(
    rs::types::WriteStallCause cause, rs::types::WriteStallCondition condition);

extern bool isCFScopeWriteStallCause(rs::types::WriteStallCause cause);
extern bool isDBScopeWriteStallCause(rs::types::WriteStallCause cause);

constexpr uint32_t kNumCFScopeWriteStallCauses =
    static_cast<uint32_t>(rs::types::WriteStallCause::CFScopeWriteStallCauseEnumMax) -
    static_cast<uint32_t>(rs::types::WriteStallCause::MemtableLimit);

constexpr uint32_t kNumDBScopeWriteStallCauses =
    static_cast<uint32_t>(rs::types::WriteStallCause::DBScopeWriteStallCauseEnumMax) -
    static_cast<uint32_t>(rs::types::WriteStallCause::WriteBufferManagerLimit);
}  // namespace rocksdb
