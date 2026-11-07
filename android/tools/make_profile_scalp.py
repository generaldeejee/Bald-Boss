#!/usr/bin/env python3
"""Generate profile-safe 360-degree scalp maps from Bald Elite's skin atlases.

Each 2k scalp map is generated from skin along the (feature-free) outer
margins of the same candidate's facial atlas. No eyes, lips, or eyebrows
are copied into the side / rear of the head; color stays matched to the face.
The shader blends face -> scalp at 45-85 degrees around the skull.
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter
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
