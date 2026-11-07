#!/usr/bin/env python3
"""Bald Elite 2.2: reproducible high-resolution photographic-style UV atlases.

Designed for the procedural 360-degree GameState.headPoint() head mesh:
- u = .5 + angle / (2*pi), v = (1.14 - y) / 2.16
- Eyes, mouth and nose are in the forward-facing u=.5 patch.
- Texture is seamless horizontally so back of the head has no line.
- Includes natural eyelid shapes, iris fibers, skin color/noise layers,
  pores, freckles, stubble and different complexions for each character.
- One packed detail atlas (normal perturbations R/G + scalp gloss B).
These assets are manufactured locally in CI, not downloaded from a model.
"""
import math
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
ROOT=Path(__file__).resolve().parents[2]
ASSETS=ROOT/"android/app/src/main/assets"
ASSETS.mkdir(parents=True,exist_ok=True)
N=2048
SEED=215544
yy,xx=np.mgrid[:N,:N].astype(np.float32)
X=xx/N;Y=yy/N
def gauss(cx,cy,rx,ry):
    return np.exp(-.5*(((X-cx)/rx)**2+((Y-cy)/ry)**2))
def noise(rng,width,height=N):
    w=max(2,int(width));h=max(2,int(height))
    # Tile in U to prevent a seam when the head rotates fully.
    coarse=rng.normal(0,1,(h,w)).astype(np.float32)
    coarse[:,-1]=coarse[:,0]
    img=Image.fromarray(coarse,mode="F")
    return np.asarray(img.resize((N,N),Image.Resampling.BICUBIC),dtype=np.float32).copy()
def noise_blend(rng):
    out=np.zeros((N,N),np.float32)
    for size,weight in [(28,1.4),(110,.7),(450,.28),(2048,.13)]:
        out+=noise(rng,size)*weight
    return out
