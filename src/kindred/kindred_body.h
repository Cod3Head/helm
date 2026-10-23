/* Clear But Complex // Kindred
 * Amelia's Body: Mutable Instruments Rings resonator, played by the whole synth.
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 * Rings DSP: Copyright Emilie Gillet, MIT License (src/rings/LICENSE).
 */

#pragma once
#ifndef KINDRED_BODY_H
#define KINDRED_BODY_H

#include "processor.h"

#include <memory>

namespace mopo {

  // Global (after the voices). The synth's own sound excites a physical model:
  // every patch becomes a struck, bowed or plucked object tuned to the last note.
  class KindredBody : public Processor {
    public:
      enum Inputs {
        kAudio,       // Mono synth output (the exciter).
        kOn,
        kNote,        // MIDI note of the most recent note-on.
        kModel,       // 0 .. kNumModels - 1 (stepped).
        kStructure,   // 0 .. 1
        kBrightness,  // 0 .. 1
        kDamping,     // 0 .. 1
        kPosition,    // 0 .. 1
        kMix,         // 0 dry .. 1 body only
        kStrum,       // > 0.5 on blocks with a new note: strikes the body.
        kNumInputs
      };

      static const int kNumModels = 6;

      KindredBody();
      KindredBody(const KindredBody& other);
      virtual ~KindredBody();

      virtual void process() override;
      virtual void setSampleRate(int sample_rate) override;
      virtual Processor* clone() const override { return new KindredBody(*this); }

      static const char* modelName(int model);

    private:
      struct State;
      std::unique_ptr<State> state_;
  };

} // namespace mopo

#endif // KINDRED_BODY_H
