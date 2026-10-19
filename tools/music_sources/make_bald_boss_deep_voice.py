#!/usr/bin/env python3
"""Bald Boss: No Hair, No Fear! — '80s Arena-Rock Mix.

Generates a new game-ready soundtrack with procedural rhythm-guitar,
analog-style synth layers, enhanced stadium percussion, a lead-hook,
and a sung E-minor melody from a Piper neural male voice using TD-PSOLA.

The source instrumental (also generated locally with no third-party samples)
is the Bald_Boss_No_Hair_No_Fear_Instrumental.wav produced by
make_bald_boss_rock.py. Both scripts are available with the game music.

This is an ORIGINAL composition with melodic pitch-controlled synthesized
singing, not recorded human singing. No Suno assets, existing songs or third-party recordings.
The neural voice is trained using public-domain LibriVox material; build only.

Requires: python3, numpy, scipy, soundfile, piper-tts==1.3.0,
praat-parselmouth==0.4.7, ffmpeg.
Model downloaded/verified by tools/generate_voice.py; never shipped to PSP.
"""
from pathlib import Path
import subprocess, io, wave
import numpy as np
import soundfile as sf
from scipy.signal import butter, sosfilt, resample_poly

OUT=Path(__file__).resolve().parent
SR=44100
BPM=144
BEAT=60.0/BPM
SOURCE=OUT/'Bald_Boss_No_Hair_No_Fear_Instrumental.wav'
if not SOURCE.exists():
    subprocess.run(['python3',str(OUT/'make_bald_boss_rock.py')],check=True)
backing, rate=sf.read(SOURCE, dtype='float32', always_2d=True)
assert rate==SR
N=len(backing)
S=np.zeros_like(backing)
V=np.zeros_like(backing)
rng=np.random.default_rng(19841992)
NOTES={'C':0,'C#':1,'D':2,'D#':3,'E':4,'F':5,'F#':6,'G':7,'G#':8,'A':9,'A#':10,'B':11}

def fq(n):
    return 440*2**(((int(n[-1])+1)*12+NOTES[n[:-1]]-69)/12)

def pos(b,beat=0):
    return (b*4+beat)*BEAT

def add(dst, at, signal, amp=1, pan=0):
    ix=int(round(at*SR)); y=np.asarray(signal,dtype=np.float32)
    if ix<0 or ix>=N or not y.size: return
    y=y[:N-ix]
    l=np.sqrt((1-pan)/2); r=np.sqrt((1+pan)/2)
    dst[ix:ix+len(y),0]+=amp*l*y
    dst[ix:ix+len(y),1]+=amp*r*y

def filter_audio(sig, low=None, high=None):
    if low is not None and high is not None:
        sos=butter(2,[low,high],btype='bandpass',fs=SR,output='sos')
    elif low is not None:
        sos=butter(2,low,btype='highpass',fs=SR,output='sos')
    else:
        sos=butter(2,high,btype='lowpass',fs=SR,output='sos')
    return sosfilt(sos,sig).astype(np.float32)

# Extra double-tracked crunchy palm-muted guitars. Harmonics and overdrive are
# produced by oscillators and nonlinear waveshaping rather than borrowed sounds.
def guitar(note, length=.68, palm=False, seed=0):
    length=length*BEAT
    t=np.arange(int((length+.13)*SR),dtype=np.float32)/SR
    fundamental=fq(note); out=np.zeros(len(t),np.float32)
    # small frequency differences between strings emulate power-chord width
    for shift,phase in [(1.0,.1),(1.003,.9),(.997,2.3)]:
        ph=2*np.pi*fundamental*shift*t+phase
        wave=(.52*np.sin(ph)+.36*np.sin(2*ph)+.20*np.sin(3*ph)
              +.125*np.sin(4*ph)+.075*np.sin(5*ph)+.048*np.sin(7*ph))
        out+=wave/3.0
    # rasp from string pick / fret attack
    noise=rng.standard_normal(len(t)).astype(np.float32)
    out += noise*np.exp(-t*55)*.06
    out=np.tanh(10.0*out)
    out=filter_audio(out,low=145,high=4900)
    att=np.minimum(1,t/.004)
    rel=np.clip((length+.1-t)/.11,0,1)
    out*=att*rel*np.exp(-t*(3.2 if palm else .7))
    return out

