/* Bald Boss 3D - PSP Edition v0.7.1 UV face mapping fix barber UI, restart and finish camera with Deep Voice soundtrack. Original homebrew source.
   Uses PSPSDK GU/GUM hardware 3D renderer. No PS1 BIOS calls. */
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspaudio.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

PSP_MODULE_INFO("Bald Boss 3D PSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define SW 480
#define SH 272
#define FBW 512
#define PI 3.14159265358979323846f
#define HR 34
#define HS 52
#define HAIRS 1250
#define FACE_STUBBLE 420
#define AUDIO_SAMPLES 1024
#define AUDIO_RATE 44100
#define MUSIC_STEP_SAMPLES 11025
#define CAM_DISTANCE 5.70f
#define CAM_Y 0.08f
#define FOCAL_PIXELS 312.5f
#define MAX_HAIR_VERTS (HAIRS*6)
#define MAX_HEAD_VERTS 11000
#define HUD_QUADS 4096 /* font is now drawn using reliable solid-color sprite rectangles */
#define BG_QUADS 768
#define CLAMP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define C(r,g,b) (0xff000000u | ((unsigned)(b)<<16) | ((unsigned)(g)<<8) | (unsigned)(r))

static unsigned int __attribute__((aligned(16))) gu_list[262144];
static unsigned short __attribute__((aligned(16))) skin_tex[256*256];

typedef struct {float u,v, nx,ny,nz, x,y,z;} TexturedVertex;
#include "head_model.h"
#include "head_texture.h"
#include "voice_sample.h"
#include "music_samples.h"
typedef struct {float nx,ny,nz, x,y,z;} LitVertex;
typedef struct {float x,y,z;} PosVertex;
typedef struct {unsigned int color;float x,y,z;} UiVertex;
typedef struct {float x,y,z,nx,ny,nz,tx,ty,tz,length;unsigned char gone;} Hair;

static TexturedVertex __attribute__((aligned(16))) head[MAX_HEAD_VERTS];
static int n_head=0;
static LitVertex __attribute__((aligned(16))) feature[4096];
static int n_feature=0;
static PosVertex __attribute__((aligned(16))) hair_mesh[MAX_HAIR_VERTS];
static PosVertex __attribute__((aligned(16))) beard_stubble[FACE_STUBBLE*3];
static int n_beard_stubble=0;
static int n_hair_verts=0;
static UiVertex __attribute__((aligned(16))) hud[HUD_QUADS*2];
static int n_hud=0;
static UiVertex __attribute__((aligned(16))) bg[BG_QUADS*2];
static int n_bg=0;
static Hair hairs[HAIRS];
static int n_hairs=0, removed=0, score=0;
static int phase=0,frame=0, play_frames=0;
static float yaw=0.0f, cursor_x=240.0f,cursor_y=128.0f;
static unsigned int prev_buttons=0;
static unsigned int rand_seed=0xB01D2026u;
static int shave_flash=0;
static int shades_on=0;
/* Renders and hair hit-testing must use the SAME perspective distance. */
static int zoom_level=0; /* 0: normal, 1: 1.4x, 2: 1.8x (approximately) */
static int paused=0;
static int pause_choice=0; /* 0 resume; 1 restart; 2 return to title */
static int finish_zoom_override=0;
static int restart_grace=0;
static float current_camera_distance=CAM_DISTANCE;
static float zoom_camera_distance(void){return current_camera_distance;}
static float target_camera_distance(void){
  static const float d[3]={CAM_DISTANCE,4.20f,3.20f};
  int remaining=n_hairs-removed;
  if(phase==1 && remaining>0 && remaining<=12 && !finish_zoom_override)
    return remaining<=5?2.95f:3.25f;
  return d[zoom_level];
}
static void update_camera_zoom(void){
  float target=target_camera_distance();
  current_camera_distance+=(target-current_camera_distance)*0.12f;
  if(fabsf(target-current_camera_distance)<0.005f)current_camera_distance=target;
}
static volatile int clipper_active=0;
static volatile int win_voice_pending=0;
static volatile int stop_voice_pending=0;
static volatile int music_enabled=1;
static int audio_channel=-1;
static short __attribute__((aligned(64))) audio_pcm[2][AUDIO_SAMPLES*2];

/* 5x7 ASCII font: glyphs 32..95. We draw it as colored sprites (no texture UVs). */
static const unsigned char font5[][5]={
{0,0,0,0,0},{0,0,0x5f,0,0},{0x07,0,0x07,0,0},{0x14,0x7f,0x14,0x7f,0x14},
{0x24,0x2a,0x7f,0x2a,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0,0x05,0x03,0,0},
{0,0x1c,0x22,0x41,0},{0,0x41,0x22,0x1c,0},{0x14,0x08,0x3e,0x08,0x14},{0x08,0x08,0x3e,0x08,0x08},
{0,0x50,0x30,0,0},{0x08,0x08,0x08,0x08,0x08},{0,0x60,0x60,0,0},{0x20,0x10,0x08,0x04,0x02},
{0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
{0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e},{0,0x36,0x36,0,0},{0,0x56,0x36,0,0},
{0x08,0x14,0x22,0x41,0},{0x14,0x14,0x14,0x14,0x14},{0,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},
{0x32,0x49,0x79,0x41,0x3e},{0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
{0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},{0x3e,0x41,0x49,0x49,0x7a},
{0x7f,0x08,0x08,0x08,0x7f},{0,0x41,0x7f,0x41,0},{0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},
{0x7f,0x40,0x40,0x40,0x40},{0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
{0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},
{0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0,0x7f,0x41,0x41,0},
{0x02,0x04,0x08,0x10,0x20},{0,0x41,0x41,0x7f,0},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40}
};

static unsigned int random_u32(void){rand_seed=rand_seed*1664525u+1013904223u;return rand_seed;}
static float rand01(void){return (float)((random_u32()>>8)&65535)/65535.0f;}
static float fsq(float a){return a*a;}
static void normalize(float *x,float *y,float *z){float l=sqrtf(fsq(*x)+fsq(*y)+fsq(*z));if(l>0.000001f){*x/=l;*y/=l;*z/=l;}}
static float wrap01dist(float a,float b){float d=fabsf(a-b);return d<(1.0f-d)?d:(1.0f-d);}

/* RGB565 texture adapted from a CC0 OpenGameArt face texture.
   The PSP keeps it at 128x128 to stay cache-friendly and lightweight. */

static void make_textures(void){
 for(int i=0;i<HEAD_TEX_W*HEAD_TEX_H;i++) skin_tex[i]=head_texture_cc0[i];
}

static void add_tri(TexturedVertex a,TexturedVertex b,TexturedVertex c){if(n_head+3>MAX_HEAD_VERTS)return;head[n_head++]=a;head[n_head++]=b;head[n_head++]=c;}

static TexturedVertex surface(float a,float p){
  TexturedVertex v;
  float st=sinf(a),ct=cosf(a),sp=sinf(p),cp=cosf(p);
  float front=CLAMP(cp,0.0f,1.0f),side=fabsf(sp);
  float x=0.79f*st*sp;
  float y=1.09f*ct;
  float z=0.84f*st*cp;
  /* Cranial mass: taller skull and slightly fuller back of the head. */
  if(ct>0.10f){y+=0.10f*ct*ct;z-=0.03f*ct;}
  if(cp<0.0f)z-=0.06f*st*(-cp);
  /* Narrower chin and softer jaw. */
  float jaw_narrow=1.0f-0.26f*CLAMP((-ct-0.02f)*1.2f,0.0f,1.0f);
  x*=jaw_narrow;
  /* Cheekbones and a flatter front face. */
  float cheek=expf(-fsq(a-1.54f)*20.0f)*front;
  x*=1.0f+0.10f*cheek*(0.30f+0.70f*side);
  z+=0.028f*cheek*(0.35f+0.65f*side);
  z-=0.11f*front*expf(-fsq(a-1.58f)*4.0f);
  /* Brow ridge and eye sockets. */
  float eye_side=expf(-fsq(sp-0.34f)*16.0f)+expf(-fsq(sp+0.34f)*16.0f);
  z+=0.026f*expf(-fsq(a-1.14f)*40.0f)*eye_side*front;
  z-=0.062f*expf(-fsq(a-1.34f)*58.0f)*eye_side*front;
  /* Integrated nose bridge and tip. */
  float mid=expf(-sp*sp*34.0f)*front;
  z+=mid*(0.055f*expf(-fsq(a-1.36f)*42.0f)+0.125f*expf(-fsq(a-1.73f)*80.0f));
  x*=1.0f-0.08f*mid;
  /* Nostril wings. */
  float nost=(expf(-fsq(sp-0.10f)*130.0f)+expf(-fsq(sp+0.10f)*130.0f))*expf(-fsq(a-1.82f)*120.0f)*front;
  x*=1.0f+0.035f*nost;
  z+=0.020f*nost;
  /* Mouth volume and chin. */
  z+=mid*(0.030f*expf(-fsq(a-2.03f)*75.0f)+0.050f*expf(-fsq(a-2.18f)*110.0f));
  z+=mid*0.060f*expf(-fsq(a-2.42f)*45.0f);
  y-=0.050f*expf(-fsq(a-2.56f)*55.0f);
  /* Slight temple pinch keeps the skull less balloon-like from the front. */
  x*=1.0f-0.08f*expf(-fsq(a-1.15f)*20.0f)*(0.3f+0.7f*side);
  v.x=x;v.y=y;v.z=z;
  v.nx=x/(0.79f*0.79f);v.ny=y/(1.09f*1.09f);v.nz=z/(0.84f*0.84f);
  normalize(&v.nx,&v.ny,&v.nz);
  v.u=p/(2.0f*PI);v.v=a/PI;
  return v;
}
static void make_head(void){
  n_head=HEAD_MODEL_INDEX_COUNT;
  for(int i=0;i<n_head;i++)head[i]=head_model_vertices[head_model_indices[i]];
}
static void add_lit(LitVertex a,LitVertex b,LitVertex c){if(n_feature+3>4096)return;feature[n_feature++]=a;feature[n_feature++]=b;feature[n_feature++]=c;}
static void make_ellipsoid(float cx,float cy,float cz,float rx,float ry,float rz,int rings,int slices){
  for(int i=0;i<rings;i++)for(int j=0;j<slices;j++){
    LitVertex v[4];
    for(int k=0;k<4;k++){
      float a=(float)(i+(k==1||k==3))*PI/rings;
      float p=(float)(j+(k>=2))*2.0f*PI/slices;
      float nx=sinf(a)*sinf(p),ny=cosf(a),nz=sinf(a)*cosf(p);
      v[k].nx=nx;v[k].ny=ny;v[k].nz=nz;
      v[k].x=cx+rx*nx;v[k].y=cy+ry*ny;v[k].z=cz+rz*nz;
    }
    add_lit(v[0],v[1],v[2]);add_lit(v[2],v[1],v[3]);
  }
}
/* Feature geometry is drawn in small separate lighting/material batches. */
static void draw_ellipsoid(float x,float y,float z,float rx,float ry,float rz,unsigned color,int rings,int slices){
  n_feature=0;
  make_ellipsoid(x,y,z,rx,ry,rz,rings,slices);
  sceGuDisable(GU_TEXTURE_2D);
  sceGuMaterial(GU_AMBIENT, color);
  sceGuMaterial(GU_DIFFUSE, color);
  LitVertex *vertex_copy=(LitVertex*)sceGuGetMemory((size_t)n_feature*sizeof(LitVertex));
  memcpy(vertex_copy,feature,(size_t)n_feature*sizeof(LitVertex));
  sceGumDrawArray(GU_TRIANGLES, GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_3D,n_feature,0,vertex_copy);
}


static void make_hairs(void){
  n_hairs=0;
  /* Sample the real imported scalp rather than projecting onto the old sphere. */
  for(int tries=0; tries<HAIRS*28 && n_hairs<HAIRS;tries++){
    int tri=((int)(rand01()*(n_head/3)))*3;
    if(tri<0||tri+2>=n_head)continue;
    TexturedVertex *a=&head[tri], *b=&head[tri+1], *c=&head[tri+2];
    float r=sqrtf(rand01()), t=rand01();
    float wa=1.0f-r,wb=r*(1.0f-t),wc=r*t;
    float x=wa*a->x+wb*b->x+wc*c->x;
    float y=wa*a->y+wb*b->y+wc*c->y;
    float z=wa*a->z+wb*b->z+wc*c->z;
    if(y<0.33f || (z>0.58f && y<0.73f))continue;
    float nx=wa*a->nx+wb*b->nx+wc*c->nx;
    float ny=wa*a->ny+wb*b->ny+wc*c->ny;
    float nz=wa*a->nz+wb*b->nz+wc*c->nz;
    normalize(&nx,&ny,&nz);
    Hair *h=&hairs[n_hairs++];
    h->x=x;h->y=y;h->z=z;h->nx=nx;h->ny=ny;h->nz=nz;
    h->tx=nz;h->ty=0;h->tz=-nx;
    normalize(&h->tx,&h->ty,&h->tz);
    h->length=.022f+.028f*rand01();h->gone=0;
  }
}

/* Dense facial shadow: tiny low-contrast triangular strands on jaw and cheeks.
   Generate this once, draw with depth testing like the scalp hair. */
static void make_beard_stubble(void){
  n_beard_stubble=0;
  for(int tries=0;tries<FACE_STUBBLE*18 && n_beard_stubble<FACE_STUBBLE*3-3;tries++){
    int tri=((int)(rand01()*(n_head/3)))*3;
    if(tri<0||tri+2>=n_head)continue;
    TexturedVertex *a=&head[tri], *b=&head[tri+1], *c=&head[tri+2];
    float r=sqrtf(rand01()), t=rand01();
    float w0=1.0f-r,w1=r*(1.0f-t),w2=r*t;
    float y=w0*a->y+w1*b->y+w2*c->y;
    float z=w0*a->z+w1*b->z+w2*c->z;
    if(y>-.16f||y<-.85f||z<.34f)continue;
    float x=w0*a->x+w1*b->x+w2*c->x;
    float nx=w0*a->nx+w1*b->nx+w2*c->nx;
    float ny=w0*a->ny+w1*b->ny+w2*c->ny;
    float nz=w0*a->nz+w1*b->nz+w2*c->nz;
    normalize(&nx,&ny,&nz);
    x+=nx*.010f;y+=ny*.010f;z+=nz*.010f;
    float w=.003f,h=.007f;
    beard_stubble[n_beard_stubble++]=(PosVertex){x-w,y,z};
    beard_stubble[n_beard_stubble++]=(PosVertex){x+w,y,z};
    beard_stubble[n_beard_stubble++]=(PosVertex){x+nx*h,y+h*.5f,z+nz*h};
  }
}
static void draw_beard_stubble(void){
 sceGuDisable(GU_TEXTURE_2D);sceGuDisable(GU_LIGHTING);
 sceGuColor(C(96,72,61));
 sceGumDrawArray(GU_TRIANGLES,GU_VERTEX_32BITF|GU_TRANSFORM_3D,n_beard_stubble,0,beard_stubble);
 sceGuEnable(GU_LIGHTING);
}

static void hair_vertices(PosVertex *dst,Hair *h,int variant){
  int remaining=n_hairs-removed;
  float visibility=(remaining<=10)?1.95f:((remaining<=24)?1.55f:((remaining<=40)?1.24f:1.0f));
  float thickness=((variant==0)?.0060f:.0035f)*visibility;
  float len=h->length*(variant==0?1.0f:0.72f)*((remaining<=10)?1.20f:((remaining<=24)?1.10f:1.0f));
  float lean=(variant==0)?.010f:-.008f;
  float bx=h->x+h->nx*.011f,by=h->y+h->ny*.011f,bz=h->z+h->nz*.011f;
  dst[0]=(PosVertex){bx+h->tx*thickness,by,bz+h->tz*thickness};
  dst[1]=(PosVertex){bx-h->tx*thickness,by,bz-h->tz*thickness};
  dst[2]=(PosVertex){bx+h->nx*len+h->tx*lean,by+h->ny*len+.013f,bz+h->nz*len+h->tz*lean};
}
static void draw_hair(void){
  n_hair_verts=0;
  int remaining=n_hairs-removed;
  /* Cache all live hairs in one GPU draw call, 2 triangles per tuft. */
  for(int i=0;i<n_hairs;i++)if(!hairs[i].gone){
    if(n_hair_verts+6>MAX_HAIR_VERTS)break;
    hair_vertices(&hair_mesh[n_hair_verts],&hairs[i],0);n_hair_verts+=3;
    hair_vertices(&hair_mesh[n_hair_verts],&hairs[i],1);n_hair_verts+=3;
  }
  sceGuDisable(GU_TEXTURE_2D);
  sceGuDisable(GU_LIGHTING);
  if(remaining<=10)sceGuColor((frame/12)%2?C(255,250,120):C(255,120,64));
  else if(remaining<=24)sceGuColor(C(255,225,120));
  else if(remaining<=40)sceGuColor(C(92,64,52));
  else sceGuColor(C(46,32,28));
  PosVertex *hair_copy=(PosVertex*)sceGuGetMemory((size_t)n_hair_verts*sizeof(PosVertex));
  memcpy(hair_copy,hair_mesh,(size_t)n_hair_verts*sizeof(PosVertex));
  sceGumDrawArray(GU_TRIANGLES,GU_VERTEX_32BITF|GU_TRANSFORM_3D,n_hair_verts,0,hair_copy);
  sceGuEnable(GU_LIGHTING);
}

static void ui_box(int x,int y,int w,int h,unsigned int color){
 if(n_hud+2>HUD_QUADS*2)return;
 hud[n_hud++]=(UiVertex){color,(float)x,(float)y,0.0f};
 hud[n_hud++]=(UiVertex){color,(float)(x+w),(float)(y+h),0.0f};
}
static void bg_box(int x,int y,int w,int h,unsigned int color){
 if(n_bg+2>BG_QUADS*2)return;
 bg[n_bg++]=(UiVertex){color,(float)x,(float)y,0.0f};
 bg[n_bg++]=(UiVertex){color,(float)(x+w),(float)(y+h),0.0f};
}
static void draw_backdrop(void){
 n_bg=0;
 bg_box(0,0,SW,218,C(18,29,40));
 bg_box(0,218,SW,54,C(27,31,36));
 for(int x=0;x<SW;x+=40) bg_box(x,43,20,174,((x/40)&1)?C(17,36,60):C(13,24,43));
 bg_box(32,55,416,145,C(9,19,34));
 bg_box(36,59,408,137,C(17,32,52));
 bg_box(52,76,376,102,phase==0?C(24,40,64):C(20,34,56));
 bg_box(58,82,364,90,C(22,38,62));
 for(int y=0;y<90;y+=18)bg_box(58,82+y,364,2,C(31,51,80));
 for(int side=0;side<2;side++){
   int px=side?429:35;
   bg_box(px,70,14,120,C(224,230,232));
   for(int y=66;y<194;y+=16){
     bg_box(px,y+((side?8:0)),14,8,C(181,48,62));
     bg_box(px,y+8,14,8,C(53,132,188));
   }
   bg_box(px-2,66,18,4,C(205,177,96));
   bg_box(px-2,190,18,4,C(205,177,96));
 }
 for(int y=218;y<272;y+=18)for(int x=0;x<SW;x+=24)
   bg_box(x,y,12,9,(((x+y)/18)&1)?C(40,45,52):C(28,32,39));
 if(phase==0){
   bg_box(250,60,178,24,C(76,40,28));
   bg_box(252,62,174,20,C(109,63,45));
   bg_box(258,67,162,10,C(133,84,64));
 }
 sceKernelDcacheWritebackAll();
 sceGuDisable(GU_LIGHTING);sceGuDisable(GU_DEPTH_TEST);
 sceGuDisable(GU_TEXTURE_2D);sceGuDisable(GU_BLEND);
 sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,n_bg,0,bg);
}
/* Rasterize the existing 5x7 font as opaque GU_SPRITES rectangles.
   The prior HUD used normalized 0..1 UVs with PSP's pixel-space texture
   coordinates: it sampled transparent texels and drew no visible text.
   This implementation reuses the UI primitive path already proven in PPSSPP.
   Consecutive vertical pixels are merged into a single rectangle. */
static void ui_text(float x,float y,const char *s,float scale){
 if(scale<1.0f)scale=1.0f;
 const unsigned int ink=C(241,244,238);
 while(*s){
   unsigned char c=(unsigned char)*s++;
   if(c>='a'&&c<='z')c=(unsigned char)(c-32);
   if(c<32||c>95)c='?';
   for(int col=0;col<5;col++){
     unsigned char bits=font5[c-32][col];
     int row=0;
     while(row<7){
       if(!(bits&(1u<<row))){row++;continue;}
       int first=row;
       do{row++;}while(row<7 && (bits&(1u<<row)));
       int x0=(int)(x+col*scale+0.5f);
       int x1=(int)(x+(col+1)*scale+0.5f);
       int y0=(int)(y+first*scale+0.5f);
       int y1=(int)(y+row*scale+0.5f);
       if(x1>x0 && y1>y0)ui_box(x0,y0,x1-x0,y1-y0,ink);
     }
   }
   x+=7.0f*scale;
 }
}

static int project_hair_screen(const Hair *h,float *sx,float *sy){
  float s=sinf(yaw),c=cosf(yaw);
  const float camera=zoom_camera_distance();
  float zz=h->z*c-h->x*s;
  float xx=h->x*c+h->z*s;
  float face=h->nz*c-h->nx*s;
  if(face<0.13f)return 0;
  float inv=1.0f/(camera-zz);
  *sx=240.0f+FOCAL_PIXELS*xx*inv;
  *sy=136.0f-FOCAL_PIXELS*(h->y+CAM_Y)*inv;
  return (*sx>=6 && *sx<SW-6 && *sy>=50 && *sy<218);
}
static void draw_last_hair_markers(void){
  int remaining=n_hairs-removed;
  if(remaining>12)return;
  int blink=((frame/10)&1)==0;
  for(int i=0;i<n_hairs;i++){
    if(hairs[i].gone)continue;
    float sx,sy;
    if(!project_hair_screen(&hairs[i],&sx,&sy))continue;
    int x=(int)sx,y=(int)sy;
    unsigned int col=blink?C(255,235,90):C(255,122,70);
    ui_box(x-6,y-1,12,2,col);
    ui_box(x-1,y-6,2,12,col);
    ui_box(x-8,y-8,3,3,col);
    ui_box(x+5,y-8,3,3,col);
    ui_box(x-8,y+5,3,3,col);
    ui_box(x+5,y+5,3,3,col);
  }
}
static void draw_ui(void){
  n_hud=0;
  ui_box(0,0,SW,44,C(10,17,26));
  ui_box(0,42,SW,3,C(193,155,80));
  ui_box(0,45,SW,2,C(57,126,139));
  ui_box(0,217,SW,3,C(193,155,80));
  ui_box(0,220,SW,52,C(10,17,26));
  ui_box(0,269,SW,3,C(57,126,139));
  ui_box(9,7,5,29,C(193,155,80));
  ui_text(19,9,"BALD BOSS",1.95f);
  ui_text(180,12,phase==0?"BOSS BARBERS":"THE FINAL HAIRCUT",1.00f);
  ui_text(419,8,music_enabled?"MUSIC":"MUTED",.93f);
  if(phase!=0){
    int finishing=phase==1 && n_hairs-removed>0 && n_hairs-removed<=12 && !finish_zoom_override;
    const char *z=finishing?"FINISH CAM":(zoom_level==0?"ZOOM 1X":(zoom_level==1?"ZOOM 1.4X":"ZOOM 1.8X"));
    ui_text(337,28,z,1.0f);
  }

  if(phase==0){
    /* A dedicated intro panel sits beside the smaller rotating portrait. */
    ui_box(236,61,231,151,C(10,18,28));
    ui_box(236,61,3,151,C(193,155,80));
    ui_text(268,66,"BOSS BARBERS",1.28f);
    ui_text(251,88,"THE BALD BOSS CREED",1.10f);
    ui_text(252,113,"BALD MEN ARE THE",1.04f);
    ui_text(252,129,"TOUGHEST ON EARTH.",1.04f);
    ui_text(252,147,"ONLY TRUE BALD BOSSES",.90f);
    ui_text(252,160,"WELCOME HERE.",.90f);
    ui_text(262,181,"PRESS X TO START",1.22f);
    ui_text(278,195,"THE CHAIR AWAITS",0.92f);
    ui_text(18,227,"NO WIGS. NO FEAR. JUST BALDNESS.",1.02f);
    ui_text(19,253,"X START  L/R TURN  O ZOOM  SELECT MUSIC",.95f);
  } else {
    ui_box(9,227,129,36,C(17,30,42));
    ui_box(146,227,143,36,C(17,30,42));
    ui_box(297,227,174,36,C(17,30,42));
    ui_box(14,239,109,4,C(43,56,67));
    ui_box(14,239,(109*removed)/(n_hairs?n_hairs:1),4,C(80,211,177));
    ui_box(140,226,2,39,C(193,155,80));
    ui_box(291,226,2,39,C(193,155,80));
    ui_text(14,225,"SHAVED",1.05f);
    char buf[32];
    snprintf(buf,sizeof(buf),"%d%%",n_hairs?removed*100/n_hairs:0);
    ui_text(83,246,buf,1.45f);
    ui_text(153,225,"SCORE",1.05f);
    snprintf(buf,sizeof(buf),"%d",score);
    ui_text(153,246,buf,1.45f);
    ui_text(306,225,"TIME",1.05f);
    int seconds=90-play_frames/60;if(seconds<0)seconds=0;
    snprintf(buf,sizeof(buf),"%02d:%02d",seconds/60,seconds%60);
    ui_text(306,246,buf,1.45f);
    snprintf(buf,sizeof(buf),"LEFT %d",n_hairs-removed);
    ui_text(396,246,buf,1.05f);
    if(phase==1 && n_hairs-removed<=12){
      ui_box(297,220,174,7,C(47,74,86));
      ui_text(397,226,"TRACK",.86f);
    }
    if(phase==2){
      ui_box(104,168,272,42,C(10,16,27));
      ui_text(158,181,"MASTER BARBER!",1.65f);
    }else if(phase==3){
      ui_box(104,168,272,42,C(10,16,27));
      ui_text(168,181,"TIME IS UP!",1.65f);
    }
    if(phase!=1 && frame%90>45)ui_text(192,202,"X TO RESTART",1.15f);
    if(phase==1 && !paused){
      draw_last_hair_markers();
      int cx=(int)cursor_x,cy=(int)cursor_y;
      ui_box(cx-9,cy-27,18,28,C(35,45,59));
      ui_box(cx-8,cy-29,16,6,C(189,199,206));
      for(int i=-6;i<=6;i+=4)ui_box(cx+i,cy-33,2,5,C(222,230,229));
      ui_box(cx-5,cy-9,10,8,shave_flash?C(61,228,183):C(42,184,202));
      ui_box(cx-1,cy+5,2,9,C(255,221,135));
    }
    if(paused){
      ui_box(103,60,274,156,C(8,14,22));
      ui_box(106,63,268,150,C(26,37,48));
      ui_box(109,66,262,25,C(14,28,40));
      ui_box(109,66,262,3,C(196,160,91));
      ui_text(207,75,"PAUSED",1.55f);
      ui_text(125,95,"CHOOSE YOUR NEXT MOVE",1.00f);
      for(int i=0;i<3;i++){
        int top=117+i*24;
        ui_box(125,top,230,20,pause_choice==i?C(58,77,92):C(16,25,35));
        if(pause_choice==i){
          ui_box(125,top,4,20,C(239,192,96));
          ui_text(137,top+5,">",1.0f);
        }
      }
      ui_text(188,122,"RESUME",1.20f);
      ui_text(174,146,"RESTART RUN",1.20f);
      ui_text(160,170,"RETURN TO TITLE",1.20f);
      ui_text(132,199,"UP/DOWN  X SELECT  O BACK",1.0f);
    }
  }
  /* PSP 2D sprite font: proven in PPSSPP, no problematic texture UVs. */
  sceKernelDcacheWritebackAll();
  sceGuDisable(GU_LIGHTING);sceGuDisable(GU_DEPTH_TEST);
  sceGuDisable(GU_TEXTURE_2D);sceGuDisable(GU_BLEND);
  sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,n_hud,0,hud);
}



static void render_head(void){
  sceGumMatrixMode(GU_PROJECTION);sceGumLoadIdentity();
  sceGumPerspective(47.0f,(float)SW/SH,0.1f,20.0f);
  sceGumMatrixMode(GU_VIEW);sceGumLoadIdentity();
  sceGumMatrixMode(GU_MODEL);sceGumLoadIdentity();
  /* Smaller portrait on the title screen makes room for the introduction. */
  ScePspFVector3 pos={phase==0?-1.02f:0.0f,
                      phase==0?0.10f:CAM_Y,
                      phase==0?-6.90f:-zoom_camera_distance()};
  sceGumTranslate(&pos);
  ScePspFVector3 rot={0,yaw,0};sceGumRotateXYZ(&rot);
  sceGuEnable(GU_DEPTH_TEST);sceGuDepthMask(GU_FALSE);
  sceGuEnable(GU_LIGHTING);sceGuEnable(GU_LIGHT0);sceGuEnable(GU_LIGHT1);
  ScePspFVector3 sun={-0.20f,0.64f,0.74f};
  ScePspFVector3 fill={0.38f,0.10f,0.92f};
  sceGuLight(0,GU_DIRECTIONAL,GU_DIFFUSE,&sun);
  sceGuLightColor(0,GU_DIFFUSE,0x00fff7ee);
  sceGuLight(1,GU_DIRECTIONAL,GU_DIFFUSE,&fill);
  sceGuLightColor(1,GU_DIFFUSE,0x003b4656);
  sceGuAmbient(0x001f2730);
  sceGuMaterial(GU_AMBIENT,0xffffffff);
  sceGuMaterial(GU_DIFFUSE,0xffffffff);
  sceGuAmbientColor(0xffffffff);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuTexMode(GU_PSM_5650,0,0,0);
  sceGuTexImage(0,HEAD_TEX_W,HEAD_TEX_H,HEAD_TEX_W,skin_tex);
  /* The imported OBJ stores UVs as 0..127. Normalize to 0..1 for
     the PSP 3D GPU, otherwise the face texture repeats 127 times. */
  sceGuTexMapMode(GU_TEXTURE_COORDS,0,0);
  sceGuTexScale(1.0f/127.0f,1.0f/127.0f);
  sceGuTexOffset(0.0f,0.0f);
  sceGuTexWrap(GU_CLAMP,GU_CLAMP);
  sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
  sceGuTexFilter(GU_LINEAR,GU_LINEAR);
  sceGuDisable(GU_LIGHTING); /* show face texture clearly instead of washing it out */
  sceGuColor(0xffffffff);
  sceGumDrawArray(GU_TRIANGLES,
    GU_TEXTURE_32BITF|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
    n_head,0,head);
  sceGuEnable(GU_LIGHTING);
  draw_beard_stubble();
  /* The CC0 imported mesh already includes anatomical nose, lips, jaw, ears
     and shoulders. Do not overlay primitive spheres on it. */
  draw_hair();
}


static void reset_game(void){
  for(int i=0;i<n_hairs;i++)hairs[i].gone=0;
  removed=0;score=0;play_frames=0;cursor_x=240;cursor_y=130;
  yaw=0;shave_flash=0;phase=1;paused=0;pause_choice=0;
  zoom_level=0;finish_zoom_override=0;current_camera_distance=CAM_DISTANCE;
  clipper_active=0;restart_grace=10;stop_voice_pending=1;
  win_voice_pending=0;
}
static void return_to_title(void){
  for(int i=0;i<n_hairs;i++)hairs[i].gone=0;
  removed=0;score=0;play_frames=0;cursor_x=240;cursor_y=130;
  yaw=0;shave_flash=0;phase=0;paused=0;pause_choice=0;
  zoom_level=0;finish_zoom_override=0;current_camera_distance=CAM_DISTANCE;
  clipper_active=0;restart_grace=0;stop_voice_pending=1;
  win_voice_pending=0;
}
static void shave_at_cursor(void){
  float s=sinf(yaw),c=cosf(yaw);
  const float camera=zoom_camera_distance();
  int hit=0;
  for(int i=0;i<n_hairs;i++){
    Hair *h=&hairs[i];if(h->gone)continue;
    float zz=h->z*c-h->x*s;
    float xx=h->x*c+h->z*s;
    float face=h->nz*c-h->nx*s;
    if(face<0.13f)continue; /* Never shave hidden back-side hair. */
    /* Camera and hit-testing share exactly the same perspective model. */
    float inv=1.0f/(camera-zz);
    float sx=240.0f+FOCAL_PIXELS*xx*inv;
    float sy=136.0f-FOCAL_PIXELS*(h->y+CAM_Y)*inv;
    if(fsq((sx-cursor_x)/17.0f)+fsq((sy-cursor_y)/20.0f)<1.0f){
      h->gone=1;removed++;score+=20;hit++;
    }
  }
  if(hit)shave_flash=7;
  if(removed>=n_hairs && n_hairs>0 && phase==1){
    phase=2;
    clipper_active=0;
    win_voice_pending=1; /* Announce a fully shaved head just once. */
  }
}
static void update(void){
 SceCtrlData pad;
 sceCtrlPeekBufferPositive(&pad,1);
 unsigned int b=pad.Buttons, just=b&~prev_buttons; prev_buttons=b;
 if(just&PSP_CTRL_SELECT)music_enabled=!music_enabled;
 if(phase!=1){
   clipper_active=0;
   if(just&PSP_CTRL_CROSS)reset_game();
   else if(phase==0)yaw=0.10f*sinf((float)frame*0.010f);
   return;
 }
 if(just&PSP_CTRL_START){
   if(!paused){paused=1;pause_choice=0;clipper_active=0;return;}
   paused=0;clipper_active=0;return;
 }
 if(paused){
   clipper_active=0;
   if(just&PSP_CTRL_UP)pause_choice=(pause_choice+2)%3;
   if(just&PSP_CTRL_DOWN)pause_choice=(pause_choice+1)%3;
   if(just&PSP_CTRL_CIRCLE){paused=0;return;}
   if(just&PSP_CTRL_CROSS){
     if(pause_choice==0){paused=0;return;}
     if(pause_choice==1){reset_game();return;}
     if(pause_choice==2){return_to_title();return;}
   }
   return;
 }
 if(just&PSP_CTRL_TRIANGLE)shades_on=!shades_on;
 if(just&PSP_CTRL_CIRCLE){
   if(n_hairs-removed<=12)finish_zoom_override=1;
   zoom_level=(zoom_level+1)%3;
 }
 update_camera_zoom();
 if(b&PSP_CTRL_LTRIGGER)yaw-=.034f;
 if(b&PSP_CTRL_RTRIGGER)yaw+=.034f;
 if(yaw>=2*PI)yaw-=2*PI;
 if(yaw<0)yaw+=2*PI;
 if(b&PSP_CTRL_LEFT)cursor_x-=3.5f;
 if(b&PSP_CTRL_RIGHT)cursor_x+=3.5f;
 if(b&PSP_CTRL_UP)cursor_y-=3.5f;
 if(b&PSP_CTRL_DOWN)cursor_y+=3.5f;
 if(pad.Lx>146||pad.Lx<110)cursor_x+=((int)pad.Lx-128)/35.0f;
 if(pad.Ly>146||pad.Ly<110)cursor_y+=((int)pad.Ly-128)/35.0f;
 cursor_x=CLAMP(cursor_x,35,445);cursor_y=CLAMP(cursor_y,60,207);
 if(restart_grace>0){restart_grace--;clipper_active=0;}
 else clipper_active=(b&(PSP_CTRL_CROSS|PSP_CTRL_SQUARE))?1:0;
 if(clipper_active)shave_at_cursor();
 if(shave_flash)shave_flash--;
 play_frames++;
 if(play_frames>=90*60 && phase==1){phase=3;clipper_active=0;}
}

/* Deep Voice Edition soundtrack.
   Both tracks are PCM8 mono at 11,025 Hz, generated from the ORIGINAL full
   and instrumental mixes during the GitHub Actions build. No OGG decoder is
   needed on PSP: we interpolate 4:1 in the existing 44,100-Hz audio thread.
   Title/results: full song; while shaving: instrumental song. */
static float triangle_wave(float p){return 1.0f-4.0f*fabsf(p-0.5f);}
static int audio_thread(SceSize args,void *argp){
 (void)args;(void)argp;
 int ch=sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,AUDIO_SAMPLES,PSP_AUDIO_FORMAT_STEREO);
 if(ch<0)return 0;
 audio_channel=ch;
 float motor_phase=0.0f,motor2_phase=0.0f,motor_env=0.0f;
 int bufno=0,voice_sample_pos=-1,last_track=-1;
 unsigned int song_pos=0,noise=0x159e1u;
 while(1){
  short *buf=audio_pcm[bufno];
  /* Take a snapshot per buffer; avoids a mid-buffer track/data mismatch. */
  int track_kind=(phase==1)?1:0;
  if(track_kind!=last_track){last_track=track_kind;song_pos=0;}
  const unsigned char *track=track_kind?bald_music_instrumental:bald_music_full;
  const unsigned int track_len=track_kind?BALD_MUSIC_INSTRUMENTAL_LEN:BALD_MUSIC_FULL_LEN;
  if(stop_voice_pending){voice_sample_pos=-1;stop_voice_pending=0;}
  if(win_voice_pending){voice_sample_pos=0;win_voice_pending=0;}
  for(int i=0;i<AUDIO_SAMPLES;i++){
   /* Recorded source is 11,025 Hz; four output frames per input sample.
      Loop naturally and start each scene's track from the beginning. */
   unsigned int k=song_pos>>2;
   unsigned int next=k+1<track_len?k+1:0;
   float sample0=((int)track[k]-128)*(1.0f/128.0f);
   float sample1=((int)track[next]-128)*(1.0f/128.0f);
   float song=sample0+(sample1-sample0)*((song_pos&3)*0.25f);
   float fade=song_pos<4096?(float)song_pos/4096.0f:1.0f;
   if(++song_pos>=track_len*4u)song_pos=0;
   noise=noise*1664525u+1013904223u;
   float target=clipper_active?1.0f:0.0f;
   motor_env+=(target-motor_env)*.0034f;
   float clipper=(triangle_wave(motor_phase)*.17f+
       triangle_wave(motor2_phase)*.060f+
       ((noise>>24)/127.5f-1.0f)*.060f)*motor_env;
   motor_phase+=107.0f/(float)AUDIO_RATE;
   motor2_phase+=221.5f/(float)AUDIO_RATE;
   if(motor_phase>=1.0f)motor_phase-=1.0f;
   if(motor2_phase>=1.0f)motor2_phase-=1.0f;
   float voice=0.0f;
   if(voice_sample_pos>=0){
     int ix=voice_sample_pos>>2;
     if(ix>=VOICE_SAMPLE_LEN)voice_sample_pos=-1;
     else{
       float v0=((int)voice_sample[ix]-128)*(1.0f/128.0f);
       float v1=((int)voice_sample[ix+1<VOICE_SAMPLE_LEN?ix+1:ix]-128)*(1.0f/128.0f);
       voice=(v0+(v1-v0)*((voice_sample_pos&3)*.25f))*.65f;
       voice_sample_pos++;
     }
   }
   /* Music ducks for clipper and announcer. SELECT mutes only music. */
   float music_gain=voice_sample_pos>=0?.20f:(motor_env>.02f?.34f:.62f);
   float sound=(music_enabled?song*fade*music_gain:0.0f)+clipper+voice;
   sound=CLAMP(sound,-.96f,.96f);
   short pcm=(short)(sound*22000.0f);
   buf[i*2]=pcm;buf[i*2+1]=pcm;
  }
  sceKernelDcacheWritebackRange(buf,AUDIO_SAMPLES*2*sizeof(short));
  if(sceAudioOutputBlocking(ch,PSP_AUDIO_VOLUME_MAX,buf)<0)break;
  bufno^=1;
 }
 sceAudioChRelease(ch);audio_channel=-1;
 return 0;
}

