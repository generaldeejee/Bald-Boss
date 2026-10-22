/* Bald Elite 3DS - CPU framebuffer renderer.
 * Recovery from first-frame PICA200/Citro3D stalls on original 3DS.
 * Intentionally uses only the libctru gfx framebuffer, with no shaders,
 * render targets, vertex buffers or GPU command queue.
 *
 * The existing game.c/mesh.c remain the source of truth for gameplay.
 */
#include <3ds.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "mesh.h"
#include "render_soft.h"

#define SW 400
#define SH 240
#define FOCAL (120.0f/0.466307658f)
#define PI 3.14159265358979323846f
#define CLAMP(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))

typedef struct {float x,y,vx,vy;int life;} Particle;
static Particle particles[20];
static unsigned p_head=0;
static uint32_t rng=0xB01D2026u;
static float random01(void){rng=rng*1664525u+1013904223u;return (float)((rng>>12)&4095)/4095.0f;}

/* The 3DS framebuffer is column-major and vertically inverted.
 * With GSP_BGR8_OES each pixel is written as B,G,R bytes. */
static inline void pixel(u8 *fb,int x,int y,int r,int g,int b){
    if((unsigned)x>=SW||(unsigned)y>=SH)return;
    u8 *p=fb+3*(x*SH+(SH-1-y));
    p[0]=(u8)b;p[1]=(u8)g;p[2]=(u8)r;
}
static void rect(u8 *fb,int x,int y,int w,int h,int r,int g,int b){
    int endx=CLAMP(x+w,0,SW),endy=CLAMP(y+h,0,SH);
    x=CLAMP(x,0,SW);y=CLAMP(y,0,SH);
    for(int i=x;i<endx;i++)for(int j=y;j<endy;j++)pixel(fb,i,j,r,g,b);
}
static void line(u8 *fb,int x0,int y0,int x1,int y1,int r,int g,int b,int width){
    int dx=x1-x0,dy=y1-y0;
    int n=(abs(dx)>abs(dy)?abs(dx):abs(dy));
    if(n>160)n=160;
    if(n==0)n=1;
    for(int i=0;i<=n;i++){
        int x=x0+(int)((float)dx*i/n),y=y0+(int)((float)dy*i/n);
        pixel(fb,x,y,r,g,b);
        if(width>1){pixel(fb,x+1,y,r,g,b);pixel(fb,x,y+1,r,g,b);}
    }
}
/* Pre-rendered mirror and barbershop backdrop, only when scenery changes. */
static void backdrop(u8 *fb,const BeGame *g){
    for(int x=0;x<SW;x++){
        int glow=CLAMP(24-abs(x-200)/11,0,24);
        for(int y=0;y<SH;y++){
            int grain=((x*13+y*7)&7);
            pixel(fb,x,y,12+glow/7+grain/4,25+glow/3,38+glow/2);
        }
    }
    rect(fb,11,10,378,218,35,43,49);
    rect(fb,15,14,370,211,134,101,48);
    rect(fb,19,18,362,203,72,52,34);
    rect(fb,25,24,350,191,15,29,40);
    for(int x=28;x<373;x++){
        int glow=CLAMP(18-abs(x-147)/12,0,18);
        rect(fb,x,29,1,182,18+glow/3,38+glow/2,52+glow);
    }
    rect(fb,31,32,3,172,96,124,137);
    rect(fb,365,30,3,180,45,73,89);
    rect(fb,25,24,350,3,213,175,90);
    rect(fb,25,212,350,3,196,152,70);
    rect(fb,15,15,370,2,230,193,110);
    /* Small salon lights and a dark countertop with a gold edge. */
    for(int x=53;x<=347;x+=49){
        rect(fb,x,17,16,2,237,221,158);
        rect(fb,x+3,19,10,3,140,158,159);
    }
    rect(fb,0,226,400,14,25,37,44);
    rect(fb,0,226,400,2,170,131,59);
    if(g->phase==BE_TITLE)rect(fb,29,30,342,2,215,172,79);
}

/* Head geometry UV and lighting are expensive on the old 3DS: compute
 * them ONLY when the camera changes. During shaving we reuse the map and
 * resample the changing scalp coverage instead of running 30,000 atan2f
 * calls or rebuilding 37,000 triangles. Full-resolution 1px rendering. */
static u8 scene_cache[SW*SH*3];
static u16 head_uv[SW*SH];
static u8 head_light[SW*SH];
static bool lookup_valid=false,scene_valid=false;
static float last_yaw=0.0f,last_pitch=0.0f;
static int last_zoom=-1,last_removed=-1,last_candidate=-1,last_phase=-1,last_mode=-1;
static unsigned move_frames=0;

