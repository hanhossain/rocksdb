// Copyright (c) 2019-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// An IOStatus encapsulates the result of an operation.  It may indicate
// success, or it may indicate an error with an associated error message.
//
// Multiple threads can invoke const methods on an IOStatus without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same IOStatus must use
// external synchronization.

#pragma once

#include <string>

#include "rocksdb/slice.h"
#ifdef OS_WIN
#include <string.h>
#endif
#include <cstring>

#include "status.h"

namespace ROCKSDB_NAMESPACE {

class IOStatus {
 public:
  enum IOErrorScope : unsigned char {
    kIOErrorScopeFileSystem,
    kIOErrorScopeFile,
    kIOErrorScopeRange,
    kIOErrorScopeMax,
  };

  // Create a success status.
  IOStatus() : IOStatus(rs::status::Code::Ok, rs::status::SubCode::None) {}
  ~IOStatus() {}

  // Copy the specified status.
  IOStatus(const IOStatus& s);
  IOStatus& operator=(const IOStatus& s);
  IOStatus(IOStatus&& s) noexcept;
  IOStatus& operator=(IOStatus&& s) noexcept;
  bool operator==(const IOStatus& rhs) const;
  bool operator!=(const IOStatus& rhs) const;

  void SetRetryable(bool retryable) { inner_status.status_.retryable = retryable; }
  void SetDataLoss(bool data_loss) { inner_status.status_.data_loss = data_loss; }
  void SetScope(IOErrorScope scope) {
    inner_status.status_.scope = static_cast<unsigned char>(scope);
  }

  bool GetRetryable() const { return inner_status.status_.retryable; }
  bool GetDataLoss() const { return inner_status.status_.data_loss; }
  IOErrorScope GetScope() const { return static_cast<IOErrorScope>(inner_status.status_.scope); }

  // Return a success status.
  static IOStatus OK() { return IOStatus(); }

  static IOStatus NotSupported(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::NotSupported, msg, msg2);
  }
  static IOStatus NotSupported(rs::status::SubCode msg = rs::status::SubCode::None) {
    return IOStatus(rs::status::Code::NotSupported, msg);
  }

  // Return error status of an appropriate type.
  static IOStatus NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::NotFound, msg, msg2);
  }
  // Fast path for not found without malloc;
  static IOStatus NotFound(rs::status::SubCode msg = rs::status::SubCode::None) {
    return IOStatus(rs::status::Code::NotFound, msg);
  }

  static IOStatus Corruption(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::Corruption, msg, msg2);
  }
  static IOStatus Corruption(rs::status::SubCode msg = rs::status::SubCode::None) {
    return IOStatus(rs::status::Code::Corruption, msg);
  }

  static IOStatus InvalidArgument(const Slice& msg,
                                  const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::InvalidArgument, msg, msg2);
  }
  static IOStatus InvalidArgument(rs::status::SubCode msg = rs::status::SubCode::None) {
    return IOStatus(rs::status::Code::InvalidArgument, msg);
  }

  static IOStatus IOError(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::IOError, msg, msg2);
  }
  static IOStatus IOError(rs::status::SubCode msg = rs::status::SubCode::None) {
    return IOStatus(rs::status::Code::IOError, msg);
  }

  static IOStatus Busy(rs::status::SubCode msg = rs::status::SubCode::None) { return IOStatus(rs::status::Code::Busy, msg); }
  static IOStatus Busy(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::Busy, msg, msg2);
  }

  static IOStatus TimedOut(rs::status::SubCode msg = rs::status::SubCode::None) {
    return IOStatus(rs::status::Code::TimedOut, msg);
  }
  static IOStatus TimedOut(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::TimedOut, msg, msg2);
  }

  static IOStatus NoSpace() { return IOStatus(rs::status::Code::IOError, rs::status::SubCode::NoSpace); }
  static IOStatus NoSpace(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::IOError, rs::status::SubCode::NoSpace, msg, msg2);
  }

  static IOStatus PathNotFound() { return IOStatus(rs::status::Code::IOError, rs::status::SubCode::PathNotFound); }
  static IOStatus PathNotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::IOError, rs::status::SubCode::PathNotFound, msg, msg2);
  }

  static IOStatus IOFenced() { return IOStatus(rs::status::Code::IOError, rs::status::SubCode::IOFenced); }
  static IOStatus IOFenced(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::IOError, rs::status::SubCode::IOFenced, msg, msg2);
  }

  static IOStatus Aborted(rs::status::SubCode msg = rs::status::SubCode::None) {
    return IOStatus(rs::status::Code::Aborted, msg);
  }
  static IOStatus Aborted(const Slice& msg, const Slice& msg2 = Slice()) {
    return IOStatus(rs::status::Code::Aborted, msg, msg2);
  }

  // Return a string representation of this status suitable for printing.
  // Returns the string "OK" for success.
  // std::string ToString() const;

  operator Status() const {
      return inner_status;
  }

  Status inner_status;

 private:
  friend IOStatus status_to_io_status(Status&&);

  explicit IOStatus(rs::status::Code _code, rs::status::SubCode _subcode = rs::status::SubCode::None)
    : inner_status(_code, _subcode, false, false, kIOErrorScopeFileSystem) {}

  IOStatus(rs::status::Code _code, rs::status::SubCode _subcode, const Slice& msg, const Slice& msg2);
  IOStatus(rs::status::Code _code, const Slice& msg, const Slice& msg2)
      : IOStatus(_code, rs::status::SubCode::None, msg, msg2) {}
};

