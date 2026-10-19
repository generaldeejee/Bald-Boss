/* Export the EXACT procedural mesh/UV coordinates and face texture used in PSP.
   Host command (run from repository root):
   gcc -O2 -std=c99 -Isrc tools/export_bald_head.c -lm -o export_head
   ./export_head Bald_Boss_Head.obj Bald_Boss_Face.ppm
   Use any image editor to convert the PPM texture to PNG. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#define PI 3.14159265358979323846f
#define CLAMP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define MAX_HEAD_VERTS 11000
static float fsq(float a){return a*a;}
static void normalize(float *x,float *y,float *z){
 float l=sqrtf(fsq(*x)+fsq(*y)+fsq(*z));
 if(l>.000001f){*x/=l;*y/=l;*z/=l;}
}
typedef struct {float u,v,nx,ny,nz,x,y,z;} TexturedVertex;
static TexturedVertex head[MAX_HEAD_VERTS];
static int n_head=0;
static unsigned short skin_tex[256*256];
#include "../src/bald_head_v2.h"
int main(int argc,char **argv){
 const char *obj=argc>1?argv[1]:"Bald_Boss_Head.obj";
 const char *ppm=argc>2?argv[2]:"Bald_Boss_Face.ppm";
 bb_build_head();bb_make_skin();
 FILE *f=fopen(obj,"w");if(!f)return 2;
 fprintf(f,"# Original Bald Boss CC0 head mesh\n");
 fprintf(f,"o BaldBoss_Head_v2\n");
 for(int i=0;i<n_head;i++)
   fprintf(f,"v %.6f %.6f %.6f\n",head[i].x,head[i].y,head[i].z);
 for(int i=0;i<n_head;i++)
   fprintf(f,"vt %.6f %.6f\n",head[i].u/127.0f,1.0f-head[i].v/127.0f);
 for(int i=0;i<n_head;i++)
   fprintf(f,"vn %.6f %.6f %.6f\n",head[i].nx,head[i].ny,head[i].nz);
 for(int i=0;i<n_head;i+=3)
   fprintf(f,"f %d/%d/%d %d/%d/%d %d/%d/%d\n",
     i+1,i+1,i+1,i+2,i+2,i+2,i+3,i+3,i+3);
 fclose(f);
 f=fopen(ppm,"wb");if(!f)return 3;
 fprintf(f,"P6\n256 256\n255\n");
 for(int i=0;i<256*256;i++){
   unsigned short c=skin_tex[i];
   unsigned char p[3]={(c&31)*255/31,((c>>5)&63)*255/63,((c>>11)&31)*255/31};
   fwrite(p,1,3,f);
 }
 fclose(f);
 printf("%d triangles, 256x256 face atlas\n",n_head/3);
 return n_head/3==3120?0:4;
}
