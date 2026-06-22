#!/usr/bin/env python3
"""Generate the menu-icon RGB565 header for the low-contrast classic-CYD (3E) panel.

Unlike icons_to_header.py (which kept faint glow for the S3 IPS panel), this:
  * boosts brightness + saturation so neon art pops on the dim 3E TN panel,
  * drops faint low-alpha haze (alpha < 24) — that haze is what "white-washes"
    on the 3E,
  * ALSO keys out near-black pixels (luminance < LUMA_KEY) so icons whose source
    has no real transparency (e.g. the SIGINT chart) don't render as a solid tile.

Usage: python icons_to_header_vivid.py <src-dir> <out.h> [--size 50]
"""
import sys, os, argparse, glob
from PIL import Image, ImageEnhance

KEY = 0xF8F8        # symmetric (byte-swap-invariant) magenta transparent key
ALPHA_KEY = 24      # drop pixels fainter than this (kills the wash haze)
LUMA_KEY  = 38      # also treat near-black art-background as transparent
SAT   = 1.7         # saturation boost
BRIGHT = 1.55       # brightness boost
CONTRAST = 1.12     # mild contrast boost

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src_dir")
    ap.add_argument("out")
    ap.add_argument("--size", type=int, default=50)
    a = ap.parse_args()
    S = a.size

    files = sorted(glob.glob(os.path.join(a.src_dir, "[0-9][0-9]_*.png")))
    if len(files) != 10:
        print(f"WARNING: expected 10 icons, found {len(files)}", file=sys.stderr)

    def to565(path):
        im = Image.open(path).convert("RGBA").resize((S, S), Image.LANCZOS)
        alpha = im.split()[3]
        rgb = im.convert("RGB")
        rgb = ImageEnhance.Color(rgb).enhance(SAT)
        rgb = ImageEnhance.Brightness(rgb).enhance(BRIGHT)
        rgb = ImageEnhance.Contrast(rgb).enhance(CONTRAST)
        rp, ap_ = rgb.load(), alpha.load()
        out = []
        for y in range(S):
            for x in range(S):
                r, g, b = rp[x, y]
                al = ap_[x, y]
                lum = (r * 30 + g * 59 + b * 11) // 100
                if al < ALPHA_KEY or lum < LUMA_KEY:
                    out.append(KEY)
                    continue
                v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                if v == KEY:
                    v ^= 0x0020
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
            vals = to565(path)
            keyed = sum(1 for v in vals if v == KEY)
            f.write(f"// {base}  ({100*keyed//len(vals)}% transparent)\n")
            f.write(f"const uint16_t {name}[{S*S}] PROGMEM = {{\n")
            for i in range(0, len(vals), 12):
                f.write("  " + ",".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",\n")
            f.write("};\n\n")
        f.write(f"const uint16_t* const wh_menu_icons[{len(names)}] = {{\n  ")
        f.write(", ".join(names))
        f.write("\n};\n\n#endif\n")
    print(f"Wrote {a.out}: {len(files)} icons @ {S}x{S}")

if __name__ == "__main__":
    main()
