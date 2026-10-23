/* Clear But Complex // Kindred
 * Amelia's Body (implementation). Compiled via kindred_dsp.inc.
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 * Rings DSP: Copyright Emilie Gillet, MIT License (src/rings/LICENSE).
 */

#include "kindred/kindred_body.h"
#include "kindred/kindred_rate_bridge.h"
#include "utils.h"

namespace mopo {

  namespace {
    const char* const kBodyNames[KindredBody::kNumModels] = {
      "Modal", "Sympathetic Strings", "String", "FM Voice", "Tuned Strings", "String + Space"
    };
  }

  const char* KindredBody::modelName(int model) {
    if (model < 0 || model >= kNumModels)
      return "";
    return kBodyNames[model];
  }

  struct KindredBody::State {
    State() { init(rings::RESONATOR_MODEL_MODAL); }

    // A fresh resonator for each model: switching models in place leaves the
    // previous model's filter state behind, which silences the String models.
    void init(rings::ResonatorModel resonator) {
      part.Init(reverb_buffer);
      part.set_polyphony(2);
      part.set_model(resonator);
    }

    uint16_t reverb_buffer[32768];
    rings::Part part;
    kindred::EffectBridge<rings::kMaxBlockSize> bridge;
    rings::Patch patch = { 0.4f, 0.5f, 0.6f, 0.3f };
    float note = 48.0f;
    float mix = 0.5f;
    bool was_on = false;
    bool pending_strum = false;
    int model = 0;

    void renderBlock(const float* in, float* out, int n) {
      rings::PerformanceState performance = {};
      performance.internal_exciter = false;   // Kindred's voices are the exciter.
      performance.internal_strum = false;     // Kindred strikes the body on each new note.
      performance.strum = pending_strum;
      pending_strum = false;
      performance.internal_note = false;
      performance.tonic = 0.0f;
      performance.note = note;
      performance.fm = 0.0f;
      performance.chord = 0;

      float input[rings::kMaxBlockSize];
      float odd[rings::kMaxBlockSize];
      float even[rings::kMaxBlockSize];
      for (int i = 0; i < n; ++i)
        input[i] = in[i];
      part.Process(performance, patch, input, odd, even, n);
      for (int i = 0; i < n; ++i)
        out[i] = 0.5f * (odd[i] + even[i]);
    }
  };

  KindredBody::KindredBody() : Processor(kNumInputs, 1), state_(new State()) { }

  KindredBody::KindredBody(const KindredBody& other) : Processor(other), state_(new State()) {
    state_->bridge.reset(other.getSampleRate() > 0 ? other.getSampleRate() : 44100);
  }

  KindredBody::~KindredBody() { }

  void KindredBody::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    state_->bridge.reset(sample_rate);
  }

  void KindredBody::process() {
    const mopo_float* audio = input(kAudio)->source->buffer;
    mopo_float* dest = output()->buffer;
    State& s = *state_;
    const bool on = input(kOn)->source->buffer[0] > 0.5;

    if (!on) {
      utils::copyBuffer(dest, audio, buffer_size_);
      s.was_on = false;
      return;
    }
    if (!s.was_on) {
      s.bridge.reset(getSampleRate());
      s.was_on = true;
      s.pending_strum = true;
    }

    int model = utils::iclamp(static_cast<int>(input(kModel)->source->buffer[0] + 0.5), 0, kNumModels - 1);
    if (model != s.model) {
      s.model = model;
      s.init(static_cast<rings::ResonatorModel>(model));
      s.pending_strum = true;  // A new body has to be struck to speak.
    }
    if (input(kStrum)->source->buffer[0] > 0.5)
      s.pending_strum = true;
    s.note = static_cast<float>(input(kNote)->source->buffer[0]);
    s.patch.structure = static_cast<float>(utils::clamp(input(kStructure)->source->buffer[0], 0.0, 1.0));
    s.patch.brightness = static_cast<float>(utils::clamp(input(kBrightness)->source->buffer[0], 0.0, 1.0));
    s.patch.damping = static_cast<float>(utils::clamp(input(kDamping)->source->buffer[0], 0.0, 1.0));
    s.patch.position = static_cast<float>(utils::clamp(input(kPosition)->source->buffer[0], 0.0, 1.0));
    float mix = static_cast<float>(utils::clamp(input(kMix)->source->buffer[0], 0.0, 1.0));

    int n = std::min(buffer_size_, static_cast<int>(MAX_BUFFER_SIZE));
    float in[MAX_BUFFER_SIZE];
    float wet[MAX_BUFFER_SIZE];
    for (int i = 0; i < n; ++i)
      in[i] = static_cast<float>(audio[i]);

    s.bridge.process(in, wet, n, [&s](const float* bin, float* bout, int count) {
      s.renderBlock(bin, bout, count);
    });

    // Equal-power dry/wet.
    float dry_gain = std::cos(mix * 0.5f * static_cast<float>(PI));
    float wet_gain = std::sin(mix * 0.5f * static_cast<float>(PI));
    for (int i = 0; i < n; ++i)
      dest[i] = dry_gain * audio[i] + wet_gain * wet[i];
  }

} // namespace mopo