static float fast_angle(float y,float x){
    /* Max error <~0.01 radians; far cheaper than libm atan2f on ARM11. */
    const float ay=fabsf(y)+0.0000001f;
    float r,angle;
    if(x<0.0f){
        r=(x+ay)/(ay-x);
        angle=2.35619449f;
    }else{
        r=(x-ay)/(x+ay);
        angle=.78539816f;
    }
    angle+=(.1963f*r*r-.9817f)*r;
    return y<0.0f?-angle:angle;
}
static void rebuild_head_lookup(const BeGame *g){
    float sn=sinf(g->yaw),cs=cosf(g->yaw);
    float sp=sinf(g->pitch),cp=cosf(g->pitch);
    float distance=be_game_distance(g);
    const float irx=1.0f/(.645f*.645f);
    const float iry=1.0f/(1.13f*1.13f);
    const float irz=1.0f/(.66f*.66f);
    float oy=cp*.06f+sp*distance;
    float oz0=-sp*.06f+cp*distance;
    float ox=-sn*oz0,oz=cs*oz0+.04f;
    float cc=ox*ox*irx+oy*oy*iry+oz*oz*irz-1.0f;
    for(int x=35;x<365;x++){
        float vx=((float)x+.5f-200.0f)/FOCAL;
        for(int y=10;y<227;y++){
            int index=x*SH+y;
            head_uv[index]=0xffff;
            float vy=(120.0f-(float)y-.5f)/FOCAL;
            float py=cp*vy-sp,pz=-sp*vy-cp;
            float dx=cs*vx-sn*pz,dz=sn*vx+cs*pz;
            float aa=dx*dx*irx+py*py*iry+dz*dz*irz;
            float bb=2.0f*(ox*dx*irx+oy*py*iry+oz*dz*irz);
            float disc=bb*bb-4.0f*aa*cc;
            if(disc<=0.0f)continue;
            float t=(-bb-sqrtf(disc))/(2.0f*aa);
            if(t<=0.0f)continue;
            float hx=ox+dx*t,hy=oy+py*t,hz=oz+dz*t;
            float u=.5f+fast_angle(hx,hz-.04f)/(2.0f*PI);
            float v=(1.14f-hy)/2.16f;
            int tx=CLAMP((int)(u*255.0f+.5f),0,255);
            int ty=CLAMP((int)(v*255.0f+.5f),0,255);
            head_uv[index]=(u16)(ty*256+tx);
            /* Soft key light + sculpted nose bridge, cheeks and crown. */
            float luminance=.70f+.15f*(hz+.66f)/1.32f+.06f*hy;
            float bridge=CLAMP((.12f-fabsf(hx))/.12f,0.0f,1.0f);
            float bridge_y=CLAMP((.48f-fabsf(hy+.08f))/.48f,0.0f,1.0f);
            if(hz>.24f)luminance+=.13f*bridge*bridge_y;
            if(hx<-.2f)luminance+=.035f;
            head_light[index]=(u8)CLAMP((int)(luminance*255),95,255);
        }
    }
    lookup_valid=true;
    last_yaw=g->yaw;last_pitch=g->pitch;last_zoom=g->zoom;
}
static void head_draw(u8 *fb,const BeMesh *m){
    for(int x=35;x<365;x++){
        for(int y=10;y<227;y++){
            int i=x*SH+y;
            u16 idx=head_uv[i];
            if(idx==0xffff)continue;
            u16 color=m->skin_base[idx];
            int l=head_light[i];
            int r=((color&31)*255/31)*l/255;
            int green=(((color>>5)&63)*255/63)*l/255;
            int b=(((color>>11)&31)*255/31)*l/255;
            int shine=m->shaved_coverage[idx];
            r=CLAMP(r+shine/6,0,255);
            green=CLAMP(green+shine/8,0,255);
            b=CLAMP(b+shine/11,0,255);
            u8 *p=fb+3*(x*SH+SH-1-y);
            p[0]=(u8)b;p[1]=(u8)green;p[2]=(u8)r;
        }
    }
}


