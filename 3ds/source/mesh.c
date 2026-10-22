/* Bald Elite - native 3DS head-and-hair geometry, portable C99.
 * Original 3D profile transcribed from src/bald_head_v2.h (PSP edition).
 * This file deliberately uses no PSP or Nintendo SDK headers so we can test
 * the exact geometry generation with a host compiler.
 */
#include "mesh.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define PI 3.14159265358979323846f
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

const char *be_candidate_names[BE_CANDIDATES]={
    "THE BUSINESSMAN","THE PUNK","THE PROFESSOR"
};

static uint32_t seed;
static uint32_t rand32(void){seed=seed*1664525u+1013904223u;return seed;}
static float rnd(void){return (float)((rand32()>>8)&65535u)/65535.0f;}
static float gauss(float x){return expf(-x*x);}


/* Original PSP face artist's 256x256 RGB565 atlas (bald_head_v2.h).
 * It is sampled into extra-fine coloured vertices, so the PICA200 can render
 * the same painted eyes/irises/lips without needing a texture shader.
 * The bald-patch coverage matches the PSP's alpha polish calculation. */
static int clampi(int n,int a,int b){return n<a?a:(n>b?b:n);}
static float clampf(float n,float a,float b){return n<a?a:(n>b?b:n);}
static unsigned short rgb565(int r,int g,int b){
    r=clampi(r,0,255);g=clampi(g,0,255);b=clampi(b,0,255);
    return (unsigned short)(((b>>3)<<11)|((g>>2)<<5)|(r>>3));
}
static void skin_paint(BeMesh *m,float cx,float cy,float rx,float ry,
                       int red,int green,int blue,float opacity){
    int l=clampi((int)(cx-rx-2),0,255),r=clampi((int)(cx+rx+2),0,255);
    int t=clampi((int)(cy-ry-2),0,255),b=clampi((int)(cy+ry+2),0,255);
    for(int y=t;y<=b;y++)for(int x=l;x<=r;x++){
        float xx=(x-cx)/rx,yy=(y-cy)/ry,d=xx*xx+yy*yy;
        if(d>1.0f)continue;
        float alpha=opacity*(1.0f-d*.30f);
        unsigned short c=m->skin_base[y*256+x];
        int cr=(c&31)*255/31,cg=((c>>5)&63)*255/63,cb=((c>>11)&31)*255/31;
        m->skin_base[y*256+x]=rgb565((int)(cr*(1-alpha)+red*alpha),
            (int)(cg*(1-alpha)+green*alpha),
            (int)(cb*(1-alpha)+blue*alpha));
    }
}
static void skin_generate(BeMesh *m){
    memset(m->shaved_coverage,0,sizeof(m->shaved_coverage));
    for(int y=0;y<256;y++)for(int x=0;x<256;x++){
        float u=(x-128.0f)/128.0f,v=(float)y/255.0f;
        float frontal=1.0f-clampf(fabsf(u)*1.1f,0.0f,1.0f);
        int grain=((x*37+y*53+(x*y)%17)%11)-5;
        int r=(int)(219.0f-24.0f*fabsf(u)+10.0f*gauss((v-.36f)/.20f)*frontal)+grain/2;
        int g=(int)(178.0f-16.0f*fabsf(u)+6.0f*gauss((v-.33f)/.23f)*frontal)+grain/2;
        int b=(int)(147.0f-11.0f*fabsf(u))+grain/2;
        m->skin_base[y*256+x]=rgb565(r,g,b);
    }
    skin_paint(m,100,156,16,30,174,119,104,.22f);
    skin_paint(m,156,156,16,30,174,119,104,.22f);
    skin_paint(m,107,137,23,16,225,155,145,.12f);
    skin_paint(m,149,137,23,16,225,155,145,.12f);
    for(int side=-1;side<=1;side+=2){
        float cx=128.0f+side*16.0f;
        skin_paint(m,cx,109,14,8,143,101,89,.27f);
        skin_paint(m,cx,115,11,5,242,235,217,.98f);
        skin_paint(m,cx,116,4.5f,5.0f,98,154,164,.96f);
        skin_paint(m,cx,116,2.25f,3.45f,22,32,38,1.0f);
        skin_paint(m,cx-1.2f,114,1.2f,1.1f,248,247,242,1.0f);
        skin_paint(m,cx,109.5f,13,2,94,63,54,.65f);
        skin_paint(m,cx,122,11,1.2f,141,104,89,.30f);
        skin_paint(m,cx,99,13,2.6f,68,47,39,.83f);
    }
    skin_paint(m,128,141,4,23,244,210,182,.20f);
    skin_paint(m,122,159,3,2.1f,101,62,55,.62f);
    skin_paint(m,134,159,3,2.1f,101,62,55,.62f);
    skin_paint(m,128,161,11,3,221,168,138,.18f);
    skin_paint(m,128,192,16,3.4f,153,93,86,.56f);
    skin_paint(m,128,197,15.5f,4.2f,178,107,98,.61f);
    skin_paint(m,128,194,14,1,90,58,55,.46f);
    skin_paint(m,128,220,11,6,155,112,95,.10f);
    for(int y=176;y<240;y+=3)for(int x=84;x<173;x+=4){
        if((x*31+y*17)%9!=0)continue;
        float dx=(x-128.0f)/49.0f,dy=(y-204.0f)/43.0f;
        if(dx*dx+dy*dy<1.0f && !(y>188&&y<204&&abs(x-128)<20))
            skin_paint(m,(float)x,(float)y,1,1,102,83,71,.12f);
    }
}
static void pixel_sample(const BeMesh *m,int x,int y,float out[4]){
    int index=clampi(y,0,255)*256+clampi(x,0,255);
    unsigned short c=m->skin_base[index];
    out[0]=(float)(c&31)*(255.0f/31.0f);
    out[1]=(float)((c>>5)&63)*(255.0f/63.0f);
    out[2]=(float)((c>>11)&31)*(255.0f/31.0f);
    out[3]=(float)m->shaved_coverage[index]/255.0f;
}
void be_mesh_recolor_head(BeMesh *m){
    for(int i=0;i<m->head_count;i++){
        BeVertex *v=&m->head[i];
        float u=clampf(v->u*255.0f,0.0f,255.0f);
        float w=clampf(v->v*255.0f,0.0f,255.0f);
        int x=(int)u,y=(int)w;
        float fx=u-x,fy=w-y;
        float p00[4],p01[4],p10[4],p11[4],col[4];
        pixel_sample(m,x,y,p00);pixel_sample(m,x+1,y,p10);
        pixel_sample(m,x,y+1,p01);pixel_sample(m,x+1,y+1,p11);
        for(int ch=0;ch<4;ch++){
            col[ch]=(p00[ch]*(1-fx)+p10[ch]*fx)*(1-fy)
                   +(p01[ch]*(1-fx)+p11[ch]*fx)*fy;
        }
        float forward=MAX(0.0f,cosf((v->u-.5f)*2.0f*PI));
        float shade=.79f+.20f*forward+.025f*(1.0f-v->v);
        float polished=.30f*col[3];
        float gloss=0.0f;
        if(v->v<.36f && col[3]>.01f){
            float lx=(v->u-.42f)*5.0f,ly=(v->v-.14f)*5.0f;
            float hotspot=expf(-(lx*lx+ly*ly)*2.8f);
            gloss=.16f*hotspot*col[3];
        }
        v->r=clampf(((col[0]*(1-polished)+248.0f*polished)/255.0f)*shade+gloss,0,1);
        v->g=clampf(((col[1]*(1-polished)+214.0f*polished)/255.0f)*shade+gloss,0,1);
        v->b=clampf(((col[2]*(1-polished)+185.0f*polished)/255.0f)*shade+gloss,0,1);
        v->a=1.0f;
    }
}
void be_mesh_polish_hair(BeMesh *m,const BeHair *h){
    int cx=clampi((int)(h->u*255.0f+.5f),0,255);
    int cy=clampi((int)(h->v*255.0f+.5f),0,255);
    for(int y=clampi(cy-9,0,255);y<=clampi(cy+9,0,255);y++)
    for(int x=clampi(cx-10,0,255);x<=clampi(cx+10,0,255);x++){
        float dx=(x-cx)/10.0f,dy=(y-cy)/9.0f,d=dx*dx+dy*dy;
        if(d>=1.0f)continue;
        int idx=y*256+x,ink=(int)(43.0f*(1.0f-d)*(1.0f-d))+4;
        m->shaved_coverage[idx]=(unsigned char)
            clampi(m->shaved_coverage[idx]+ink,0,255);
    }
}