# Deterministic skin height field reused in the separate roughness/bump atlas.
rng=np.random.default_rng(SEED)
tiny=noise_blend(rng)
smooth=noise(rng,40)
# Per-pixel photographic color field with small warm/cool color variation.
light_from_side=.07*np.cos((X-.5)*2*math.pi)
forehead=gauss(.5,.20,.12,.20)
cheeks=gauss(.36,.62,.10,.11)+gauss(.64,.62,.10,.11)
temples=gauss(.27,.39,.05,.24)+gauss(.73,.39,.05,.24)
jaw=gauss(.5,.89,.18,.08)
facecenter=np.clip(np.cos(2*math.pi*(X-.5)),0,1)
shaveShadow=gauss(.5,.82,.17,.13)
for person in range(3):
    rng=np.random.default_rng(SEED+person*1979)
    if person==0:
        base=np.array([211,158,125.],dtype=np.float32)
        freckleStrength=.35
        beardStrength=.18
    elif person==1:
        base=np.array([196,141,120.],dtype=np.float32)
        freckleStrength=.62
        beardStrength=.28
    else:
        base=np.array([218,173,139.],dtype=np.float32)
        freckleStrength=.25
        beardStrength=.35
    broad=smooth*3.4 + tiny*1.25 + rng.normal(0,.80,(N,N)).astype(np.float32)
    tone=(light_from_side*35+forehead*9-cheeks*5-temples*7-jaw*5)
    blush=cheeks*7
    undertone=facecenter*2.8
    skin=np.empty((N,N,3),np.uint8)
    skin[:,:,0]=np.clip(base[0]+broad+tone+blush+undertone,0,255).astype(np.uint8)
    skin[:,:,1]=np.clip(base[1]+broad*.94+tone*.83+undertone*.5,0,255).astype(np.uint8)
    skin[:,:,2]=np.clip(base[2]+broad*.90+tone*.72-blush*.18,0,255).astype(np.uint8)
    del broad
    img=Image.fromarray(skin,"RGB").convert("RGBA")
    del skin
    art=Image.new("RGBA",(N,N),(0,0,0,0))
    brush=ImageDraw.Draw(art,"RGBA")
    S=N/256.
    def xy(x,y):return (round(x*S),round(y*S))
    def ellipse(box,fill,width=0,outline=None):
        bounds=tuple(round(v*S) for v in box)
        if outline:brush.ellipse(bounds,fill=fill,outline=outline,width=round(width*S))
        else:brush.ellipse(bounds,fill=fill)
    def smooth_shape(points,fill):
        brush.polygon([xy(*p) for p in points],fill=fill)
    def line(points,fill,width):
        brush.line([xy(*p) for p in points],fill=fill,width=max(1,round(width*S)),joint="curve")
    # Broad transparent shadows are feathered, rather than drawn as flat circles.
    shaded=Image.new("RGBA",(N,N))
    sh=ImageDraw.Draw(shaded,"RGBA")
    for x in [105,151]:
        sh.ellipse(tuple(round(v*S) for v in (x-17,102,x+17,132)),fill=(70,39,35,45))
        sh.ellipse(tuple(round(v*S) for v in (x-15,91,x+15,108)),fill=(62,43,35,25))
    sh.ellipse(tuple(round(v*S) for v in (115,139,141,169)),fill=(80,43,31,28))
    sh.ellipse(tuple(round(v*S) for v in (99,179,157,211)),fill=(107,51,38,18))
    sh.ellipse(tuple(round(v*S) for v in (98,211,158,234)),fill=(95,50,39,20))
    shade=shaded.filter(ImageFilter.GaussianBlur(5*S))
    img=Image.alpha_composite(img,shade)
    del shaded,shade
    # Upper and lower lids are asymmetrical almond shapes; blue-green irises
    # have individual fibers and small corneal highlights.
    art=Image.new("RGBA",(N,N),(0,0,0,0));brush=ImageDraw.Draw(art,"RGBA")
    for side,x in enumerate([105,151]):
        irisx=x+(1 if side else -1)
        # Subtle brow ridge and sparse tiny hair fibers instead of solid ovals.
        browPts=[(x-17,103),(x-11,100),(x-4,98),(x+3,97),(x+11,99),(x+17,101)]
        line(browPts,(69,43,36,138),1.15)
        for i in range(60):
            u=rng.uniform(-16,16)
            yb=99+1.8*(u/16)**2+float(rng.normal(0,0.6))
            length=rng.uniform(1,3.5)
            line([(x+u,yb),(x+u+length,yb-.72)],(52,35,31,int(rng.integers(35,95))),.24)
        # Crease and softer lower folds.
        line([(x-15,109),(x-8,105),(x+3,104),(x+13,107)],(95,58,50,108),.65)
        line([(x-14,123),(x-4,125),(x+7,124),(x+14,121)],(133,83,65,92),.52)
        # Sclera lens shape is a narrow tapered eye (not a white circle).
        eyeTop=[(x-15,114),(x-11,111.4),(x-5,109.6),(x+3,109.5),(x+11,111),(x+16,114)]
        eyeBottom=[(x+14,116),(x+7,118.6),(x-3,119),(x-10,117),(x-15,114)]
        smooth_shape(eyeTop+eyeBottom,(231,218,198,245))
        ellipse((irisx-5.3,109.0,irisx+5.3,120.4),(56,102,110,255))
        ellipse((irisx-4.35,110.0,irisx+4.35,119.8),(71,125,135,255))
        for k in range(28):
            a=k*math.tau/28
            r0=1.8;r1=rng.uniform(3.25,4.5)
            line([(irisx+r0*math.cos(a),114.7+r0*math.sin(a)),
                  (irisx+r1*math.cos(a),114.7+r1*math.sin(a))],
                  (24,55,65,int(rng.integers(100,180))),.2)
        ellipse((irisx-2.4,112.2,irisx+2.4,117.5),(28,38,43,255))
        ellipse((irisx-3.4,111.0,irisx-1.3,113.4),(250,247,240,245))
        ellipse((irisx+2.4,116,irisx+3.5,117.2),(239,239,238,190))
        line([(x-15,114),(x-8,111),(x,110),(x+8,111.6),(x+16,114)],
             (53,36,33,195),.78)
        line([(x-12,117),(x-5,120),(x+6,120),(x+14,116)],
             (117,74,63,150),.35)
        ellipse((x-15.5,113,x-12.9,116),(177,113,103,135))
    # An organic nose: bridge specular, under-tip occlusion, alar wing and nostrils.
    line([(127,128),(126,137),(125,147),(125.5,154)],(238,205,168,55),1.2)
    ellipse((120.5,155,135.5,167),(175,118,95,38))
    ellipse((120.3,158.6,123.9,164),(119,68,58,122))
    ellipse((132.1,158.6,135.7,164),(119,68,58,122))
    line([(118,161),(121,164.5),(124.5,165)],(138,85,68,65),.35)
    line([(131.5,165),(135,164),(138,161)],(138,85,68,65),.35)
    ellipse((124,153.4,132,164.8),(242,188,150,75))
    # Subtle philtrum shadow and smooth natural lip contours.
    line([(125,174),(127,183)],(117,67,54,32),.45)
    line([(131,174),(129,183)],(117,67,54,32),.45)
    topLip=[(105,193),(110,189),(119,187.7),(124,188.5),(128,190.0),
            (132,188.5),(137,187.7),(146,189),(151,193),
            (146,194),(137,194.1),(128,195),(119,194.1),(110,194)]
    smooth_shape(topLip,(153,89,83,124))
    bottomLip=[(106,194),(115,195.0),(128,195.5),(141,195.0),(150,194),
               (142,199.0),(132,201),(119,200),(110,198)]
    smooth_shape(bottomLip,(184,115,105,108))
    line([(109,194),(118,194.6),(126,194.9),(135,194.6),(146,194)],
         (114,65,64,155),.55)
    line([(114,201.8),(126,203.3),(141,202)],(241,184,151,63),.45)
    # Speckled beard shadow / stubble in moustache and chin zone.
    if beardStrength>0.17:
        for k in range(3000 if person==2 else 1100):
            x=float(rng.uniform(83,173));y=float(rng.uniform(171,236))
            if 185<y<204 and 100<x<156:continue
            if ((x-128)/43)**2+((y-210)/30)**2>1:continue
            alpha=int(rng.uniform(3,22)*beardStrength/.3)
            r=rng.uniform(.11,.38)
            ellipse((x-r,y-r,x+r,y+r),(65,50,45,alpha))
    # Tiny freckles and skin pores, no big repetitive stipple.
    for i in range(24000):
        x=float(rng.uniform(30,225));y=float(rng.uniform(27,250))
        if i%5==0 and 132<y<177:
            x=float(rng.uniform(78,178))
        radius=rng.uniform(.035,.12)
        alpha=int(rng.uniform(4,20)*(freckleStrength if i%9==0 else .66))
        ellipse((x-radius,y-radius,x+radius,y+radius),(100,61,47,alpha))
    # Vertical glossy forehead highlight, not flat circular shine.
    line([(120,55),(118,62),(122,70)],(255,234,201,42),1.3)
    img=Image.alpha_composite(img,art).convert("RGB")
    file=ASSETS/f"skin_{person}.jpg"
    img.save(file,quality=93,subsampling=0,optimize=True)
    print(file,file.stat().st_size)
    del img,art
# Packed normal/detail atlas: centered normal components + realistic gloss
# (blue channel): high gloss on bald dome, lower on eyes, cheeks and lips.
h=tiny*.18 + smooth*.10
gx=np.gradient(h,axis=1)
gy=np.gradient(h,axis=0)
normalR=np.clip(128-gx*110,0,255).astype(np.uint8)
normalG=np.clip(128-gy*110,0,255).astype(np.uint8)
crown=(1-np.clip((Y-.30)/.27,0,1))
mouth=gauss(.5,.76,.12,.055)
eyes=gauss(.41,.44,.08,.05)+gauss(.59,.44,.08,.05)
gloss=np.clip(110+115*crown-34*eyes-30*mouth+tiny*7,45,250).astype(np.uint8)
detail=np.stack([normalR,normalG,gloss],axis=2)
p=ASSETS/"skin_detail.jpg"
Image.fromarray(detail,'RGB').save(p,quality=90,subsampling=0,optimize=True)
print(p,p.stat().st_size)
print("Created: three seamless 2K portrait maps + normal/gloss atlas.")
