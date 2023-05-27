//  Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <assert.h>

#include <cstdint>
#include <type_traits>

#include "rocksdb-rs-cxx/lib.h"
#include "rocksdb/rocksdb_namespace.h"

namespace ROCKSDB_NAMESPACE {

// Every output bit depends on many input bits in the same and higher
// positions, but not lower positions. Specifically, this function
// * Output highest bit set to 1 is same as input (same FloorLog2, or
//   equivalently, same number of leading zeros)
// * Is its own inverse (an involution)
// * Guarantees that b bottom bits of v and c bottom bits of
//   DownwardInvolution(v) uniquely identify b + c bottom bits of v
//   (which is all of v if v < 2**(b + c)).
// ** A notable special case is that modifying c adjacent bits at
//    some chosen position in the input is bijective with the bottom c
//    output bits.
// * Distributes over xor, as in DI(a ^ b) == DI(a) ^ DI(b)
//
// This transformation is equivalent to a matrix*vector multiplication in
// GF(2) where the matrix is recursively defined by the pattern matrix
// P = | 1 1 |
//     | 0 1 |
// and replacing 1's with P and 0's with 2x2 zero matices to some depth,
// e.g. depth of 6 for 64-bit T. An essential feature of this matrix
// is that all square sub-matrices that include the top row are invertible.
template <typename T>
inline T DownwardInvolution(T v) {
  static_assert(std::is_integral<T>::value, "non-integral type");
  static_assert(sizeof(T) <= 8, "only supported up to 64 bits");

  uint64_t r = static_cast<uint64_t>(v);
  if constexpr (sizeof(T) > 4) {
    r ^= r >> 32;
  }
  if constexpr (sizeof(T) > 2) {
    r ^= (r & 0xffff0000ffff0000U) >> 16;
  }
  if constexpr (sizeof(T) > 1) {
    r ^= (r & 0xff00ff00ff00ff00U) >> 8;
  }
  r ^= (r & 0xf0f0f0f0f0f0f0f0U) >> 4;
  r ^= (r & 0xccccccccccccccccU) >> 2;
  r ^= (r & 0xaaaaaaaaaaaaaaaaU) >> 1;
  return static_cast<T>(r);
}

}  // namespace ROCKSDB_NAMESPACE
