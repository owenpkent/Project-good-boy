#pragma once
#include <stdint.h>
#include <algorithm>

namespace core {

// Header-only exponential backoff with jitter.
// Portable (no Arduino deps). Works on native and embedded.
class Backoff {
 public:
  Backoff(uint32_t initial_ms = 500,
          uint32_t max_ms = 30000,
          float factor = 2.0f,
          float jitter = 0.1f)
      : initial_(initial_ms),
        max_(max_ms),
        factor_(factor),
        jitter_(jitter),
        current_(initial_ms),
        rng_state_(0x12345678u) {}

  // Set PRNG seed to make sequences deterministic in tests
  void setSeed(uint32_t seed) { rng_state_ = seed ? seed : 0x12345678u; }

  void reset() { current_ = initial_; }

  // Returns the next delay in ms including jitter, then advances base by factor.
  uint32_t next() {
    const uint32_t base = current_;
    // Advance base for the next call (without jitter), capping at max
    double grown = static_cast<double>(current_) * static_cast<double>(factor_);
    if (grown < 1.0) grown = 1.0; // guard against weird factors
    current_ = static_cast<uint32_t>(grown + 0.999999); // ceil
    if (current_ < initial_) current_ = initial_;
    if (current_ > max_) current_ = max_;

    if (jitter_ <= 0.0f) return base;

    // Apply jitter: value in [base*(1-j), base*(1+j)]
    const double j = static_cast<double>(jitter_);
    const double span = static_cast<double>(base) * j;
    const double lo = static_cast<double>(base) - span;
    const double hi = static_cast<double>(base) + span;

    const double r01 = rnd01();
    double val = lo + (hi - lo) * r01;
    if (val < 0.0) val = 0.0;
    uint32_t out = static_cast<uint32_t>(val + 0.5); // round
    return out;
  }

  // Inspect the current base delay (without jitter) that will be used by next().
  uint32_t peek() const { return current_; }

  uint32_t initial() const { return initial_; }
  uint32_t maximum() const { return max_; }
  float factor() const { return factor_; }
  float jitter() const { return jitter_; }

 private:
  // Simple LCG for portability
  double rnd01() {
    rng_state_ = rng_state_ * 1664525u + 1013904223u;
    // Take upper bits, scale to [0,1)
    uint32_t x = (rng_state_ >> 8) & 0xFFFFFFu;
    return static_cast<double>(x) / static_cast<double>(0x1000000u);
  }

  uint32_t initial_;
  uint32_t max_;
  float factor_;
  float jitter_;
  uint32_t current_;
  uint32_t rng_state_;
};

} // namespace core
