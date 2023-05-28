// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A Status encapsulates the result of an operation.  It may indicate success,
// or it may indicate an error with an associated error message.
//
// Multiple threads can invoke const methods on a Status without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Status must use
// external synchronization.

#pragma once

#ifdef ROCKSDB_ASSERT_STATUS_CHECKED
#include <stdio.h>
#include <stdlib.h>
#endif

#include <memory>
#include <string>

#include "rocksdb-rs-cxx/lib.h"
#include "rocksdb/slice.h"

namespace ROCKSDB_NAMESPACE {

class Status {
 public:
  // Create a success status.
  Status()
      : code_(rs::status::Code::kOk),
        subcode_(rs::status::SubCode::kNone),
        sev_(rs::status::Severity::kNoError),
        retryable_(false),
        data_loss_(false),
        scope_(0),
        state_(nullptr) {}
  ~Status() {
#ifdef ROCKSDB_ASSERT_STATUS_CHECKED
    if (!checked_) {
      fprintf(stderr, "Failed to check Status %p\n", this);
      std::abort();
    }
#endif  // ROCKSDB_ASSERT_STATUS_CHECKED
  }

  // Copy the specified status.
  Status(const Status& s);
  Status& operator=(const Status& s);
  Status(Status&& s) noexcept;
  Status& operator=(Status&& s) noexcept;
  bool operator==(const Status& rhs) const;
  bool operator!=(const Status& rhs) const;

  // In case of intentionally swallowing an error, user must explicitly call
  // this function. That way we are easily able to search the code to find where
  // error swallowing occurs.
  inline void PermitUncheckedError() const { MarkChecked(); }

  inline void MustCheck() const {
#ifdef ROCKSDB_ASSERT_STATUS_CHECKED
    checked_ = false;
#endif  // ROCKSDB_ASSERT_STATUS_CHECKED
  }

  rs::status::Code code() const {
    MarkChecked();
    return code_;
  }

  rs::status::SubCode subcode() const {
    MarkChecked();
    return subcode_;
  }

  Status(const Status& s, rs::status::Severity sev);

  Status(rs::status::Code _code, rs::status::SubCode _subcode, rs::status::Severity _sev, const Slice& msg)
      : Status(_code, _subcode, msg, "", _sev) {}

  static Status CopyAppendMessage(const Status& s, const Slice& delim,
                                  const Slice& msg);

  rs::status::Severity severity() const {
    MarkChecked();
    return sev_;
  }

  // Returns a C style string indicating the message of the Status
  const char* getState() const {
    MarkChecked();
    return state_.get();
  }

  // Return a success status.
  static Status OK() { return Status(); }

  // Successful, though an existing something was overwritten
  // Note: using variants of OK status for program logic is discouraged,
  // but it can be useful for communicating statistical information without
  // changing public APIs.
  static Status OkOverwritten() { return Status(rs::status::Code::kOk, rs::status::SubCode::kOverwritten); }

