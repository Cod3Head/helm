/* Clear But Complex // Kindred
 * The Kindred crew from SignalForge: Amelia, Catalina and Persefone.
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 * See COPYING for details.
 */

#pragma once
#ifndef CREW_SPRITE_H
#define CREW_SPRITE_H

#include "JuceHeader.h"
#include "load_save.h"

#include <cmath>

// One animated crew member, drawn from a 4-frame strip (see tools/cbc_crew_assets.py).
// Decorative only: no audio-thread work, nothing here measures or changes sound.
// Follows the "Animate graphics" setting and pauses on click, as in SignalForge.
class CrewSprite : public Component, public Timer {
  public:
    enum Member { kAmelia, kCatalina, kPersefone };

    static String nameOf(Member m) {
      return m == kAmelia ? "Amelia" : m == kCatalina ? "Catalina" : "Persefone";
    }

    static String roleOf(Member m) {
      return m == kAmelia ? "Oscillators" : m == kCatalina ? "Filter + FX" : "Modulation";
    }

    explicit CrewSprite(Member member) : member_(member), step_(0), paused_(false) {
      if (member_ == kAmelia)
        sheet_ = ImageCache::getFromMemory(BinaryData::crew_amelia_png,
                                           BinaryData::crew_amelia_pngSize);
      else if (member_ == kCatalina)
        sheet_ = ImageCache::getFromMemory(BinaryData::crew_catalina_png,
                                           BinaryData::crew_catalina_pngSize);
      else
        sheet_ = ImageCache::getFromMemory(BinaryData::crew_persefone_png,
                                           BinaryData::crew_persefone_pngSize);

      setName(nameOf(member_));
      if (LoadSave::shouldAnimateWidgets())
        startTimer(160);
    }

    ~CrewSprite() { stopTimer(); }

    void timerCallback() override {
      if (!isShowing())
        return;
      step_ = (step_ + 1) % 32;
      repaint();
    }

    void mouseUp(const MouseEvent& e) override {
      paused_ = !paused_;
      if (paused_)
        stopTimer();
      else if (LoadSave::shouldAnimateWidgets())
        startTimer(160);
      repaint();
    }

    void paint(Graphics& g) override {
      if (sheet_.isNull())
        return;

      g.setImageResamplingQuality(Graphics::mediumResamplingQuality);
      const float cell_w = sheet_.getWidth() / 4.0f;
      const float cell_h = (float)sheet_.getHeight();
      const float scale = jmin(getWidth() / cell_w, getHeight() / cell_h);
      const float w = cell_w * scale;
      const float h = cell_h * scale;
      float x = (getWidth() - w) * 0.5f;
      float y = getHeight() - h;

      if (member_ == kAmelia) {
        drawCell(g, (step_ / 2) % 4, 0.0f, 1.0f, x, y, w, h);
      }
      else if (member_ == kCatalina) {
        // "No, no, no": the body always comes from frame 0; only the head moves.
        static const int head_frames[16] = { 0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 3, 3, 3, 3 };
        const int frame = head_frames[(step_ / 2) % 16];
        const float split = 0.335f;
        drawCell(g, 0, split, 1.0f, x, y, w, h);
        drawCell(g, frame, 0.0f, split, x, y, w, h);
      }
      else {
        // Wave, run, run, jump - with a little sway, like SignalForge.
        const int frame = step_ < 5 ? 0 : (step_ < 24 ? 1 + step_ % 2 : 3);
        if (!paused_) {
          x += 0.06f * w * std::sin(step_ * 2.0 * double_Pi / 32.0);
          if (frame == 3)
            y -= 0.04f * h * std::abs(std::sin(step_ * double_Pi / 4.0));
        }
        drawCell(g, frame, 0.0f, 1.0f, x, y, w, h);
      }
    }

  private:
    // Draws the vertical band [top, bottom) (fractions of height) of one frame.
    void drawCell(Graphics& g, int frame, float top, float bottom,
                  float x, float y, float w, float h) {
      const int cell_w = sheet_.getWidth() / 4;
      const int src_y = (int)(sheet_.getHeight() * top);
      const int src_h = (int)(sheet_.getHeight() * bottom) - src_y;
      g.drawImage(sheet_,
                  (int)x, (int)(y + h * top), (int)w, (int)(h * (bottom - top)),
                  frame * cell_w, src_y, cell_w, src_h);
    }

    Member member_;
    Image sheet_;
    int step_;
    bool paused_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrewSprite)
};

#endif // CREW_SPRITE_H
