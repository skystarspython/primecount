///
/// @file  BitSieve.cpp
/// @brief The BitSieve class is a bit array for use with
///        Eratosthenes-like prime sieving algorithms. BitSieve
///        assigns 64 numbers to the bits of an 8 byte word thus
///        reducing the memory usage by a factor of 8.
///
/// Copyright (C) 2016 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#if !defined(__STDC_CONSTANT_MACROS)
  #define __STDC_CONSTANT_MACROS
#endif

#include <BitSieve.hpp>
#include <popcount.hpp>
#include <pmath.hpp>

#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

using namespace std;

namespace {

// 1-indexing: primes[1] = 2, primes[2] = 3, ...
const uint64_t primes[] = { 0, 2, 3, 5, 7, 11, 13, 17, 19 };

}

namespace primecount {

const uint64_t BitSieve::unset_bit_[64] =
{
  ~(UINT64_C(1) <<  0), ~(UINT64_C(1) <<  1), ~(UINT64_C(1) <<  2),
  ~(UINT64_C(1) <<  3), ~(UINT64_C(1) <<  4), ~(UINT64_C(1) <<  5),
  ~(UINT64_C(1) <<  6), ~(UINT64_C(1) <<  7), ~(UINT64_C(1) <<  8),
  ~(UINT64_C(1) <<  9), ~(UINT64_C(1) << 10), ~(UINT64_C(1) << 11),
  ~(UINT64_C(1) << 12), ~(UINT64_C(1) << 13), ~(UINT64_C(1) << 14),
  ~(UINT64_C(1) << 15), ~(UINT64_C(1) << 16), ~(UINT64_C(1) << 17),
  ~(UINT64_C(1) << 18), ~(UINT64_C(1) << 19), ~(UINT64_C(1) << 20),
  ~(UINT64_C(1) << 21), ~(UINT64_C(1) << 22), ~(UINT64_C(1) << 23),
  ~(UINT64_C(1) << 24), ~(UINT64_C(1) << 25), ~(UINT64_C(1) << 26),
  ~(UINT64_C(1) << 27), ~(UINT64_C(1) << 28), ~(UINT64_C(1) << 29),
  ~(UINT64_C(1) << 30), ~(UINT64_C(1) << 31), ~(UINT64_C(1) << 32),
  ~(UINT64_C(1) << 33), ~(UINT64_C(1) << 34), ~(UINT64_C(1) << 35),
  ~(UINT64_C(1) << 36), ~(UINT64_C(1) << 37), ~(UINT64_C(1) << 38),
  ~(UINT64_C(1) << 39), ~(UINT64_C(1) << 40), ~(UINT64_C(1) << 41),
  ~(UINT64_C(1) << 42), ~(UINT64_C(1) << 43), ~(UINT64_C(1) << 44),
  ~(UINT64_C(1) << 45), ~(UINT64_C(1) << 46), ~(UINT64_C(1) << 47),
  ~(UINT64_C(1) << 48), ~(UINT64_C(1) << 49), ~(UINT64_C(1) << 50),
  ~(UINT64_C(1) << 51), ~(UINT64_C(1) << 52), ~(UINT64_C(1) << 53),
  ~(UINT64_C(1) << 54), ~(UINT64_C(1) << 55), ~(UINT64_C(1) << 56),
  ~(UINT64_C(1) << 57), ~(UINT64_C(1) << 58), ~(UINT64_C(1) << 59),
  ~(UINT64_C(1) << 60), ~(UINT64_C(1) << 61), ~(UINT64_C(1) << 62),
  ~(UINT64_C(1) << 63)
};

BitSieve::BitSieve(std::size_t size) :
  sieve_(ceil_div(size, 64)),
  size_(size)
{ }

/// Pre-sieve the multiples (>= low) of the first c primes.
/// @cross_off_primes  Use false to cross-off multiples,
///                    Use true  to cross-off multiples and primes.
/// @pre c < 9
///
void BitSieve::pre_sieve(uint64_t c,
                         uint64_t low,
                         bool sieve_primes)
{
  assert(c < 9);

  if (!sieve_.empty())
  {
    // unset multiples of 2 in first word
    sieve_[0] = UINT64_C(0xAAAAAAAAAAAAAAAA) >> (low % 2);

    uint64_t sieved = 1;
    uint64_t base = 1;
    uint64_t sieve_size = sieve_.size();

    // pre-sieve multiples of the first c primes
    for (uint64_t i = 2; i <= c; i++)
    {
      uint64_t prime = primes[i];
      uint64_t end_copy = min(sieved * prime, sieve_size);

      // pre-sieve multiples of primes < i-th prime
      // by copying a small pre-sieved buffer
      while (base < end_copy)
      {
        uint64_t last = min(sieved, sieve_size - base);
        copy(sieve_.begin(), sieve_.begin() + last, sieve_.begin() + base);
        base += last;
      }

      // calculate first multiple >= low of prime
      uint64_t next_multiple = ceil_div(low, prime) * prime;
      next_multiple += prime * (~next_multiple & 1);
      uint64_t start = next_multiple - low;
      uint64_t stop = base * 64;

      // cross-off multiples of prime inside [low, low + stop[
      for (uint64_t j = start; j < stop; j += prime * 2)
        unset(j);

      sieved = base;
    }

    // fill up the rest of the sieve
    while (base < sieve_size)
    {
      uint64_t last = min(sieved, sieve_size - base);
      copy(sieve_.begin(), sieve_.begin() + last, sieve_.begin() + base);
      base += last;
    }

    if (sieve_primes)
    {
      // reset first c primes previously unset 
      for (uint64_t i = c; i > 0 && primes[i] >= low; i--)
        set(primes[i] - low);
    }
  }
}

/// Count the number of 1 bits inside [start, stop]
uint64_t BitSieve::count(uint64_t start,
                         uint64_t stop) const
{
  if (start > stop)
    return 0;

  assert(stop < size_);

  uint64_t start_idx = start / 64;
  uint64_t stop_idx = stop / 64;
  uint64_t m1 = UINT64_C(0xffffffffffffffff) << (start % 64);
  uint64_t m2 = UINT64_C(0xffffffffffffffff) >> (63 - stop % 64);
  uint64_t bit_count;

  if (start_idx == stop_idx)
    bit_count = popcount_u64(sieve_[start_idx] & (m1 & m2));
  else
  {
    bit_count = popcount_u64(sieve_[start_idx] & m1);
    bit_count += popcount_u64(&sieve_[start_idx + 1], stop_idx - (start_idx + 1));
    bit_count += popcount_u64(sieve_[stop_idx] & m2);
  }

  return bit_count;
}

} // namespace
