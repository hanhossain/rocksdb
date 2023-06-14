// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Slice is a simple structure containing a pointer into some external
// storage and a size.  The user of a Slice must ensure that the slice
// is not used after the corresponding external storage has been
// deallocated.
//
// Multiple threads can invoke const methods on a Slice without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Slice must use
// external synchronization.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>  // RocksDB now requires C++17 support

#include "rocksdb-sys-cxx/lib.h"
#include "rocksdb/cleanable.h"
#include "rust/cxx.h"

namespace ROCKSDB_NAMESPACE {

class Slice {
 public:
  // Create an empty slice.
  Slice() : slice_(rs::slice::Slice_new()) {}

  // Create a slice that refers to d[0,n-1].
  Slice(const char* d, size_t n) : slice_(rs::slice::Slice_new(d, n)) {}

  // Create a slice that refers to the contents of "s"
  /* implicit */
  Slice(const std::string& s) : Slice(s.data(), s.size()) {}

  Slice(const rust::Vec<uint8_t>& v) : Slice((const char*)v.data(), v.size()) {}

  Slice(const rust::Vec<char>& v) : Slice(v.data(), v.size()) {}

  // Create a slice that refers to the same contents as "sv"
  /* implicit */
  Slice(const std::string_view& sv) : Slice(sv.data(), sv.size()) {}

  // Create a slice that refers to s[0,strlen(s)-1]
  /* implicit */
  Slice(const char* s) : slice_(rs::slice::Slice_new(s)) {}

  // Create a single slice from SliceParts using buf as storage.
  // buf must exist as long as the returned Slice exists.
  Slice(const struct SliceParts& parts, std::string* buf);

  void set_data(const char* data) { slice_.set_data(data); }

  // Return a pointer to the beginning of the referenced data
  const char* data() const { return slice_.data(); }

  // Return the length (in bytes) of the referenced data
  size_t size() const { return slice_.size(); }

  void set_size(size_t size) { slice_.set_size(size); }

  // Return true iff the length of the referenced data is zero
  bool empty() const { return slice_.size() == 0; }

  // Return the ith byte in the referenced data.
  // REQUIRES: n < size()
  char operator[](size_t n) const {
    assert(n < size());
    return slice_.data()[n];
  }

  // Change this slice to refer to an empty array
  void clear() {
    slice_.set_data("");
    slice_.set_size(0);
  }

  // Drop the first "n" bytes from this slice.
  void remove_prefix(size_t n) {
    assert(n <= size());
    slice_.set_data(slice_.data() + n);
    slice_.set_size(slice_.size() - n);
  }

  void remove_suffix(size_t n) {
    assert(n <= size());
    slice_.set_size(slice_.size() - n);
  }

  // Return a string that contains the copy of the referenced data.
  // when hex is true, returns a string of twice the length hex encoded (0-9A-F)
  std::string ToString(bool hex = false) const;

  // Return a string_view that references the same data as this slice.
  std::string_view ToStringView() const {
    return std::string_view(slice_.data(), slice_.size());
  }

  // Decodes the current slice interpreted as an hexadecimal string into result,
  // if successful returns true, if this isn't a valid hex string
  // (e.g not coming from Slice::ToString(true)) DecodeHex returns false.
  // This slice is expected to have an even number of 0-9A-F characters
  // also accepts lowercase (a-f)
  bool DecodeHex(std::string* result) const;

  // Three-way comparison.  Returns value:
  //   <  0 iff "*this" <  "b",
  //   == 0 iff "*this" == "b",
  //   >  0 iff "*this" >  "b"
  int compare(const Slice& b) const;

  // Return true iff "x" is a prefix of "*this"
  bool starts_with(const Slice& x) const {
    return ((slice_.size() >= x.slice_.size()) && (memcmp(slice_.data(), x.slice_.data(), x.slice_.size()) == 0));
  }

  bool ends_with(const Slice& x) const {
    return ((slice_.size() >= x.slice_.size()) &&
            (memcmp(slice_.data() + slice_.size() - x.slice_.size(), x.slice_.data(), x.slice_.size()) == 0));
  }

