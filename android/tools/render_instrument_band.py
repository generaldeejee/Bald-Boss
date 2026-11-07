#!/usr/bin/env python3
"""Bald Elite: sample-instrument band soundtrack, rendered offline in CI.

Original music, arranged with recorded instrument sounds from the TimGM6mb
SoundFont (GPL-2; soundfont package copyright details under
/usr/share/doc/timgm6mb-soundfont/copyright). No live musician is claimed.
Android receives pre-rendered Ogg Vorbis, not MIDI files or a MIDI synthesizer.
"""
import ctypes as C
from ctypes.util import find_library
import math
import os
import subprocess
from pathlib import Path
import numpy as np
import soundfile as sf
from scipy.signal import butter, sosfilt

SR=44100
SF2=Path(os.environ.get("BALD_ELITE_SF2","/usr/share/sounds/sf2/TimGM6mb.sf2"))
DEST=Path(__file__).resolve().parents[1]/"app"/"src"/"main"/"assets"
DEST.mkdir(parents=True,exist_ok=True)
RNG=np.random.default_rng(20261107)
CHORDS=[(40,[52,55,59],[64,67,71]),(36,[48,52,55],[60,64,67]),
        (43,[50,55,59],[62,67,71]),(38,[50,54,57],[62,66,69])]
PROGRAM={0:33,1:27,2:29,3:4,4:65,6:18,7:25,8:56}
PAN={0:64,1:24,2:102,3:84,4:72,5:64,6:48,7:103,8:78}
VOLUME={0:104,1:101,2:88,3:83,4:105,5:113,6:70,7:91,8:88}
HOOK=[(0,71,.75),(1,74,.44),(1.5,76,.54),(2.25,74,.46),(3,71,.65),
      (4,67,.65),(5,69,.5),(5.5,71,.44),(6.25,69,.46),(7,67,.70),
      (8,74,.70),(9,76,.46),(9.5,79,.48),(10.25,76,.46),(11,74,.7),
      (12,69,.65),(13,71,.48),(13.5,74,.50),(14.25,71,.5),(15,69,.77)]

