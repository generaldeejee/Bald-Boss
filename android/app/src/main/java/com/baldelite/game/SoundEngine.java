package com.baldelite.game;

import android.content.Context;
import android.content.res.AssetManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;

/**
 * Original Bald Elite PSP sound playback, from the generated PSP audio arrays.
 * Title/results: 11.025 kHz sung track; game: 11.025 kHz instrumental;
 * victory: 22.05 kHz "Another bald boss!" Piper sample.
 *
 * Motor, cut SFX and volume ducking match the PSP's original audio_thread.
 * No online resources, no chiptune substitute and no runtime decoder.
 */
public final class SoundEngine {
    private static final String TAG="BaldEliteAudio";
    private static final int RATE=22050,CHUNK=1024;
    private final GameState game;
    private final AssetManager assets;
    private volatile boolean running;
    private Thread worker;
    private volatile AudioTrack current;
    private byte[] full;
    private byte[] instrumental;
    private byte[] victory;
    private static float triangle(float p){return 1.0f-4.0f*Math.abs(p-0.5f);}
    public SoundEngine(GameState state,Context context){
        game=state;assets=context.getAssets();
    }
    private byte[] read(String file) throws Exception {
        try(InputStream stream=assets.open(file)){
            ByteArrayOutputStream out=new ByteArrayOutputStream();
            byte[] buf=new byte[16384];
            int count;
            while((count=stream.read(buf))!=-1)out.write(buf,0,count);
            return out.toByteArray();
        }
    }
    private synchronized void loadAudio() throws Exception {
        if(full!=null&&instrumental!=null&&victory!=null)return;
        byte[] f=read("psp_title_vocals.pcm");
        byte[] i=read("psp_game_instrumental.pcm");
        byte[] v=read("psp_victory_voice.pcm");
        if(f.length<200000||f.length!=i.length||v.length<10000)
            throw new IllegalStateException("Invalid original PSP sound resources.");
        full=f;instrumental=i;victory=v;
    }
    public synchronized void start(){
        if(running)return;
        running=true;
        worker=new Thread(this::mixLoop,"BaldElite-PSP-Audio");
        worker.setDaemon(true);
        worker.start();
    }
    public synchronized void stop(){
        running=false;
        if(worker!=null)worker.interrupt();
        AudioTrack audio=current;
        if(audio!=null){
            try{audio.pause();audio.flush();}catch(Exception ignored){}
        }
    }
    private void mixLoop(){
        AudioTrack audio=null;
        try {
            loadAudio();
            if(!running)return;
            int required=AudioTrack.getMinBufferSize(RATE,AudioFormat.CHANNEL_OUT_MONO,
                                                    AudioFormat.ENCODING_PCM_16BIT);
            if(required<0)required=8192;
            audio=new AudioTrack(AudioManager.STREAM_MUSIC,RATE,
                                 AudioFormat.CHANNEL_OUT_MONO,AudioFormat.ENCODING_PCM_16BIT,
                                 Math.max(required*2,8192),AudioTrack.MODE_STREAM);
            if(audio.getState()!=AudioTrack.STATE_INITIALIZED)return;
            current=audio;
            audio.play();
            short[] output=new short[CHUNK];
            int songPos=0,voicePos=-1,lastTrack=-1,lastCuts=-1,lastWin=-1,noise=0x159e1;
            float motorPhase=0,motor2Phase=0,motorEnvelope=0;
            int cutPulse=0;
            while(running){
                int phase,cuts,wins;
                boolean musicEnabled,clipperActive;
                synchronized(game){
                    phase=game.phase;
                    cuts=game.cutSerial;
                    wins=game.winSerial;
                    musicEnabled=game.sound;
                    clipperActive=game.shaving();
                }
                int trackType=phase==GameState.PLAY?1:0;
                if(trackType!=lastTrack){
                    songPos=0;
                    lastTrack=trackType;
                }
                byte[] song=trackType==0?full:instrumental;
                if(lastCuts!=-1&&cuts!=lastCuts)cutPulse=1700;
                if(lastWin!=-1&&wins!=lastWin)voicePos=0;
                if(phase==GameState.WIN&&lastWin==-1)voicePos=0;
                lastCuts=cuts;lastWin=wins;
                for(int n=0;n<CHUNK;n++){
                    int k=songPos>>1;
                    int next=(k+1<song.length)?k+1:0;
                    float a=((song[k]&255)-128)/128.0f;
                    float b=((song[next]&255)-128)/128.0f;
                    float signal=a+(b-a)*((songPos&1)*0.5f);
                    float fade=Math.min(1f,songPos/2048f);
                    songPos++;
                    if(songPos>=song.length*2)songPos=0;
                    float target=clipperActive?1f:0f;
                    motorEnvelope+=(target-motorEnvelope)*.0068f;
                    noise=noise*1664525+1013904223;
                    float white=((noise>>>24)/127.5f-1f);
                    float motor=(triangle(motorPhase)*.17f
                                +triangle(motor2Phase)*.060f
                                +white*.060f)*motorEnvelope;
                    if(cutPulse>0){
                        motor+=(triangle(motor2Phase)*.075f+white*.032f)
                              *(cutPulse/1700.0f);
                        cutPulse--;
                    }
                    motorPhase+=107.0f/RATE;
                    motor2Phase+=221.5f/RATE;
                    if(motorPhase>=1)motorPhase-=1;
                    if(motor2Phase>=1)motor2Phase-=1;
                    float voice=0f;
                    if(voicePos>=0&&voicePos<victory.length){
                        voice=(((victory[voicePos]&255)-128)/128f)*.72f;
                        voicePos++;
                    }else if(voicePos>=victory.length)voicePos=-1;
                    float musicGain=voicePos>=0?.20f:(motorEnvelope>.02f?.34f:.62f);
                    float sample=(musicEnabled?signal*fade*musicGain:0f)+motor+voice;
                    sample=Math.max(-.96f,Math.min(.96f,sample));
                    output[n]=(short)(sample*22000.0f);
                }
                int written=audio.write(output,0,CHUNK);
                if(written<0)break;
            }
        }catch(Exception ex){
            Log.e(TAG,"PSP audio failed; video and gameplay remain available",ex);
        }finally{
            if(audio!=null){
                try{audio.stop();}catch(Exception ignored){}
                try{audio.release();}catch(Exception ignored){}
            }
            if(current==audio)current=null;
        }
    }
}
