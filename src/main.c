/* Bald Boss 3D - PSP Edition v0.4 (integrated low-poly head, face texture and finer stubble). Original homebrew source.
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
#define CAM_DISTANCE 4.72f
#define CAM_Y 0.10f
#define FOCAL_PIXELS 304.0f
#define MAX_HAIR_VERTS (HAIRS*6)
#define MAX_HEAD_VERTS (HR*HS*6)
#define HUD_QUADS 4096 /* font is now drawn using reliable solid-color sprite rectangles */
#define CLAMP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define C(r,g,b) (0xff000000u | ((unsigned)(b)<<16) | ((unsigned)(g)<<8) | (unsigned)(r))

static unsigned int __attribute__((aligned(16))) gu_list[262144];
static unsigned short __attribute__((aligned(16))) skin_tex[128*128];

typedef struct {float u,v, nx,ny,nz, x,y,z;} TexturedVertex;
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
static Hair hairs[HAIRS];
static int n_hairs=0, removed=0, score=0;
static int phase=0,frame=0, play_frames=0;
static float yaw=0.0f, cursor_x=240.0f,cursor_y=128.0f;
static unsigned int prev_buttons=0;
static unsigned int rand_seed=0xB01D2026u;
static int shave_flash=0;
static int shades_on=0;
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

/* RGB565 offers smoother complexion gradients than the original 4-bit RGB4444.
   The colour noise is subtle, rather than high-contrast random mottling. */

