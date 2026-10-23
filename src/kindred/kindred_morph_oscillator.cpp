/* Clear But Complex // Kindred
 * Persefone's Morph oscillator (implementation). Compiled via kindred_dsp.inc.
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 * Plaits DSP: Copyright Emilie Gillet, MIT License (src/plaits/LICENSE).
 */

#include "kindred/kindred_morph_oscillator.h"
#include "kindred/kindred_rate_bridge.h"
#include "utils.h"
#include "voice_handler.h"

#include <cmath>

namespace mopo {

  namespace {
    const char* const kModelNames[KindredMorphOscillator::kNumModels] = {
      "Classic + Filter", "Phase Distortion", "FM Bank A", "FM Bank B", "FM Bank C",
      "Wave Terrain", "String Machine", "Chiptune", "Virtual Analog", "Waveshaper",
      "Two-Op FM", "Granular Formant", "Harmonics", "Wavetable", "Chords", "Speech",
      "Swarm", "Filtered Noise", "Particles", "Inharmonic String", "Modal", "Bass Drum",
      "Snare", "Hi-Hat"
    };

    // Gate held low for ~0.5 ms at note start so Plaits sees a rising edge.
    const int kTriggerGapBlocks = 2;
    const float kWeightEpsilon = 1e-4f;
  }

  const char* KindredMorphOscillator::modelName(int model) {
    if (model < 0 || model >= kNumModels)
      return "";
    return kModelNames[model];
  }

  // One Plaits voice with its own working memory.
  struct PlaitsSlot {
    PlaitsSlot() : allocator(ram, sizeof(ram)), engine(0), weight(0.0f) {
      voice.Init(&allocator);
    }
    char ram[16384];
    stmlib::BufferAllocator allocator;
    plaits::Voice voice;
    int engine;
    float weight;
  };

  struct KindredMorphOscillator::State {
    PlaitsSlot slots[2];   // [0] even models, [1] odd models.
    kindred::GeneratorBridge<plaits::kBlockSize> bridge;
    int trigger_blocks = 0;
    bool gate = false;
    float note = 60.0f;
    float harmonics = 0.5f;
    float timbre = 0.5f;
    float morph = 0.5f;
    bool silent = true;

    void renderBlock(float* out, int n) {
      plaits::Patch patch = {};
      patch.note = note;
      patch.harmonics = harmonics;
      patch.timbre = timbre;
      patch.morph = morph;
      patch.decay = 0.5f;
      patch.lpg_colour = 0.5f;

      plaits::Modulations mods = {};
      mods.level = 1.0f;
      mods.level_patched = true;      // LPG fully open; Kindred's envelopes shape the note.
      mods.trigger_patched = true;    // Percussive models are struck on each note.
      // Plaits reads the trigger as a gate: a short low gap re-strikes on each
      // note, then it stays high while the key is held (the FM banks sustain).
      mods.trigger = (gate && trigger_blocks == 0) ? 1.0f : 0.0f;
      if (trigger_blocks > 0)
        --trigger_blocks;

      for (int i = 0; i < n; ++i)
        out[i] = 0.0f;

      plaits::Voice::Frame frames[plaits::kBlockSize];
      for (PlaitsSlot& slot : slots) {
        if (slot.weight < kWeightEpsilon)
          continue;
        patch.engine = slot.engine;
        slot.voice.Render(patch, mods, frames, n);
        const float gain = slot.weight / 32768.0f;
        for (int i = 0; i < n; ++i)
          out[i] += gain * frames[i].out;
      }
    }
  };

  KindredMorphOscillator::KindredMorphOscillator()
      : Processor(kNumInputs, 1), state_(new State()) { }

  // mopo clones one processor per voice: every voice gets fresh Plaits memory.
  KindredMorphOscillator::KindredMorphOscillator(const KindredMorphOscillator& other)
      : Processor(other), state_(new State()) {
    state_->bridge.reset(other.getSampleRate() > 0 ? other.getSampleRate() : 44100);
  }

  KindredMorphOscillator::~KindredMorphOscillator() { }

  void KindredMorphOscillator::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    state_->bridge.reset(sample_rate);
  }

  void KindredMorphOscillator::process() {
    mopo_float* dest = output()->buffer;
    const mopo_float level = input(kLevel)->source->buffer[0];
    State& s = *state_;

    const Output* event = input(kNoteOn)->source;
    if (event->triggered) {
      const int type = static_cast<int>(event->trigger_value);
      if (type == kVoiceOn || type == kVoiceReset) {
        s.gate = true;
        s.trigger_blocks = kTriggerGapBlocks;
      }
      else if (type == kVoiceOff || type == kVoiceKill)
        s.gate = false;
    }

    if (level <= 0.0) {
      if (!s.silent) {
        utils::zeroBuffer(dest, buffer_size_);
        s.silent = true;
      }
      return;
    }
    s.silent = false;

    // Continuous model -> two neighbouring engines on parity slots.
    mopo_float model = utils::clamp(input(kModel)->source->buffer[0], 0.0, kNumModels - 1.0);
    int base = static_cast<int>(model);
    if (base >= kNumModels - 1)
      base = kNumModels - 1;
    float frac = static_cast<float>(model - base);
    int next = std::min(base + 1, kNumModels - 1);

    // Equal-power crossfade; each slot keeps its engine while it is audible.
    float w_base = std::cos(frac * 0.5f * static_cast<float>(PI));
    float w_next = std::sin(frac * 0.5f * static_cast<float>(PI));
    PlaitsSlot& base_slot = s.slots[base % 2];
    PlaitsSlot& next_slot = s.slots[(base + 1) % 2];
    base_slot.engine = base;
    base_slot.weight = w_base;
    next_slot.engine = next;
    next_slot.weight = (next == base) ? 0.0f : w_next;

    s.note = static_cast<float>(input(kMidi)->source->buffer[0]);
    s.harmonics = static_cast<float>(utils::clamp(input(kHarmonics)->source->buffer[0], 0.0, 1.0));
    s.timbre = static_cast<float>(utils::clamp(input(kTimbre)->source->buffer[0], 0.0, 1.0));
    s.morph = static_cast<float>(utils::clamp(input(kMorph)->source->buffer[0], 0.0, 1.0));

    float block[MAX_BUFFER_SIZE];
    int n = std::min(buffer_size_, static_cast<int>(MAX_BUFFER_SIZE));
    s.bridge.process(block, n, [&s](float* out, int count) { s.renderBlock(out, count); });

    const float gain = static_cast<float>(level);
    for (int i = 0; i < n; ++i)
      dest[i] = gain * block[i];
  }

} // namespace mopo
