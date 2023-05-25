//  Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once


// This file declares a wrapper around the efficient folly DistributedMutex
// that falls back on a standard mutex when not available. See
// https://github.com/facebook/folly/blob/main/folly/synchronization/DistributedMutex.h
// for benefits and limitations.

// At the moment, only scoped locking is supported using DMutexLock
// RAII wrapper, because lock/unlock APIs will vary.

#ifdef USE_FOLLY

#include <folly/synchronization/DistributedMutex.h>

namespace rocksdb {

class DMutex : public folly::DistributedMutex {
 public:
  static const char* kName() { return "folly::DistributedMutex"; }

  explicit DMutex(bool IGNORED_adaptive = false) { (void)IGNORED_adaptive; }

  // currently no-op
  void AssertHeld() {}
};
using DMutexLock = std::lock_guard<folly::DistributedMutex>;

}  // namespace rocksdb

#else

#include <mutex>

#include "port/port.h"

namespace rocksdb {

using DMutex = port::Mutex;
using DMutexLock = std::lock_guard<DMutex>;

}  // namespace rocksdb

#endif
