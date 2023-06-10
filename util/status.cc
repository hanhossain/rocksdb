//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "rocksdb/status.h"

#include <stdio.h>
#ifdef OS_WIN
#include <string.h>
#endif
#include <cstring>

#include "port/port.h"

namespace ROCKSDB_NAMESPACE {

static const char* msgs[static_cast<int>(rs::status::SubCode::MaxSubCode)] = {
    "",                                                   // kNone
    "Timeout Acquiring Mutex",                            // kMutexTimeout
    "Timeout waiting to lock key",                        // kLockTimeout
    "Failed to acquire lock due to max_num_locks limit",  // kLockLimit
    "No space left on device",                            // kNoSpace
    "Deadlock",                                           // kDeadlock
    "Stale file handle",                                  // kStaleFile
    "Memory limit reached",                               // kMemoryLimit
    "Space limit reached",                                // kSpaceLimit
    "No such file or directory",                          // kPathNotFound
    // KMergeOperandsInsufficientCapacity
    "Insufficient capacity for merge operands",
    // kManualCompactionPaused
    "Manual compaction paused",
    " (overwritten)",         // kOverwritten, subcode of OK
    "Txn not prepared",       // kTxnNotPrepared
    "IO fenced off",          // kIOFenced
    "Merge operator failed",  // kMergeOperatorFailed
};

Status::Status(rs::status::Code _code, rs::status::SubCode _subcode, const Slice& msg,
               const Slice& msg2, rs::status::Severity sev)
    : status_(rs::status::Status_new(_code, _subcode, false, false, 0)) {
  assert(status_.subcode_ != rs::status::SubCode::MaxSubCode);
  status_.sev = sev;
  const size_t len1 = msg.size();
  const size_t len2 = msg2.size();
  const size_t size = len1 + (len2 ? (2 + len2) : 0);
  char* const result = new char[size + 1];  // +1 for null terminator
  memcpy(result, msg.data(), len1);
  if (len2) {
    result[len1] = ':';
    result[len1 + 1] = ' ';
    memcpy(result + len1 + 2, msg2.data(), len2);
  }
  result[size] = '\0';  // null terminator for C style string
  status_.set_state_unsafe(result);
}

Status Status::CopyAppendMessage(const Status& s, const Slice& delim,
                                 const Slice& msg) {
  // (No attempt at efficiency)
  return Status(s.code(), s.subcode(), s.severity(),
                std::string(s.getState()) + delim.ToString() + msg.ToString());
}

std::string Status::ToString() const {
  return std::string(status_.ToString());
}

}  // namespace ROCKSDB_NAMESPACE