static void make_textures(void){
  for(int y=0;y<128;y++)for(int x=0;x<128;x++){
    float u=(float)x/127.0f;
    float v=(float)y/127.0f;
    float ang=u*2.0f*PI;
    float front=fmaxf(cosf(ang),0.0f);
    float side=sinf(ang);
    float cheek=expf(-fsq(v-0.56f)*55.0f)*(expf(-fsq(side-0.42f)*18.0f)+expf(-fsq(side+0.42f)*18.0f))*front;
    int grain=(int)(((x*1973u+y*9277u+89173u)^rand_seed)&7)-3;
    int r=208+grain+(int)(cheek*12.0f);
    int g=169+grain+(int)(cheek*4.0f);
    int b=141+grain-(int)(front*3.0f);
    /* Soft facial shading and temple falloff. */
    float temple=(expf(-fsq(side-0.70f)*10.0f)+expf(-fsq(side+0.70f)*10.0f))*expf(-fsq(v-0.42f)*18.0f)*front;
    r-=(int)(temple*10.0f); g-=(int)(temple*9.0f); b-=(int)(temple*8.0f);
    /* Brow shadows. */
    float brow=(expf(-fsq(side-0.26f)*58.0f)+expf(-fsq(side+0.26f)*58.0f))*expf(-fsq(v-0.36f)*240.0f)*front;
    r-=(int)(brow*72.0f); g-=(int)(brow*60.0f); b-=(int)(brow*49.0f);
    /* Eye whites, irises and pupils painted into the texture to keep the face unified. */
    float eyeL=expf(-fsq(side+0.24f)*105.0f-fsq(v-0.455f)*520.0f)*front;
    float eyeR=expf(-fsq(side-0.24f)*105.0f-fsq(v-0.455f)*520.0f)*front;
    float irisL=expf(-fsq(side+0.24f)*370.0f-fsq(v-0.458f)*1250.0f)*front;
    float irisR=expf(-fsq(side-0.24f)*370.0f-fsq(v-0.458f)*1250.0f)*front;
    float pupilL=expf(-fsq(side+0.24f)*1000.0f-fsq(v-0.458f)*2300.0f)*front;
    float pupilR=expf(-fsq(side-0.24f)*1000.0f-fsq(v-0.458f)*2300.0f)*front;
    if(eyeL+eyeR>0.04f){r+=(int)((eyeL+eyeR)*42.0f);g+=(int)((eyeL+eyeR)*41.0f);b+=(int)((eyeL+eyeR)*32.0f);}    
    if(irisL>0.02f){r=(int)(r*(1.0f-irisL)+irisL*90.0f);g=(int)(g*(1.0f-irisL)+irisL*126.0f);b=(int)(b*(1.0f-irisL)+irisL*116.0f);}    
    if(irisR>0.02f){r=(int)(r*(1.0f-irisR)+irisR*90.0f);g=(int)(g*(1.0f-irisR)+irisR*126.0f);b=(int)(b*(1.0f-irisR)+irisR*116.0f);}    
    if(pupilL+pupilR>0.03f){r=(int)(r*(1.0f-(pupilL+pupilR))+7.0f*(pupilL+pupilR));g=(int)(g*(1.0f-(pupilL+pupilR))+12.0f*(pupilL+pupilR));b=(int)(b*(1.0f-(pupilL+pupilR))+14.0f*(pupilL+pupilR));}
    /* Nose bridge highlight and nostril shadow. */
    float bridge=expf(-fsq(side)*180.0f-fsq(v-0.53f)*55.0f)*front;
    r+=(int)(bridge*10.0f); g+=(int)(bridge*6.0f); b+=(int)(bridge*4.0f);
    float nost=(expf(-fsq(side-0.06f)*900.0f)+expf(-fsq(side+0.06f)*900.0f))*expf(-fsq(v-0.63f)*1500.0f)*front;
    r-=(int)(nost*56.0f); g-=(int)(nost*42.0f); b-=(int)(nost*38.0f);
    /* Lips. */
    float upper_lip=expf(-fsq(side)*80.0f-fsq(v-0.73f)*1000.0f)*front;
    float lower_lip=expf(-fsq(side)*65.0f-fsq(v-0.775f)*620.0f)*front;
    if(upper_lip>0.02f||lower_lip>0.02f){
      float lip=fminf(1.0f,upper_lip*1.2f+lower_lip);
      r=(int)(r*(1.0f-lip)+lip*145.0f);
      g=(int)(g*(1.0f-lip)+lip*96.0f);
      b=(int)(b*(1.0f-lip)+lip*95.0f);
    }
    /* Moustache shadow and beard stubble dots. */
    float moust=expf(-fsq(side)*70.0f-fsq(v-0.69f)*650.0f)*front;
    r-=(int)(moust*10.0f); g-=(int)(moust*9.0f); b-=(int)(moust*7.0f);
    unsigned int h=(unsigned int)(x*7411u+y*1597u+12345u);
    float beard_zone=front*CLAMP((v-0.67f)/0.23f,0.0f,1.0f)*(1.0f-CLAMP((v-0.95f)/0.06f,0.0f,1.0f));
    if(beard_zone>0.0f && ((h>>2)&31u)==0u && fabsf(side)<0.58f+0.12f*(v-0.67f)){
      r-=(int)(18.0f*beard_zone); g-=(int)(16.0f*beard_zone); b-=(int)(15.0f*beard_zone);
    }
    /* Hairline and scalp stubble. */
    float top_hair=(v<0.27f?1.0f-v/0.27f:0.0f)*(0.45f+0.55f*front);
    if(top_hair>0.0f && ((h>>5)&7u)==0u && fabsf(side)<0.88f){
      r=(int)(r*(1.0f-top_hair*0.62f)+28.0f*(top_hair*0.62f));
      g=(int)(g*(1.0f-top_hair*0.62f)+22.0f*(top_hair*0.62f));
      b=(int)(b*(1.0f-top_hair*0.62f)+20.0f*(top_hair*0.62f));
    }
    /* Soft cheek tint. */
    r+= (int)(cheek*4.0f); g-= (int)(cheek*1.0f);
    r=CLAMP(r,0,255); g=CLAMP(g,0,255); b=CLAMP(b,0,255);
    skin_tex[y*128+x]=(unsigned short)(((b>>3)<<11)|((g>>2)<<5)|(r>>3));
  }
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
  n_head=0;
  for(int a=0;a<HR;a++)for(int p=0;p<HS;p++){
    float a0=(float)a*PI/HR,a1=(float)(a+1)*PI/HR;
    float p0=(float)p*2.0f*PI/HS,p1=(float)(p+1)*2.0f*PI/HS;
    TexturedVertex v0=surface(a0,p0),v1=surface(a1,p0),v2=surface(a0,p1),v3=surface(a1,p1);
    add_tri(v0,v1,v2);add_tri(v2,v1,v3);
  }
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
 for(int i=0;i<HAIRS;i++){
   float a=0.10f+1.38f*sqrtf(rand01());
   float p=2.0f*PI*rand01();
   float cp=cosf(p),sp=sinf(p);
   /* Receding temples and slightly thinner crown, like close-cropped stubble. */
   float density=1.0f;
   if(cp>0.42f && fabsf(sp)>0.62f && a<0.58f)density*=0.18f;
   if(a<0.26f)density*=0.42f;
   if(rand01()>density)continue;
   TexturedVertex v=surface(a,p);
   Hair *h=&hairs[n_hairs++];
   h->x=v.x;h->y=v.y;h->z=v.z;
   h->nx=v.nx;h->ny=v.ny;h->nz=v.nz;
   h->tx=cp;h->ty=0;h->tz=-sp;
   float twist=(rand01()-.5f)*.35f;
   h->tx+=twist;h->tz+=twist*.25f;
   normalize(&h->tx,&h->ty,&h->tz);
   h->length=0.020f+0.020f*rand01();
   h->gone=0;
 }
}

