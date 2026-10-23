#!/usr/bin/env python3
"""Train Catalina's Character models and export them for Kindred (RTNeural).

Each model is a small neural network: a window of the most recent 16 input
samples -> 16 tanh units -> 16 tanh units -> 1 output sample, at 48 kHz.

Built-in models are trained on simulated sources:
  Diode - diode clipper circuit (R, C, silicon diode pair), solved numerically
          (backward Euler, safeguarded Newton) at 4x oversampling.
  Tube  - 12AX7 common-cathode triode stage (Koren model), load line solved per
          sample, with grid-current limiting and plate (Miller) roll-off.
  Fold  - smooth triangle wavefolder (a mathematical shaper, not a circuit).

Train on your own sound instead (a dry recording and the same recording through
the gear or plugin you love, both 48 kHz mono WAV, same length and alignment):
  python3 tools/kindred_train_character.py --pair dry.wav wet.wav --name "My Amp"
New models are appended to the built-in ones.

Writes src/kindred/kindred_character_models.h. Requires numpy, scipy, scikit-learn.
"""
import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np
from scipy import signal
from scipy.io import wavfile
from sklearn.neural_network import MLPRegressor

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "src" / "kindred" / "kindred_character_models.h"
FS = 48000
WINDOW = 16
HIDDEN = 16
RNG = np.random.default_rng(7)


# ----------------------------------------------------------------------------- data
def training_input(seconds=24.0):
    """Synth-like material over a wide level range: saws, squares, sines, chords,
    plucks and noise bursts with envelopes. Values are in network-input units."""
    n_chunk = int(0.4 * FS)
    chunks = []
    t = np.arange(n_chunk) / FS
    for _ in range(int(seconds / 0.4)):
        kind = RNG.integers(0, 5)
        f0 = 40.0 * 2 ** RNG.uniform(0, 6.5)
        if kind == 0:
            x = signal.sawtooth(2 * np.pi * f0 * t)
        elif kind == 1:
            x = signal.square(2 * np.pi * f0 * t, duty=RNG.uniform(0.2, 0.8))
        elif kind == 2:
            x = np.sin(2 * np.pi * f0 * t)
        elif kind == 3:
            x = sum(signal.sawtooth(2 * np.pi * f0 * r * t) for r in (1, 1.26, 1.5)) / 3
        else:
            x = RNG.standard_normal(n_chunk)
            b, a = signal.butter(2, RNG.uniform(500, 12000) / (FS / 2))
            x = signal.lfilter(b, a, x)
            x /= np.max(np.abs(x)) + 1e-9
        env = np.exp(-t * RNG.uniform(0, 12)) if RNG.random() < 0.5 else np.minimum(1, t * 40)
        level = 10 ** RNG.uniform(np.log10(0.03), np.log10(6.0))
        chunks.append(x * env * level)
    return np.concatenate(chunks).astype(np.float64)


def windows(x):
    pad = np.concatenate([np.zeros(WINDOW - 1), x])
    return np.lib.stride_tricks.sliding_window_view(pad, WINDOW)


# ------------------------------------------------------------------------- circuits
def diode_clipper(x, volts=1.0):
    """Diode clipper: C dV/dt = (Vin - V)/R - 2 Is sinh(V / (n Vt)).

    The diode makes the circuit stiff (its time constant is far shorter than a
    sample), so each step uses backward Euler - stable for stiff circuits - and
    solves the implicit equation with Newton safeguarded by bisection. The
    equation is monotone in V, so the bracketed solve always converges."""
    R, C, Is, n, Vt = 2.2e3, 10e-9, 2.52e-9, 1.752, 0.02585
    os = 4
    T = 1.0 / (FS * os)
    vin = signal.resample_poly(x * volts, os, 1)
    out = np.empty_like(vin)
    a = T / (R * C)
    k = 2 * Is / C * T
    nvt = n * Vt
    sinh, cosh = math.sinh, math.cosh
    v = 0.0
    for i, u in enumerate(vin.tolist()):
        rhs = v + a * u
        lo, hi = -3.0, 3.0
        v1 = v
        for _ in range(60):
            g = v1 * (1 + a) + k * sinh(v1 / nvt) - rhs
            if g > 0:
                hi = v1
            else:
                lo = v1
            dg = 1 + a + k * cosh(v1 / nvt) / nvt
            nxt = v1 - g / dg
            if not (lo < nxt < hi):
                nxt = 0.5 * (lo + hi)
            if abs(nxt - v1) < 1e-10:
                v1 = nxt
                break
            v1 = nxt
        v = v1
        out[i] = v
    return signal.resample_poly(out, 1, os)