chords=[('E2',['E3','B3']),('C2',['C3','G3']),('G2',['G3','D4']),('D2',['D3','A3'])]
for bar in range(40):
    verse = 4<=bar<16
    chorus=16<=bar<24 or 28<=bar<36
    solo=24<=bar<28
    root, notes=chords[bar%4]
    # chorus wall of guitars and chugging eighth-notes across the stereo field
    accents=[(0,1.1),(1.5,.33),(2,.73),(3.5,.36)]
    if chorus: accents +=[(.5,.28),(2.5,.31)]
    for beat,length in accents:
        vol=.070 if verse else (.105 if chorus else .083)
        if bar>=36: vol=.12
        for j,note in enumerate(notes):
            y=guitar(note,length,palm=(length<.5))
            add(S,pos(bar,beat),y,vol,pan=-.72)
            add(S,pos(bar,beat)+.019,y*.94,vol*.90,pan=.72)

# Big, delayed electric-guitar-like solos and melodic hooks with vibrato.
def solo_note(note, beats=1, brightness=1):
    length=beats*BEAT
    t=np.arange(int((length+.26)*SR),dtype=np.float32)/SR
    f=fq(note)
    wobble=.006*np.sin(2*np.pi*5.8*t)
    phase=2*np.pi*f*t+(f*.25)*wobble
    y=(.46*np.sin(phase)+.32*np.sin(2*phase)+.20*np.sin(3*phase)
       +.09*np.sin(5*phase)+.03*np.sin(7*phase))
    y=np.tanh(y*brightness*3)
    y=filter_audio(y,low=190,high=6300)
    y*=np.clip(t/.008,0,1)*np.clip((length+.22-t)/.23,0,1)*np.exp(-t*.38)
    return y

# Earworm lead figure over each chorus transition.
melody=[('E5',0,.65),('G5',.9,.65),('B5',1.75,.8),('A5',2.7,.5),('G5',3.15,.65)]
for bar in [16,20,28,32]:
    for note, bt, dur in melody:
        sig=solo_note(note,dur)
        add(S,pos(bar,bt),sig,.11,-.27)
        add(S,pos(bar,bt)+.12,sig,.062,.4)

# Big synth-brass 'stadium' stabs
for bar in (16,18,20,22,28,30,32,34,36):
    for note in ('E4','G4','B4'):
        y=solo_note(note,2.3,brightness=.65)
        add(S,pos(bar),y,.038,pan=-.13)

# More triumphant drums with reverby snare on choruses.
def snare_flam(t0, level=.17):
    t=np.arange(int(.37*SR),dtype=np.float32)/SR
    n=rng.standard_normal(len(t)).astype(np.float32)
    n=filter_audio(n,low=470,high=9100)
    y=(.74*n+.19*np.sin(2*np.pi*175*t))*np.exp(-t*17)
    add(S,t0-.018,y,level*.35,pan=-.25)
    add(S,t0,y,level,pan=.2)
    add(S,t0+.11,y,level*.22,pan=.45)
    add(S,t0+.23,y,level*.10,pan=-.43)

def tom(t0,base,level=.13):
    t=np.arange(int(.33*SR),dtype=np.float32)/SR
    ph=2*np.pi*(base*t+24*(1-np.exp(-t*16))/16)
    y=np.sin(ph)*np.exp(-t*12)
    add(S,t0,y,level)

for bar in list(range(16,24))+list(range(28,36)):
    for beat in (1,3): snare_flam(pos(bar,beat),.10)
for bar in (15,23,27,35):
    for j,base in enumerate((215,175,135,110)):
        tom(pos(bar,2+.5*j),base,.19)

# Sung melodic rock lead: source voice is neural Piper speech, transformed
# offline into an E-minor melody using formant-preserving TD-PSOLA.
# Consonants remain intelligible and vowels acquire real notes and vibrato.
import os
import sys
from math import gcd
from piper import PiperVoice
from piper.config import SynthesisConfig
from melodic_voice import sing_melody, foreground_vocal_mix

sys.path.insert(0, str(OUT.parent))
from generate_voice import get_voice

