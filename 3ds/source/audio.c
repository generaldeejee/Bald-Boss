/* Bald Elite original Nintendo 3DS arcade sound.
 * Small synthesised instrumental loop, real clipper buzz and success fanfare.
 * No copyrighted samples, external files, or DSP runtime requirement.
 * NDSP may be unavailable on some homebrew environments: fail safely.
 */
#include <3ds.h>
#include <math.h>
#include <string.h>
#include "audio.h"

#define RATE 22050
#define LOOP_SAMPLES (RATE*4)
#define BUZZ_SAMPLES 2205
#define WIN_SAMPLES 11025
#define TWO_PI 6.28318530717958647692f
static s16 *music_pcm,*buzz_pcm,*win_pcm;
static ndspWaveBuf music_buffer,buzz_buffer,win_buffer;
static bool running=false;
static bool last_music=true,last_buzz=false;

static float tri(float p){return 1.0f-4.0f*fabsf(p-.5f);}
static float clamp_audio(float sample){
    if(sample>1.0f)return 1.0f;
    if(sample< -1.0f)return -1.0f;
    return sample;
}
static void fill_music(void){
    /* 4-second instrumental chip-rock loop at 120bpm. 8 eighth notes per bar.
       This is temporary original 3DS music, NOT the PSP Piper vocal track. */
    static const float melody[16]={
        329.63f,392.0f,493.88f,392.0f,
        329.63f,440.0f,493.88f,587.33f,
        493.88f,392.0f,329.63f,392.0f,
        440.0f,493.88f,392.0f,329.63f
    };
    for(int i=0;i<LOOP_SAMPLES;i++){
        float t=(float)i/RATE;
        int step=(int)(t*4.0f)&15;
        float note=melody[step],beat=fmodf(t,.5f);
        float phase=fmodf(t*note,1.0f);
        float lead=.21f*tri(phase)*(1.0f-fminf(beat*3.0f,.65f));
        float bass=.16f*sinf(TWO_PI*82.41f*t);
        float kick= .25f*sinf(TWO_PI*55.0f*beat)*expf(-beat*20.0f);
        float snare=((i*1103515245u+12345u)&256?1.0f:-1.0f);
        snare*=.045f*expf(-fmodf(t+.25f,.5f)*24.0f);
        music_pcm[i]=(s16)(clamp_audio(lead+bass+kick+snare)*15000.0f);
    }
}
static void fill_buzz(void){
    for(int i=0;i<BUZZ_SAMPLES;i++){
        float t=(float)i/RATE;
        float hum=sinf(TWO_PI*109.0f*t)*.52f;
        float raspy=sinf(TWO_PI*219.0f*t)*.23f;
        float noise=(float)((i*1741u+11u)%251u)/251.0f-.5f;
        buzz_pcm[i]=(s16)(clamp_audio(hum+raspy+noise*.18f)*9500.0f);
    }
}
static void fill_win(void){
    const float freqs[4]={329.63f,415.30f,493.88f,659.25f};
    for(int i=0;i<WIN_SAMPLES;i++){
        float t=(float)i/RATE;
        int note=(int)(t*8.0f);
        if(note>3)note=3;
        float envelope=1.0f-t*.55f;
        float x=.42f*envelope*sinf(TWO_PI*freqs[note]*t);
        win_pcm[i]=(s16)(x*18000.0f);
    }
}
static void channel_setup(int id,float level){
    ndspChnReset(id);
    ndspChnSetInterp(id,NDSP_INTERP_LINEAR);
    ndspChnSetRate(id,RATE);
    ndspChnSetFormat(id,NDSP_FORMAT_MONO_PCM16);
    float mix[12]={0};
    mix[0]=level;mix[1]=level;
    ndspChnSetMix(id,mix);
}
bool be_audio_init(void){
    if(R_FAILED(ndspInit()))return false;
    music_pcm=(s16*)linearAlloc(LOOP_SAMPLES*sizeof(s16));
    buzz_pcm=(s16*)linearAlloc(BUZZ_SAMPLES*sizeof(s16));
    win_pcm=(s16*)linearAlloc(WIN_SAMPLES*sizeof(s16));
    if(!music_pcm||!buzz_pcm||!win_pcm){
        be_audio_shutdown();return false;
    }
    fill_music();fill_buzz();fill_win();
    DSP_FlushDataCache(music_pcm,LOOP_SAMPLES*sizeof(s16));
    DSP_FlushDataCache(buzz_pcm,BUZZ_SAMPLES*sizeof(s16));
    DSP_FlushDataCache(win_pcm,WIN_SAMPLES*sizeof(s16));
    memset(&music_buffer,0,sizeof(music_buffer));
    memset(&buzz_buffer,0,sizeof(buzz_buffer));
    memset(&win_buffer,0,sizeof(win_buffer));
    music_buffer.data_pcm16=music_pcm;music_buffer.nsamples=LOOP_SAMPLES;
    music_buffer.looping=true;
    buzz_buffer.data_pcm16=buzz_pcm;buzz_buffer.nsamples=BUZZ_SAMPLES;
    buzz_buffer.looping=true;
    win_buffer.data_pcm16=win_pcm;win_buffer.nsamples=WIN_SAMPLES;
    channel_setup(0,.28f);
    channel_setup(1,.48f);
    channel_setup(2,.7f);
    ndspChnWaveBufAdd(0,&music_buffer);
    ndspChnWaveBufAdd(1,&buzz_buffer);
    ndspChnSetPaused(1,true);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    running=true;last_music=true;last_buzz=false;
    return true;
}
void be_audio_update(bool music,bool shaving){
    if(!running)return;
    if(music!=last_music){ndspChnSetPaused(0,!music);last_music=music;}
    if(shaving!=last_buzz){ndspChnSetPaused(1,!shaving);last_buzz=shaving;}
}
void be_audio_victory(void){
    if(!running)return;
    ndspChnWaveBufClear(2);
    memset(&win_buffer,0,sizeof(win_buffer));
    win_buffer.data_pcm16=win_pcm;
    win_buffer.nsamples=WIN_SAMPLES;
    ndspChnWaveBufAdd(2,&win_buffer);
}
void be_audio_shutdown(void){
    if(running){
        ndspChnReset(0);ndspChnReset(1);ndspChnReset(2);
    }
    if(music_pcm){linearFree(music_pcm);music_pcm=NULL;}
    if(buzz_pcm){linearFree(buzz_pcm);buzz_pcm=NULL;}
    if(win_pcm){linearFree(win_pcm);win_pcm=NULL;}
    ndspExit();
    running=false;
}
