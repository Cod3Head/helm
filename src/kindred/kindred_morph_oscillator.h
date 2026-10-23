/* Clear But Complex // Kindred
 * Persefone's Morph oscillator: Mutable Instruments Plaits models with a continuous
 * morph between whole synthesis methods.
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 * Plaits DSP: Copyright Emilie Gillet, MIT License (src/plaits/LICENSE).
 */

#pragma once
#ifndef KINDRED_MORPH_OSCILLATOR_H
#define KINDRED_MORPH_OSCILLATOR_H

#include "processor.h"

#include <memory>

namespace mopo {

  // One per voice. Two Plaits voices run side by side: one owns the even-numbered
  // models, the other the odd ones. "Model" is continuous; its fractional part
  // crossfades between neighbouring models, and an engine only ever switches while
  // its weight is zero, so sweeping the model never clicks.
  class KindredMorphOscillator : public Processor {
    public:
      enum Inputs {
        kMidi,        // MIDI note (pitch bend included), control rate.
        kModel,       // 0 .. kNumModels - 1, continuous.
        kHarmonics,   // 0 .. 1
        kTimbre,      // 0 .. 1
        kMorph,       // 0 .. 1
        kLevel,       // 0 .. 1; 0 skips all processing.
        kNoteOn,      // Trigger at each note start (excites percussive models).
        kNumInputs
      };

      static const int kNumModels = 24;

      KindredMorphOscillator();
      KindredMorphOscillator(const KindredMorphOscillator& other);
      virtual ~KindredMorphOscillator();

      virtual void process() override;
      virtual void setSampleRate(int sample_rate) override;
      virtual Processor* clone() const override { return new KindredMorphOscillator(*this); }

      static const char* modelName(int model);

    private:
      struct State;
      std::unique_ptr<State> state_;
  };

} // namespace mopo

#endif // KINDRED_MORPH_OSCILLATOR_H
