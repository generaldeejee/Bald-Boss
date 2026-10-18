/* Bald Boss 3D - PSP Edition v0.2 (HUD fix). Original homebrew source.
   Uses PSPSDK GU/GUM hardware 3D renderer. No PS1 BIOS calls. */
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
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
#define HR 28
#define HS 44
#define HAIRS 1250
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

static void make_textures(void){
  int x,y;
  for(y=0;y<128;y++)for(x=0;x<128;x++){
    int grain=(int)(random_u32()%5)-2;
    int cheek=(x>37&&x<90&&y>55&&y<85)?1:0;
    int r=CLAMP(12+grain/2+cheek,0,15),g=CLAMP(9+grain/2,0,15),b=CLAMP(7+grain/3,0,15);
    skin_tex[y*128+x]=(unsigned short)(0xf000|(b<<8)|(g<<4)|r);
  }

}

static void add_tri(TexturedVertex a,TexturedVertex b,TexturedVertex c){if(n_head+3>MAX_HEAD_VERTS)return;head[n_head++]=a;head[n_head++]=b;head[n_head++]=c;}
static TexturedVertex surface(float a,float p){
  TexturedVertex v;
  float st=sinf(a),ct=cosf(a),sp=sinf(p),cp=cosf(p);
  float jaw=1.0f-0.22f*CLAMP((-ct-.23f),0,1);
  float cheek=1.0f+0.045f*expf(-fsq(a-1.79f)*20.0f)*CLAMP(cp,0,1);
  v.x=0.84f*st*sp*jaw*cheek;
  v.y=1.05f*ct+0.04f*st*cp;
  v.z=0.87f*st*cp;
  /* Flatten the facial plane slightly and allow cheekbone definition. */
  if(cp>0.55f&&a>1.1f&&a<2.4f)v.z-=0.035f*(cp-.55f)*2.0f;
  v.nx=v.x/(0.84f*0.84f);v.ny=v.y/(1.05f*1.05f);v.nz=v.z/(0.87f*0.87f);
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
   /* Uniform spherical dome with gently receding temples and patchy crown. */
   float a=0.13f+1.32f*sqrtf(rand01());
   float p=2.0f*PI*rand01();
   float cp=cosf(p),sp=sinf(p);
   float density=(cp>0.48f && a>1.05f)?0.28f:1.0f;
   if(rand01()>density)continue;
   TexturedVertex v=surface(a,p);
   Hair *h=&hairs[n_hairs++];
   h->x=v.x;h->y=v.y;h->z=v.z;
   h->nx=v.nx;h->ny=v.ny;h->nz=v.nz;
   /* Tangent to the scalp so hairs can lean instead of pointing vertically. */
   h->tx=cp;h->ty=0;h->tz=-sp;
   float twist=(rand01()-.5f)*.52f;
   h->tx+=twist;h->tz+=twist*.3f;
   normalize(&h->tx,&h->ty,&h->tz);
   h->length=0.040f+0.041f*rand01();
   h->gone=0;
 }
}

static void hair_vertices(PosVertex *dst,Hair *h,int variant){
  float thickness=(variant==0)?.0085f:.0050f;
  float len=h->length*(variant==0?1.0f:0.76f);
  float lean=(variant==0)?.017f:-.014f;
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
  ui_text(388,13,"360 DEG",1.3f);
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
  if(phase==0)ui_text(170,58,"THE FINAL HAIRCUT",1.2f);
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
  ScePspFVector3 pos={0,-0.03f,-3.55f};sceGumTranslate(&pos);
  ScePspFVector3 rot={0,yaw,0};sceGumRotateXYZ(&rot);
  sceGuEnable(GU_DEPTH_TEST);sceGuDepthMask(GU_FALSE);
  sceGuEnable(GU_LIGHTING);sceGuEnable(GU_LIGHT0);
  ScePspFVector3 sun={-0.45f,0.55f,0.76f};
  sceGuLight(0,GU_DIRECTIONAL,GU_DIFFUSE,&sun);
  sceGuLightColor(0,GU_DIFFUSE,0x00ddd6c9);
  sceGuAmbient(0x003d4c5b);
  sceGuMaterial(GU_AMBIENT,0xffad856f);
  sceGuMaterial(GU_DIFFUSE,0xfff2c09b);
  sceGuAmbientColor(0xffffffff);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuTexMode(GU_PSM_4444,0,0,0);
  sceGuTexImage(0,128,128,128,skin_tex);
  sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
  sceGuTexFilter(GU_LINEAR,GU_LINEAR);
  sceGuColor(0xffffffff);
  sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_3D,n_head,0,head);
  /* Anatomical face elements and rounded ears. */
  draw_ellipsoid(-0.83f,-0.07f,0.0f,.19f,.28f,.11f,C(209,143,115),10,12);
  draw_ellipsoid(0.83f,-0.07f,0.0f,.19f,.28f,.11f,C(209,143,115),10,12);
  draw_ellipsoid(-0.89f,-0.07f,.077f,.075f,.175f,.034f,C(141,78,64),7,10);
  draw_ellipsoid(0.89f,-0.07f,.077f,.075f,.175f,.034f,C(141,78,64),7,10);
  draw_ellipsoid(0.0f,-0.08f,.91f,.105f,.26f,.16f,C(214,154,124),10,14);
  draw_ellipsoid(0.0f,-0.23f,1.03f,.16f,.075f,.10f,C(208,137,111),8,12);
  /* Strong nose bridge, brow and sunglasses, all true 3D objects. */
  draw_ellipsoid(-.38f,.22f,.74f,.35f,.075f,.12f,C(48,34,32),8,15);
  draw_ellipsoid(.38f,.22f,.74f,.35f,.075f,.12f,C(48,34,32),8,15);
  draw_ellipsoid(-.39f,.11f,.835f,.32f,.18f,.06f,C(11,29,39),8,16);
  draw_ellipsoid(.39f,.11f,.835f,.32f,.18f,.06f,C(11,29,39),8,16);
  draw_ellipsoid(0.0f,.16f,.95f,.14f,.035f,.055f,C(187,142,81),5,12);
  draw_ellipsoid(-.38f,.11f,.89f,.30f,.025f,.025f,C(208,176,94),5,12);
  draw_ellipsoid(.38f,.11f,.89f,.30f,.025f,.025f,C(208,176,94),5,12);
  /* Separate lower-lip shadows and goatee instead of holes through the mesh. */
  draw_ellipsoid(0.0f,-.53f,.78f,.24f,.041f,.04f,C(109,64,63),7,14);
  draw_ellipsoid(0.0f,-.605f,.765f,.18f,.045f,.037f,C(185,119,112),6,14);
  draw_ellipsoid(0.0f,-.75f,.70f,.31f,.13f,.025f,C(80,55,51),7,16);
  /* Neck and a broad shirt make the head appear proportionate. */
  draw_ellipsoid(0.0f,-1.18f,-.10f,.37f,.45f,.32f,C(188,123,100),10,14);
  draw_ellipsoid(0.0f,-1.77f,-.30f,1.15f,.58f,.54f,C(31,47,68),9,18);
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
    float inv=1.0f/(3.55f-zz);
    float sx=240.0f+303.0f*xx*inv;
    float sy=136.0f-303.0f*(h->y-.03f)*inv;
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
 make_textures();make_head();make_hairs();
 sceKernelDcacheWritebackAll();
 init_graphics();
 while(1){update();draw_frame();frame++;}
 sceGuTerm();sceKernelExitGame();return 0;
}
