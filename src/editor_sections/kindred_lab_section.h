/* Clear But Complex // Kindred
 * The Kindred Lab: one panel per crew engine.
 *   MORPH     - Persefone's Morph oscillator (Mutable Instruments Plaits, 24 models)
 *   BODY      - Amelia's Body resonator (Mutable Instruments Rings, 6 models)
 *   CHARACTER - Catalina's Character stage (RTNeural networks of circuits)
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 * Compiled from synthesis_interface.cpp so that no build files change.
 */

#pragma once
#ifndef KINDRED_LAB_SECTION_H
#define KINDRED_LAB_SECTION_H

#include "JuceHeader.h"
#include "crew_sprite.h"
#include "synth_button.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "text_selector.h"

#include <vector>

// Shared look: crew tag in the title bar, a small animated crew member at the right.
class KindredCrewSection : public SynthSection {
  public:
    KindredCrewSection(String name, CrewSprite::Member member, Colour accent);
    ~KindredCrewSection();

    void paintBackground(Graphics& g) override;
    void resized() override;

  protected:
    // Area left for controls (right of any switch, left of the crew member).
    Rectangle<int> controlArea();
    void placeKnobs(std::vector<Component*> controls, Rectangle<int> area);
    void labelKnobs(Graphics& g, std::vector<Component*> controls, std::vector<String> labels);

    Colour accent_;
    ScopedPointer<CrewSprite> crew_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KindredCrewSection)
};

class KindredMorphSection : public KindredCrewSection, public SynthSlider::SliderListener {
  public:
    KindredMorphSection(String name);
    ~KindredMorphSection();

    void paintBackground(Graphics& g) override;
    void resized() override;
    void guiChanged(SynthSlider* slider) override;

  private:
    void updateModelName();

    ScopedPointer<SynthSlider> model_;
    ScopedPointer<SynthSlider> harmonics_;
    ScopedPointer<SynthSlider> timbre_;
    ScopedPointer<SynthSlider> shape_;
    ScopedPointer<SynthSlider> level_;
    ScopedPointer<Label> model_name_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KindredMorphSection)
};

class KindredBodySection : public KindredCrewSection {
  public:
    KindredBodySection(String name);
    ~KindredBodySection();

    void paintBackground(Graphics& g) override;
    void resized() override;

  private:
    ScopedPointer<SynthButton> on_;
    ScopedPointer<TextSelector> model_;
    ScopedPointer<SynthSlider> structure_;
    ScopedPointer<SynthSlider> brightness_;
    ScopedPointer<SynthSlider> damping_;
    ScopedPointer<SynthSlider> position_;
    ScopedPointer<SynthSlider> mix_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KindredBodySection)
};

class KindredCharacterSection : public KindredCrewSection {
  public:
    KindredCharacterSection(String name);
    ~KindredCharacterSection();

    void paintBackground(Graphics& g) override;
    void resized() override;

  private:
    ScopedPointer<SynthButton> on_;
    ScopedPointer<TextSelector> model_;
    ScopedPointer<SynthSlider> drive_;
    ScopedPointer<SynthSlider> mix_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KindredCharacterSection)
};

#endif // KINDRED_LAB_SECTION_H