/* Dense facial shadow: tiny low-contrast triangular strands on jaw and cheeks.
   Generate this once, draw with depth testing like the scalp hair. */
static void make_beard_stubble(void){
 n_beard_stubble=0;
 for(int i=0;i<FACE_STUBBLE;i++){
  float a=1.87f+.83f*rand01();
  float p=(rand01()-.5f)*2.25f;
  TexturedVertex v=surface(a,p);
  if(v.y>-.27f && fabsf(v.x)<.19f)continue;
  float side=cosf(p),forward=-sinf(p);
  float off=.013f;
  float x=v.x+v.nx*off,y=v.y+v.ny*off,z=v.z+v.nz*off;
  float w=.0030f+.0025f*rand01();
  float h=.006f+.006f*rand01();
  if(n_beard_stubble+3>FACE_STUBBLE*3)break;
  beard_stubble[n_beard_stubble++]=(PosVertex){x-w*side,y,z-w*forward};
  beard_stubble[n_beard_stubble++]=(PosVertex){x+w*side,y,z+w*forward};
  beard_stubble[n_beard_stubble++]=(PosVertex){x+v.nx*h,y+h*.35f,z+v.nz*h};
 }
}
static void draw_beard_stubble(void){
 sceGuDisable(GU_TEXTURE_2D);sceGuDisable(GU_LIGHTING);
 sceGuColor(C(108,76,65));
 sceGumDrawArray(GU_TRIANGLES,GU_VERTEX_32BITF|GU_TRANSFORM_3D,n_beard_stubble,0,beard_stubble);
 sceGuEnable(GU_LIGHTING);
}

