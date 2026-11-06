package com.baldelite.game;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/* Fully synthesised original chiptune + clipper buzz; no media files or
 * microphone/network permissions. The sound thread never blocks rendering. */
public final class SoundEngine {
    private final GameState game;
    private volatile boolean running=false;
    private Thread worker;
    private AudioTrack track;
    private static final int RATE=22050,CHUNK=1024;
    private static final float TWO_PI=(float)(2*Math.PI);
    private static final float[] NOTES={
        329.63f,392.00f,493.88f,392.00f,329.63f,440.00f,493.88f,587.33f,
        493.88f,392.00f,329.63f,392.00f,440.00f,493.88f,392.00f,329.63f
    };
    public SoundEngine(GameState state){game=state;}
    public synchronized void start(){
        if(running)return;
        running=true;
        worker=new Thread(this::loop,"BaldEliteAudio");
        worker.setDaemon(true);
        worker.start();
    }
    public synchronized void stop(){
        running=false;
        if(track!=null){
            try{track.pause();track.flush();}catch(Exception ignored){}
        }
    }
    private void loop(){
        AudioTrack audio=null;
        try{
            int min=AudioTrack.getMinBufferSize(RATE,AudioFormat.CHANNEL_OUT_MONO,AudioFormat.ENCODING_PCM_16BIT);
            if(min<0)min=4096;
            audio=new AudioTrack(AudioManager.STREAM_MUSIC,RATE,AudioFormat.CHANNEL_OUT_MONO,
                                 AudioFormat.ENCODING_PCM_16BIT,Math.max(8192,min*2),AudioTrack.MODE_STREAM);
            if(audio.getState()!=AudioTrack.STATE_INITIALIZED)return;
            track=audio;audio.play();
            short[] buffer=new short[CHUNK];long position=0;
            while(running){
                boolean enable,buzz;
                synchronized(game){
                    enable=game.sound;
                    buzz=game.shaving();
                }
                for(int i=0;i<CHUNK;i++){
                    float t=(position+i)/(float)RATE;
                    int step=((int)(t*4))&15;
                    float phase=(t*NOTES[step])%1.0f;
                    float tri=1f-4f*Math.abs(phase-.5f);
                    float beat=t%.5f;
                    float lead=.13f*tri*(1f-Math.min(beat*2f,.7f));
                    float bass=.09f*(float)Math.sin(TWO_PI*82.4f*t);
                    float kick=.08f*(float)Math.sin(TWO_PI*55f*beat)*(float)Math.exp(-beat*18);
                    float buzzSample=buzz?.11f*((float)Math.sin(TWO_PI*113*t)+.4f*(float)Math.sin(TWO_PI*233*t)):0;
                    float sample=enable?(lead+bass+kick+buzzSample):0;
                    sample=Math.max(-1,Math.min(1,sample));
                    buffer[i]=(short)(sample*14500f);
                }
                int written=audio.write(buffer,0,CHUNK);
                if(written<=0)break;
                position+=CHUNK;
            }
        }catch(Exception ignored){
            /* No audio hardware or temporary focus failure: gameplay survives. */
        }finally{
            if(audio!=null){
                try{audio.stop();}catch(Exception ignored){}
                audio.release();
            }
            track=null;
        }
    }
}
