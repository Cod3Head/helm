/* Clear But Complex // Kindred
 * Catalina's Character: a neural tone stage (RTNeural) with models trained on
 * simulated analog circuits, or on your own recordings (tools/kindred_train_character.py).
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 * RTNeural: Copyright Jatin Chowdhury, BSD 3-Clause License (src/RTNeural/LICENSE).
 */

#pragma once
#ifndef KINDRED_CHARACTER_H
#define KINDRED_CHARACTER_H

#include "processor.h"

#include <memory>

namespace mopo {

  class KindredCharacter : public Processor {
    public:
      enum Inputs {
        kAudio,
        kOn,
        kModel,   // 0 .. numModels() - 1 (stepped).
        kDrive,   // dB into the network.
        kMix,     // 0 dry .. 1 character only
        kNumInputs
      };

      KindredCharacter();
      KindredCharacter(const KindredCharacter& other);
      virtual ~KindredCharacter();

      virtual void process() override;
      virtual void setSampleRate(int sample_rate) override;
      virtual Processor* clone() const override { return new KindredCharacter(*this); }

      static int numModels();
      static const char* modelName(int model);

    private:
      struct State;
      std::unique_ptr<State> state_;
  };

} // namespace mopo

#endif // KINDRED_CHARACTER_H
