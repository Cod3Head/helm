/* Clear But Complex // Kindred
 * Catalina's Character (implementation). Compiled via kindred_dsp.inc.
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 * RTNeural: Copyright Jatin Chowdhury, BSD 3-Clause License (src/RTNeural/LICENSE).
 */

#include "kindred/kindred_character.h"
#include "kindred/kindred_character_models.h"
#include "kindred/kindred_rate_bridge.h"
#include "utils.h"

#ifndef RTNEURAL_DEFAULT_ALIGNMENT
  #define RTNEURAL_DEFAULT_ALIGNMENT 16
#endif
#include "RTNeural/RTNeural.h"

#include <algorithm>
#include <cmath>

namespace mopo {

  namespace {
    // Window of recent samples -> two tanh layers -> one output sample.
    // Matches tools/kindred_train_character.py.
    typedef RTNeural::ModelT<float, kindred::kCharacterWindow, 1,
        RTNeural::DenseT<float, kindred::kCharacterWindow, kindred::kCharacterHidden>,
        RTNeural::TanhActivationT<float, kindred::kCharacterHidden>,
        RTNeural::DenseT<float, kindred::kCharacterHidden, kindred::kCharacterHidden>,
        RTNeural::TanhActivationT<float, kindred::kCharacterHidden>,
        RTNeural::DenseT<float, kindred::kCharacterHidden, 1>> CharacterNet;

    const int kBlock = 16;
    const float kLevelCoef = 1.0f / (0.15f * 48000.0f);  // ~150 ms level followers.
    const float kGainCoef = 1.0f / (0.05f * 48000.0f);   // ~50 ms gain smoothing.
  }

  int KindredCharacter::numModels() { return kindred::kNumCharacterModels; }

  const char* KindredCharacter::modelName(int model) {
    if (model < 0 || model >= kindred::kNumCharacterModels)
      return "";
    return kindred::kCharacterModels[model].name;
  }

  struct KindredCharacter::State {
    State() {
      for (int m = 0; m < kindred::kNumCharacterModels; ++m) {
        nets[m].parseJson(nlohmann::json::parse(kindred::kCharacterModels[m].json), false);
        nets[m].reset();
        input_scale[m] = kindred::kCharacterModels[m].input_scale;
        output_scale[m] = kindred::kCharacterModels[m].output_scale;
        // Idle output (silence in): subtracted so the stage is silent when the synth is.
        float zeros[kindred::kCharacterWindow] = {};
        idle[m] = nets[m].forward(zeros);
        nets[m].reset();
      }
      for (float& w : window)
        w = 0.0f;
    }

    CharacterNet nets[kindred::kNumCharacterModels];
    float input_scale[kindred::kNumCharacterModels];
    float output_scale[kindred::kNumCharacterModels];
    float idle[kindred::kNumCharacterModels];
    float window[kindred::kCharacterWindow];
    kindred::EffectBridge<kBlock> bridge;
    int model = 0;
    float drive = 1.0f;
    float dc_x = 0.0f;
    float dc_y = 0.0f;
    float level_in = 0.0f;    // Slow mean-square of the dry signal...
    float level_out = 0.0f;   // ...and of the shaped signal, for level matching.
    float gain = 1.0f;
    bool was_on = false;

    void renderBlock(const float* in, float* out, int n) {
      CharacterNet& net = nets[model];
      const float scale_in = input_scale[model] * drive;
      const float scale_out = output_scale[model];
      for (int i = 0; i < n; ++i) {
        for (int k = 0; k < kindred::kCharacterWindow - 1; ++k)
          window[k] = window[k + 1];
        window[kindred::kCharacterWindow - 1] = in[i] * scale_in;
        float y = (net.forward(window) - idle[model]) * scale_out;
        // ~10 Hz DC blocker at 48 kHz: asymmetric circuits shift the bias.
        float hp = y - dc_x + 0.9987f * dc_y;
        dc_x = y;
        dc_y = hp;
        // Level matching: Drive changes the tone, not the volume.
        level_in += kLevelCoef * (in[i] * in[i] - level_in);
        level_out += kLevelCoef * (hp * hp - level_out);
        float target = std::sqrt((level_in + 1e-9f) / (level_out + 1e-9f));
        target = std::min(4.0f, std::max(0.1f, target));
        gain += kGainCoef * (target - gain);
        out[i] = hp * gain;
      }
    }
  };

  KindredCharacter::KindredCharacter() : Processor(kNumInputs, 1), state_(new State()) { }

  KindredCharacter::KindredCharacter(const KindredCharacter& other)
      : Processor(other), state_(new State()) {
    state_->bridge.reset(other.getSampleRate() > 0 ? other.getSampleRate() : 44100);
  }

  KindredCharacter::~KindredCharacter() { }

  void KindredCharacter::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    state_->bridge.reset(sample_rate);
  }

  void KindredCharacter::process() {
    const mopo_float* audio = input(kAudio)->source->buffer;
    mopo_float* dest = output()->buffer;
    State& s = *state_;

    if (input(kOn)->source->buffer[0] <= 0.5) {
      utils::copyBuffer(dest, audio, buffer_size_);
      s.was_on = false;
      return;
    }

    int model = utils::iclamp(static_cast<int>(input(kModel)->source->buffer[0] + 0.5),
                              0, kindred::kNumCharacterModels - 1);
    if (!s.was_on || model != s.model) {
      s.model = model;
      s.nets[model].reset();
      for (float& w : s.window)
        w = 0.0f;
      s.level_in = s.level_out = 0.0f;
      s.gain = 1.0f;
      if (!s.was_on)
        s.bridge.reset(getSampleRate());
      s.was_on = true;
    }
    s.drive = static_cast<float>(std::pow(10.0, input(kDrive)->source->buffer[0] / 20.0));
    float mix = static_cast<float>(utils::clamp(input(kMix)->source->buffer[0], 0.0, 1.0));

    int n = std::min(buffer_size_, static_cast<int>(MAX_BUFFER_SIZE));
    float in[MAX_BUFFER_SIZE];
    float wet[MAX_BUFFER_SIZE];
    for (int i = 0; i < n; ++i)
      in[i] = static_cast<float>(audio[i]);

    s.bridge.process(in, wet, n, [&s](const float* bin, float* bout, int count) {
      s.renderBlock(bin, bout, count);
    });

    float dry_gain = std::cos(mix * 0.5f * static_cast<float>(PI));
    float wet_gain = std::sin(mix * 0.5f * static_cast<float>(PI));
    for (int i = 0; i < n; ++i)
      dest[i] = dry_gain * audio[i] + wet_gain * wet[i];
  }

} // namespace mopo
