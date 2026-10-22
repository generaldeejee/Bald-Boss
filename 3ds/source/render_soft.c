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
static void backdrop(u8 *fb,const BeGame *g){
    for(int x=0;x<SW;x++)for(int y=0;y<SH;y++){
        int glow=(int)(34.0f*(1.0f-fabsf((float)(x-200)/220.0f)));
        glow=CLAMP(glow,0,36);
        int grain=((x*13+y*7)&7);
        pixel(fb,x,y,13+glow/5+grain/4,26+glow/3,40+glow/2);
    }
    /* Barber-shop mirror, gold trim, side columns, tiled counter. */
    rect(fb,18,18,364,207,43,61,75);
    rect(fb,23,23,354,196,8,20,31);
    rect(fb,27,27,346,188,18,34,46);
    rect(fb,27,27,346,3,151,116,54);
    rect(fb,27,212,346,3,151,116,54);
    rect(fb,23,26,3,190,145,111,47);
    rect(fb,373,26,3,190,145,111,47);
    for(int x=40;x<=360;x+=40)rect(fb,x,230,2,10,51,65,70);
    rect(fb,0,228,400,12,24,38,45);
    /* Proper title-phase still paints both LCDs, even with no input. */
    if(g->phase==BE_TITLE)rect(fb,27,27,346,4,203,159,76);
}
static void head_draw(u8 *fb,const BeMesh *m,const BeGame *g){
    const float sy=sinf(g->yaw),cy=cosf(g->yaw);
    const float sp=sinf(g->pitch),cp=cosf(g->pitch);
    const float distance=be_game_distance(g);
    const float xrad2=1.0f/(.645f*.645f);
    const float yrad2=1.0f/(1.13f*1.13f);
    const float zrad2=1.0f/(.66f*.66f);
    /* Transform the view ray into local head coordinates. */
    float oy=cp*.06f+sp*distance;
    float oz0=-sp*.06f+cp*distance;
    float ox=-sy*oz0,oz=cy*oz0+.04f;
    float cc=ox*ox*xrad2+oy*oy*yrad2+oz*oz*zrad2-1.0f;
    for(int x=36;x<364;x+=2){
        float vx=((float)x+.5f-200.0f)/FOCAL;
        for(int y=12;y<226;y+=2){
            float vy=(120.0f-((float)y+.5f))/FOCAL;
            float py=cp*vy-sp;
            float pz=-sp*vy-cp;
            float dx=cy*vx-sy*pz;
            float dz=sy*vx+cy*pz;
            float aa=dx*dx*xrad2+py*py*yrad2+dz*dz*zrad2;
            float bb=2.0f*(ox*dx*xrad2+oy*py*yrad2+oz*dz*zrad2);
            float discriminant=bb*bb-4.0f*aa*cc;
            if(discriminant<=0.0f)continue;
            float t=(-bb-sqrtf(discriminant))/(2.0f*aa);
            if(t<=0.0f)continue;
            float hx=ox+dx*t,hy=oy+py*t,hz=oz+dz*t;
            float u=.5f+atan2f(hx,hz-.04f)/(2.0f*PI);
            float v=(1.14f-hy)/2.16f;
            int tx=CLAMP((int)(u*255.0f),0,255);
            int ty=CLAMP((int)(v*255.0f),0,255);
            int idx=ty*256+tx;
            unsigned short c=m->skin_base[idx];
            float pol=(float)m->shaved_coverage[idx]/255.0f;
            float light=.82f+.08f*hy+.05f*(hz/.66f);
            light=CLAMP(light,.58f,1.0f);
            int r=(int)(((c&31)*255.0f/31.0f)*(1.0f-.10f*pol)*light+pol*34.0f);
            int green=(int)((((c>>5)&63)*255.0f/63.0f)*(1.0f-.09f*pol)*light+pol*24.0f);
            int b=(int)((((c>>11)&31)*255.0f/31.0f)*(1.0f-.07f*pol)*light+pol*20.0f);
            r=CLAMP(r,0,255);green=CLAMP(green,0,255);b=CLAMP(b,0,255);
            pixel(fb,x,y,r,green,b);pixel(fb,x+1,y,r,green,b);
            pixel(fb,x,y+1,r,green,b);pixel(fb,x+1,y+1,r,green,b);
        }
    }
}
static int project_point(const BeGame *g,float x,float y,float z,float *sx,float *sy){
    const float cy=cosf(g->yaw),sn=sinf(g->yaw);
    const float cp=cosf(g->pitch),sp=sinf(g->pitch);
    float xx=cy*x+sn*z,zz=cy*z-sn*x;
    float yy=cp*y-sp*zz,zr=sp*y+cp*zz;
    float depth=be_game_distance(g)-zr;
    if(depth<.5f)return 0;
    float f=FOCAL/depth;
    *sx=200.0f+xx*f;*sy=120.0f-(yy-.06f)*f;
    return 1;
}
static void hair_draw(u8 *fb,const BeMesh *m,const BeGame *g){
    const int colors[3][3]={{61,37,27},{224,58,116},{204,194,180}};
    int candidate=CLAMP(m->candidate,0,2);
    for(int i=0;i<m->hair_count;i++){
        const BeHair *h=&m->strands[i];
        if(h->gone)continue;
        float sx,sy,ex,ey;
        if(!be_game_project(g,h,&sx,&sy))continue;
        float length=h->length;
        if(!project_point(g,h->x+h->nx*length*.55f,
                          h->y+length*.7f,h->z+h->nz*length*.55f,&ex,&ey))continue;
        int shade=(i*31)&15;
        line(fb,(int)sx,(int)sy,(int)ex,(int)ey,
             CLAMP(colors[candidate][0]-shade,0,255),
             CLAMP(colors[candidate][1]-shade,0,255),
             CLAMP(colors[candidate][2]-shade,0,255),candidate==1?2:1);
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
    backdrop(fb,game);
    head_draw(fb,mesh,game);
    hair_draw(fb,mesh,game);
    hud_draw(fb,game);
}
