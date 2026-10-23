/* Clear But Complex // Kindred
 * The Kindred Lab panels. See kindred_lab_section.h.
 *
 * This file is part of Kindred, a fork of helm by Matt Tytel, and is free
 * software under the GNU General Public License, version 3 or later.
 */

#include "kindred_lab_section.h"

#include "colors.h"
#include "fonts.h"
#include "text_look_and_feel.h"

namespace {
  const float kSelectorWidth = 64.0f;
  const float kSelectorHeight = 16.0f;
  const float kKnobY = 30.0f;
  const float kCrewWidth = 50.0f;
  const float kModelSlotWidth = 104.0f;
}

// ------------------------------------------------------------------ shared crew panel

KindredCrewSection::KindredCrewSection(String name, CrewSprite::Member member, Colour accent)
    : SynthSection(name), accent_(accent) {
  crew_ = new CrewSprite(member);
  addAndMakeVisible(crew_);
}

KindredCrewSection::~KindredCrewSection() {
  crew_ = nullptr;
}

void KindredCrewSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  // Crew colour: a slim bar along the title and a glow behind the crew member.
  int title_width = getTitleWidth();
  g.setColour(accent_.withAlpha(0.9f));
  g.fillRect(0.0f, title_width - size_ratio_ * 2.0f, (float)getWidth(), size_ratio_ * 2.0f);

  Rectangle<float> glow = crew_->getBounds().toFloat().reduced(size_ratio_ * 2.0f);
  g.setGradientFill(ColourGradient(accent_.withAlpha(0.22f), glow.getCentreX(), glow.getBottom(),
                                   accent_.withAlpha(0.0f), glow.getCentreX(), glow.getY(),
                                   true));
  g.fillEllipse(glow);
}

void KindredCrewSection::resized() {
  int title_width = getTitleWidth();
  int crew_width = size_ratio_ * kCrewWidth;
  int crew_height = getHeight() - title_width - size_ratio_ * 4.0f;
  crew_->setBounds(getWidth() - crew_width - size_ratio_ * 4.0f, title_width + size_ratio_ * 2.0f,
                   crew_width, crew_height);
  SynthSection::resized();
}

Rectangle<int> KindredCrewSection::controlArea() {
  int right = crew_->getX() - size_ratio_ * 4.0f;
  return Rectangle<int>(0, getTitleWidth(), jmax(0, right), getHeight() - getTitleWidth());
}

void KindredCrewSection::placeKnobs(std::vector<Component*> controls, Rectangle<int> area) {
  if (controls.empty())
    return;
  int knob_width = getStandardKnobSize();
  float slot = area.getWidth() / (float)controls.size();
  int y = size_ratio_ * kKnobY;
  for (int i = 0; i < (int)controls.size(); ++i) {
    int x = area.getX() + slot * (i + 0.5f) - knob_width / 2;
    controls[i]->setBounds(x, y, knob_width, knob_width);
  }
}

void KindredCrewSection::labelKnobs(Graphics& g, std::vector<Component*> controls,
                                    std::vector<String> labels) {
  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 10.0f));
  for (size_t i = 0; i < controls.size() && i < labels.size(); ++i)
    drawTextForComponent(g, labels[i], controls[i]);
}

// --------------------------------------------------------------- Persefone: Morph

KindredMorphSection::KindredMorphSection(String name)
    : KindredCrewSection(name, CrewSprite::kPersefone, Colors::modulation) {
  addSlider(model_ = new SynthSlider("morph_model"));
  model_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  model_->setStringLookup(mopo::strings::kindred_morph_models);
  model_->addSliderListener(this);

  addSlider(harmonics_ = new SynthSlider("morph_harmonics"));
  harmonics_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider(timbre_ = new SynthSlider("morph_timbre"));
  timbre_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider(shape_ = new SynthSlider("morph_shape"));
  shape_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider(level_ = new SynthSlider("morph_level"));
  level_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  model_name_ = new Label("morph_model_name");
  model_name_->setJustificationType(Justification::centred);
  model_name_->setColour(Label::textColourId, Colors::modulation);
  model_name_->setInterceptsMouseClicks(false, false);
  model_name_->setTooltip("Persefone's Morph: drag MODEL, or modulate it, to morph between models");
  addAndMakeVisible(model_name_);
  updateModelName();
}

KindredMorphSection::~KindredMorphSection() {
  model_ = nullptr;
  harmonics_ = nullptr;
  timbre_ = nullptr;
  shape_ = nullptr;
  level_ = nullptr;
  model_name_ = nullptr;
}

void KindredMorphSection::updateModelName() {
  double value = model_->getValue();
  int base = mopo::utils::iclamp(value, 0, 23);
  double frac = value - base;
  String text = mopo::strings::kindred_morph_models[base];
  if (frac > 0.02 && base < 23)
    text = text + String(" > ") + String(mopo::strings::kindred_morph_models[base + 1]);
  model_name_->setText(text, dontSendNotification);
}

void KindredMorphSection::guiChanged(SynthSlider* slider) {
  if (slider == model_)
    updateModelName();
}

void KindredMorphSection::paintBackground(Graphics& g) {
  KindredCrewSection::paintBackground(g);
  labelKnobs(g, { harmonics_.get(), timbre_.get(), shape_.get(), level_.get() },
             { TRANS("HARMONICS"), TRANS("TIMBRE"), TRANS("SHAPE"), TRANS("LEVEL") });
}