static float linear(float y,const float *values){
    static const float heights[9]={1.14f,.98f,.75f,.45f,.18f,-.18f,-.48f,-.79f,-1.02f};
    if(y>=heights[0])return values[0];
    for(int i=0;i<8;i++)if(y>=heights[i+1]){
        float t=(heights[i]-y)/(heights[i]-heights[i+1]);
        t=t*t*(3.0f-2.0f*t);
        return values[i]*(1.0f-t)+values[i+1]*t;
    }
    return values[8];
}

/* Match the existing PSP's specific nose, brow, eye-socket, chin and ears. */
static void head_point(float y,float angle,float *x,float *outy,float *z){
    static const float widths[9]={.025f,.44f,.60f,.66f,.65f,.63f,.59f,.43f,.12f};
    static const float depths[9]={.035f,.40f,.62f,.66f,.67f,.65f,.63f,.50f,.20f};
    static const float centerz[9]={-.045f,-.05f,-.06f,-.05f,-.04f,-.02f,.02f,.12f,.27f};
    float sx=sinf(angle),cz=cosf(angle),front=cz>0?cz:0;
    float rx=linear(y,widths),rz=linear(y,depths);
    *x=rx*sx;*outy=y;*z=linear(y,centerz)+rz*cz;
    float bridge=gauss(angle/.20f)*gauss((y+.04f)/.29f);
    float tip=gauss(angle/.16f)*gauss((y+.25f)/.12f);
    float wing=(gauss((angle-.16f)/.085f)+gauss((angle+.16f)/.085f))*gauss((y+.31f)/.09f);
    *z+=.078f*bridge+.168f*tip+.035f*wing;
    float eyes=gauss((angle-.40f)/.18f)+gauss((angle+.40f)/.18f);
    *z-=.043f*eyes*gauss((y-.25f)/.125f);
    *z+=.019f*eyes*gauss((y-.38f)/.09f);
    *z+=.026f*eyes*gauss((y+.08f)/.20f);
    float mid=gauss(angle/.34f);
    *z+=mid*(.022f*gauss((y+.53f)/.050f)
              +.034f*gauss((y+.62f)/.060f)
              +.035f*gauss((y+.80f)/.125f));
    float ear=(gauss((angle-1.58f)/.22f)+gauss((angle+1.58f)/.22f))*gauss((y+.11f)/.18f);
    *x+=sx*.095f*ear;*z+=front*.012f*ear;
}

