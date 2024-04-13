#include "wrapping_integers.hh"
#include <algorithm>
#include <iostream>
#include <cstdint>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // absolute sequence number n and the zero point sequence number.
  // plus the zero point to the absolute sequence number n to get sequence number, 
  // nature modulo 2^32 intend to convert to 32 bit
  (void)n;
  (void)zero_point;
  return Wrap32 { uint32_t(zero_point.raw_value_ + n) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // zero point sequence number and a checkpoint absolute sequence number.
  // try to add uint32_max to zero point to get the base value
  // which is the closest to the checkpoint
  auto abs_diff = [&](uint64_t a, uint64_t b) -> uint64_t {
    return (a > b) ? a - b : b - a;
  };
  auto get_abs_diff_min = [&](uint64_t a, uint64_t b, uint64_t c) -> uint64_t {
    return (abs_diff(a , c) < abs_diff(b , c)) ? a : b; 
  };

  uint64_t mod = 1ULL << 32;
  uint64_t attempt_first = raw_value_ - zero_point.raw_value_;
  
  if(checkpoint > attempt_first) attempt_first += (checkpoint - attempt_first) / mod * mod;
  else attempt_first -= (attempt_first - checkpoint) / mod * mod;
  
  return get_abs_diff_min(attempt_first-mod,
  get_abs_diff_min(attempt_first, attempt_first + mod, checkpoint),
  checkpoint);
}
