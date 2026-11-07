package com.baldelite.game;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaPlayer;
import android.util.Log;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;

/** Bald Elite Deluxe's original high-definition PSP soundtrack.
 * SOURCE: the same 44.1kHz stereo wav generators used by the PSP homebrew,
 * now delivered as high-quality Vorbis instead of 11.025kHz 8-bit PCM.
 * Game music (instrumental) and club/menu music (sung) loop independently.
 *
 * The PSP victory voice, clipper motor, shaving swishes, UI chimes and crowd
 * cheers are mixed in a separate 44.1-kHz AudioTrack. Volume ducking lets
 * the 3D barber ambience breathe while sound stays crisp.
 *
 * Missing HQ .ogg -> safe fallback to the tested v1.2 PCM tracks.
 */
public final class SoundEngine {
    private static final String TAG="BaldEliteDeluxeAudio";
    private static final int RATE=44100,CHUNK=1024;
    private final GameState game;
    private final AssetManager assets;
    private volatile boolean running;
    private Thread worker;
    private volatile AudioTrack current;
    private MediaPlayer music;
    private boolean hqAvailable=true;
    private byte[] full,instrumental,victory;
    private static float triangle(float p){return 1f-4f*Math.abs(p-.5f);}
    public SoundEngine(GameState state,Context context){
        game=state;assets=context.getAssets();
    }
    private byte[] read(String file)throws Exception{
        try(InputStream input=assets.open(file)){
            ByteArrayOutputStream out=new ByteArrayOutputStream();
            byte[] b=new byte[16384];int n;
            while((n=input.read(b))!=-1)out.write(b,0,n);
            return out.toByteArray();
        }
    }
    private void loadPCM()throws Exception{
        if(full!=null&&instrumental!=null&&victory!=null)return;
        full=read("psp_title_vocals.pcm");
        instrumental=read("psp_game_instrumental.pcm");
        victory=read("psp_victory_voice.pcm");
        if(full.length<100000||instrumental.length!=full.length||victory.length<10000)
            throw new IllegalStateException("Original PSP sound resources not found");
    }
    public synchronized void start(){
        if(running)return;
        running=true;
        worker=new Thread(this::loop,"BaldEliteDeluxe-Audio");
        worker.setDaemon(true);
        worker.start();
    }
    public synchronized void stop(){
        running=false;
        if(worker!=null)worker.interrupt();
        AudioTrack a=current;
        if(a!=null){try{a.pause();a.flush();}catch(Exception ignored){}}
        // The worker closes the MediaPlayer after leaving its audio loop.
    }
    private MediaPlayer newTrack(int kind)throws Exception{
        String file=kind==1?"psp_game_hq.ogg":"psp_title_hq.ogg";
        MediaPlayer p=new MediaPlayer();
        try(AssetFileDescriptor source=assets.openFd(file)){
            p.setAudioAttributes(new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build());
            p.setDataSource(source.getFileDescriptor(),source.getStartOffset(),source.getLength());
        }catch(Exception ex){p.release();throw ex;}
        try{
            p.setLooping(true);
            p.prepare();
            p.setVolume(0f,0f);
            p.start();
            return p;
        }catch(Exception ex){p.release();throw ex;}
    }
    private void closeTrack(){
        if(music!=null){
            try{music.stop();}catch(Exception ignored){}
            try{music.release();}catch(Exception ignored){}
            music=null;
        }
    }
    private void loop(){
        AudioTrack audio=null;
        try{
            loadPCM();
            if(!running)return;
            int req=AudioTrack.getMinBufferSize(RATE,AudioFormat.CHANNEL_OUT_MONO,
                     AudioFormat.ENCODING_PCM_16BIT);
            if(req<0)req=8192;
            audio=new AudioTrack(AudioManager.STREAM_MUSIC,RATE,
                AudioFormat.CHANNEL_OUT_MONO,AudioFormat.ENCODING_PCM_16BIT,
                Math.max(req*2,8192),AudioTrack.MODE_STREAM);
            if(audio.getState()!=AudioTrack.STATE_INITIALIZED)return;
            current=audio;audio.play();
            short[] out=new short[CHUNK];
            int songPos=0,voicePos=-1,lastTrack=-1,lastCuts=-1,lastWin=-1,lastUi=-1;
            int noise=0x159e1,cutPulse=0,uiPulse=0,cheerPulse=0;
            float phase1=0,phase2=0,motorEnv=0,uiPhase=0;
            while(running){
                int phase,cuts,wins,ui;
                boolean musicEnabled,clipping;
                synchronized(game){
                    phase=game.phase;
                    cuts=game.cutSerial;
                    wins=game.winSerial;
                    ui=game.uiSerial;
                    musicEnabled=game.sound;
                    clipping=game.shaving();
                }
                int type=phase==GameState.PLAY?1:0;
                if(type!=lastTrack){
                    songPos=0;
                    lastTrack=type;
                    closeTrack();
                    if(hqAvailable){
                        try{music=newTrack(type);}
                        catch(Exception missing){
                            Log.w(TAG,"HD music unavailable, falling back to original PSP PCM",missing);
                            hqAvailable=false;
                        }
                    }
                }
                if(lastCuts!=-1&&cuts!=lastCuts)cutPulse=RATE/9;
                if(lastUi!=-1&&ui!=lastUi)uiPulse=RATE/11;
                if(lastWin!=-1&&wins!=lastWin){
                    voicePos=0;cheerPulse=RATE*2;
                }
                if(lastWin==-1&&phase==GameState.WIN){voicePos=0;cheerPulse=RATE*2;}
                lastCuts=cuts;lastWin=wins;lastUi=ui;

                float gain=musicEnabled?(voicePos>=0?.20f:(motorEnv>.02f?.34f:.68f)):0f;
                if(music!=null)try{music.setVolume(gain,gain);}catch(Exception ignored){}
                byte[] pcm=(type==1)?instrumental:full;
                for(int n=0;n<CHUNK;n++){
                    // v1.2 low-bandwidth music is a fallback only; HQ Ogg is stereo.
                    float song=0;
                    if(!hqAvailable&&musicEnabled){
                        int k=songPos>>2;
                        int next=(k+1<pcm.length)?k+1:0;
                        float a=((pcm[k]&255)-128)/128f;
                        float b=((pcm[next]&255)-128)/128f;
                        song=(a+(b-a)*((songPos&3)*.25f))*.50f;
                    }
                    songPos++;
                    if(songPos>=pcm.length*4)songPos=0;
                    noise=noise*1664525+1013904223;
                    float white=(noise>>>24)/127.5f-1f;
                    motorEnv+=((clipping?1f:0f)-motorEnv)*.0034f;
                    float clip=(triangle(phase1)*.17f+
                                triangle(phase2)*.06f+white*.06f)*motorEnv;
                    if(cutPulse>0){
                        clip+=(triangle(phase2)*.075f+white*.032f)*(cutPulse/(RATE/9f));
                        cutPulse--;
                    }
                    phase1+=107f/RATE;phase2+=221.5f/RATE;
                    if(phase1>=1f)phase1-=1f;
                    if(phase2>=1f)phase2-=1f;
                    float voice=0;
                    if(voicePos>=0){
                        int ix=voicePos>>1;
                        if(ix>=victory.length)voicePos=-1;
                        else {
                            int next=Math.min(ix+1,victory.length-1);
                            float a=((victory[ix]&255)-128)/128f;
                            float b=((victory[next]&255)-128)/128f;
                            voice=(a+(b-a)*((voicePos&1)*.5f))*.76f;
                            voicePos++;
                        }
                    }
                    float uiFx=0;
                    if(uiPulse>0){
                        uiPhase+=700f/RATE;
                        if(uiPhase>=1f)uiPhase-=1f;
                        float env=uiPulse/(RATE/11f);
                        uiFx=(float)Math.sin(uiPhase*Math.PI*2)*.09f*env*env;
                        uiPulse--;
                    }
                    float crowd=0;
                    if(cheerPulse>0){
                        float elapsed=(RATE*2-cheerPulse)/(float)RATE;
                        float envelope=Math.min(1f,elapsed*4)*Math.min(1f,(2f-elapsed)*2);
                        float applauding=((int)(elapsed*8)%3==0)?white*.06f:0f;
                        float roar=white*.08f+
                          (float)Math.sin(elapsed*2f*Math.PI*380f)*.025f;
                        crowd=(roar+applauding)*envelope;
                        cheerPulse--;
                    }
                    float combined=(hqAvailable?0:song)*gain+clip+voice+uiFx+crowd;
                    combined=Math.max(-.96f,Math.min(.96f,combined));
                    out[n]=(short)(combined*22000f);
                }
                if(audio.write(out,0,CHUNK)<0)break;
            }
        }catch(Exception ex){
            Log.e(TAG,"Audio mixer failed, but visuals and gameplay remain usable",ex);
        }finally{
            closeTrack();
            if(audio!=null){
                try{audio.stop();}catch(Exception ignored){}
                try{audio.release();}catch(Exception ignored){}
            }
            if(current==audio)current=null;
        }
    }
}