  // Return error status of an appropriate type.
  static Status NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kNotFound, msg, msg2);
  }

  // Fast path for not found without malloc;
  static Status NotFound(rs::status::SubCode msg = rs::status::SubCode::kNone) { return Status(rs::status::Code::kNotFound, msg); }

  static Status NotFound(rs::status::SubCode sc, const Slice& msg,
                         const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kNotFound, sc, msg, msg2);
  }

  static Status Corruption(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kCorruption, msg, msg2);
  }
  static Status Corruption(rs::status::SubCode msg = rs::status::SubCode::kNone) {
    return Status(rs::status::Code::kCorruption, msg);
  }

  static Status NotSupported(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kNotSupported, msg, msg2);
  }
  static Status NotSupported(rs::status::SubCode msg = rs::status::SubCode::kNone) {
    return Status(rs::status::Code::kNotSupported, msg);
  }

  static Status InvalidArgument(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kInvalidArgument, msg, msg2);
  }
  static Status InvalidArgument(rs::status::SubCode msg = rs::status::SubCode::kNone) {
    return Status(rs::status::Code::kInvalidArgument, msg);
  }

  static Status IOError(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kIOError, msg, msg2);
  }
  static Status IOError(rs::status::SubCode msg = rs::status::SubCode::kNone) { return Status(rs::status::Code::kIOError, msg); }

  static Status MergeInProgress(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kMergeInProgress, msg, msg2);
  }
  static Status MergeInProgress(rs::status::SubCode msg = rs::status::SubCode::kNone) {
    return Status(rs::status::Code::kMergeInProgress, msg);
  }

  static Status Incomplete(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kIncomplete, msg, msg2);
  }
  static Status Incomplete(rs::status::SubCode msg = rs::status::SubCode::kNone) {
    return Status(rs::status::Code::kIncomplete, msg);
  }

  static Status ShutdownInProgress(rs::status::SubCode msg = rs::status::SubCode::kNone) {
    return Status(rs::status::Code::kShutdownInProgress, msg);
  }
  static Status ShutdownInProgress(const Slice& msg,
                                   const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kShutdownInProgress, msg, msg2);
  }
  static Status Aborted(rs::status::SubCode msg = rs::status::SubCode::kNone) { return Status(rs::status::Code::kAborted, msg); }
  static Status Aborted(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kAborted, msg, msg2);
  }

  static Status Busy(rs::status::SubCode msg = rs::status::SubCode::kNone) { return Status(rs::status::Code::kBusy, msg); }
  static Status Busy(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kBusy, msg, msg2);
  }

  static Status TimedOut(rs::status::SubCode msg = rs::status::SubCode::kNone) { return Status(rs::status::Code::kTimedOut, msg); }
  static Status TimedOut(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kTimedOut, msg, msg2);
  }

  static Status Expired(rs::status::SubCode msg = rs::status::SubCode::kNone) { return Status(rs::status::Code::kExpired, msg); }
  static Status Expired(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kExpired, msg, msg2);
  }

  static Status TryAgain(rs::status::SubCode msg = rs::status::SubCode::kNone) { return Status(rs::status::Code::kTryAgain, msg); }
  static Status TryAgain(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kTryAgain, msg, msg2);
  }

  static Status CompactionTooLarge(rs::status::SubCode msg = rs::status::SubCode::kNone) {
    return Status(rs::status::Code::kCompactionTooLarge, msg);
  }
  static Status CompactionTooLarge(const Slice& msg,
                                   const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kCompactionTooLarge, msg, msg2);
  }

  static Status ColumnFamilyDropped(rs::status::SubCode msg = rs::status::SubCode::kNone) {
    return Status(rs::status::Code::kColumnFamilyDropped, msg);
  }

  static Status ColumnFamilyDropped(const Slice& msg,
                                    const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kColumnFamilyDropped, msg, msg2);
  }

  static Status NoSpace() { return Status(rs::status::Code::kIOError, rs::status::SubCode::kNoSpace); }
  static Status NoSpace(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kIOError, rs::status::SubCode::kNoSpace, msg, msg2);
  }

  static Status MemoryLimit() { return Status(rs::status::Code::kAborted, rs::status::SubCode::kMemoryLimit); }
  static Status MemoryLimit(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kAborted, rs::status::SubCode::kMemoryLimit, msg, msg2);
  }

  static Status SpaceLimit() { return Status(rs::status::Code::kIOError, rs::status::SubCode::kSpaceLimit); }
  static Status SpaceLimit(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kIOError, rs::status::SubCode::kSpaceLimit, msg, msg2);
  }

  static Status PathNotFound() { return Status(rs::status::Code::kIOError, rs::status::SubCode::kPathNotFound); }
  static Status PathNotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kIOError, rs::status::SubCode::kPathNotFound, msg, msg2);
  }

  static Status TxnNotPrepared() {
    return Status(rs::status::Code::kInvalidArgument, rs::status::SubCode::kTxnNotPrepared);
  }
  static Status TxnNotPrepared(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::kInvalidArgument, rs::status::SubCode::kTxnNotPrepared, msg, msg2);
  }

  // Returns true iff the status indicates success.
  bool ok() const {
    MarkChecked();
    return code() == rs::status::Code::kOk;
  }

  // Returns true iff the status indicates success *with* something
  // overwritten
  bool IsOkOverwritten() const {
    MarkChecked();
    return code() == rs::status::Code::kOk && subcode() == rs::status::SubCode::kOverwritten;
  }

  // Returns true iff the status indicates a NotFound error.
  bool IsNotFound() const {
    MarkChecked();
    return code() == rs::status::Code::kNotFound;
  }

  // Returns true iff the status indicates a Corruption error.
  bool IsCorruption() const {
    MarkChecked();
    return code() == rs::status::Code::kCorruption;
  }

  // Returns true iff the status indicates a NotSupported error.
  bool IsNotSupported() const {
    MarkChecked();
    return code() == rs::status::Code::kNotSupported;
  }

  // Returns true iff the status indicates an InvalidArgument error.
  bool IsInvalidArgument() const {
    MarkChecked();
    return code() == rs::status::Code::kInvalidArgument;
  }

  // Returns true iff the status indicates an IOError.
  bool IsIOError() const {
    MarkChecked();
    return code() == rs::status::Code::kIOError;
  }

  // Returns true iff the status indicates an MergeInProgress.
  bool IsMergeInProgress() const {
    MarkChecked();
    return code() == rs::status::Code::kMergeInProgress;
  }

  // Returns true iff the status indicates Incomplete
  bool IsIncomplete() const {
    MarkChecked();
    return code() == rs::status::Code::kIncomplete;
  }

  // Returns true iff the status indicates Shutdown In progress
  bool IsShutdownInProgress() const {
    MarkChecked();
    return code() == rs::status::Code::kShutdownInProgress;
  }

  bool IsTimedOut() const {
    MarkChecked();
    return code() == rs::status::Code::kTimedOut;
  }

  bool IsAborted() const {
    MarkChecked();
    return code() == rs::status::Code::kAborted;
  }

  bool IsLockLimit() const {
    MarkChecked();
    return code() == rs::status::Code::kAborted && subcode() == rs::status::SubCode::kLockLimit;
  }

  // Returns true iff the status indicates that a resource is Busy and
  // temporarily could not be acquired.
  bool IsBusy() const {
    MarkChecked();
    return code() == rs::status::Code::kBusy;
  }

  bool IsDeadlock() const {
    MarkChecked();
    return code() == rs::status::Code::kBusy && subcode() == rs::status::SubCode::kDeadlock;
  }

  // Returns true iff the status indicated that the operation has Expired.
  bool IsExpired() const {
    MarkChecked();
    return code() == rs::status::Code::kExpired;
  }

  // Returns true iff the status indicates a TryAgain error.
  // This usually means that the operation failed, but may succeed if
  // re-attempted.
  bool IsTryAgain() const {
    MarkChecked();
    return code() == rs::status::Code::kTryAgain;
  }

  // Returns true iff the status indicates the proposed compaction is too large
  bool IsCompactionTooLarge() const {
    MarkChecked();
    return code() == rs::status::Code::kCompactionTooLarge;
  }

  // Returns true iff the status indicates Column Family Dropped
  bool IsColumnFamilyDropped() const {
    MarkChecked();
    return code() == rs::status::Code::kColumnFamilyDropped;
  }

  // Returns true iff the status indicates a NoSpace error
  // This is caused by an I/O error returning the specific "out of space"
  // error condition. Stricto sensu, an NoSpace error is an I/O error
  // with a specific subcode, enabling users to take the appropriate action
  // if needed
  bool IsNoSpace() const {
    MarkChecked();
    return (code() == rs::status::Code::kIOError) && (subcode() == rs::status::SubCode::kNoSpace);
  }

  // Returns true iff the status indicates a memory limit error.  There may be
  // cases where we limit the memory used in certain operations (eg. the size
  // of a write batch) in order to avoid out of memory exceptions.
  bool IsMemoryLimit() const {
    MarkChecked();
    return (code() == rs::status::Code::kAborted) && (subcode() == rs::status::SubCode::kMemoryLimit);
  }

  // Returns true iff the status indicates a PathNotFound error
  // This is caused by an I/O error returning the specific "no such file or
  // directory" error condition. A PathNotFound error is an I/O error with
  // a specific subcode, enabling users to take appropriate action if necessary
  bool IsPathNotFound() const {
    MarkChecked();
    return (code() == rs::status::Code::kIOError || code() == rs::status::Code::kNotFound) &&
           (subcode() == rs::status::SubCode::kPathNotFound);
  }

  // Returns true iff the status indicates manual compaction paused. This
  // is caused by a call to PauseManualCompaction
  bool IsManualCompactionPaused() const {
    MarkChecked();
    return (code() == rs::status::Code::kIncomplete) && (subcode() == rs::status::SubCode::kManualCompactionPaused);
  }

  // Returns true iff the status indicates a TxnNotPrepared error.
  bool IsTxnNotPrepared() const {
    MarkChecked();
    return (code() == rs::status::Code::kInvalidArgument) && (subcode() == rs::status::SubCode::kTxnNotPrepared);
  }

  // Returns true iff the status indicates a IOFenced error.
  bool IsIOFenced() const {
    MarkChecked();
    return (code() == rs::status::Code::kIOError) && (subcode() == rs::status::SubCode::kIOFenced);
  }

  // Return a string representation of this status suitable for printing.
  // Returns the string "OK" for success.
  std::string ToString() const;

 protected:
  rs::status::Code code_;
  rs::status::SubCode subcode_;
  rs::status::Severity sev_;
  bool retryable_;
  bool data_loss_;
  unsigned char scope_;
  // A nullptr state_ (which is at least the case for OK) means the extra
  // message is empty.
  std::unique_ptr<const char[]> state_;