def compose(menu):
    bpm=113 if menu else 140
    bars=24 if menu else 40
    beat=60.0/bpm
    events=[]
    def add(ch,pitch,bar,bt,length,velocity=88,jitter=0):
        t=max(0.0,(bar*4+bt)*beat+float(RNG.uniform(-jitter,jitter)))
        dur=length*beat*float(RNG.uniform(.94,1.06))
        v=int(np.clip(velocity+RNG.normal(0,2),1,127))
        events.append((t,1,ch,int(pitch),v))
        events.append((t+dur,0,ch,int(pitch),0))
    for bar in range(bars):
        root,chord,top=CHORDS[bar%4]
        energy=int(bar>=8)+int(bar>=16 and not menu)
        for j in range(8):
            add(9,42,bar,j*.5,.10,55+int(j%2==0)*12+int(RNG.uniform(-8,8)),.007)
        for bt in (0,2):add(9,36,bar,bt,.18,105+energy*4,.008)
        for bt in (1,3):
            add(9,38,bar,bt,.18,99+energy*5,.006)
            if bar%2:add(9,37,bar,bt-.25,.07,44,.009)
        for bt in ((1.5,3.5) if bar%2==0 else (2.5,)):
            add(9,36,bar,bt,.15,71+energy*4,.008)
        if bar%4==0:add(9,49,bar,0,.5,90)
        if bar%4==3:
            for j,p in enumerate((45,47,50)):add(9,p,bar,3+j/3,.1,69+j*8,.004)
        if energy and bar%2:add(9,46,bar,3.5,.14,71,.012)
        # Humanized walking/syncopated bass guitar.
        for bt,p,v in [(0,root,94),(.75,root+7,75),(1.5,root+12,84),
                       (2,root,102),(2.75,root+7,77),
                       (3.25,root+12,81),(3.5,root+7,72)]:
            if bt==.75 and bar%3==0:continue
            add(0,p,bar,bt,.40 if bt%1 else .53,v+energy*4,.012)
        # Offbeat clean guitar, individually timed strings.
        for bt in (.5,1.5,2.5,3.5):
            if bar%4==3 and bt==3.5:continue
            for j,n in enumerate(chord):add(1,n,bar,bt+j*.018,.21,66+energy*5+j*4,.013)
        if menu or bar%2==0:
            for j,bt in enumerate((0,.75,1.5,2.25,3)):
                add(7,top[j%3]+(12 if j==4 else 0),bar,bt,.5,64+energy*7,.021)
        if bar%2==0 or menu:
            for j,n in enumerate(top):add(3,n,bar,j*.012,3.35,51+energy*6,.006)
        if bar>=8:
            for j,n in enumerate(chord):add(6,n,bar,j*.015,3.55,41+energy*7,.005)
        if not menu and bar%8>=4:
            for bt,n in ((0,root+12),(1.5,root+19),(2.5,root+24)):
                add(2,n,bar,bt,.65,70+energy*7,.013)
        elif menu and bar%8>=4:
            for j,n in enumerate(chord):add(2,n,bar,2+j*.02,.78,53,.015)
        if bar%8>=4:
            for bt in (0,2.5):add(8,top[0]+12,bar,bt,.32,67+energy*6,.014)
    # Sax melody provides a true acoustic-instrument sample-based lead.
    for block in range(0,bars,4):
        section=block//4
        for bt,pitch,length in HOOK:
            bar=block+int(bt//4)
            if bar>=bars or (menu and section==0 and bt>8):continue
            add(4,pitch if section%2==0 else pitch-12,
                bar,bt%4,length*.90,78+(8 if block>=8 else 0),.019)
    events.sort(key=lambda event:(event[0],event[1]))
    return events,bars*4*beat,bpm

class Fluid:
    def __init__(self):
        dll=C.CDLL(find_library("fluidsynth") or "libfluidsynth.so.3")
        ptr=C.c_void_p
        def api(name,args,ret):
            obj=getattr(dll,name);obj.argtypes=args;obj.restype=ret
            return obj
        self.settings=api("new_fluid_settings",[],ptr)()
        settingnum=api("fluid_settings_setnum",[ptr,C.c_char_p,C.c_double],C.c_int)
        settingint=api("fluid_settings_setint",[ptr,C.c_char_p,C.c_int],C.c_int)
        settingnum(self.settings,b"synth.sample-rate",float(SR))
        settingnum(self.settings,b"synth.gain",.30)
        settingnum(self.settings,b"synth.reverb.room-size",.37)
        settingnum(self.settings,b"synth.reverb.level",.18)
        settingint(self.settings,b"synth.polyphony",192)
        self.synth=api("new_fluid_synth",[ptr],ptr)(self.settings)
        sfload=api("fluid_synth_sfload",[ptr,C.c_char_p,C.c_int],C.c_int)
        if sfload(self.synth,str(SF2).encode(),1)<0:
            raise RuntimeError("Unable to load instrument samples from "+str(SF2))
        prog=api("fluid_synth_program_change",[ptr,C.c_int,C.c_int],C.c_int)
        cc=api("fluid_synth_cc",[ptr,C.c_int,C.c_int,C.c_int],C.c_int)
        self.on=api("fluid_synth_noteon",[ptr,C.c_int,C.c_int,C.c_int],C.c_int)
        self.off=api("fluid_synth_noteoff",[ptr,C.c_int,C.c_int],C.c_int)
        self.write=api("fluid_synth_write_float",[ptr,C.c_int,ptr,C.c_int,C.c_int,ptr,C.c_int,C.c_int],C.c_int)
        self.delete=api("delete_fluid_synth",[ptr],None)
        self.delete_settings=api("delete_fluid_settings",[ptr],None)
        for ch,pr in PROGRAM.items():
            prog(self.synth,ch,pr)
            cc(self.synth,ch,10,PAN[ch])
            cc(self.synth,ch,7,VOLUME[ch])
            cc(self.synth,ch,91,20)
        cc(self.synth,9,10,64);cc(self.synth,9,7,113)
    def render(self,events,seconds):
        count=int((seconds+.14)*SR)
        stereo=np.zeros((count,2),np.float32)
        frame=0;eventIndex=0
        while frame<count:
            nextEvent=int(events[eventIndex][0]*SR) if eventIndex<len(events) else count
            if frame<nextEvent:
                n=min(4096,nextEvent-frame,count-frame)
                left=np.zeros(n,np.float32);right=np.zeros(n,np.float32)
                self.write(self.synth,n,C.c_void_p(left.ctypes.data),0,1,
                           C.c_void_p(right.ctypes.data),0,1)
                stereo[frame:frame+n,0]=left
                stereo[frame:frame+n,1]=right
                frame+=n
            elif eventIndex<len(events):
                t,on,ch,pitch,vel=events[eventIndex]
                if on:self.on(self.synth,ch,pitch,vel)
                else:self.off(self.synth,ch,pitch)
                eventIndex+=1
            else:break
        return stereo
    def close(self):
        self.delete(self.synth);self.delete_settings(self.settings)

def master(samples):
    samples=sosfilt(butter(2,34,btype="highpass",fs=SR,output="sos"),
                    samples,axis=0).astype(np.float32)
    samples=np.tanh(samples*1.08)
    delay=int(.034*SR)
    delayed=samples.copy()
    delayed[delay:,0]+=.11*samples[:-delay,1]
    delayed[delay:,1]+=.11*samples[:-delay,0]
    peak=float(np.max(np.abs(delayed)))
    if peak>0:delayed*=.90/peak
    fade=int(.10*SR)
    ramp=np.sin(np.linspace(0,np.pi/2,fade))**2
    delayed[:fade]*=ramp[:,None];delayed[-fade:]*=ramp[::-1,None]
    return delayed

if __name__=="__main__":
    if not SF2.is_file():raise SystemExit("Instrument sample pack missing: "+str(SF2))
    for menu,filename in [(True,"psp_title_hq.ogg"),(False,"psp_game_hq.ogg")]:
        events,duration,bpm=compose(menu)
        print("Studio band",filename,"BPM",bpm,"duration",round(duration,1),flush=True)
        synth=Fluid()
        try:pcm=synth.render(events,duration)
        finally:synth.close()
        wav=DEST/filename.replace(".ogg",".wav")
        sf.write(wav,master(pcm),SR,subtype="PCM_16")
        subprocess.run(["ffmpeg","-y","-loglevel","error","-i",str(wav),
                        "-c:a","libvorbis","-q:a","7",str(DEST/filename)],check=True)
        wav.unlink()
        print("Rendered",filename,(DEST/filename).stat().st_size,flush=True)