static void hair_vertices(PosVertex *dst,Hair *h,int variant){
  float thickness=(variant==0)?.0060f:.0035f;
  float len=h->length*(variant==0?1.0f:0.72f);
  float lean=(variant==0)?.010f:-.008f;
  float bx=h->x+h->nx*.011f,by=h->y+h->ny*.011f,bz=h->z+h->nz*.011f;
  dst[0]=(PosVertex){bx+h->tx*thickness,by,bz+h->tz*thickness};
  dst[1]=(PosVertex){bx-h->tx*thickness,by,bz-h->tz*thickness};
  dst[2]=(PosVertex){bx+h->nx*len+h->tx*lean,by+h->ny*len+.013f,bz+h->nz*len+h->tz*lean};
}
static void draw_hair(void){
  n_hair_verts=0;
  /* Cache all live hairs in one GPU draw call, 2 triangles per tuft. */
  for(int i=0;i<n_hairs;i++)if(!hairs[i].gone){
    if(n_hair_verts+6>MAX_HAIR_VERTS)break;
    hair_vertices(&hair_mesh[n_hair_verts],&hairs[i],0);n_hair_verts+=3;
    hair_vertices(&hair_mesh[n_hair_verts],&hairs[i],1);n_hair_verts+=3;
  }
  sceGuDisable(GU_TEXTURE_2D);
  sceGuDisable(GU_LIGHTING);
  sceGuColor(C(41,29,25));
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
/* Rasterize the existing 5x7 font as opaque GU_SPRITES rectangles.
   The prior HUD used normalized 0..1 UVs with PSP's pixel-space texture
   coordinates: it sampled transparent texels and drew no visible text.
   This implementation reuses the UI primitive path already proven in PPSSPP.
   Consecutive vertical pixels are merged into a single rectangle. */
static void ui_text(float x,float y,const char *s,float scale){
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
static void draw_ui(void){
  n_hud=0;
  /* Overlay backgrounds and barber-shop graphic accents. */
  ui_box(0,0,SW,43,C(10,16,27));
  ui_box(0,43,SW,2,C(64,184,179));
  ui_box(0,218,SW,54,C(10,16,27));
  ui_box(0,217,SW,1,C(67,136,142));
  ui_box(14,233,110,6,C(43,56,67));
  ui_box(14,233,(110*removed)/(n_hairs?n_hairs:1),6,C(61,222,171));
  ui_box(141,227,1,31,C(46,65,80));
  ui_box(298,227,1,31,C(46,65,80));
  if(phase==0){
    ui_box(82,174,318,30,C(10,16,27));
    ui_text(130,179,"PRESS X TO START",2.1f);
  } else if(phase==2){
    ui_box(87,169,315,38,C(10,16,27));
    ui_text(133,179,"MASTER BARBER!",2.1f);
  } else if(phase==3){
    ui_box(75,169,332,38,C(10,16,27));
    ui_text(133,179,"TIME IS UP!",2.1f);
  }
  if(phase!=1 && frame%90>45){ui_text(170,203,"X TO RESTART",1.35f);}
  /* Cursor and clipper in the gameplay, and a short crosshair. */
  if(phase==1){
    int cx=(int)cursor_x,cy=(int)cursor_y;
    ui_box(cx-9,cy-27,18,28,C(35,45,59));
    ui_box(cx-8,cy-29,16,6,C(189,199,206));
    for(int i=-6;i<=6;i+=4)ui_box(cx+i,cy-33,2,5,C(222,230,229));
    ui_box(cx-5,cy-9,10,8,shave_flash?C(61,228,183):C(42,184,202));
    ui_box(cx-1,cy+5,2,9,C(255,221,135));
  }
  ui_text(13,9,"BALD BOSS",2.2f);
  ui_text(198,14,"PSP EDITION",1.15f);
  ui_text(389,13,music_enabled?"MUSIC ON":"MUSIC OFF",1.0f);
  char buf[32];
  ui_text(13,224,"SHAVED",1.15f);
  snprintf(buf,sizeof(buf),"%d%%",n_hairs?removed*100/n_hairs:0);
  ui_text(14,243,buf,1.85f);
  ui_text(155,224,"SCORE",1.15f);
  snprintf(buf,sizeof(buf),"%d",score);
  ui_text(155,243,buf,1.85f);
  ui_text(311,224,"TIME",1.15f);
  int seconds=90-play_frames/60;if(seconds<0)seconds=0;
  snprintf(buf,sizeof(buf),"%02d:%02d",seconds/60,seconds%60);
  ui_text(312,243,buf,1.85f);
  if(phase==0){ui_text(172,56,"THE FINAL HAIRCUT",1.2f);
    ui_text(162,209,"TRI SHADES  SELECT MUSIC",.9f);}
  /* HUD geometry is CPU-generated and must be visible to the GE. */
  sceKernelDcacheWritebackAll();
  sceGuDisable(GU_LIGHTING);sceGuDisable(GU_DEPTH_TEST);sceGuDisable(GU_TEXTURE_2D);sceGuDisable(GU_BLEND);
  sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,n_hud,0,hud);
  /* All HUD rectangles and letters are now drawn by the call above. */
}


static void render_head(void){
  /* Perspective projection, real depth test, smooth per-vertex normals. */
  sceGumMatrixMode(GU_PROJECTION);sceGumLoadIdentity();sceGumPerspective(47.0f,(float)SW/SH,0.1f,20.0f);
  sceGumMatrixMode(GU_VIEW);sceGumLoadIdentity();
  sceGumMatrixMode(GU_MODEL);sceGumLoadIdentity();
  ScePspFVector3 pos={0,CAM_Y,-CAM_DISTANCE};sceGumTranslate(&pos);
  ScePspFVector3 rot={0,yaw,0};sceGumRotateXYZ(&rot);
  sceGuEnable(GU_DEPTH_TEST);sceGuDepthMask(GU_FALSE);
  sceGuEnable(GU_LIGHTING);sceGuEnable(GU_LIGHT0);
  ScePspFVector3 sun={-0.40f,0.55f,0.74f};
  sceGuLight(0,GU_DIRECTIONAL,GU_DIFFUSE,&sun);
  sceGuLightColor(0,GU_DIFFUSE,0x00efe6dc);
  sceGuAmbient(0x0033404c);
  sceGuMaterial(GU_AMBIENT,0xff9b755f);
  sceGuMaterial(GU_DIFFUSE,0xfff1c8a8);
  sceGuAmbientColor(0xffffffff);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuTexMode(GU_PSM_5650,0,0,0);
  sceGuTexImage(0,128,128,128,skin_tex);
  sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
  sceGuTexFilter(GU_LINEAR,GU_LINEAR);
  sceGuColor(0xffffffff);
  sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_3D,n_head,0,head);
  draw_beard_stubble();
  /* Ears, eye globes, pupils and eyelids stay as light geometry accents only. */
  for(int side=-1;side<=1;side+=2){
    float sign=(float)side;
    draw_ellipsoid(sign*.80f,-.07f,-.02f,.17f,.24f,.12f,C(204,151,126),10,14);
    draw_ellipsoid(sign*.85f,-.055f,.05f,.070f,.14f,.040f,C(138,92,79),8,12);
    draw_ellipsoid(sign*.34f,.11f,.84f,.160f,.076f,.042f,C(235,228,210),8,14);
    draw_ellipsoid(sign*.34f,.11f,.877f,.055f,.055f,.020f,C(84,117,108),8,12);
    draw_ellipsoid(sign*.34f,.11f,.897f,.025f,.038f,.011f,C(20,24,28),7,10);
    draw_ellipsoid(sign*.34f,.18f,.862f,.16f,.020f,.026f,C(182,125,104),5,12);
    draw_ellipsoid(sign*.34f,.045f,.862f,.16f,.020f,.026f,C(173,120,101),5,12);
  }
  /* Optional sunglasses stay available but default off. */
  if(shades_on){
    draw_ellipsoid(-.375f,.140f,.918f,.332f,.169f,.043f,C(17,29,39),10,18);
    draw_ellipsoid(.375f,.140f,.918f,.332f,.169f,.043f,C(17,29,39),10,18);
    draw_ellipsoid(-.375f,.280f,.906f,.335f,.030f,.034f,C(33,29,28),7,18);
    draw_ellipsoid(.375f,.280f,.906f,.335f,.030f,.034f,C(33,29,28),7,18);
    draw_ellipsoid(0.0f,.195f,.970f,.148f,.027f,.039f,C(178,140,80),6,14);
    draw_ellipsoid(-.392f,.139f,.962f,.275f,.013f,.014f,C(205,174,95),5,14);
    draw_ellipsoid(.392f,.139f,.962f,.275f,.013f,.014f,C(205,174,95),5,14);
  }
  draw_ellipsoid(0.0f,-1.13f,-.11f,.34f,.42f,.30f,C(186,126,103),10,14);
  draw_ellipsoid(0.0f,-1.72f,-.28f,1.12f,.56f,.52f,C(31,47,68),9,18);
  draw_hair();
}