  // Compare two slices and returns the first byte where they differ
  size_t difference_offset(const Slice& b) const;

  // private: make these public for rocksdbjni access
private:
  rs::slice::Slice slice_;

  // Intentionally copyable
};

/**
 * A Slice that can be pinned with some cleanup tasks, which will be run upon
 * ::Reset() or object destruction, whichever is invoked first. This can be used
 * to avoid memcpy by having the PinnableSlice object referring to the data
 * that is locked in the memory and release them after the data is consumed.
 */
class PinnableSlice : public Slice, public Cleanable {
 public:
  PinnableSlice() { buf_ = &self_space_; }
  explicit PinnableSlice(std::string* buf) { buf_ = buf; }

  PinnableSlice(PinnableSlice&& other);
  PinnableSlice& operator=(PinnableSlice&& other);

  // No copy constructor and copy assignment allowed.
  PinnableSlice(PinnableSlice&) = delete;
  PinnableSlice& operator=(PinnableSlice&) = delete;

  inline void PinSlice(const Slice& s, CleanupFunction f, void* arg1,
                       void* arg2) {
    assert(!pinned_);
    pinned_ = true;
    set_data(s.data());
    set_size(s.size());
    RegisterCleanup(f, arg1, arg2);
    assert(pinned_);
  }

  inline void PinSlice(const Slice& s, Cleanable* cleanable) {
    assert(!pinned_);
    pinned_ = true;
    set_data(s.data());
    set_size(s.size());
    if (cleanable != nullptr) {
      cleanable->DelegateCleanupsTo(this);
    }
    assert(pinned_);
  }

  inline void PinSelf(const Slice& slice) {
    assert(!pinned_);
    buf_->assign(slice.data(), slice.size());
    set_data(buf_->data());
    set_size(buf_->size());
    assert(!pinned_);
  }

  inline void PinSelf() {
    assert(!pinned_);
    set_data(buf_->data());
    set_size(buf_->size());
    assert(!pinned_);
  }

  void remove_suffix(size_t n) {
    assert(n <= size());
    if (pinned_) {
      set_size(size() - n);
    } else {
      buf_->erase(size() - n, n);
      PinSelf();
    }
  }

  void remove_prefix(size_t n) {
    assert(n <= size());
    if (pinned_) {
      set_data(data() + n);
      set_size(size() - n);
    } else {
      buf_->erase(0, n);
      PinSelf();
    }
  }

  void Reset() {
    Cleanable::Reset();
    pinned_ = false;
    set_size(0);
  }

  inline std::string* GetSelf() { return buf_; }

  inline bool IsPinned() const { return pinned_; }

 private:
  friend class PinnableSlice4Test;
  std::string self_space_;
  std::string* buf_;
  bool pinned_ = false;
};

// A set of Slices that are virtually concatenated together.  'parts' points
// to an array of Slices.  The number of elements in the array is 'num_parts'.
struct SliceParts {
  SliceParts(const Slice* _parts, int _num_parts)
      : parts(_parts), num_parts(_num_parts) {}
  SliceParts() : parts(nullptr), num_parts(0) {}

  const Slice* parts;
  int num_parts;
};

inline bool operator==(const Slice& x, const Slice& y) {
  return ((x.size() == y.size()) &&
          (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const Slice& x, const Slice& y) { return !(x == y); }

inline int Slice::compare(const Slice& b) const {
  assert(slice_.data() != nullptr && b.slice_.data() != nullptr);
  const size_t min_len = (slice_.size() < b.slice_.size()) ? slice_.size() : b.slice_.size();
  int r = memcmp(slice_.data(), b.slice_.data(), min_len);
  if (r == 0) {
    if (slice_.size() < b.slice_.size())
      r = -1;
    else if (slice_.size() > b.slice_.size())
      r = +1;
  }
  return r;
}

inline size_t Slice::difference_offset(const Slice& b) const {
  size_t off = 0;
  const size_t len = (slice_.size() < b.slice_.size()) ? slice_.size() : b.slice_.size();
  for (; off < len; off++) {
    if (slice_.data()[off] != b.slice_.data()[off]) break;
  }
  return off;
}

}  // namespace ROCKSDB_NAMESPACE