static BeVertex head_vertex(float y,float a){
    BeVertex v;
    head_point(y,a,&v.x,&v.y,&v.z);
    v.u=(a/(2.0f*PI))+.5f;
    v.v=(1.14f-y)/2.16f;
    v.r=v.g=v.b=v.a=1.0f;
    return v;
}

void be_mesh_rebuild_hair(BeMesh *m){
    int offset=0;
    const float color[BE_CANDIDATES][3]={
        {.24f,.16f,.12f}, {.86f,.20f,.45f}, {.76f,.76f,.70f}
    };
    for(int i=0;i<m->hair_count;i++){
        const BeHair *h=&m->strands[i];
        float x=h->x+h->nx*.010f, y=h->y+h->ny*.010f,
              z=h->z+h->nz*.010f;
        float len=h->length;
        float tx=h->nz,tz=-h->nx;
        float width=m->candidate==1?.010f:.007f;
        float tipx=x+h->nx*len,tipy=y+len*.70f,tipz=z+h->nz*len;
        if(h->gone){tipx=x;tipy=y;tipz=z;width=0.0f;}
        BeVertex a={x-tx*width,y,z-tz*width,color[m->candidate][0],color[m->candidate][1],color[m->candidate][2],1,0,0};
        BeVertex b={x+tx*width,y,z+tz*width,color[m->candidate][0],color[m->candidate][1],color[m->candidate][2],1};
        BeVertex c={tipx,tipy,tipz,MIN(1.0f,color[m->candidate][0]*1.12f),
                    MIN(1.0f,color[m->candidate][1]*1.12f),
                    MIN(1.0f,color[m->candidate][2]*1.12f),1,0,0};
        m->hair[offset++]=a;m->hair[offset++]=b;m->hair[offset++]=c;
        m->hair[offset++]=c;m->hair[offset++]=b;m->hair[offset++]=a;
    }
}

