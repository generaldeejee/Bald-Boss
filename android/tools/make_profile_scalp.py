#!/usr/bin/env python3
"""Generate profile-safe 360-degree scalp maps from Bald Elite's skin atlases.

Each 2k scalp map is generated from skin along the (feature-free) outer
margins of the same candidate's facial atlas. No eyes, lips, or eyebrows
are copied into the side / rear of the head; color stays matched to the face.
The shader blends face -> scalp at 45-85 degrees around the skull.
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter, ImageDraw
A=Path(__file__).resolve().parents[1]/"app"/"src"/"main"/"assets"
for candidate in range(3):
    p=A/f"skin_{candidate}.jpg"
    image=Image.open(p).convert("RGB")
    width=min(image.width,2048)
    n=1024
    # The original photo atlas has free, unpainted skin at u<0.17 and u>0.83.
    # Take row-wise medians, preserving naturally varying forehead/jaw color.
    left=np.asarray(image.crop((int(.045*image.width),0,int(.13*image.width),image.height)).resize((32,n)),dtype=np.float32)
    right=np.asarray(image.crop((int(.87*image.width),0,int(.955*image.width),image.height)).resize((32,n)),dtype=np.float32)
    col=(np.median(left,axis=1)+np.median(right,axis=1))*.5
    # continuous wrap, smooth L/R neutral skin, gentle temple and occiput relief
    x=np.linspace(-.5,.5,n,endpoint=False)[None,:,None]
    y=np.linspace(0,1,n)[:,None,None]
    rng=np.random.default_rng(2300+candidate)
    perlin=np.asarray(Image.fromarray(rng.normal(127,24,(36,36)).astype('uint8')).resize((n,n),Image.Resampling.BICUBIC),dtype=np.float32)
    perlin=(perlin-127)[:,:,None]
    pores=rng.normal(0,2.0,(n,n,1))
    sides=np.cos(x*np.pi*2)
    warmth=2.4*sides+1.7*np.cos((y-.5)*np.pi)
    field=col[:,None,:]+warmth+perlin*.08+pores*.24
    # keep within photographic skin gamut, no visible repeating eyes
    field=np.clip(field,0,255).astype(np.uint8)
    result=Image.fromarray(field,"RGB")
    out=A/f"scalp_{candidate}.jpg"
    result.save(out,quality=91,optimize=True,subsampling=0)
    print("Profile scalp",candidate, result.size,out.stat().st_size)


# Independent, distortion-free 512x512 ear materials.
# On the ear's UV island, the middle is a softly shadowed concha, and the
# outer helix is a softly lit raised cartilage rim. No portrait pixels.
E=512
for candidate in range(3):
    scalp=Image.open(A/f"scalp_{candidate}.jpg").convert("RGB")
    center=np.asarray(scalp.getpixel((int(scalp.width*.67),int(scalp.height*.53))),dtype=np.float32)
    yy,xx=np.mgrid[:E,:E].astype(np.float32)
    nx=(xx+0.5-E/2)/(E*.43);ny=(yy+0.5-E/2)/(E*.43)
    r=np.sqrt(nx*nx+ny*ny)
    theta=np.arctan2(ny,nx)
    seed=np.random.default_rng(2415+candidate)
    grain=seed.normal(0,1.1,(E,E)).astype(np.float32)
    outer=np.exp(-.5*((r-.79)/.12)**2)
    antihelix=np.exp(-.5*((r-.47)/.12)**2)
    concha=np.exp(-.5*((r-.25)/.22)**2)
    tragus=np.exp(-.5*((nx-.39)/.15)**2-.5*((ny+.16)/.15)**2)
    warm=outer*15+antihelix*6-concha*26+tragus*10+grain
    fields=np.empty((E,E,3),dtype=np.uint8)
    for channel,mul in enumerate([1.0,.81,.72]):
        fields[:,:,channel]=np.clip(center[channel]+warm*mul,0,255)
    ear=Image.fromarray(fields,"RGB")
    # Feathered anatomically curved creases, separate from the full face.
    overlay=Image.new("RGBA",(E,E),(0,0,0,0))
    draw=ImageDraw.Draw(overlay,"RGBA")
    draw.arc((115,78,400,436),55,315,fill=(116,64,54,60),width=14)
    draw.arc((178,139,347,358),45,280,fill=(240,190,150,105),width=9)
    overlay=overlay.filter(ImageFilter.GaussianBlur(5))
    ear=Image.alpha_composite(ear.convert("RGBA"),overlay).convert("RGB")
    out=A/f"ear_{candidate}.jpg"
    ear.save(out,quality=90,optimize=True)
    print("Anatomical ear material",candidate,ear.size,out.stat().st_size)