static void start_music(void){
 int th=sceKernelCreateThread("bald_boss_music",audio_thread,0x18,0x2000,0,NULL);
 if(th>=0)sceKernelStartThread(th,0,NULL);
}

static int exit_cb(int arg1,int arg2,void *arg){(void)arg1;(void)arg2;(void)arg;sceKernelExitGame();return 0;}
static int callbacks_thread(SceSize args,void *argp){
 (void)args;(void)argp;
 int cb=sceKernelCreateCallback("exit_cb",exit_cb,NULL);
 sceKernelRegisterExitCallback(cb);
 sceKernelSleepThreadCB();return 0;
}
static void setup_callbacks(void){
 int th=sceKernelCreateThread("callback_thread",callbacks_thread,0x11,0x1000,0,NULL);
 if(th>=0)sceKernelStartThread(th,0,NULL);
}
static void init_graphics(void){
 sceGuInit();
 sceGuStart(GU_DIRECT,gu_list);
 sceGuDrawBuffer(GU_PSM_8888,(void*)0,FBW);
 sceGuDispBuffer(SW,SH,(void*)(FBW*SH*4),FBW);
 sceGuDepthBuffer((void*)(FBW*SH*8),FBW);
 sceGuOffset(2048-SW/2,2048-SH/2);
 sceGuViewport(2048,2048,SW,SH);
 sceGuDepthRange(65535,0);
 sceGuScissor(0,0,SW,SH);sceGuEnable(GU_SCISSOR_TEST);
 sceGuDepthFunc(GU_GEQUAL);sceGuEnable(GU_DEPTH_TEST);
 sceGuDepthMask(GU_FALSE);
 sceGuShadeModel(GU_SMOOTH);
 sceGuEnable(GU_CLIP_PLANES);
 sceGuDisable(GU_CULL_FACE);
 sceGuFinish();sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
 sceDisplayWaitVblankStart();sceGuDisplay(GU_TRUE);
}
static void draw_frame(void){
 sceGuStart(GU_DIRECT,gu_list);
 sceGuClearColor(C(13,26,42));sceGuClearDepth(0);
 sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
 draw_backdrop();
 render_head();
 draw_ui();
 sceGuFinish();sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
 sceDisplayWaitVblankStart();sceGuSwapBuffers();
}
int main(int argc,char**argv){
 (void)argc;(void)argv;
 setup_callbacks();
 sceCtrlSetSamplingCycle(0);
 sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 make_textures();make_head();make_hairs();make_beard_stubble();
 sceKernelDcacheWritebackAll();
 init_graphics();
 start_music();
 while(1){update();draw_frame();frame++;}
 sceGuTerm();sceKernelExitGame();return 0;
}
