#!/usr/bin/env python3
"""Kindred crew artwork: Amelia, Catalina and Persefone from SignalForge.

Builds three 4-frame animation strips (cell 192x256, drawn at half size on
normal screens and full size on HiDPI) from the SignalForge source artwork and
embeds them in both JuceLibraryCode/BinaryData copies as crew_*_png.

  Amelia    - SignalForge startup sheet, row 1 (four dance poses).
              The flat #05080e backdrop is removed by flood fill from the cell
              edges only, so her dark outlines are never keyed out.
  Catalina  - catalina_animation_v3.png (head-shake "no, no, no").
  Persefone - persefone_animation_v3.png (wave, run, run, jump).

Only generated character artwork is used, as in SignalForge; no photographs.

Usage (from the repository root):
  python3 tools/cbc_crew_assets.py <path to SignalForge CBC_Mastering_Tool/assets>
Requires Pillow, NumPy and SciPy.
"""
import io
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parent.parent
CELL_W, CELL_H = 192, 256


def fit_cells(cells) -> list:
    """Scale character cells into CELL_W x CELL_H with ONE shared crop and scale,
    so a body that is fixed in the source stays fixed in every frame."""
    cells = [c.convert("RGBA") for c in cells]
    boxes = [c.getbbox() or (0, 0, c.width, c.height) for c in cells]
    box = (min(b[0] for b in boxes), min(b[1] for b in boxes),
           max(b[2] for b in boxes), max(b[3] for b in boxes))
    w, h = box[2] - box[0], box[3] - box[1]
    scale = min((CELL_W - 8) / w, (CELL_H - 6) / h)
    size = (max(1, round(w * scale)), max(1, round(h * scale)))
    out = []
    for c in cells:
        frame = c.crop(box).resize(size, Image.LANCZOS)
        cell = Image.new("RGBA", (CELL_W, CELL_H), (0, 0, 0, 0))
        cell.alpha_composite(frame, ((CELL_W - size[0]) // 2, CELL_H - 3 - size[1]))
        out.append(cell)
    return out


def strip(cells) -> Image.Image:
    out = Image.new("RGBA", (CELL_W * 4, CELL_H), (0, 0, 0, 0))
    for i, c in enumerate(cells):
        out.alpha_composite(c, (i * CELL_W, 0))
    return out


def remove_flat_backdrop(img: Image.Image, tol: float = 26.0) -> Image.Image:
    """Make the border-connected flat backdrop transparent."""
    rgb = np.asarray(img.convert("RGB")).astype(np.float32)
    corners = np.array([rgb[0, 0], rgb[0, -1], rgb[-1, 0], rgb[-1, -1]])
    bg = np.median(corners, axis=0)
    near = np.linalg.norm(rgb - bg, axis=2) < tol
    labels, _ = ndimage.label(near)
    edge = np.unique(np.concatenate([labels[0], labels[-1], labels[:, 0], labels[:, -1]]))
    backdrop = np.isin(labels, edge[edge > 0])
    # Soft 1px edge so the sprite does not carry a hard dark fringe.
    soft = ndimage.binary_dilation(backdrop) & ~backdrop
    alpha = np.where(backdrop, 0, 255).astype(np.uint8)
    alpha[soft] = 255
    rgba = np.dstack([rgb.astype(np.uint8), alpha])
    return Image.fromarray(rgba, "RGBA")


def build(assets: Path) -> dict:
    sheet = Image.open(assets / "startup_mascot_sheet_v1.png")
    cw, ch = sheet.width / 4, sheet.height / 3
    amelia = fit_cells([remove_flat_backdrop(sheet.crop((round(i * cw), 0, round((i + 1) * cw), round(ch))))
                        for i in range(4)])

    def four(name):
        s = Image.open(assets / f"{name}_animation_v3.png").convert("RGBA")
        w = s.width / 4
        return fit_cells([s.crop((round(i * w), 0, round((i + 1) * w), s.height)) for i in range(4)])

    return {"crew_amelia.png": strip(amelia),
            "crew_catalina.png": strip(four("catalina")),
            "crew_persefone.png": strip(four("persefone"))}


def png_bytes(img: Image.Image) -> bytes:
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def c_array(data: bytes, nl: str) -> str:
    rows = [",".join(str(b) for b in data[i:i + 40]) for i in range(0, len(data), 40)]
    return "{ " + ("," + nl).join(rows) + ",0,0};"


def juce_hash(name: str) -> int:
    h = 0
    for ch in name.encode():
        h = (31 * h + ch) & 0xFFFFFFFF
    return h


def embed(folder: Path, files: dict) -> None:
    cpp_path, hdr_path = folder / "BinaryData.cpp", folder / "BinaryData.h"
    cpp, hdr = cpp_path.read_bytes().decode(), hdr_path.read_bytes().decode()
    nl = "\r\n" if "\r\n" in cpp else "\n"
    hnl = "\r\n" if "\r\n" in hdr else "\n"
    for fname, data in files.items():
        ident = fname.replace(".", "_")
        block = re.compile(r"(//=+ " + re.escape(fname) + r" =+\r?\nstatic const unsigned char temp_binary_data_\d+\[\] =\r?\n)\{.*?\};", re.S)
        if block.search(cpp):  # already embedded: refresh data and sizes
            cpp = block.sub(lambda m: m.group(1) + c_array(data, nl), cpp, count=1)
            cpp = re.sub(r"(numBytes = )\d+(; return " + re.escape(ident) + r";)",
                         lambda m: m.group(1) + str(len(data)) + m.group(2), cpp)
            hdr = re.sub(r"(const int\s+" + re.escape(ident) + r"Size = )\d+;",
                         lambda m: m.group(1) + str(len(data)) + ";", hdr)
            continue
        idx = max(int(n) for n in re.findall(r"temp_binary_data_(\d+)\[\]", cpp)) + 1
        res = (f"//================== {fname} =================={nl}"
               f"static const unsigned char temp_binary_data_{idx}[] ={nl}{c_array(data, nl)}{nl}{nl}"
               f"const char* {ident} = (const char*) temp_binary_data_{idx};{nl}{nl}{nl}")
        anchor = "const char* getNamedResource ("
        cpp = cpp.replace(anchor, res + anchor, 1)
        case = f"        case 0x{juce_hash(ident):08x}:  numBytes = {len(data)}; return {ident};{nl}"
        cpp = re.sub(r"(    switch \(hash\)\r?\n    \{\r?\n(?:.*\r?\n)*?)(    \}\r?\n)",
                     lambda m: m.group(1) + case + m.group(2), cpp, count=1)
        cpp = re.sub(r"(const char\* namedResourceList\[\] =\r?\n\{(?:.*\r?\n)*?)(    \"[^\"]+\")(\r?\n\};)",
                     lambda m: m.group(1) + m.group(2) + "," + nl + f'    "{ident}"' + m.group(3), cpp, count=1)
        cpp = re.sub(r"(const char\* originalFilenames\[\] =\r?\n\{(?:.*\r?\n)*?)(    \"[^\"]+\")(\r?\n\};)",
                     lambda m: m.group(1) + m.group(2) + "," + nl + f'    "{fname}"' + m.group(3), cpp, count=1)
        decl = (f"    extern const char*   {ident};{hnl}"
                f"    const int            {ident}Size = {len(data)};{hnl}{hnl}")
        hdr = hdr.replace("    // Number of elements in the namedResourceList", decl + "    // Number of elements in the namedResourceList", 1)
        hdr = re.sub(r"(const int namedResourceListSize = )(\d+);", lambda m: m.group(1) + str(int(m.group(2)) + 1) + ";", hdr)
    cpp_path.write_bytes(cpp.encode())
    hdr_path.write_bytes(hdr.encode())


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    images = build(Path(sys.argv[1]))
    files = {}
    for name, img in images.items():
        data = png_bytes(img)
        (ROOT / "images" / name).write_bytes(data)
        files[name] = data
        print(f"{name}: {img.size[0]}x{img.size[1]}, {len(data)} bytes")
    for folder in (ROOT / "JuceLibraryCode", ROOT / "standalone" / "JuceLibraryCode"):
        embed(folder, files)
        print(f"embedded in {folder.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
