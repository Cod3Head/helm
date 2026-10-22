## Clear But Complex // SignalForge Synth
A free, open-source, cross-platform polyphonic synthesizer from Clear But Complex, styled after SignalForge. It runs on GNU/Linux, Mac and Windows as a standalone program and as an LV2/VST/AU/AAX plugin.

**Based on [Helm](https://github.com/mtytel/helm) by Matt Tytel.** This project is a fork of Helm and, like Helm, is free software under the GNU General Public License v3 (see `COPYING`). All original copyright notices are kept. Bug reports for this fork go to this repository's Issues, not to the original author.

### What is different from Helm
- Clear But Complex / SignalForge visual design: near-black base (`#08090b`), slate panels (`#111318`, `#14161c`), mint primary accent (`#54f2c6`) and hot pink (`#ff2d55`) for modulation and the main action.
- New name, icon, window title and plugin identity (manufacturer code `CbCx`, plugin code `SFSy`), so it installs side by side with Helm. Settings are stored separately from Helm's.
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

Original Helm screenshot (pre-restyle): http://tytel.org/static/images/helm_screenshot.png
