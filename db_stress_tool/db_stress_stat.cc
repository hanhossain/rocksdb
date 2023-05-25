//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#ifdef GFLAGS

#include "db_stress_tool/db_stress_stat.h"

namespace rocksdb {

std::shared_ptr<rocksdb::Statistics> dbstats;
std::shared_ptr<rocksdb::Statistics> dbstats_secondaries;

}  // namespace rocksdb

#endif  // GFLAGS
