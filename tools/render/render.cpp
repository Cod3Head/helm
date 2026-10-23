// Headless Kindred engine renderer: plays notes through mopo::HelmEngine and writes a WAV.
// Usage: render out.wav [param=value ...] [sweep=param:from:to ...] [hold]
// Build: see tools/render/build.sh. No JUCE needed; used to verify DSP changes.
#include "helm_engine.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <cmath>

static void writeWav(const char* path, const std::vector<float>& l, const std::vector<float>& r, int sr) {
  FILE* f = fopen(path, "wb");
  uint32_t n = l.size(), data = n * 4, riff = 36 + data, sr32 = sr, br = sr * 4, fmt = 16;
  uint16_t pcm = 1, ch = 2, ba = 4, bits = 16;
  fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
  fwrite(&fmt, 4, 1, f); fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr32, 4, 1, f);
  fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&data, 4, 1, f);
  for (uint32_t i = 0; i < n; ++i) {
    int16_t a = (int16_t)std::max(-32767.0f, std::min(32767.0f, l[i] * 32767.0f));
    int16_t b = (int16_t)std::max(-32767.0f, std::min(32767.0f, r[i] * 32767.0f));
    fwrite(&a, 2, 1, f); fwrite(&b, 2, 1, f);
  }
  fclose(f);
}

int main(int argc, char** argv) {
  int sr = 48000;
  const char* out = argc > 1 ? argv[1] : "out.wav";
  mopo::HelmEngine engine;
  engine.setSampleRate(sr);
  engine.setBufferSize(256);
  mopo::control_map controls = engine.getControls();
  struct Sweep { mopo::Value* control; double from, to; };
  std::vector<Sweep> sweeps;
  bool hold = false;
  for (int a = 2; a < argc; ++a) {
    std::string kv = argv[a];
    if (kv == "hold") { hold = true; continue; }
    if (kv.compare(0, 6, "sweep=") == 0) {  // sweep=param:from:to, linear over the render
      std::string spec = kv.substr(6);
      size_t c1 = spec.find(':'), c2 = spec.rfind(':');
      std::string k = spec.substr(0, c1);
      if (c1 != std::string::npos && c2 != c1 && controls.count(k))
        sweeps.push_back({controls[k], atof(spec.substr(c1 + 1, c2 - c1 - 1).c_str()), atof(spec.substr(c2 + 1).c_str())});
      else fprintf(stderr, "bad sweep %s\n", kv.c_str());
      continue;
    }
    size_t eq = kv.find('=');
    if (eq == std::string::npos) continue;
    std::string k = kv.substr(0, eq);
    if (k == "sr") continue;
    if (controls.count(k)) controls[k]->set(atof(kv.substr(eq + 1).c_str()));
    else fprintf(stderr, "unknown control %s\n", k.c_str());
  }
  // Phrase: C3, E3, G3 chord then a C4 melody note; 4 seconds total.
  struct Ev { double t; int note; bool on; };
  std::vector<Ev> evs = { {0.0,48,true},{0.0,52,true},{0.0,55,true},{1.2,48,false},{1.2,52,false},{1.2,55,false},
                          {1.5,60,true},{2.5,60,false},{2.7,43,true},{3.4,43,false} };
  if (hold) evs = { {0.0,48,true},{3.8,48,false} };  // one held C3
  double total = 4.0;
  std::vector<float> L, R;
  int frames = (int)(total * sr), done = 0, buf = 256;
  size_t ev = 0;
  while (done < frames) {
    double t = done / (double)sr;
    while (ev < evs.size() && evs[ev].t <= t) {
      if (evs[ev].on) engine.noteOn(evs[ev].note, 0.8, 0, 0); else engine.noteOff(evs[ev].note, 0);
      ++ev;
    }
    for (const Sweep& sw : sweeps)
      sw.control->set(sw.from + (sw.to - sw.from) * t / total);
    engine.process();
    const mopo::mopo_float* l = engine.output(0)->buffer;
    const mopo::mopo_float* r = engine.output(1)->buffer;
    for (int i = 0; i < buf && done < frames; ++i, ++done) { L.push_back(l[i]); R.push_back(r[i]); }
  }
  writeWav(out, L, R, sr);
  double peak = 0, rms = 0;
  for (size_t i = 0; i < L.size(); ++i) { peak = std::max(peak, (double)std::fabs(L[i])); rms += L[i] * L[i]; }
  printf("%s: %zu frames, peak %.3f, rms %.4f\n", out, L.size(), peak, std::sqrt(rms / L.size()));
  return 0;
}
