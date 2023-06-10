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

#include <memory>
#include <string>

#include "rocksdb-sys-cxx/lib.h"
#include "rocksdb/slice.h"

namespace ROCKSDB_NAMESPACE {

class Status final {
 public:
  // Create a success status.
  Status() : status_(rs::status::Status_new()) {}

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
  inline void PermitUncheckedError() const {
    // leaving as a no-op to denote which methods we can discard the result in once more is moved to rust
  }

  inline void MustCheck() const {
    // leaving as a no-op to denote which methods we need to check the result for once more is moved to rust
  }

  rs::status::Code code() const {
    return status_.code();
  }

  rs::status::SubCode subcode() const {
    return status_.subcode();
  }

  Status(const Status& s, rs::status::Severity sev);

  Status(rs::status::Code _code, rs::status::SubCode _subcode, rs::status::Severity _sev, const Slice& msg)
      : Status(_code, _subcode, msg, "", _sev) {}

  static Status CopyAppendMessage(const Status& s, const Slice& delim,
                                  const Slice& msg);

  rs::status::Severity severity() const {
    return status_.severity();
  }

  // Returns a C style string indicating the message of the Status
  const char* getState() const {
    return status_.getState();
  }

  // Return a success status.
  static Status OK() { return Status(); }

  // Successful, though an existing something was overwritten
  // Note: using variants of OK status for program logic is discouraged,
  // but it can be useful for communicating statistical information without
  // changing public APIs.
  static Status OkOverwritten() { return Status(rs::status::Code::Ok, rs::status::SubCode::Overwritten); }