static void reset_game(void){
  for(int i=0;i<n_hairs;i++)hairs[i].gone=0;
  removed=0;score=0;play_frames=0;cursor_x=240;cursor_y=130;
  yaw=0;shave_flash=0;phase=1;
}
static void shave_at_cursor(void){
  float s=sinf(yaw),c=cosf(yaw);
  int hit=0;
  for(int i=0;i<n_hairs;i++){
    Hair *h=&hairs[i];if(h->gone)continue;
    float zz=h->z*c-h->x*s;
    float xx=h->x*c+h->z*s;
    float face=h->nz*c-h->nx*s;
    if(face<0.13f)continue; /* Never shave hidden back-side hair. */
    /* Camera and hit-testing share exactly the same perspective model. */
    float inv=1.0f/(CAM_DISTANCE-zz);
    float sx=240.0f+FOCAL_PIXELS*xx*inv;
    float sy=136.0f-FOCAL_PIXELS*(h->y+CAM_Y)*inv;
    if(fsq((sx-cursor_x)/17.0f)+fsq((sy-cursor_y)/20.0f)<1.0f){
      h->gone=1;removed++;score+=20;hit++;
    }
  }
  if(hit)shave_flash=7;
  if(removed>=n_hairs && n_hairs>0)phase=2;
}
static void update(void){
 SceCtrlData pad;
 sceCtrlPeekBufferPositive(&pad,1);
 unsigned int b=pad.Buttons, just=b&~prev_buttons; prev_buttons=b;
 /* These toggles work in menus and while playing. */
 if(just&PSP_CTRL_TRIANGLE)shades_on=!shades_on;
 if(just&PSP_CTRL_SELECT)music_enabled=!music_enabled;
 if(phase!=1){
   if(just&PSP_CTRL_CROSS)reset_game();
   else if(phase==0)yaw+=.007f;
   return;
 }
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
 if(b&(PSP_CTRL_CROSS|PSP_CTRL_SQUARE))shave_at_cursor();
 if(shave_flash)shave_flash--;
 play_frames++;
 if(play_frames>=90*60)phase=3;
}

