#!/usr/bin/env python3
"""Create the 144x80 PSP XMB icon for Bald Elite.

Original procedural illustration, no external logo or copyrighted graphics.
Run before make: python3 tools/generate_psp_icon.py
Requires: Pillow (on the build computer only).
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "ICON0.PNG"
W, H, SCALE = 144, 80, 3

canvas = Image.new("RGB", (W * SCALE, H * SCALE))
pixels = canvas.load()
for y in range(H * SCALE):
    amount = y / (H * SCALE - 1)
    for x in range(W * SCALE):
        glow = max(0., 1. - abs(x / SCALE - 25) / 92.)
        pixels[x, y] = (
            int(11 + 4 * amount + 6 * glow),
            int(20 + 10 * amount + 8 * glow),
            int(32 + 14 * amount + 10 * glow),
        )
draw = ImageDraw.Draw(canvas)

def rect(box, fill, **kw):
    draw.rectangle(tuple(int(v * SCALE) for v in box), fill=fill, **kw)

def oval(box, fill, **kw):
    draw.ellipse(tuple(int(v * SCALE) for v in box), fill=fill, **kw)

def polygon(points, fill):
    draw.polygon([(int(x * SCALE), int(y * SCALE)) for x, y in points], fill=fill)

def line(points, fill, width=1):
    draw.line([(int(x * SCALE), int(y * SCALE)) for x, y in points],
              fill=fill, width=int(width * SCALE), joint="curve")

FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

def font(size):
    try:
        return ImageFont.truetype(FONT, int(size * SCALE))
    except OSError:
        try:
            return ImageFont.truetype("DejaVuSans-Bold.ttf", int(size * SCALE))
        except OSError:
            return ImageFont.load_default()

def fitted(text, desired, max_px):
    size = desired
    while size >= 7:
        f = font(size)
        if draw.textbbox((0, 0), text, font=f)[2] <= max_px * SCALE:
            return f
        size -= 1
    return font(7)

# Club-coloured frame and understated accent rules.
rect((2, 2, W - 3, H - 3), None, outline=(194, 154, 79), width=SCALE)
rect((3, 3, W - 4, 5), (194, 154, 79))
rect((3, H - 7, W - 4, H - 4), (41, 132, 143))

# A shiny, deliberately bald club applicant's profile.
oval((7, 50, 49, 79), (37, 93, 104))
oval((14, 10, 43, 55), (205, 164, 122))
oval((10, 32, 18, 43), (189, 141, 105))
oval((18, 13, 34, 27), (238, 205, 161))
oval((20, 16, 30, 19), (255, 229, 185))
line([(33, 32), (39, 36), (35, 38)], (97, 74, 60), 1)
line([(31, 44), (38, 45)], (115, 80, 63), 1)
oval((25, 31, 28, 34), (31, 34, 41))
line([(16, 54), (46, 58)], (194, 154, 79), 2)
# A small glint on the polished scalp.
line([(39, 15), (39, 24)], (255, 225, 140), 1)
line([(35, 20), (43, 20)], (255, 225, 140), 1)

# The full PUBLIC game name, with no competing "Bald Boss" brand.
x = 51
for text, y, size, color in [
    ("BALD", 11, 19, (245, 216, 143)),
    ("ELITE", 34, 19, (245, 246, 242)),
    ("PSP GAME", 61, 10, (245, 216, 143)),
]:
    draw.text((x * SCALE, y * SCALE), text,
              font=fitted(text, size, 86), fill=color,
              stroke_width=0)

canvas = canvas.resize((W, H), Image.Resampling.LANCZOS)
OUT.parent.mkdir(parents=True, exist_ok=True)
canvas.save(OUT, format="PNG", optimize=True)
assert Image.open(OUT).size == (144, 80)
print(f"Generated branded PSP XMB icon: {OUT} ({OUT.stat().st_size} bytes)")
