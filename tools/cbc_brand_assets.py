#!/usr/bin/env python3
"""Clear But Complex // SignalForge Synth brand assets.

Draws the product icon (a sibling of the SignalForge icon: mint rounded frame,
dark field, one hot-pink accent) at every size the app uses, writes the PNG/ICO
files in images/, and re-embeds the icons in both JuceLibraryCode/BinaryData
copies so no Projucer run is needed.

Usage (from the repository root):  python3 tools/cbc_brand_assets.py
Requires Pillow. Internal file names (helm_icon_*) are kept on purpose so the
build files do not change; only the pixels do.
"""
import io
import math
import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
BG = (8, 9, 11, 255)          # #08090b
FIELD = (17, 19, 24, 255)     # #111318
MINT = (84, 242, 198, 255)    # #54f2c6
PINK = (255, 45, 85, 255)     # #ff2d55
GREY = (156, 166, 179, 255)   # #9ca6b3
WHITE = (241, 244, 247, 255)  # #f1f4f7

MASTER = 1024


def draw_master() -> Image.Image:
    s = MASTER
    im = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    pad, stroke, radius = int(s * 0.07), int(s * 0.055), int(s * 0.20)
    d.rounded_rectangle([pad, pad, s - pad, s - pad], radius=radius, fill=FIELD,
                        outline=MINT, width=stroke)

    # Baseline, like SignalForge's meter axis.
    left, right = int(s * 0.20), int(s * 0.80)
    mid = s // 2
    d.line([(left, mid), (right, mid)], fill=GREY, width=max(2, s // 110))

    # A waveform that morphs from sine (left) to saw (right): the synth's job.
    pts = []
    n = 400
    cycles = 2.5
    amp = s * 0.20
    for i in range(n + 1):
        t = i / n
        phase = (t * cycles) % 1.0
        sine = math.sin(2 * math.pi * phase)
        saw = 1.0 - 2.0 * phase
        y = (1 - t) * sine + t * saw
        pts.append((left + t * (right - left), mid - y * amp))
    # Stamp round dots along the path: smooth joins at every corner.
    w = s * 0.0175
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        steps = max(1, int(math.hypot(x1 - x0, y1 - y0) / (w * 0.35)))
        for k in range(steps + 1):
            x = x0 + (x1 - x0) * k / steps
            y = y0 + (y1 - y0) * k / steps
            d.ellipse([x - w, y - w, x + w, y + w], fill=MINT)

    # Single hot-pink accent: the modulation point.
    px, py = pts[int(n * 0.62)]
    r = int(s * 0.045)
    d.ellipse([px - r, py - r, px + r, py + r], fill=PINK)

    # "CBC" mark, as on the SignalForge icon.
    try:
        font = ImageFont.truetype("DejaVuSans.ttf", int(s * 0.07))
    except OSError:
        font = ImageFont.load_default()
    d.text((int(s * 0.27), int(s * 0.74)), "CBC", fill=WHITE, font=font)
    return im


def png_bytes(img: Image.Image) -> bytes:
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def write_images(master: Image.Image) -> dict:
    out = {}
    for f in sorted((ROOT / "images").glob("helm_icon_*.png")):
        w, h = Image.open(f).size
        data = png_bytes(master.resize((w, h), Image.LANCZOS))
        f.write_bytes(data)
        out[f.name] = data
    ico = ROOT / "images" / "helm_icon_128_1x.ico"
    if ico.exists():
        master.save(ico, format="ICO", sizes=[(16, 16), (32, 32), (48, 48), (128, 128)])
    return out


def c_array(data: bytes) -> str:
    rows = []
    for i in range(0, len(data), 40):
        rows.append(",".join(str(b) for b in data[i:i + 40]))
    return "{ " + ",\n".join(rows) + ",0,0};"


def patch_binary_data(folder: Path, images: dict) -> int:
    cpp, hdr = folder / "BinaryData.cpp", folder / "BinaryData.h"
    if not cpp.exists():
        return 0
    src = cpp.read_bytes().decode()
    head = hdr.read_bytes().decode()  # bytes I/O keeps the files' CRLF endings
    count = 0
    for fname, data in images.items():
        ident = fname.replace(".", "_")
        block = re.compile(
            r"(//=+ " + re.escape(fname) + r" =+\nstatic const unsigned char (temp_binary_data_\d+)\[\] =\n)\{.*?\};",
            re.S)
        if not block.search(src):
            continue
        src = block.sub(lambda m: m.group(1) + c_array(data), src, count=1)
        src = re.sub(r"(numBytes = )\d+(; return " + re.escape(ident) + r";)",
                     lambda m: m.group(1) + str(len(data)) + m.group(2), src)
        head = re.sub(r"(const int\s+" + re.escape(ident) + r"Size = )\d+;",
                      lambda m: m.group(1) + str(len(data)) + ";", head)
        count += 1
    cpp.write_bytes(src.encode())
    hdr.write_bytes(head.encode())
    return count


def main() -> None:
    master = draw_master()
    images = write_images(master)
    for folder in (ROOT / "JuceLibraryCode", ROOT / "standalone" / "JuceLibraryCode"):
        n = patch_binary_data(folder, images)
        print(f"{folder.relative_to(ROOT)}: re-embedded {n} icons")
    print(f"wrote {len(images)} PNG sizes")


if __name__ == "__main__":
    main()
