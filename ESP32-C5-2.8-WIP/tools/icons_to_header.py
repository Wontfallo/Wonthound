#!/usr/bin/env python3
"""Convert the 10 split-icon PNGs into one RGB565 C header for the menu grid.

Files 01..10 in split-icons/ map directly onto the menu order
(WiFi, Bluetooth, 2.4GHz, SubGHz, RFID, JamDetect, SIGINT, Tools, Setting, About).
Each tile is composited over black (so anti-aliased rounded corners blend into the
dark background), resized square, and emitted as a PROGMEM uint16_t array.

Usage: python icons_to_header.py <split-icons-dir> <out.h> [--size 100]
"""
import sys, os, argparse, glob
from PIL import Image

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src_dir")
    ap.add_argument("out")
    ap.add_argument("--size", type=int, default=100)
    a = ap.parse_args()

    files = sorted(glob.glob(os.path.join(a.src_dir, "[0-9][0-9]_*.png")))
    if len(files) != 10:
        print(f"WARNING: expected 10 icons, found {len(files)}", file=sys.stderr)
    S = a.size

    KEY = 0xF8F8   # symmetric magenta key = byte-swap-invariant transparent color (absent from blue/grey icons)

    def to565(im):
        im = im.convert("RGBA").resize((S, S), Image.LANCZOS)
        px = im.load()
        out = []
        for y in range(S):
            for x in range(S):
                r, g, b, a = px[x, y]
                if a < 8:                       # only fully-transparent pixels keyed; keep the art (faint corners intentional)
                    out.append(KEY)
                    continue
                # composite the (possibly edge-antialiased) pixel over black
                r = r * a // 255; g = g * a // 255; b = b * a // 255
                v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                if v == KEY:
                    v ^= 0x0020   # nudge any accidental pure-green so it isn't keyed out
                out.append(v)
        return out

    with open(a.out, "w") as f:
        f.write("#ifndef WONTHOUND_ICONS_H\n#define WONTHOUND_ICONS_H\n")
        f.write("#include <pgmspace.h>\n\n")
        f.write(f"#define WH_ICON_W {S}\n#define WH_ICON_H {S}\n#define WH_ICON_COUNT {len(files)}\n")
        f.write(f"#define WH_ICON_TRANSPARENT 0x{KEY:04X}\n\n")
        names = []
        for idx, path in enumerate(files):
            base = os.path.splitext(os.path.basename(path))[0]
            name = f"wh_icon_{idx:02d}"
            names.append(name)
            vals = to565(Image.open(path))
            f.write(f"// {base}\nconst uint16_t {name}[{S*S}] PROGMEM = {{\n")
            for i in range(0, len(vals), 12):
                f.write("  " + ",".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",\n")
            f.write("};\n\n")
        f.write(f"const uint16_t* const wh_menu_icons[{len(names)}] = {{\n  ")
        f.write(", ".join(names))
        f.write("\n};\n\n#endif\n")
    print(f"Wrote {a.out}: {len(files)} icons @ {S}x{S} ({S*S*2} bytes each, {S*S*2*len(files)} total)")

if __name__ == "__main__":
    main()