  // Return error status of an appropriate type.
  static Status NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::NotFound, msg, msg2);
  }

  // Fast path for not found without malloc;
  static Status NotFound(rs::status::SubCode msg = rs::status::SubCode::None) { return Status(rs::status::Code::NotFound, msg); }

  static Status NotFound(rs::status::SubCode sc, const Slice& msg,
                         const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::NotFound, sc, msg, msg2);
  }

  static Status Corruption(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::Corruption, msg, msg2);
  }
  static Status Corruption(rs::status::SubCode msg = rs::status::SubCode::None) {
    return Status(rs::status::Code::Corruption, msg);
  }

  static Status NotSupported(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::NotSupported, msg, msg2);
  }
  static Status NotSupported(rs::status::SubCode msg = rs::status::SubCode::None) {
    return Status(rs::status::Code::NotSupported, msg);
  }

  static Status InvalidArgument(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::InvalidArgument, msg, msg2);
  }
  static Status InvalidArgument(rs::status::SubCode msg = rs::status::SubCode::None) {
    return Status(rs::status::Code::InvalidArgument, msg);
  }

  static Status IOError(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::IOError, msg, msg2);
  }
  static Status IOError(rs::status::SubCode msg = rs::status::SubCode::None) { return Status(rs::status::Code::IOError, msg); }

  static Status MergeInProgress(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::MergeInProgress, msg, msg2);
  }
  static Status MergeInProgress(rs::status::SubCode msg = rs::status::SubCode::None) {
    return Status(rs::status::Code::MergeInProgress, msg);
  }

  static Status Incomplete(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::Incomplete, msg, msg2);
  }
  static Status Incomplete(rs::status::SubCode msg = rs::status::SubCode::None) {
    return Status(rs::status::Code::Incomplete, msg);
  }

  static Status ShutdownInProgress(rs::status::SubCode msg = rs::status::SubCode::None) {
    return Status(rs::status::Code::ShutdownInProgress, msg);
  }
  static Status ShutdownInProgress(const Slice& msg,
                                   const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::ShutdownInProgress, msg, msg2);
  }
  static Status Aborted(rs::status::SubCode msg = rs::status::SubCode::None) { return Status(rs::status::Code::Aborted, msg); }
  static Status Aborted(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::Aborted, msg, msg2);
  }

  static Status Busy(rs::status::SubCode msg = rs::status::SubCode::None) { return Status(rs::status::Code::Busy, msg); }
  static Status Busy(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::Busy, msg, msg2);
  }

  static Status TimedOut(rs::status::SubCode msg = rs::status::SubCode::None) { return Status(rs::status::Code::TimedOut, msg); }

  static Status Expired(rs::status::SubCode msg = rs::status::SubCode::None) { return Status(rs::status::Code::Expired, msg); }

  static Status TryAgain(rs::status::SubCode msg = rs::status::SubCode::None) { return Status(rs::status::Code::TryAgain, msg); }
  static Status TryAgain(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::TryAgain, msg, msg2);
  }

  static Status CompactionTooLarge(rs::status::SubCode msg = rs::status::SubCode::None) {
    return Status(rs::status::Code::CompactionTooLarge, msg);
  }

  static Status ColumnFamilyDropped(rs::status::SubCode msg = rs::status::SubCode::None) {
    return Status(rs::status::Code::ColumnFamilyDropped, msg);
  }

  static Status ColumnFamilyDropped(const Slice& msg,
                                    const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::ColumnFamilyDropped, msg, msg2);
  }

  static Status NoSpace() { return Status(rs::status::Code::IOError, rs::status::SubCode::NoSpace); }
  static Status NoSpace(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::IOError, rs::status::SubCode::NoSpace, msg, msg2);
  }

  static Status MemoryLimit() { return Status(rs::status::Code::Aborted, rs::status::SubCode::MemoryLimit); }
  static Status MemoryLimit(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::Aborted, rs::status::SubCode::MemoryLimit, msg, msg2);
  }

  static Status SpaceLimit(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::IOError, rs::status::SubCode::SpaceLimit, msg, msg2);
  }

  static Status PathNotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(rs::status::Code::IOError, rs::status::SubCode::PathNotFound, msg, msg2);
  }

  static Status TxnNotPrepared() {
    return Status(rs::status::Code::InvalidArgument, rs::status::SubCode::TxnNotPrepared);
  }

  // Returns true iff the status indicates success.
  bool ok() const {
    return code() == rs::status::Code::Ok;
  }

  // Returns true iff the status indicates success *with* something
  // overwritten
  bool IsOkOverwritten() const {
    return code() == rs::status::Code::Ok && subcode() == rs::status::SubCode::Overwritten;
  }

  // Returns true iff the status indicates a NotFound error.
  bool IsNotFound() const {
    return code() == rs::status::Code::NotFound;
  }

  // Returns true iff the status indicates a Corruption error.
  bool IsCorruption() const {
    return code() == rs::status::Code::Corruption;
  }

  // Returns true iff the status indicates a NotSupported error.
  bool IsNotSupported() const {
    return code() == rs::status::Code::NotSupported;
  }

  // Returns true iff the status indicates an InvalidArgument error.
  bool IsInvalidArgument() const {
    return code() == rs::status::Code::InvalidArgument;
  }

  // Returns true iff the status indicates an IOError.
  bool IsIOError() const {
    return code() == rs::status::Code::IOError;
  }

  // Returns true iff the status indicates an MergeInProgress.
  bool IsMergeInProgress() const {
    return code() == rs::status::Code::MergeInProgress;
  }

  // Returns true iff the status indicates Incomplete
  bool IsIncomplete() const {
    return code() == rs::status::Code::Incomplete;
  }

  // Returns true iff the status indicates Shutdown In progress
  bool IsShutdownInProgress() const {
    return code() == rs::status::Code::ShutdownInProgress;
  }

  bool IsTimedOut() const {
    return code() == rs::status::Code::TimedOut;
  }

  bool IsAborted() const {
    return code() == rs::status::Code::Aborted;
  }

  // Returns true iff the status indicates that a resource is Busy and
  // temporarily could not be acquired.
  bool IsBusy() const {
    return code() == rs::status::Code::Busy;
  }

  bool IsDeadlock() const {
    return code() == rs::status::Code::Busy && subcode() == rs::status::SubCode::Deadlock;
  }

  // Returns true iff the status indicated that the operation has Expired.
  bool IsExpired() const {
    return code() == rs::status::Code::Expired;
  }

  // Returns true iff the status indicates a TryAgain error.
  // This usually means that the operation failed, but may succeed if
  // re-attempted.
  bool IsTryAgain() const {
    return code() == rs::status::Code::TryAgain;
  }

  // Returns true iff the status indicates the proposed compaction is too large
  bool IsCompactionTooLarge() const {
    return code() == rs::status::Code::CompactionTooLarge;
  }

  // Returns true iff the status indicates Column Family Dropped
  bool IsColumnFamilyDropped() const {
    return code() == rs::status::Code::ColumnFamilyDropped;
  }

  // Returns true iff the status indicates a NoSpace error
  // This is caused by an I/O error returning the specific "out of space"
  // error condition. Stricto sensu, an NoSpace error is an I/O error
  // with a specific subcode, enabling users to take the appropriate action
  // if needed
  bool IsNoSpace() const {
    return (code() == rs::status::Code::IOError) && (subcode() == rs::status::SubCode::NoSpace);
  }

  // Returns true iff the status indicates a memory limit error.  There may be
  // cases where we limit the memory used in certain operations (eg. the size
  // of a write batch) in order to avoid out of memory exceptions.
  bool IsMemoryLimit() const {
    return (code() == rs::status::Code::Aborted) && (subcode() == rs::status::SubCode::MemoryLimit);
  }

  // Returns true iff the status indicates a PathNotFound error
  // This is caused by an I/O error returning the specific "no such file or
  // directory" error condition. A PathNotFound error is an I/O error with
  // a specific subcode, enabling users to take appropriate action if necessary
  bool IsPathNotFound() const {
    return (code() == rs::status::Code::IOError || code() == rs::status::Code::NotFound) &&
           (subcode() == rs::status::SubCode::PathNotFound);
  }

  // Returns true iff the status indicates manual compaction paused. This
  // is caused by a call to PauseManualCompaction
  bool IsManualCompactionPaused() const {
    return (code() == rs::status::Code::Incomplete) && (subcode() == rs::status::SubCode::ManualCompactionPaused);
  }

  // Returns true iff the status indicates a TxnNotPrepared error.
  bool IsTxnNotPrepared() const {
    return (code() == rs::status::Code::InvalidArgument) && (subcode() == rs::status::SubCode::TxnNotPrepared);
  }

  // Returns true iff the status indicates a IOFenced error.
  bool IsIOFenced() const {
    return (code() == rs::status::Code::IOError) && (subcode() == rs::status::SubCode::IOFenced);
  }

  // Return a string representation of this status suitable for printing.
  // Returns the string "OK" for success.
  std::string ToString() const;

 protected:
    friend class IOStatus;
    explicit Status(rs::status::Code _code, rs::status::SubCode _subcode = rs::status::SubCode::None)
            : status_(rs::status::Status_new(_code, _subcode)) {}

    explicit Status(rs::status::Code _code, rs::status::SubCode _subcode, bool retryable, bool data_loss,
                    unsigned char scope)
            : status_(rs::status::Status_new(_code, _subcode, retryable, data_loss, scope)) {}

    Status(rs::status::Code _code, rs::status::SubCode _subcode, const Slice& msg, const Slice& msg2,
           rs::status::Severity sev = rs::status::Severity::NoError);
    Status(rs::status::Code _code, const Slice& msg, const Slice& msg2)
            : Status(_code, rs::status::SubCode::None, msg, msg2) {}
    rs::status::Status status_;
};

