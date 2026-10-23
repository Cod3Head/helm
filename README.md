## Clear But Complex // KINDRED
A free, open-source, cross-platform polyphonic synthesizer from Clear But Complex, built in the SignalForge design language and crewed by the SignalForge girls. It runs on GNU/Linux, Mac and Windows as a standalone program and as an LV2/VST/AU/AAX plugin.

**Based on [Helm](https://github.com/mtytel/helm) by Matt Tytel.** This project is a fork of Helm and, like Helm, is free software under the GNU General Public License v3 (see `COPYING`). All original copyright notices are kept. Bug reports for this fork go to this repository's Issues, not to the original author.

### The Kindred crew
Kindred is crewed by the three characters from Clear But Complex // SignalForge, each looking after the part of the synth that matches her personality:
- **Amelia** - the producer-ninja - *Oscillators*: where the waveforms are cut.
- **Catalina** - the "no, no, no" critic - *Filter + FX*: filtering is saying no to frequencies. She also guards the Delete dialog.
- **Persefone** - always running and jumping - *Modulation*: everything that keeps the sound moving. She keeps you company while saving.

They live, animated, in the About panel (click the logo). Their artwork comes from SignalForge (`tools/cbc_crew_assets.py`); only generated character art is used, never photographs. Click a character to pause her; the "Animate graphics" setting turns all animation off.

### The Kindred Lab: three engines, one chain
Below the LFOs sits the **Kindred Lab**, a row of three engines that work together. Each one is looked after by a crew member:

**Voice -> Morph -> Body -> Distortion -> Character -> Delay -> Reverb**

- **MORPH // PERSEFONE** - a second oscillator per voice with 24 synthesis models from Mutable Instruments Plaits: virtual analog, waveshaping, FM, the DX7-style FM banks, granular formants, additive harmonics, wavetables, chords, speech, swarms, physical strings and modal bodies, and drums. **MODEL** is continuous: between two models the oscillator crossfades between them, so modulating MODEL from an LFO, envelope or the step sequencer *morphs* the sound from one way of making sound into another. HARMONICS, TIMBRE and SHAPE are the Plaits controls of the current model. LEVEL mixes it in with the two classic oscillators (0 = off).
- **BODY // AMELIA** - the whole synth becomes the striker of a physical object, from Mutable Instruments Rings: modal (bars, bells, plates), sympathetic strings, a single string, an FM voice, tuned strings and a string with space. It is tuned to the last note you played and struck on every new note. STRUCTURE, BRIGHTNESS, DAMPING and POSITION shape the object; MIX blends it with the dry synth.
- **CHARACTER // CATALINA** - neural-network models of analog circuits (diode clipper, 12AX7 tube stage, wavefolder), run with RTNeural. DRIVE pushes harder into the circuit; Character matches its own loudness, so Drive changes the tone, not the volume. Train a model of your own gear from a dry/wet recording pair with `tools/kindred_train_character.py --pair dry.wav wet.wav --name "My Amp"`. The model count is fixed by the `character_model` parameter in `src/common/helm_common.cpp`, so raise its maximum when you add one.

All Lab controls can be modulated like any other Kindred control. Turning Body and Character off and leaving Morph's LEVEL at 0 gives the classic Helm sound, and old patches load exactly as before.

`tools/render` builds a headless renderer of the sound engine (no GUI needed) that writes WAV files, for testing and for checking the sound of a change: `sh tools/render/build.sh && build/kindred_render out.wav morph_level=1 sweep=morph_model:0:23 hold`.

### What is different from Helm
- Clear But Complex / SignalForge visual design: near-black base (`#08090b`), slate panels (`#111318`, `#14161c`), mint primary accent (`#54f2c6`) and hot pink (`#ff2d55`) for modulation and the main action.
- New name, icon, window title and plugin identity (manufacturer code `CbCx`, plugin code `Kndr`), so it installs side by side with Helm. Settings are stored separately from Helm's.
- Build fixes for current compilers (modern GCC, Visual Studio 2022) and automated Linux/Windows builds in GitHub Actions.
- Patch files still use the `.helm` format, so all Helm patches load unchanged.

Asset tools: `tools/cbc_brand_assets.py` (icon) and `tools/cbc_apply_palette.py` (colour mapping) document and reproduce the design changes.

### Building
Every push is built automatically by GitHub Actions (`.github/workflows/build.yml`); download the result from the Actions tab.

#### Linux
```bash
make standalone      # standalone app, built to standalone/builds/linux/build
make lv2             # LV2 plugin
make vst             # VST plugin
```

#### Windows
Open `standalone/builds/vs17/Helm.sln` (standalone) or `builds/vs17/Helm.sln` (plugins) in Visual Studio 2022.

#### Mac
Open `/standalone/builds/osx/Helm.xcodeproj` (standalone) or `/builds/osx/Helm.xcodeproj` (plugins).

### Features:
 - 32 voice polyphony
 - Interactive visual interface
 - Powerful modulation system with live visual feedback
 - Dual oscillators with cross modulation and up to 15 unison oscillators each
 - Sub oscillator with shuffle waveshaping
 - Oscillator feedback and saturation for waveshaping
 - 12 different waveforms
 - Blending between 12 or 24dB low/band/high pass filter
 - Low/Band/High Shelf filters
 - 2 monophonic and 1 polyphonic LFO
 - Step sequencer
 - Lots of modulation sources including polyphonic aftertouch
 - Simple arpeggiator
 - Effects: Formant filter, stutter, delay, distortion, reverb
 - Kindred Lab: 24-model Morph oscillator, physical-model Body resonator, neural Character stage

### Third-party code
Kindred includes, unchanged except where marked "Kindred:", code from:
- **Mutable Instruments Plaits, Rings and stmlib** by Emilie Gillet - MIT License (`src/plaits/LICENSE`, `src/rings/LICENSE`, `src/stmlib/LICENSE`). Kindred is not affiliated with or endorsed by Mutable Instruments.
- **RTNeural** by Jatin Chowdhury - BSD 3-Clause License (`src/RTNeural/LICENSE`), with **nlohmann/json** by Niels Lohmann - MIT License (licence in `src/RTNeural/json/json.hpp`).
- **Helm** and **mopo** by Matt Tytel - GPL-3.0.
Both the MIT and BSD 3-Clause licences are compatible with the GPL-3.0, which covers Kindred as a whole.

Original Helm screenshot (pre-restyle): http://tytel.org/static/images/helm_screenshot.png
