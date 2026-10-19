#!/usr/bin/env python3
"""Short, network-free checks for the sung-vocal melody and vocal-first mix."""
from pathlib import Path
import sys
import numpy as np
import parselmouth

sys.path.insert(0,str(Path(__file__).resolve().parent/"music_sources"))
from melodic_voice import sing_melody, foreground_vocal_mix, note_frequency

sr=22050
t=np.arange(int(sr*1.5),dtype=np.float32)/sr
source=(.20*np.sin(2*np.pi*140*t)).astype(np.float32)
sung=sing_melody(source,sr,("E3","G3","B3"))
assert sung.shape == source.shape
assert np.isfinite(sung).all()
pitch=parselmouth.Sound(sung,sr).to_pitch(
    time_step=.02,pitch_floor=75,pitch_ceiling=380
).selected_array["frequency"]
voiced=pitch[pitch>0]
assert len(voiced)>8 and np.min(voiced)<185 and np.max(voiced)>225, (
    "Melody no longer reaches its low and high notes"
)
assert note_frequency("B3") > note_frequency("E3")

backing=np.full((len(t),2),.20,dtype=np.float32)
vocals=np.zeros_like(backing)
vocals[int(sr*.25):int(sr*1.2)]=.06
mixed,stats=foreground_vocal_mix(backing,vocals,sr)
assert mixed.shape==backing.shape and np.isfinite(mixed).all()
assert stats["min_instrument_gain"] < .45, (
    "Sidechain is not making the music quieter under vocals"
)
assert stats["voice_active_fraction"] > 0.2
print("Melodic-singing and vocal-sidechain smoke tests passed:",stats)
