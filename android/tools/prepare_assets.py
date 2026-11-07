#!/usr/bin/env python3
"""Package the ACTUAL PSP Bald Elite sounds into the Android APK.

Input: generated PSP audio headers from the successful Oct 20 2026 PSP build.
These are ORIGINAL, self-created Bald Elite tracks, not commercial game audio.
Outputs: unsigned PCM8 .pcm audio streams for mixing with Android AudioTrack,
plus platform-specific PNG launcher icons made from the branded PSP palette.
No on-device synthesis of the soundtrack, internet or file permissions.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
APP = ROOT / "android" / "app" / "src" / "main"
WAVS = {
    "bald_music_full": "psp_title_vocals.pcm",
    "bald_music_instrumental": "psp_game_instrumental.pcm",
    "voice_sample": "psp_victory_voice.pcm",
}
SIZES = {"bald_music_full": 751538, "bald_music_instrumental": 751538}
FONTS = (
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf",
)

def extract(header: Path, symbol: str) -> bytes:
    data = header.read_text(encoding="ascii")
    pattern = r"static\s+const\s+unsigned\s+char\s+" + re.escape(symbol) + r"\s*\[[^\]]*\]\s*[^=]*=\s*\{(.*?)\};"
    m = re.search(pattern, data, re.S)
    if not m:
        raise RuntimeError(f"{symbol} is missing in {header}")
    values = re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1))
    if not values:
        raise RuntimeError("Header didn't contain PCM8 bytes for " + symbol)
    pcm = bytes(int(t, 16) for t in values)
    limit = re.search(r"#define\s+" + ("BALD_MUSIC_FULL_LEN" if symbol=="bald_music_full" else
                                     "BALD_MUSIC_INSTRUMENTAL_LEN" if symbol=="bald_music_instrumental" else
                                     "VOICE_SAMPLE_LEN") + r"\s+(\d+)", data)
    if not limit or len(pcm) != int(limit.group(1)):
        raise RuntimeError(f"PCM length mismatch for {symbol}: {len(pcm)}")
    return pcm

def generate_icon():
    """Crisp, original icon: salon blue, gold accents, bald head and hair clippers."""
    dim = 640
    icon=Image.new("RGBA",(dim,dim))
    px=icon.load()
    for y in range(dim):
        for x in range(dim):
            glow=max(0,1-((x-245)**2+(y-230)**2)**.5/580)
            px[x,y]=(int(8+24*glow),int(17+33*glow),int(29+42*glow),255)
    d=ImageDraw.Draw(icon)
    d.rounded_rectangle((20,20,620,620),radius=115,outline=(210,170,99,255),width=14)
    d.rounded_rectangle((31,31,609,609),radius=105,outline=(77,151,156,255),width=5)
    d.ellipse((126,40,514,428),fill=(29,66,77,255),outline=(185,142,68,255),width=7)
    # Shoulder / cape, nape, ears, warm golden bald face.
    d.ellipse((134,358,496,579),fill=(43,105,114,255))
    d.rounded_rectangle((259,326,408,465),radius=28,fill=(181,127,93,255))
    d.ellipse((201,110,453,430),fill=(216,163,120,255),outline=(134,89,69,255),width=8)
    d.ellipse((199,255,229,314),fill=(184,126,94,255))
    d.ellipse((416,257,454,312),fill=(181,121,89,255))
    d.ellipse((247,125,359,190),fill=(244,208,164,255))
    d.arc((254,131,366,198),210,330,fill=(255,232,192,255),width=10)
    # Profile features.
    d.arc((253,246,325,274),180,345,fill=(112,72,55,255),width=7)
    d.ellipse((280,264,299,277),fill=(28,31,36,255))
    d.arc((297,326,386,355),8,158,fill=(128,72,62,255),width=6)
    d.arc((282,359,392,389),18,160,fill=(149,90,69,255),width=6)
    # Original stylised salon clippers, handle, teeth and shine.
    d.rounded_rectangle((411,201,481,420),radius=17,fill=(36,47,59,255),outline=(234,188,103,255),width=8)
    d.rounded_rectangle((425,221,465,385),radius=13,fill=(95,113,126,255))
    d.rectangle((418,181,475,222),fill=(190,202,201,255))
    for x in range(421,473,9):
        d.rectangle((x,172,x+5,199),fill=(224,231,224,255))
    d.rounded_rectangle((435,252,454,316),radius=8,fill=(211,187,110,255))
    # Glint above the polished bald head.
    d.line([(239,167),(239,208)],fill=(255,240,183,255),width=7)
    d.line([(222,187),(255,187)],fill=(255,240,183,255),width=7)
    d.rounded_rectangle((82,444,560,587),radius=32,fill=(10,23,37,245),outline=(201,159,82,255),width=6)
    def font(size):
        for file in FONTS:
            if Path(file).exists():
                return ImageFont.truetype(file,size)
        return ImageFont.load_default()
    for t,y,size,fill in [
        ("BALD",454,64,(248,209,129,255)),
        ("ELITE",518,64,(243,239,230,255)),
    ]:
        f=font(size)
        b=d.textbbox((0,0),t,font=f,stroke_width=1)
        d.text(((640-(b[2]-b[0]))/2,y),t,font=f,fill=fill,stroke_width=2,stroke_fill=(36,43,50,255))
    d.line((125,602,515,602),fill=(75,190,188,255),width=6)
    for density,scale in [("mdpi",48),("hdpi",72),("xhdpi",96),("xxhdpi",144),("xxxhdpi",192)]:
        dest=APP/"res"/("mipmap-"+density)
        dest.mkdir(parents=True,exist_ok=True)
        icon.resize((scale,scale),Image.Resampling.LANCZOS).save(dest/"ic_launcher.png",optimize=True)
    # Full-resolution source is also packaged for sharing/previews.
    folder=ROOT/"android"/"generated"
    folder.mkdir(parents=True,exist_ok=True)
    icon.resize((512,512),Image.Resampling.LANCZOS).save(folder/"BaldElite-Android-icon.png",optimize=True)
    print("Generated 5 launcher icon densities and a 512x512 master.")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--music-header",type=Path,required=True)
    ap.add_argument("--voice-header",type=Path,required=True)
    args=ap.parse_args()
    dest=APP/"assets"
    dest.mkdir(parents=True,exist_ok=True)
    for symbol,filename in WAVS.items():
        source=args.voice_header if symbol=="voice_sample" else args.music_header
        audio=extract(source,symbol)
        if symbol in SIZES and len(audio)!=SIZES[symbol]:
            raise RuntimeError(f"Unexpected soundtrack length for {symbol}: {len(audio)}")
        (dest/filename).write_bytes(audio)
        print(f"{symbol}: {len(audio)} raw PCM8 bytes -> {filename}")
    generate_icon()

if __name__=="__main__":
    main()
