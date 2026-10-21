/* Bald Elite - native 3DS head-and-hair geometry, portable C99.
 * Original 3D profile transcribed from src/bald_head_v2.h (PSP edition).
 * This file deliberately uses no PSP or Nintendo SDK headers so we can test
 * the exact geometry generation with a host compiler.
 */
#include "mesh.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

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
    /* Temporary vertex painting: the full PSP texture is a later milestone.
       Keep visible eyes, eyebrows and lips to distinguish the real face mesh. */
    float frontal=MAX(0.0f,cosf(a));
    float shade=.75f+.23f*frontal+.045f*(y+.5f);
    float eyes=(gauss((a-.40f)/.105f)+gauss((a+.40f)/.105f));
    float eye=eyes*gauss((y-.27f)/.055f);
    float eyebrow=eyes*gauss((y-.39f)/.035f);
    float mouth=gauss(a/.31f)*gauss((y+.59f)/.037f);
    float iris=eyes*gauss((y-.272f)/.017f);
    float tint=MAX(0.22f,1.0f-.62f*eye-.35f*eyebrow-.38f*mouth-.48f*iris);
    v.r=MIN(1.0f,.86f*shade*tint);
    v.g=MIN(1.0f,.69f*shade*tint);
    v.b=MIN(1.0f,.57f*shade*tint);
    v.a=1.0f;
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
        BeVertex a={x-tx*width,y,z-tz*width,color[m->candidate][0],color[m->candidate][1],color[m->candidate][2],1};
        BeVertex b={x+tx*width,y,z+tz*width,color[m->candidate][0],color[m->candidate][1],color[m->candidate][2],1};
        BeVertex c={tipx,tipy,tipz,MIN(1.0f,color[m->candidate][0]*1.12f),
                    MIN(1.0f,color[m->candidate][1]*1.12f),
                    MIN(1.0f,color[m->candidate][2]*1.12f),1};
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
        h->length=m->candidate==0?(.026f+.020f*rnd()):
                  (m->candidate==1?(.115f+.075f*rnd()):(.032f+.035f*rnd()));
        h->gone=0;
    }
    be_mesh_rebuild_hair(m);
}