voice_cache = Path(os.environ.get(
    "BALD_BOSS_PIPER_CACHE", str(OUT.parent.parent / ".cache" / "piper")
))
model, voice_cfg = get_voice(voice_cache)
singer = PiperVoice.load(str(model), config_path=str(voice_cfg))
if singer.config.sample_rate != 22050:
    raise ValueError("The neural voice must be 22,050 Hz")
print("Singing a composed E-minor melody with Piper + Praat TD-PSOLA")

def speak_natural(words, style="verse", take=0):
    chorus = style != "verse"
    syn = SynthesisConfig(
        length_scale=(.87 if chorus else .99) + (take % 3)*.020,
        noise_scale=(.77 if chorus else .72) + (take % 3)*.03,
        noise_w_scale=(.94 if chorus else .83) + (take % 4)*.02,
        normalize_audio=False,
    )
    parts = [c.audio_float_array.astype(np.float32)
             for c in singer.synthesize(words, syn_config=syn)]
    if not parts:
        raise ValueError("Empty voice line: " + words)
    y = np.concatenate(parts)
    factor = gcd(int(singer.config.sample_rate), SR)
    y = resample_poly(y, SR//factor,
                      int(singer.config.sample_rate)//factor).astype(np.float32)
    active = np.flatnonzero(np.abs(y) > .010)
    if active.size:
        a = max(0, int(active[0]) - int(.045*SR))
        b = min(len(y), int(active[-1]) + int(.11*SR))
        y = y[a:b]
    if len(y) < int(.12*SR):
        raise ValueError("Unexpected short voice phrase: " + words)
    y = filter_audio(y, low=82, high=6500)
    rms = float(np.sqrt(np.mean(y*y) + 1e-10))
    gain = min(2.3, .18/max(rms, .035))
    y = np.tanh(y*gain*1.2)/np.tanh(1.2)
    n = len(y)
    attack = min(n//3,int(.02*SR))
    release = min(n//3,int(.09*SR))
    y[:attack] *= np.linspace(0,1,attack,dtype=np.float32)
    y[-release:] *= np.linspace(1,0,release,dtype=np.float32)
    return y

def vocal_event(at, words, notes, strength=.36, style="verse", take=0):
    y = speak_natural(words, style=style, take=take)
    y = sing_melody(y, SR, notes)
    add(V,at,y,strength,pan=0)
    # Very quiet studio reflections, not a robot choir.
    if style=="chorus":
        add(V,at+.16,y,strength*.095,pan=-.30)
    elif style=="outro":
        add(V,at+.20,y,strength*.075,pan=.30)

# Each phrase carries actual pitches, replacing the former spoken chant.
verse_lines=[
    (4,"Mirror, mirror, clear the way.", ("E3","G3","A3","G3","E3","D3")),
    (6,"Every hair has had its day.", ("G3","A3","B3","A3","G3","E3")),
    (8,"Clippers buzzing, feel the power!", ("E3","G3","A3","B3","A3","G3")),
    (10,"Time to shine, this is the hour.", ("G3","B3","A3","G3","E3","G3")),
    (12,"Raise your razors, make it clear!", ("E3","G3","B3","A3","G3","E3")),
    (14,"The shiny legend now is here.", ("G3","A3","B3","A3","G3","E3")),
]
for idx,(bar,words,notes) in enumerate(verse_lines):
    vocal_event(pos(bar,.08),words,notes,
                strength=.34+(.017 if idx%2 else 0),style="verse",take=idx)

# Repeatable arena-rock hook with clearly changing notes.
chant_pattern=[
    (0,0,"No hair!", ("E3","G3")),
    (0,2,"No fear!", ("G3","B3")),
    (2,0,"Bald boss!", ("B3","A3")),
    (2,2,"Is here!", ("G3","E3")),
    (4,0,"Shave it off!", ("E3","G3","B3")),
    (4,2.7,"Make it clear!", ("A3","G3","E3")),
    (6,0,"Shiny heads!", ("G3","A3","B3")),
    (6,2.5,"We cheer!", ("B3","E4")),
]
for repeat,base in enumerate((16,28)):
    for idx,(offset,beat,words,notes) in enumerate(chant_pattern):
        vocal_event(pos(base+offset,beat+.05),words,notes,
                    strength=.43+(.025 if idx%2 else 0),
                    style="chorus",take=idx+repeat*3)

vocal_event(pos(0,.15),"Bald boss!",("E3","G3"),
            strength=.44,style="chorus",take=3)
vocal_event(pos(36,.12),"Bald boss!",("B3","G3"),
            strength=.49,style="outro",take=7)
vocal_event(pos(38,.12),"No hair, no fear!",("E3","G3","B3","E4"),
            strength=.48,style="outro",take=8)

# Avoid high-pitched vowel pads as well: only guitar, bass, drums and one deep lead.
background=backing*.72+S*.95
instrumental=background.copy()
full, mix_stats = foreground_vocal_mix(background,V,SR)
print("Vocal-first mixing stats:",mix_stats)

def master(y):
    y=np.tanh(y*1.09)
    y-=y.mean(axis=0,keepdims=True)
    # Intentional slight fade-in/out prevents transient clicks at start/end.
    fade=int(.35*SR)
    y[:fade]*=np.linspace(0,1,fade,dtype=np.float32)[:,None]
    y[-fade:]*=np.linspace(1,0,fade,dtype=np.float32)[:,None]
    maxabs=float(np.max(np.abs(y)))
    if maxabs>0: y*=.94/maxabs
    return y

full=master(full)
instrumental=master(instrumental)
# compact audio exports ideal for Linux, Windows, macOS and Android game builds.
for stem,data in [('Bald_Boss_80s_Deep_Voice',full),('Bald_Boss_80s_Deep_Instrumental',instrumental)]:
    sf.write(OUT/f'{stem}.wav',data,SR,subtype='PCM_16')
    subprocess.run(['ffmpeg','-y','-loglevel','error','-i',str(OUT/f'{stem}.wav'),'-c:a','libvorbis','-q:a','5',str(OUT/f'{stem}.ogg')],check=True)

# Intro-into-chorus can be used as a trailer. Ends before the outro.
clip=full[int(pos(16)*SR):int(pos(24)*SR)].copy()
clip[:int(.09*SR)]*=np.linspace(0,1,int(.09*SR),dtype=np.float32)[:,None]
clip[-int(.3*SR):]*=np.linspace(1,0,int(.3*SR),dtype=np.float32)[:,None]
sf.write(OUT/'Bald_Boss_80s_Deep_Chorus.wav',clip,SR,subtype='PCM_16')
subprocess.run(['ffmpeg','-y','-loglevel','error','-i',str(OUT/'Bald_Boss_80s_Deep_Chorus.wav'),'-c:a','libvorbis','-q:a','5',str(OUT/'Bald_Boss_80s_Deep_Chorus.ogg')],check=True)

(OUT/'Bald_Boss_80s_Deep_Lyrics.txt').write_text('''BALD BOSS — NO HAIR, NO FEAR!\nOriginal 80s arena rock mix\n\nVERSE\nMirror, mirror, clear the way\nEvery hair has had its day\nClippers buzzing, feel the power\nTime to shine, this is the hour\nRaise your razors, make it clear\nThe shiny legend now is here\n\nCHORUS\nNO HAIR! NO FEAR!\nBALD BOSS IS HERE!\nSHAVE IT OFF! MAKE IT CLEAR!\nSHINY HEADS, WE CHEER!\n\n[Guitar-style solo]\n\nCHORUS [repeat]\nNO HAIR! NO FEAR!\nBALD BOSS IS HERE!\nSHAVE IT OFF! MAKE IT CLEAR!\nSHINY HEADS, WE CHEER!\n\nOUTRO\nBALD BOSS!\nNO HAIR, NO FEAR!\n\nVoice: synthetic melodic male rock singing using Piper and formant-preserving Praat TD-PSOLA; sung notes/vibrato and dynamic vocal-priority mixing. Not a recording of a human singer.\nInstrumentation: synth-generated, no third-party music recordings.\n''',encoding='utf-8')
print('Completed ',round(len(full)/SR,2),'seconds')
for p in sorted(OUT.glob('Bald_Boss_80s_Deep_*')):
    print(p.name, round(p.stat().st_size/1024,1),'KiB')