/* Clear But Complex // Kindred
 * Fixed-rate bridge: runs 48 kHz block DSP (Mutable Instruments engines) at any host rate.
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 */

#pragma once
#ifndef KINDRED_RATE_BRIDGE_H
#define KINDRED_RATE_BRIDGE_H

#include <algorithm>
#include <cstring>

namespace kindred {

  const double kEngineRate = 48000.0;

  // Fixed-capacity single-threaded FIFO. No allocation on the audio thread.
  template <int kCapacity>
  class Fifo {
    public:
      Fifo() : read_(0), size_(0) { std::memset(data_, 0, sizeof(data_)); }
      void clear() { read_ = 0; size_ = 0; }
      int size() const { return size_; }
      int space() const { return kCapacity - size_; }
      void push(float value) {
        if (size_ >= kCapacity)
          return;
        data_[(read_ + size_) % kCapacity] = value;
        ++size_;
      }
      float pop() {
        if (size_ == 0)
          return 0.0f;
        float value = data_[read_];
        read_ = (read_ + 1) % kCapacity;
        --size_;
        return value;
      }

    private:
      float data_[kCapacity];
      int read_;
      int size_;
  };

  // Streaming 4-point Hermite resampler. step = input rate / output rate.
  // With step == 1 it is an exact (2-sample delayed) pass-through.
  class HermiteResampler {
    public:
      HermiteResampler() { reset(1.0); }

      void reset(double step) {
        step_ = step;
        phase_ = 0.0;
        x0_ = x1_ = x2_ = x3_ = 0.0f;
      }

      // Pushes one input sample and appends every output sample it completes.
      template <class Sink>
      void push(float in, Sink& sink) {
        x0_ = x1_;
        x1_ = x2_;
        x2_ = x3_;
        x3_ = in;
        while (phase_ < 1.0) {
          sink.push(interpolate(static_cast<float>(phase_)));
          phase_ += step_;
        }
        phase_ -= 1.0;
      }

    private:
      inline float interpolate(float t) const {
        float c0 = x1_;
        float c1 = 0.5f * (x2_ - x0_);
        float c2 = x0_ - 2.5f * x1_ + 2.0f * x2_ - 0.5f * x3_;
        float c3 = 0.5f * (x3_ - x0_) + 1.5f * (x1_ - x2_);
        return ((c3 * t + c2) * t + c1) * t + c0;
      }

      double step_;
      double phase_;
      float x0_, x1_, x2_, x3_;
  };

  // Runs a fixed-48k block effect (input -> output) at the host rate.
  // Adds a few milliseconds of latency, constant for a given rate.
  template <int kBlock>
  class EffectBridge {
    public:
      EffectBridge() : host_rate_(kEngineRate) { reset(kEngineRate); }

      void reset(double host_rate) {
        host_rate_ = host_rate;
        up_.reset(host_rate / kEngineRate);
        down_.reset(kEngineRate / host_rate);
        in48_.clear();
        out_host_.clear();
        int latency = static_cast<int>(2.0 * kBlock * host_rate / kEngineRate) + 8;
        for (int i = 0; i < latency; ++i)
          out_host_.push(0.0f);
      }

      // block(in, out, n) processes n == kBlock samples at 48 kHz.
      template <class BlockFn>
      void process(const float* in, float* out, int n, BlockFn block) {
        for (int i = 0; i < n; ++i) {
          up_.push(in[i], in48_);
          while (in48_.size() >= kBlock) {
            float block_in[kBlock];
            float block_out[kBlock];
            for (int k = 0; k < kBlock; ++k)
              block_in[k] = in48_.pop();
            block(block_in, block_out, kBlock);
            for (int k = 0; k < kBlock; ++k)
              down_.push(block_out[k], out_host_);
          }
          out[i] = out_host_.pop();
        }
      }

    private:
      double host_rate_;
      HermiteResampler up_;
      HermiteResampler down_;
      Fifo<4096> in48_;
      Fifo<8192> out_host_;
  };

  // Runs a fixed-48k block generator (no input) at the host rate.
  template <int kBlock>
  class GeneratorBridge {
    public:
      GeneratorBridge() { reset(kEngineRate); }

      void reset(double host_rate) {
        down_.reset(kEngineRate / host_rate);
        out_host_.clear();
      }

      template <class BlockFn>
      void process(float* out, int n, BlockFn block) {
        while (out_host_.size() < n) {
          float block_out[kBlock];
          block(block_out, kBlock);
          for (int k = 0; k < kBlock; ++k)
            down_.push(block_out[k], out_host_);
        }
        for (int i = 0; i < n; ++i)
          out[i] = out_host_.pop();
      }

    private:
      HermiteResampler down_;
      Fifo<8192> out_host_;
  };

} // namespace kindred

#endif // KINDRED_RATE_BRIDGE_H