/* This is an early rendering/selection milestone; no collision-based clipping
   or score/time logic is silently approximated here. */
void be_mesh_generate(BeMesh *m,int candidate){
    if(candidate<0)candidate=0;
    m->candidate=candidate%BE_CANDIDATES;
    m->head_count=0;m->hair_count=0;m->removed=0;
    skin_generate(m);
    for(int i=0;i<BE_RINGS;i++){
        float ya=1.14f-2.16f*i/BE_RINGS,yb=1.14f-2.16f*(i+1)/BE_RINGS;
        for(int j=0;j<BE_SLICES;j++){
            float aa=((float)j/BE_SLICES-.5f)*2.0f*PI;
            float ab=((float)(j+1)/BE_SLICES-.5f)*2.0f*PI;
            BeVertex a=head_vertex(ya,aa),b=head_vertex(ya,ab);
            BeVertex c=head_vertex(yb,aa),d=head_vertex(yb,ab);
            m->head[m->head_count++]=a;
            m->head[m->head_count++]=c;
            m->head[m->head_count++]=b;
            m->head[m->head_count++]=b;
            m->head[m->head_count++]=c;
            m->head[m->head_count++]=d;
        }
    }
    for(int j=0;j<BE_SLICES;j++){
        float aa=((float)j/BE_SLICES-.5f)*2.0f*PI;
        float ab=((float)(j+1)/BE_SLICES-.5f)*2.0f*PI;
        BeVertex a=head_vertex(1.14f,aa),b=head_vertex(1.14f,ab);
        BeVertex center=head_vertex(1.17f,(aa+ab)*.5f);
        center.x=0;center.y=1.17f;center.z=-.045f;
        m->head[m->head_count++]=center;
        m->head[m->head_count++]=a;
        m->head[m->head_count++]=b;
    }
    seed=0xB01D2026u+0x9e3779b9u*(uint32_t)m->candidate;
    const int goal=m->candidate==0?900:(m->candidate==1?BE_MAX_HAIR:760);
    for(int tries=0;tries<BE_MAX_HAIR*70 && m->hair_count<goal;tries++){
        float y=.34f+.79f*rnd();
        float a=(rnd()-.5f)*2.0f*PI;
        float x,z,ny;
        head_point(y,a,&x,&ny,&z);
        if(m->candidate==1 && fabsf(x)>.28f)continue;
        if(m->candidate==2){
            if(y>.86f&&fabsf(x)<.58f)continue;
            if(z>.27f&&y>.57f&&fabsf(x)<.52f)continue;
            if(rnd()<.24f)continue;
        }
        float nx=sinf(a),nz=cosf(a),n_y=MAX(.10f,(y-.2f)*.40f);
        float inv=1.0f/sqrtf(nx*nx+n_y*n_y+nz*nz);
        BeHair *h=&m->strands[m->hair_count++];
        h->x=x;h->y=y;h->z=z;
        h->nx=nx*inv;h->ny=n_y*inv;h->nz=nz*inv;
        h->u=(a/(2.0f*PI))+.5f;
        h->v=(1.14f-y)/2.16f;
        h->length=m->candidate==0?(.026f+.020f*rnd()):
                  (m->candidate==1?(.115f+.075f*rnd()):(.032f+.035f*rnd()));
        h->gone=0;
    }
    be_mesh_rebuild_hair(m);
    be_mesh_recolor_head(m);
}