/* Original generative chiptune: no audio files, no licensed samples.
   Three gentle synthesizer voices in a dedicated PSP audio thread.
   44100 Hz, 16-bit signed interleaved stereo PCM, continuously streamed. */
static const unsigned char melody_notes[32]={
  72,76,79,76,74,77,81,77, 76,79,84,79,77,74,72,0,
  74,77,81,77,76,79,83,79, 72,76,79,76,74,71,72,0
};
static const unsigned char bass_notes[8]={48,48,53,53,45,50,48,48};
static float midi_hz(int midi){return 440.0f*powf(2.0f,(midi-69)/12.0f);}
static float triangle_wave(float phase){return 1.0f-4.0f*fabsf(phase-0.5f);}
static int audio_thread(SceSize args,void *argp){
 (void)args;(void)argp;
 int ch=sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,AUDIO_SAMPLES,PSP_AUDIO_FORMAT_STEREO);
 if(ch<0)return 0;
 audio_channel=ch;
 float melody_phase=0.0f,bass_phase=0.0f,chord_phase=0.0f;
 int step=0,within_step=0,bufno=0;
 float mel_hz=midi_hz(72),bass_hz=midi_hz(48),chord_hz=midi_hz(60);
 unsigned int noise=0x159e1u;
 while(1){
  short *buf=audio_pcm[bufno];
  for(int i=0;i<AUDIO_SAMPLES;i++){
   if(within_step==0){
     mel_hz=melody_notes[step]?midi_hz(melody_notes[step]):0.0f;
     bass_hz=midi_hz(bass_notes[(step/4)%8]);
     chord_hz=midi_hz((step/8)%2?67:64);
   }
   float env=1.0f-(float)within_step/(float)MUSIC_STEP_SAMPLES;
   float melody=mel_hz>0.0f?triangle_wave(melody_phase)*env*.32f:0.0f;
   float bass=triangle_wave(bass_phase)*.16f;
   float chord=triangle_wave(chord_phase)*.065f;
   noise=noise*1664525u+1013904223u;
   float beat=(within_step<1350 && (step%2)==0)
      ?(1.0f-(float)within_step/1350.0f)*((noise&65535)/32768.0f-1.0f)*.070f:0.0f;
   float s=CLAMP(melody+bass+chord+beat,-.87f,.87f);
   short pcm=music_enabled?(short)(s*14500.0f):0;
   buf[i*2]=pcm;buf[i*2+1]=pcm;
   melody_phase+=mel_hz/AUDIO_RATE; if(melody_phase>=1.0f)melody_phase-=1.0f;
   bass_phase+=bass_hz/AUDIO_RATE; if(bass_phase>=1.0f)bass_phase-=1.0f;
   chord_phase+=chord_hz/AUDIO_RATE; if(chord_phase>=1.0f)chord_phase-=1.0f;
   within_step++;
   if(within_step>=MUSIC_STEP_SAMPLES){within_step=0;step=(step+1)%32;}
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
