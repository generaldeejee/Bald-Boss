#!/usr/bin/env python3
"""Reproducible Bald Elite Deluxe fallback graphics.

The shipped Deluxe APK can use the commissioned full-resolution club artworks
by replacing these three JPEGs and crest PNG before signing the application.
This script creates a fully functioning independent open-source build even
without access to the commissioned image originals.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math, random
ROOT=Path(__file__).resolve().parents[2]
ASSETS=ROOT/"android/app/src/main/assets"
ASSETS.mkdir(parents=True,exist_ok=True)
W,H=1280,720
FONT_PATHS=[
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf",
]
def font(size):
    for p in FONT_PATHS:
        if Path(p).exists():return ImageFont.truetype(p,size)
    return ImageFont.load_default()
def centered(d,msg,y,sz,col):
    f=font(sz);box=d.textbbox((0,0),msg,font=f,stroke_width=1)
    d.text(((W-box[2]+box[0])//2,y),msg,font=f,fill=col,stroke_width=2,stroke_fill=(18,7,27))
def panel(img,draw):
    for y in range(H):
        t=y/(H-1)
        for x in range(W):
            glow=max(0,1-abs(x-W*.5)/(W*.8))
            img.putpixel((x,y),(int(12+10*glow+14*t),int(7+10*glow+4*t),int(24+28*glow+28*t)))
    d=draw
    d.rectangle((30,28,1250,690),outline="#ad7843",width=8)
    for x in [90,1185]:
        d.rounded_rectangle((x-24,70,x+24,550),radius=9,fill="#3c2247",outline="#d9ad5e",width=7)
    d.ellipse((360,-105,930,118),fill="#5c3f4c",outline="#ffe4a4",width=14)
    d.ellipse((402,-83,887,93),fill="#2c163d",outline="#b97848",width=8)
    d.rectangle((0,644,W,H),fill="#1b1833")
    d.rectangle((0,642,W,651),fill="#d0a65b")
    d.rounded_rectangle((921,205,1150,540),radius=30,fill="#261932",outline="#bf824b",width=10)
    for x in [962,1075]:
        d.rounded_rectangle((x,278,x+83,452),radius=20,fill="#1c2941",outline="#edc47a",width=5)
        d.line((x+15,280,x+75,280),fill="#feeaac",width=7)
    d.ellipse((80,511,400,735),fill="#201a36",outline="#9d6b46",width=10)
    d.ellipse((135,551,351,655),fill="#3c2458")
    for x in range(120,1200,120):
        d.ellipse((x,15,x+16,31),fill="#ffe8b2")
        d.line((x+8,30,x+8,63),fill="#d9ac5e",width=3)
    # Left framed club champion portraits.
    for k,y in enumerate([90,260]):
        d.rounded_rectangle((143,y,315,y+147),radius=12,fill="#1c1325",outline="#d3a452",width=9)
        d.ellipse((199,y+26,260,y+101),fill="#c7956c")
        d.polygon([(175,y+147),(205,y+92),(249,y+90),(283,y+147)],fill="#3e2b4f")
    d.rounded_rectangle((1090,92,1210,186),radius=12,fill="#401b53",outline="#f3c474",width=5)
    d.ellipse((1130,98,1170,142),outline="#dfb76b",width=4)
    d.arc((1130,127,1170,159),10,170,fill="#dfb76b",width=4)
def make(kind):
    im=Image.new("RGB",(W,H))
    d=ImageDraw.Draw(im);panel(im,d)
    if kind=="intro":
        centered(d,"BALD ELITE",168,104,"#f7d993")
        centered(d,"THE ELITE BALD MAN CLUB",292,28,"#f9eacb")
        centered(d,"THE MOST EXCLUSIVE CLUB IN TOWN",460,22,"#f4d4a2")
        centered(d,"NO HAIR. NO FEAR.",507,42,"#ffe1a3")
        centered(d,"YOUR INITIATION BEGINS HERE",576,20,"#d6b889")
    elif kind=="menu":
        centered(d,"BALD ELITE",64,52,"#ffe0a5")
        centered(d,"CLUB INITIATION",134,28,"#d5bbd7")
    elif kind=="victory":
        rng=random.Random(1992)
        for i in range(240):
            x=rng.randrange(45,1220);y=rng.randrange(80,635)
            colour=rng.choice(["#ffe8aa","#cf9b43","#d6acf7","#f18fbb"])
            d.polygon([(x,y),(x+4,y+8),(x+9,y+4),(x+4,y-5)],fill=colour)
        centered(d,"ANOTHER BALD BOSS!",185,74,"#ffe19b")
        centered(d,"MEMBERSHIP APPROVED!",287,35,"#f3f1e8")
        centered(d,"★  ★  ★",356,74,"#ffdc67")
    dest=ASSETS/("club_"+kind+".jpg")
    im.save(dest,quality=87,optimize=True,subsampling=0)
    print(dest, dest.stat().st_size)
for name in ["intro","menu","victory"]: make(name)
# Transparent emblem using the same completely original branding.
crest=Image.new("RGBA",(512,512),(0,0,0,0))
d=ImageDraw.Draw(crest)
d.ellipse((26,26,486,486),fill="#36163d",outline="#f6d174",width=24)
d.ellipse((53,53,459,459),outline="#b96d50",width=10)
d.ellipse((145,102,375,321),fill="#d5a27b",outline="#f8d99c",width=7)
d.ellipse((175,150,345,207),fill="#f2c9a4")
d.polygon([(178,330),(334,330),(399,462),(109,462)],fill="#26253d")
d.polygon([(185,79),(237,110),(255,59),(286,108),(345,76),(321,143),(198,145)],fill="#fbd87b")
d.rounded_rectangle((89,386,423,463),radius=20,fill="#34162e",outline="#ebbd69",width=7)
f=font(37);d.text((126,397),"BALD ELITE",font=f,fill="#fff0c2")
dest=ASSETS/"club_crest.png"
crest.save(dest,optimize=True)
print(dest,dest.stat().st_size)
