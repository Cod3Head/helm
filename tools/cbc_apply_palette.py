#!/usr/bin/env python3
"""Map Helm's original UI colours to the Clear But Complex / SignalForge palette.

SignalForge palette (sampled from the SignalForge app):
  base #08090b, panel #111318, raised #14161c, text #f1f4f7, muted #9ca6b3,
  mint #54f2c6 (primary accent), hot pink #ff2d55 (hero action / modulation).

Rules:
  * Accent colours are mapped for every alpha value (alpha is preserved).
  * Greys are mapped only when fully opaque, so shadows (black with alpha) and
    hover overlays (white with alpha) keep working exactly as before.
  * src/look_and_feel/colors.cpp holds the named palette and is written by hand.

Usage (from the repository root):  python3 tools/cbc_apply_palette.py
Idempotent: palette outputs are never inputs.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

ACCENTS = {  # rgb -> rgb, any alpha
    "03a9f4": "54f2c6",  # Helm audio blue      -> mint
    "2196f3": "54f2c6",  # blue                 -> mint
    "4fc3f7": "8ff7da",  # light blue           -> light mint
    "00e676": "ff2d55",  # Helm modulation green-> hot pink
    "69f0ae": "ff6b87",  # light green          -> light pink
    "b9f6ca": "ff8fa6",  # pale green           -> pale pink
    "ffd740": "54f2c6",  # amber links          -> mint
    "ffc400": "54f2c6",  # amber highlight      -> mint
    "ffab00": "54f2c6",  # amber focus          -> mint
    "55bbff": "ff2d55",  # pay button           -> hero pink
}

GREYS = {  # opaque only
    "111111": "0b0c0f", "181818": "08090b", "212121": "0d0f13", "222222": "0f1115",
    "2a2a2a": "111318", "303030": "111318", "313131": "121419", "323232": "14161c",
    "333333": "14161c", "383838": "181b22", "424242": "1c1f27", "444444": "1e222a",
    "464646": "20242c", "484848": "22262f", "4a4a4a": "242832", "505050": "2a2f3a",
    "555555": "2e3440", "565656": "2a2f3a", "666666": "5f6875", "777777": "6f7885",
    "888888": "8a94a1", "999999": "9ca6b3", "aaaaaa": "b3bcc7", "bbbbbb": "c9d1da",
    "cccccc": "d5dce3", "dddddd": "e3e8ed", "ffffff": "f1f4f7",
}

TOKEN = re.compile(r"0x([0-9a-fA-F]{2})([0-9a-fA-F]{6})\b")


def remap(match: re.Match) -> str:
    alpha, rgb = match.group(1).lower(), match.group(2).lower()
    if rgb in ACCENTS:
        return f"0x{alpha}{ACCENTS[rgb]}"
    if alpha == "ff" and rgb in GREYS:
        return f"0xff{GREYS[rgb]}"
    return match.group(0)


# GLSL colours in the OpenGL shaders (oscilloscope line, modulation arcs, meter).
SHADER_SUBS = [
    ("vec4(0.04, 0.8, 1.0, 1.0)", "vec4(0.329, 0.949, 0.776, 1.0)"),   # blue line -> mint
    ("vec4(0.2, 0.941, 0.45, dist_amp", "vec4(1.0, 0.176, 0.333, dist_amp"),  # green arc -> pink
    ("amp * vec4(0.5 * position_out.x + 0.5, 1.0, 0.4, 1.0)",
     "amp * vec4(0.329 + 0.4 * position_out.x, 0.949, 0.776 - 0.3 * position_out.x, 1.0)"),  # meter -> mint
]


def main() -> None:
    changed = 0
    for path in sorted((ROOT / "src").rglob("*")):
        if path.suffix not in (".cpp", ".h") or path.name == "colors.cpp":
            continue
        text = path.read_bytes().decode()  # bytes I/O keeps line endings
        new = TOKEN.sub(remap, text)
        if path.name == "shaders.cpp":
            for old, rep in SHADER_SUBS:
                new = new.replace(old, rep)
        if new != text:
            path.write_bytes(new.encode())
            changed += 1
    print(f"recoloured {changed} files")


if __name__ == "__main__":
    main()