static void hair_draw(u8 *fb,const BeMesh *m,const BeGame *g){
    static const int rgb[3][3]={{72,45,29},{232,62,127},{202,193,169}};
    const int candidate=CLAMP(m->candidate,0,2);
    const float cs=cosf(g->yaw),sn=sinf(g->yaw);
    const float cp=cosf(g->pitch),sp=sinf(g->pitch);
    const float distance=be_game_distance(g);
    for(int i=0;i<m->hair_count;i++){
        const BeHair *h=&m->strands[i];
        if(h->gone)continue;
        float facing=sp*h->ny+cp*(cs*h->nz-sn*h->nx);
        if(facing<-.03f)continue;
        float xx=cs*h->x+sn*h->z;
        float zz=cs*h->z-sn*h->x;
        float yy=cp*h->y-sp*zz;
        float zr=sp*h->y+cp*zz;
        float depth=distance-zr;
        if(depth<.8f)continue;
        float scale=FOCAL/depth;
        float sx=200.0f+xx*scale;
        float sy=120.0f-(yy-.06f)*scale;
        if(sx<0||sx>=400||sy<0||sy>=240)continue;
        float tip=h->length;
        float tipx=h->x+h->nx*tip*.55f;
        float tipy=h->y+tip*.7f;
        float tipz=h->z+h->nz*tip*.55f;
        float tx=cs*tipx+sn*tipz;
        float tz=cs*tipz-sn*tipx;
        float ty=cp*tipy-sp*tz;
        float td=distance-(sp*tipy+cp*tz);
        if(td<.7f)continue;
        float ts=FOCAL/td;
        int brightness=(i*17)&17;
        line(fb,(int)sx,(int)sy,(int)(200.0f+tx*ts),
             (int)(120.0f-(ty-.06f)*ts),
             rgb[candidate][0]-brightness,
             rgb[candidate][1]-brightness,
             rgb[candidate][2]-brightness,
             candidate==1?2:1);
    }
}

void be_soft_emit(float x,float y,int hits){
    int count=CLAMP(hits,0,8);
    for(int i=0;i<count;i++){
        Particle *p=&particles[p_head++%20];
        p->x=x+(random01()-.5f)*20.0f;
        p->y=y+(random01()-.5f)*10.0f;
        p->vx=(random01()-.5f)*3.0f;p->vy=-2.0f-random01();
        p->life=20;
    }
}
static void hud_draw(u8 *fb,const BeGame *g){
    rect(fb,0,0,400,8,12,24,34);
    rect(fb,0,0,400,3,186,151,71);
    if(g->phase==BE_PLAY){
        int progress=CLAMP(be_game_percentage(g)*4,0,400);
        rect(fb,0,230,400,10,28,46,53);
        rect(fb,0,230,progress,10,62,209,166);
        int x=(int)g->cursor_x,y=(int)g->cursor_y;
        line(fb,x-12,y,x+12,y,240,204,81,1);
        line(fb,x,y-12,x,y+12,240,204,81,1);
        rect(fb,x-9,y+14,18,21,45,71,83);
        rect(fb,x-11,y+11,22,6,192,201,197);
        rect(fb,x-2,y-2,4,4,70,244,189);
        for(int i=0;i<20;i++){
            Particle *p=&particles[i];
            if(p->life<=0)continue;
            pixel(fb,(int)p->x,(int)p->y,200,160,90);
            p->x+=p->vx;p->y+=p->vy;p->vy+=.13f;p->life--;
        }
        if(g->paused){
            rect(fb,100,92,200,9,198,157,69);
            rect(fb,100,139,200,9,198,157,69);
        }
    }else if(g->phase==BE_WIN){
        rect(fb,40,208,320,15,216,174,75);
        for(int i=0;i<be_game_stars(g);i++)rect(fb,163+i*27,211,19,8,255,242,163);
    }else if(g->phase==BE_FAIL)rect(fb,45,210,310,10,192,45,46);
}
void be_soft_draw(const BeMesh *mesh,const BeGame *game){
    u8 *fb=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL);
    if(!fb)return;
    const bool new_level=(last_candidate!=game->candidate||
                          last_zoom!=game->zoom);
    const bool camera_moved=(game->yaw!=last_yaw||game->pitch!=last_pitch);
    bool recalc=!lookup_valid||new_level;
    /* Reproject moving camera at ~30Hz; input, cursor and clipping still
       run at full frame rate. Never stall every shave stroke on ray tracing. */
    if(!recalc && camera_moved){
        if(++move_frames>=2){recalc=true;move_frames=0;}
    }else if(!camera_moved)move_frames=0;
    if(recalc){
        rebuild_head_lookup(game);
        scene_valid=false;
    }
    if(!scene_valid||last_removed!=mesh->removed||
       last_candidate!=game->candidate||last_phase!=game->phase||
       last_mode!=game->mode){
        backdrop(scene_cache,game);
        head_draw(scene_cache,mesh);
        hair_draw(scene_cache,mesh,game);
        last_removed=mesh->removed;
        last_candidate=game->candidate;
        last_phase=game->phase;
        last_mode=game->mode;
        scene_valid=true;
    }
    /* Copy the cached non-interactive scene; only cursor/progress/particles
       are composited fresh. Makes A-held shaving cheap even on old 3DS. */
    memcpy(fb,scene_cache,sizeof(scene_cache));
    hud_draw(fb,game);
}
