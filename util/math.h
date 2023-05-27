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

// Number of low-order zero bits before the first 1 bit. Undefined for 0.
template <typename T>
inline int CountTrailingZeroBits(T v) {
  static_assert(std::is_integral<T>::value, "non-integral type");
  assert(v != 0);
  static_assert(sizeof(T) <= sizeof(unsigned long long), "type too big");
  if (sizeof(T) <= sizeof(unsigned int)) {
    return __builtin_ctz(static_cast<unsigned int>(v));
  } else if (sizeof(T) <= sizeof(unsigned long)) {
    return __builtin_ctzl(static_cast<unsigned long>(v));
  } else {
    return __builtin_ctzll(static_cast<unsigned long long>(v));
  }
}

// Number of bits set to 1. Also known as "population count".
template <typename T>
inline int BitsSetToOne(T v) {
  static_assert(std::is_integral<T>::value, "non-integral type");
  static_assert(sizeof(T) <= sizeof(unsigned long long), "type too big");
  if (sizeof(T) < sizeof(unsigned int)) {
    // This bit mask is to avoid a compiler warning on unused path
    constexpr auto mm = 8 * sizeof(unsigned int) - 1;
    // This bit mask is to neutralize sign extension on small signed types
    constexpr unsigned int m = (1U << ((8 * sizeof(T)) & mm)) - 1;
    return __builtin_popcount(static_cast<unsigned int>(v) & m);
  } else if (sizeof(T) == sizeof(unsigned int)) {
    return __builtin_popcount(static_cast<unsigned int>(v));
  } else if (sizeof(T) <= sizeof(unsigned long)) {
    return __builtin_popcountl(static_cast<unsigned long>(v));
  } else {
    return __builtin_popcountll(static_cast<unsigned long long>(v));
  }
}

template <typename T>
inline int BitParity(T v) {
  static_assert(std::is_integral<T>::value, "non-integral type");
  static_assert(sizeof(T) <= sizeof(unsigned long long), "type too big");
  if (sizeof(T) <= sizeof(unsigned int)) {
    // On any sane systen, potential sign extension here won't change parity
    return __builtin_parity(static_cast<unsigned int>(v));
  } else if (sizeof(T) <= sizeof(unsigned long)) {
    return __builtin_parityl(static_cast<unsigned long>(v));
  } else {
    return __builtin_parityll(static_cast<unsigned long long>(v));
  }
}

// Swaps between big and little endian. Can be used in combination with the
// little-endian encoding/decoding functions in coding_lean.h and coding.h to
// encode/decode big endian.
template <typename T>
inline T EndianSwapValue(T v) {
  static_assert(std::is_integral<T>::value, "non-integral type");

  if (sizeof(T) == 2) {
    return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(v)));
  } else if (sizeof(T) == 4) {
    return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(v)));
  } else if (sizeof(T) == 8) {
    return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(v)));
  }
  // Recognized by clang as bswap, but not by gcc :(
  T ret_val = 0;
  for (std::size_t i = 0; i < sizeof(T); ++i) {
    ret_val |= ((v >> (8 * i)) & 0xff) << (8 * (sizeof(T) - 1 - i));
  }
  return ret_val;
}

// Reverses the order of bits in an integral value
template <typename T>
inline T ReverseBits(T v) {
  T r = EndianSwapValue(v);
  const T kHighestByte = T{1} << ((sizeof(T) - 1) * 8);
  const T kEveryByte = kHighestByte | (kHighestByte / 255);

  r = ((r & (kEveryByte * 0x0f)) << 4) | ((r >> 4) & (kEveryByte * 0x0f));
  r = ((r & (kEveryByte * 0x33)) << 2) | ((r >> 2) & (kEveryByte * 0x33));
  r = ((r & (kEveryByte * 0x55)) << 1) | ((r >> 1) & (kEveryByte * 0x55));

  return r;
}

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
