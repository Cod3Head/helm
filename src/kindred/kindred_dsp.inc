/* Clear But Complex // Kindred - DSP compilation unit.
 *
 * Included once, at the end of src/synthesis/helm_engine.cpp (with kindred_dsp_isolated.inc
 * at the end of src/synthesis/helm_voice_handler.cpp), so the Kindred engines and
 * the libraries they use build everywhere (Linux, Windows, Mac, plugin and standalone)
 * without changing any build files. Do not include this file anywhere else.
 *
 *   Mutable Instruments Plaits, Rings, stmlib - Copyright Emilie Gillet, MIT License.
 *   RTNeural - Copyright Jatin Chowdhury, BSD 3-Clause License.
 *   Kindred engines - GPL-3.0-or-later, part of this fork of helm.
 */

#include <cstdio>

// Mutable Instruments code targets STM32 hardware unless TEST is defined; TEST selects the
// portable C++ paths (no ARM assembly, no flash access). Scoped to this block only.
// MSVC (Windows builds): the Mutable Instruments code is written for GCC.
#ifdef _MSC_VER
  #pragma push_macro("__attribute__")
  #define __attribute__(x)
  #ifndef M_PI
    #define M_PI 3.14159265358979323846
  #endif
  #pragma warning(push)
  #pragma warning(disable: 4244 4267 4305 4100 4189 4456 4458 4701)
#endif

#pragma push_macro("TEST")
#undef TEST
#define TEST
#include "stmlib/dsp/atan.cc"
#include "stmlib/dsp/units.cc"
#include "stmlib/utils/random.cc"
#include "plaits/resources.cc"
#include "plaits/dsp/chords/chord_bank.cc"
#include "plaits/dsp/engine/additive_engine.cc"
#include "plaits/dsp/engine/bass_drum_engine.cc"
#include "plaits/dsp/engine/chord_engine.cc"
#include "plaits/dsp/engine/fm_engine.cc"
#include "plaits/dsp/engine/grain_engine.cc"
#include "plaits/dsp/engine/hi_hat_engine.cc"
#include "plaits/dsp/engine/modal_engine.cc"
#include "plaits/dsp/engine/noise_engine.cc"
#include "plaits/dsp/engine/particle_engine.cc"
#include "plaits/dsp/engine/snare_drum_engine.cc"
#include "plaits/dsp/engine/speech_engine.cc"
#include "plaits/dsp/engine/string_engine.cc"
#include "plaits/dsp/engine/swarm_engine.cc"
#include "plaits/dsp/engine/virtual_analog_engine.cc"
#include "plaits/dsp/engine/waveshaping_engine.cc"
#include "plaits/dsp/engine/wavetable_engine.cc"
#include "plaits/dsp/engine2/chiptune_engine.cc"
#include "plaits/dsp/engine2/phase_distortion_engine.cc"
#include "plaits/dsp/engine2/virtual_analog_vcf_engine.cc"
#include "plaits/dsp/engine2/wave_terrain_engine.cc"
#include "plaits/dsp/fm/algorithms.cc"
#include "plaits/dsp/fm/dx_units.cc"
#include "plaits/dsp/physical_modelling/modal_voice.cc"
#include "plaits/dsp/physical_modelling/resonator.cc"
#include "plaits/dsp/physical_modelling/string.cc"
#include "plaits/dsp/physical_modelling/string_voice.cc"
#include "plaits/dsp/speech/lpc_speech_synth.cc"
#include "plaits/dsp/speech/lpc_speech_synth_controller.cc"
#include "plaits/dsp/speech/lpc_speech_synth_phonemes.cc"
#include "plaits/dsp/speech/lpc_speech_synth_words.cc"
#include "plaits/dsp/speech/naive_speech_synth.cc"
#include "plaits/dsp/speech/sam_speech_synth.cc"
#include "plaits/dsp/voice.cc"
#include "rings/resources.cc"
#include "rings/dsp/fm_voice.cc"
#include "rings/dsp/part.cc"
#include "rings/dsp/resonator.cc"
#include "rings/dsp/string.cc"
// rings/dsp/string_synth_part.cc (the "Disastrous Peace" easter-egg synth) is not used and its
// chord table clashes with part.cc, so it is not compiled.
#pragma pop_macro("TEST")
#ifdef _MSC_VER
  #pragma warning(pop)
  #pragma pop_macro("__attribute__")
#endif


#ifndef RTNEURAL_DEFAULT_ALIGNMENT
  #define RTNEURAL_DEFAULT_ALIGNMENT 16
#endif

#include "kindred/kindred_morph_oscillator.cpp"
#include "kindred/kindred_body.cpp"
#include "kindred/kindred_character.cpp"