def triode_stage(x, swing=1.0):
    """12AX7 common cathode, Koren model; returns re-inverted plate swing."""
    mu, ex, kg1, kp, kvb = 100.0, 1.4, 1060.0, 600.0, 300.0
    bplus, rp, bias = 250.0, 100e3, -1.5
    vg = bias + x * swing
    # Grid current: the grid cannot go far above the cathode.
    vg = np.where(vg > 0, 0.4 * np.tanh(vg / 0.4), vg)

    def plate_current(vp, vgk):
        e1 = vp / kp * np.log1p(np.exp(np.clip(kp * (1 / mu + vgk / np.sqrt(kvb + vp * vp)), -50, 50)))
        return np.where(e1 > 0, np.power(np.maximum(e1, 0), ex) / kg1, 0.0)

    lo = np.zeros_like(vg)
    hi = np.full_like(vg, bplus)
    for _ in range(60):  # bisection on the load line (monotone)
        mid = 0.5 * (lo + hi)
        f = mid - (bplus - rp * plate_current(mid, vg))
        lo = np.where(f < 0, mid, lo)
        hi = np.where(f >= 0, mid, hi)
    vp = 0.5 * (lo + hi)
    vp_idle = vp[np.argmin(np.abs(x))] if np.any(np.abs(x) < 1e-3) else np.median(vp)
    y = -(vp - vp_idle)
    b, a = signal.butter(1, 14000 / (FS / 2))  # plate / Miller capacitance roll-off
    return signal.lfilter(b, a, y)


def wavefolder(x):
    tri = (2 / np.pi) * np.arcsin(np.sin(np.pi / 2 * x))
    return np.tanh(1.6 * tri) / np.tanh(1.6)


BUILTIN = [
    {"name": "Diode", "input_scale": 1.5, "fn": lambda x: diode_clipper(x, 1.0)},
    {"name": "Tube", "input_scale": 1.0, "fn": lambda x: triode_stage(x, 1.0)},
    {"name": "Fold", "input_scale": 1.2, "fn": wavefolder},
]


# ------------------------------------------------------------------------- training
def train(x, y, label):
    scale = 1.0 / (np.percentile(np.abs(y), 99.5) + 1e-9)
    X = windows(x)
    Y = y * scale
    idx = RNG.choice(len(X), size=min(len(X), 220000), replace=False)
    net = MLPRegressor(hidden_layer_sizes=(HIDDEN, HIDDEN), activation="tanh", solver="adam",
                       learning_rate_init=2e-3, batch_size=512, max_iter=120, tol=1e-7,
                       n_iter_no_change=12, early_stopping=True, validation_fraction=0.1,
                       random_state=1)
    net.fit(X[idx].astype(np.float32), Y[idx].astype(np.float32))
    pred = net.predict(X.astype(np.float32))
    err = np.sqrt(np.mean((pred - Y) ** 2)) / (np.sqrt(np.mean(Y ** 2)) + 1e-12)
    print(f"{label}: relative RMS error {err:.3f} ({net.n_iter_} epochs)")
    return net, 1.0 / scale, err


def to_rtneural_json(net):
    layers = []
    n = len(net.coefs_)
    for i, (W, b) in enumerate(zip(net.coefs_, net.intercepts_)):
        layers.append({
            "type": "dense",
            "activation": "tanh" if i < n - 1 else "",
            "shape": [None, None, int(W.shape[1])],
            "weights": [W.astype(float).tolist(), b.astype(float).tolist()],
        })
    return json.dumps({"in_shape": [None, None, WINDOW], "layers": layers}, separators=(",", ":"))


def net_forward(model_json, X):
    """Run an exported model (RTNeural JSON) in numpy; X is (n, WINDOW)."""
    h = X
    for layer in json.loads(model_json)["layers"]:
        W, b = (np.asarray(a, dtype=np.float64) for a in layer["weights"])
        h = h @ W + b
        if layer["activation"] == "tanh":
            h = np.tanh(h)
    return h[:, 0]


