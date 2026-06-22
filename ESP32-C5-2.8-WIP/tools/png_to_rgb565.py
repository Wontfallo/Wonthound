#!/usr/bin/env python3
"""Convert a PNG into a 240x320 RGB565 C header for TFT_eSPI pushImage().

Center-crops the source to the 3:4 (portrait) aspect of the 2.8" panel, resizes
to 240x320, optionally darkens it so overlaid magenta menu text stays readable,
and emits a PROGMEM uint16_t array.

Usage:
  python png_to_rgb565.py <in.png> <out.h> [--name wonthound_bg] [--dim 0.55]
"""
import sys, argparse
from PIL import Image

SCREEN_W, SCREEN_H = 240, 320

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("out")
    ap.add_argument("--name", default="wonthound_bg")
    ap.add_argument("--dim", type=float, default=0.55,
                    help="brightness multiplier 0..1 (lower = darker bg)")
    a = ap.parse_args()

    img = Image.open(a.src).convert("RGB")
    # Center-crop to 3:4 portrait aspect
    tw, th = SCREEN_W, SCREEN_H
    target_ar = tw / th
    w, h = img.size
    ar = w / h
    if ar > target_ar:          # too wide -> crop width
        nw = int(h * target_ar)
        left = (w - nw) // 2
        img = img.crop((left, 0, left + nw, h))
    else:                        # too tall -> crop height
        nh = int(w / target_ar)
        top = (h - nh) // 2
        img = img.crop((0, top, w, top + nh))
    img = img.resize((tw, th), Image.LANCZOS)

    dim = max(0.0, min(1.0, a.dim))
    px = img.load()
    vals = []
    for y in range(th):
        for x in range(tw):
            r, g, b = px[x, y]
            r = int(r * dim); g = int(g * dim); b = int(b * dim)
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            vals.append(rgb565)

    with open(a.out, "w") as f:
        guard = f"{a.name.upper()}_HEADER_INCLUDED"
        f.write(f"#ifndef {guard}\n#define {guard}\n")
        f.write("#include <pgmspace.h>\n\n")
        f.write(f"#define {a.name.upper()}_W {tw}\n")
        f.write(f"#define {a.name.upper()}_H {th}\n\n")
        f.write(f"// RGB565, {tw}x{th}, dim={dim}. Use: tft.setSwapBytes(true); "
                f"tft.pushImage(0,0,{tw},{th},{a.name});\n")
        f.write(f"const uint16_t {a.name}[{tw*th}] PROGMEM = {{\n")
        for i in range(0, len(vals), 12):
            f.write("  " + ",".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",\n")
        f.write("};\n\n#endif\n")
    print(f"Wrote {a.out}: {tw}x{th}, {len(vals)} px, {len(vals)*2} bytes")

if __name__ == "__main__":
    main()