inline Status::Status(const Status& s)
    : status_(s.status_) {}

inline Status::Status(const Status& s, rs::status::Severity sev)
    : status_(s.status_) {
  status_.sev = sev;
}
inline Status& Status::operator=(const Status& s) {
  if (this != &s) {
    MustCheck();
    status_.code_ = s.status_.code();
    status_.subcode_ = s.status_.subcode();
    status_.sev = s.status_.severity();
    status_.retryable = s.status_.retryable;
    status_.data_loss = s.status_.data_loss;
    status_.scope = s.status_.scope;
    status_.state = s.status_.state;
  }
  return *this;
}

inline Status::Status(Status&& s) noexcept : Status() {
  *this = std::move(s);
}

inline Status& Status::operator=(Status&& s) noexcept {
  if (this != &s) {
    MustCheck();
    status_.code_ = s.status_.code_;
    s.status_.code_ = rs::status::Code::Ok;
    status_.subcode_ = s.status_.subcode_;
    s.status_.subcode_ = rs::status::SubCode::None;
    status_.sev = s.status_.sev;
    s.status_.sev = rs::status::Severity::NoError;
    status_.retryable = s.status_.retryable;
    s.status_.retryable = false;
    status_.data_loss = s.status_.data_loss;
    s.status_.data_loss = false;
    status_.scope = s.status_.scope;
    s.status_.scope = 0;
    status_.state = std::move(s.status_.state);
  }
  return *this;
}

inline bool Status::operator==(const Status& rhs) const {
  return (status_.code_ == rhs.status_.code_);
}

inline bool Status::operator!=(const Status& rhs) const {
  return !(*this == rhs);
}

}  // namespace ROCKSDB_NAMESPACE