def calibrate(model):
    """Output scale for unity loudness: a 220 Hz saw at a typical synth level
    (peak 0.5) comes out at the same RMS it went in at."""
    t = np.arange(FS // 2) / FS
    x = 0.5 * signal.sawtooth(2 * np.pi * 220 * t)
    y = net_forward(model["json"], windows(x * model["input_scale"]))
    y -= y.mean()
    return float(np.sqrt(np.mean(x ** 2)) / (np.sqrt(np.mean(y ** 2)) + 1e-12))


def read_header():
    """Models from the current header (for --recalibrate)."""
    import re
    text = OUT.read_text()
    models = []
    for m in re.finditer(r'\{ "([^"]+)", ([-0-9.e]+)f, ([-0-9.e]+)f,\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\}', text):
        js = "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(4)))
        js = js.replace('\\"', '"').replace("\\\\", "\\")
        models.append({"name": m.group(1), "input_scale": float(m.group(2)),
                       "output_scale": float(m.group(3)), "json": js})
    return models


def write_header(models):
    lines = [
        "/* Clear But Complex // Kindred - Catalina's Character models.",
        " * GENERATED by tools/kindred_train_character.py - do not edit by hand.",
        " * Each model: 16-sample window -> Dense(16, tanh) -> Dense(16, tanh) -> Dense(1), 48 kHz.",
        " */",
        "",
        "#pragma once",
        "#ifndef KINDRED_CHARACTER_MODELS_H",
        "#define KINDRED_CHARACTER_MODELS_H",
        "",
        "namespace kindred {",
        "",
        f"  const int kCharacterWindow = {WINDOW};",
        f"  const int kCharacterHidden = {HIDDEN};",
        f"  const int kNumCharacterModels = {len(models)};",
        "",
        "  struct CharacterModel {",
        "    const char* name;",
        "    float input_scale;   // synth level -> network input",
        "    float output_scale;  // network output -> synth level",
        "    const char* json;    // RTNeural model JSON",
        "  };",
        "",
        "  const CharacterModel kCharacterModels[kNumCharacterModels] = {",
    ]
    for m in models:
        js = m["json"].replace("\\", "\\\\").replace('"', '\\"')
        chunks = [js[i:i + 2000] for i in range(0, len(js), 2000)]
        body = "\n      ".join('"' + c + '"' for c in chunks)
        lines.append(f'    {{ "{m["name"]}", {m["input_scale"]:.6f}f, {m["output_scale"]:.6f}f,')
        lines.append(f"      {body} }},")
    lines += ["  };", "", "} // namespace kindred", "", "#endif // KINDRED_CHARACTER_MODELS_H", ""]
    OUT.write_text("\n".join(lines))
    print(f"wrote {OUT.relative_to(ROOT)} ({len(models)} models)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--pair", nargs=2, metavar=("DRY", "WET"), help="train an extra model from recordings")
    ap.add_argument("--name", default="Custom", help="name for the --pair model")
    ap.add_argument("--report", help="write per-model accuracy JSON here")
    ap.add_argument("--recalibrate", action="store_true",
                    help="only recompute output levels of the models already in the header")
    args = ap.parse_args()

    if args.recalibrate:
        models = read_header()
        for m in models:
            m["output_scale"] = calibrate(m)
            print(f'{m["name"]}: output scale {m["output_scale"]:.4f}')
        write_header(models)
        return

    x = training_input()
    models, report = [], {}
    for spec in BUILTIN:
        y = spec["fn"](x)
        net, out_scale, err = train(x, y, spec["name"])
        models.append({"name": spec["name"], "input_scale": spec["input_scale"],
                       "output_scale": out_scale, "json": to_rtneural_json(net)})
        report[spec["name"]] = err

    if args.pair:
        sr_d, dry = wavfile.read(args.pair[0])
        sr_w, wet = wavfile.read(args.pair[1])
        if sr_d != FS or sr_w != FS:
            sys.exit("recordings must be 48 kHz")
        dry = dry.astype(np.float64).reshape(len(dry), -1).mean(axis=1)
        wet = wet.astype(np.float64).reshape(len(wet), -1).mean(axis=1)
        n = min(len(dry), len(wet))
        dry, wet = dry[:n], wet[:n]
        dry /= np.max(np.abs(dry)) + 1e-9
        net, out_scale, err = train(dry, wet / (np.max(np.abs(wet)) + 1e-9), args.name)
        models.append({"name": args.name, "input_scale": 1.0, "output_scale": out_scale,
                       "json": to_rtneural_json(net)})
        report[args.name] = err

    for m in models:
        m["output_scale"] = calibrate(m)
    write_header(models)
    if args.report:
        Path(args.report).write_text(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