#ifdef ROCKSDB_ASSERT_STATUS_CHECKED
  mutable bool checked_ = false;
#endif  // ROCKSDB_ASSERT_STATUS_CHECKED

  explicit Status(rs::status::Code _code, rs::status::SubCode _subcode = rs::status::SubCode::kNone)
      : code_(_code),
        subcode_(_subcode),
        sev_(rs::status::Severity::kNoError),
        retryable_(false),
        data_loss_(false),
        scope_(0) {}

  explicit Status(rs::status::Code _code, rs::status::SubCode _subcode, bool retryable, bool data_loss,
                  unsigned char scope)
      : code_(_code),
        subcode_(_subcode),
        sev_(rs::status::Severity::kNoError),
        retryable_(retryable),
        data_loss_(data_loss),
        scope_(scope) {}

  Status(rs::status::Code _code, rs::status::SubCode _subcode, const Slice& msg, const Slice& msg2,
         rs::status::Severity sev = rs::status::Severity::kNoError);
  Status(rs::status::Code _code, const Slice& msg, const Slice& msg2)
      : Status(_code, rs::status::SubCode::kNone, msg, msg2) {}

  static std::unique_ptr<const char[]> CopyState(const char* s);

  inline void MarkChecked() const {
#ifdef ROCKSDB_ASSERT_STATUS_CHECKED
    checked_ = true;
#endif  // ROCKSDB_ASSERT_STATUS_CHECKED
  }
};