inline IOStatus::IOStatus(rs::status::Code _code, rs::status::SubCode _subcode, const Slice& msg,const Slice& msg2)
: inner_status(_code, _subcode, false, false, kIOErrorScopeFileSystem) {
  assert(inner_status.status_.code_ != rs::status::Code::Ok);
  assert(inner_status.status_.subcode_ != rs::status::SubCode::MaxSubCode);
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
  inner_status.status_.set_state_unsafe(result);
}

inline IOStatus::IOStatus(const IOStatus& s)
    : inner_status(s.inner_status) {}

inline IOStatus& IOStatus::operator=(const IOStatus& s) {
  // The following condition catches both aliasing (when this == &s),
  // and the common case where both s and *this are ok.
  if (this != &s) {
    inner_status.status_.code_ = s.inner_status.status_.code_;
    inner_status.status_.subcode_ = s.inner_status.status_.subcode_;
    inner_status.status_.retryable = s.inner_status.status_.retryable;
    inner_status.status_.data_loss = s.inner_status.status_.data_loss;
    inner_status.status_.scope = s.inner_status.status_.scope;
    inner_status.status_.state = s.inner_status.status_.state;
  }
  return *this;
}

inline IOStatus::IOStatus(IOStatus&& s) noexcept : IOStatus() {
  *this = std::move(s);
}

inline IOStatus& IOStatus::operator=(IOStatus&& s) noexcept {
  if (this != &s) {
    inner_status.status_.code_ = s.inner_status.status_.code_;
    s.inner_status.status_.code_ = rs::status::Code::Ok;
    inner_status.status_.subcode_ = s.inner_status.status_.subcode_;
    s.inner_status.status_.subcode_ = rs::status::SubCode::None;
    inner_status.status_.retryable = s.inner_status.status_.retryable;
    inner_status.status_.data_loss = s.inner_status.status_.data_loss;
    inner_status.status_.scope = s.inner_status.status_.scope;
    s.inner_status.status_.scope = kIOErrorScopeFileSystem;
    inner_status.status_.state = std::move(s.inner_status.status_.state);
  }
  return *this;
}

inline bool IOStatus::operator==(const IOStatus& rhs) const {
  return (inner_status.status_.code_ == rhs.inner_status.status_.code_);
}

inline bool IOStatus::operator!=(const IOStatus& rhs) const {
  return !(*this == rhs);
}

inline IOStatus status_to_io_status(Status&& status) {
  IOStatus io_s;
  Status& s = io_s.inner_status;
  s = std::move(status);
  return io_s;
}

}  // namespace ROCKSDB_NAMESPACE