void KindredMorphSection::resized() {
  KindredCrewSection::resized();

  Rectangle<int> area = controlArea();
  int knob_width = getStandardKnobSize();
  int model_slot = size_ratio_ * kModelSlotWidth;
  int y = size_ratio_ * kKnobY;

  model_->setBounds(area.getX() + (model_slot - knob_width) / 2, y, knob_width, knob_width);
  model_name_->setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 10.0f));
  model_name_->setBounds(area.getX() + size_ratio_ * 2.0f, model_->getBottom() + size_ratio_ * 1.0f,
                         model_slot - size_ratio_ * 4.0f, size_ratio_ * 16.0f);

  placeKnobs({ harmonics_.get(), timbre_.get(), shape_.get(), level_.get() }, area.withTrimmedLeft(model_slot));
  SynthSection::resized();
}

// ----------------------------------------------------------------- Amelia: Body

KindredBodySection::KindredBodySection(String name)
    : KindredCrewSection(name, CrewSprite::kAmelia, Colors::audio) {
  addSlider(model_ = new TextSelector("body_model"));
  model_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  model_->setStringLookup(mopo::strings::kindred_body_models_short);
  model_->setLongStringLookup(mopo::strings::kindred_body_models_long);
  model_->setLookAndFeel(TextLookAndFeel::instance());

  addSlider(structure_ = new SynthSlider("body_structure"));
  structure_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider(brightness_ = new SynthSlider("body_brightness"));
  brightness_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider(damping_ = new SynthSlider("body_damping"));
  damping_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider(position_ = new SynthSlider("body_position"));
  position_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider(mix_ = new SynthSlider("body_mix"));
  mix_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addButton(on_ = new SynthButton("body_on"));
  setActivator(on_);
}

KindredBodySection::~KindredBodySection() {
  on_ = nullptr;
  model_ = nullptr;
  structure_ = nullptr;
  brightness_ = nullptr;
  damping_ = nullptr;
  position_ = nullptr;
  mix_ = nullptr;
}

void KindredBodySection::paintBackground(Graphics& g) {
  KindredCrewSection::paintBackground(g);
  labelKnobs(g, { structure_.get(), brightness_.get(), damping_.get(), position_.get(), mix_.get() },
             { TRANS("STRUCTURE"), TRANS("BRIGHT"), TRANS("DAMPING"), TRANS("POSITION"), TRANS("MIX") });

  g.drawText(TRANS("MODEL"), model_->getX(), structure_->getBottom() + size_ratio_ * 4.0f,
             model_->getWidth(), size_ratio_ * 10.0f, Justification::centred, false);
}

void KindredBodySection::resized() {
  int title_width = getTitleWidth();
  on_->setBounds(size_ratio_ * 2.0f, 0, title_width, title_width);
  KindredCrewSection::resized();

  Rectangle<int> area = controlArea();
  int selector_width = size_ratio_ * kSelectorWidth;
  int selector_slot = selector_width + size_ratio_ * 10.0f;
  model_->setBounds(area.getX() + size_ratio_ * 6.0f, size_ratio_ * 44.0f,
                    selector_width, size_ratio_ * kSelectorHeight);
  placeKnobs({ structure_.get(), brightness_.get(), damping_.get(), position_.get(), mix_.get() },
             area.withTrimmedLeft(selector_slot));
  SynthSection::resized();
}

// ------------------------------------------------------------ Catalina: Character

KindredCharacterSection::KindredCharacterSection(String name)
    : KindredCrewSection(name, CrewSprite::kCatalina, Colour(0xffb48cff)) {
  addSlider(model_ = new TextSelector("character_model"));
  model_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  model_->setStringLookup(mopo::strings::kindred_character_models_short);
  model_->setLongStringLookup(mopo::strings::kindred_character_models_long);
  model_->setLookAndFeel(TextLookAndFeel::instance());

  addSlider(drive_ = new SynthSlider("character_drive"));
  drive_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider(mix_ = new SynthSlider("character_mix"));
  mix_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addButton(on_ = new SynthButton("character_on"));
  setActivator(on_);
}

KindredCharacterSection::~KindredCharacterSection() {
  on_ = nullptr;
  model_ = nullptr;
  drive_ = nullptr;
  mix_ = nullptr;
}

void KindredCharacterSection::paintBackground(Graphics& g) {
  KindredCrewSection::paintBackground(g);
  labelKnobs(g, { drive_.get(), mix_.get() }, { TRANS("DRIVE"), TRANS("MIX") });
  g.drawText(TRANS("CIRCUIT"), model_->getX(), drive_->getBottom() + size_ratio_ * 4.0f,
             model_->getWidth(), size_ratio_ * 10.0f, Justification::centred, false);
}

void KindredCharacterSection::resized() {
  int title_width = getTitleWidth();
  on_->setBounds(size_ratio_ * 2.0f, 0, title_width, title_width);
  KindredCrewSection::resized();

  Rectangle<int> area = controlArea();
  int selector_width = size_ratio_ * (kSelectorWidth - 8.0f);
  int selector_slot = selector_width + size_ratio_ * 10.0f;
  model_->setBounds(area.getX() + size_ratio_ * 6.0f, size_ratio_ * 44.0f,
                    selector_width, size_ratio_ * kSelectorHeight);
  placeKnobs({ drive_.get(), mix_.get() }, area.withTrimmedLeft(selector_slot));
  SynthSection::resized();
}
