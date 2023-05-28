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

std::unique_ptr<const char[]> Status::CopyState(const char* s) {
  const size_t cch = std::strlen(s) + 1;  // +1 for the null terminator
  char* rv = new char[cch];
  std::strncpy(rv, s, cch);
  return std::unique_ptr<const char[]>(rv);
}

static const char* msgs[static_cast<int>(Status::SubCode::kMaxSubCode)] = {
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

Status::Status(Code _code, SubCode _subcode, const Slice& msg,
               const Slice& msg2, rs::status::Severity sev)
    : code_(_code),
      subcode_(_subcode),
      sev_(sev),
      retryable_(false),
      data_loss_(false),
      scope_(0) {
  assert(subcode_ != Status::SubCode::kMaxSubCode);
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
  state_.reset(result);
}

Status Status::CopyAppendMessage(const Status& s, const Slice& delim,
                                 const Slice& msg) {
  // (No attempt at efficiency)
  return Status(s.code(), s.subcode(), s.severity(),
                std::string(s.getState()) + delim.ToString() + msg.ToString());
}

std::string Status::ToString() const {
#ifdef ROCKSDB_ASSERT_STATUS_CHECKED
  checked_ = true;
#endif  // ROCKSDB_ASSERT_STATUS_CHECKED
  const char* type = nullptr;
  switch (code_) {
    case Status::Code::kOk:
      return "OK";
    case Status::Code::kNotFound:
      type = "NotFound: ";
      break;
    case Status::Code::kCorruption:
      type = "Corruption: ";
      break;
    case Status::Code::kNotSupported:
      type = "Not implemented: ";
      break;
    case Status::Code::kInvalidArgument:
      type = "Invalid argument: ";
      break;
    case Status::Code::kIOError:
      type = "IO error: ";
      break;
    case Status::Code::kMergeInProgress:
      type = "Merge in progress: ";
      break;
    case Status::Code::kIncomplete:
      type = "Result incomplete: ";
      break;
    case Status::Code::kShutdownInProgress:
      type = "Shutdown in progress: ";
      break;
    case Status::Code::kTimedOut:
      type = "Operation timed out: ";
      break;
    case Status::Code::kAborted:
      type = "Operation aborted: ";
      break;
    case Status::Code::kBusy:
      type = "Resource busy: ";
      break;
    case Status::Code::kExpired:
      type = "Operation expired: ";
      break;
    case Status::Code::kTryAgain:
      type = "Operation failed. Try again.: ";
      break;
    case Status::Code::kCompactionTooLarge:
      type = "Compaction too large: ";
      break;
    case Status::Code::kColumnFamilyDropped:
      type = "Column family dropped: ";
      break;
    case Status::Code::kMaxCode:
      assert(false);
      break;
  }
  char tmp[30];
  if (type == nullptr) {
    // This should not happen since `code_` should be a valid non-`kMaxCode`
    // member of the `Code` enum. The above switch-statement should have had a
    // case assigning `type` to a corresponding string.
    assert(false);
    snprintf(tmp, sizeof(tmp), "Unknown code(%d): ", static_cast<int>(code()));
    type = tmp;
  }
  std::string result(type);
  if (subcode_ != Status::SubCode::kNone) {
    uint32_t index = static_cast<int32_t>(subcode_);
    assert(sizeof(msgs) / sizeof(msgs[0]) > index);
    result.append(msgs[index]);
  }

  if (state_ != nullptr) {
    if (subcode_ != Status::SubCode::kNone) {
      result.append(": ");
    }
    result.append(state_.get());
  }
  return result;
}

}  // namespace ROCKSDB_NAMESPACE
