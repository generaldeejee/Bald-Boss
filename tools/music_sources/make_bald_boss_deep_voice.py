#!/usr/bin/env python3
"""Bald Boss: No Hair, No Fear! — '80s Arena-Rock Mix.

Generates a new game-ready soundtrack with procedural rhythm-guitar,
analog-style synth layers, enhanced stadium percussion, a lead-hook,
and a deliberately synthetic male-voice chant made using eSpeak.

The source instrumental (also generated locally with no third-party samples)
is the Bald_Boss_No_Hair_No_Fear_Instrumental.wav produced by
make_bald_boss_rock.py. Both scripts are available with the game music.

This is an ORIGINAL composition and sound design; it is not a recording of a
human singer. No Suno samples, existing songs, or copyrighted recordings used.

Requires: python3, numpy, scipy, soundfile, eSpeak, ffmpeg.
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

# Single low male voice. No chipmunk layers or sped-up syllables.
# Speech is rendered at a natural pace rather than squeezed into a beat.
def speak_deep(words, speed=150, pitch=16):
    raw=subprocess.run(['espeak','-v','en-us+m3','-p',str(pitch),'-s',str(speed),
        '-a','190','--stdout',words],stdout=subprocess.PIPE,check=True).stdout
    with wave.open(io.BytesIO(raw),'rb') as f:
        x=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').astype(np.float32)/32768
        rate=f.getframerate()
    y=resample_poly(x,SR,rate).astype(np.float32)
    # Remove eSpeak's lengthy ending silence without deleting the last sound.
    active=np.flatnonzero(np.abs(y)>.009)
    if active.size:
        y=y[:min(len(y),active[-1]+int(.09*SR))]
    # Playback at ~94% speed lowers the voice naturally, with no fast-pitch artifact.
    y=resample_poly(y,100,94).astype(np.float32)
    y=filter_audio(y,low=84,high=3700)
    y=np.tanh(y*1.4)
    n=len(y)
    fade_in=min(n//3,int(.025*SR))
    fade_out=min(n//3,int(.060*SR))
    if fade_in: y[:fade_in]*=np.linspace(0,1,fade_in)
    if fade_out: y[-fade_out:]*=np.linspace(1,0,fade_out)
    return y

def vocal_event(t, words, strength=.32, speed=150):
    y=speak_deep(words,speed=speed)
    add(V,t,y,strength,pan=0)

# Deliberately paced spoken verse; entire lines play without being sped up.
verse_lines=[
 (4,'Mirror mirror clear the way'),
 (6,'Every hair has had its day'),
 (8,'Clippers buzzing feel the power'),
 (10,'Time to shine this is the hour'),
 (12,'Raise your razors make it clear'),
 (14,'The shiny legend now is here'),
]
for bar,words in verse_lines:
    vocal_event(pos(bar,.08),words,strength=.23,speed=148)

# Chorus chant: ONE deep lead singer only, no gang shouts/stacked voices.
chant_pattern=[
 (0,0,'NO HAIR'),(0,2,'NO FEAR'),
 (2,0,'BALD BOSS'),(2,2,'IS HERE'),
 (4,0,'SHAVE IT OFF'),(4,2.7,'MAKE IT CLEAR'),
 (6,0,'SHINY HEADS'),(6,2.5,'WE CHEER'),
]
for base in (16,28):
    for bo,bt,words in chant_pattern:
        vocal_event(pos(base+bo,bt+.05),words,strength=.49,speed=153)

vocal_event(pos(0,.15),'BALD BOSS',strength=.45,speed=148)
vocal_event(pos(36,.12),'BALD BOSS',strength=.57,speed=143)
vocal_event(pos(38,.12),'NO HAIR, NO FEAR',strength=.52,speed=148)

# Avoid high-pitched vowel pads as well: only guitar, bass, drums and one deep lead.
background=backing*.72+S*.95
full=background*.91+V*1.55
instrumental=background.copy()

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

(OUT/'Bald_Boss_80s_Deep_Lyrics.txt').write_text('''BALD BOSS — NO HAIR, NO FEAR!\nOriginal 80s arena rock mix\n\nVERSE\nMirror, mirror, clear the way\nEvery hair has had its day\nClippers buzzing, feel the power\nTime to shine, this is the hour\nRaise your razors, make it clear\nThe shiny legend now is here\n\nCHORUS\nNO HAIR! NO FEAR!\nBALD BOSS IS HERE!\nSHAVE IT OFF! MAKE IT CLEAR!\nSHINY HEADS, WE CHEER!\n\n[Guitar-style solo]\n\nCHORUS [repeat]\nNO HAIR! NO FEAR!\nBALD BOSS IS HERE!\nSHAVE IT OFF! MAKE IT CLEAR!\nSHINY HEADS, WE CHEER!\n\nOUTRO\nBALD BOSS!\nNO HAIR, NO FEAR!\n\nVoice: a single, low-pitched synthesized male voice, deliberately spoken at an intelligible pace.\nInstrumentation: synth-generated, no third-party music recordings.\n''',encoding='utf-8')
print('Completed ',round(len(full)/SR,2),'seconds')
for p in sorted(OUT.glob('Bald_Boss_80s_Deep_*')):
    print(p.name, round(p.stat().st_size/1024,1),'KiB')