inline Status::Status(const Status& s)
    : code_(s.code_),
      subcode_(s.subcode_),
      sev_(s.sev_),
      retryable_(s.retryable_),
      data_loss_(s.data_loss_),
      scope_(s.scope_) {
  s.MarkChecked();
  state_ = (s.state_ == nullptr) ? nullptr : CopyState(s.state_.get());
}
inline Status::Status(const Status& s, rs::status::Severity sev)
    : code_(s.code_),
      subcode_(s.subcode_),
      sev_(sev),
      retryable_(s.retryable_),
      data_loss_(s.data_loss_),
      scope_(s.scope_) {
  s.MarkChecked();
  state_ = (s.state_ == nullptr) ? nullptr : CopyState(s.state_.get());
}
inline Status& Status::operator=(const Status& s) {
  if (this != &s) {
    s.MarkChecked();
    MustCheck();
    code_ = s.code_;
    subcode_ = s.subcode_;
    sev_ = s.sev_;
    retryable_ = s.retryable_;
    data_loss_ = s.data_loss_;
    scope_ = s.scope_;
    state_ = (s.state_ == nullptr) ? nullptr : CopyState(s.state_.get());
  }
  return *this;
}

inline Status::Status(Status&& s) noexcept : Status() {
  s.MarkChecked();
  *this = std::move(s);
}

inline Status& Status::operator=(Status&& s) noexcept {
  if (this != &s) {
    s.MarkChecked();
    MustCheck();
    code_ = std::move(s.code_);
    s.code_ = rs::status::Code::kOk;
    subcode_ = std::move(s.subcode_);
    s.subcode_ = rs::status::SubCode::kNone;
    sev_ = std::move(s.sev_);
    s.sev_ = rs::status::Severity::kNoError;
    retryable_ = std::move(s.retryable_);
    s.retryable_ = false;
    data_loss_ = std::move(s.data_loss_);
    s.data_loss_ = false;
    scope_ = std::move(s.scope_);
    s.scope_ = 0;
    state_ = std::move(s.state_);
  }
  return *this;
}

inline bool Status::operator==(const Status& rhs) const {
  MarkChecked();
  rhs.MarkChecked();
  return (code_ == rhs.code_);
}

inline bool Status::operator!=(const Status& rhs) const {
  MarkChecked();
  rhs.MarkChecked();
  return !(*this == rhs);
}

}  // namespace ROCKSDB_NAMESPACE
