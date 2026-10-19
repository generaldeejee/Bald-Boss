/* Bald Boss original male low-poly head, UV atlas and paint engine.
   New, purpose-built geometry. No third-party model or image dependencies.
   48 longitudinal slices * 32 vertical strips, plus closed crown.
   Editable deterministic mesh for PSP. Original asset: CC0 dedication. */
#ifndef BALD_BOSS_HEAD_V2_H
#define BALD_BOSS_HEAD_V2_H
#define NEW_HEAD_SLICES 48
#define NEW_HEAD_RINGS 32
static float bb_exp(float v){return expf(-v*v);}
static float bb_linear(float y,const float *values){
  static const float heights[9]={1.14f,.98f,.75f,.45f,.18f,-.18f,-.48f,-.79f,-1.02f};
  if(y>=heights[0])return values[0];
  for(int i=0;i<8;i++)if(y>=heights[i+1]){
    float f=(heights[i]-y)/(heights[i]-heights[i+1]);
    f=f*f*(3.0f-2.0f*f);
    return values[i]*(1.0f-f)+values[i+1]*f;
  }
  return values[8];
}
static void bb_head_point(float y,float angle,float *x,float *outy,float *z){
  static const float widths[9]={.025f,.44f,.60f,.66f,.65f,.63f,.59f,.43f,.12f};
  static const float depths[9]={.035f,.40f,.62f,.66f,.67f,.65f,.63f,.50f,.20f};
  static const float centerz[9]={-.045f,-.05f,-.06f,-.05f,-.04f,-.02f,.02f,.12f,.27f};
  float sx=sinf(angle),cz=cosf(angle),front=cz>0?cz:0;
  float rX=bb_linear(y,widths),rZ=bb_linear(y,depths);
  *x=rX*sx;*outy=y;*z=bb_linear(y,centerz)+rZ*cz;
  float bridge=bb_exp(angle/.20f)*bb_exp((y+.04f)/.29f);
  float tip=bb_exp(angle/.16f)*bb_exp((y+.25f)/.12f);
  float wing=(bb_exp((angle-.16f)/.085f)+bb_exp((angle+.16f)/.085f))*bb_exp((y+.31f)/.09f);
  *z+=.078f*bridge+.168f*tip+.035f*wing;
  float eyes=(bb_exp((angle-.40f)/.18f)+bb_exp((angle+.40f)/.18f));
  *z-=.043f*eyes*bb_exp((y-.25f)/.125f);
  *z+=.019f*eyes*bb_exp((y-.38f)/.09f);
  *z+=.026f*eyes*bb_exp((y+.08f)/.20f);
  float mid=bb_exp(angle/.34f);
  *z+=mid*(.022f*bb_exp((y+.53f)/.050f)
           +.034f*bb_exp((y+.62f)/.060f)
           +.035f*bb_exp((y+.80f)/.125f));
  float ear=(bb_exp((angle-1.58f)/.22f)+bb_exp((angle+1.58f)/.22f))*bb_exp((y+.11f)/.18f);
  *x+=sx*.095f*ear;
  *z+=front*.012f*ear;
}
static TexturedVertex bb_head_vert(float y,float a,float u,float v){
  TexturedVertex t;
  bb_head_point(y,a,&t.x,&t.y,&t.z);
  float ax,ay,az,bx,by,bz;
  bb_head_point(y,a+.007f,&ax,&ay,&az);
  bb_head_point(y+.007f,a,&bx,&by,&bz);
  float dx=ax-t.x,dy=ay-t.y,dz=az-t.z;
  float ex=bx-t.x,ey=by-t.y,ez=bz-t.z;
  t.nx=dy*ez-dz*ey;t.ny=dz*ex-dx*ez;t.nz=dx*ey-dy*ex;
  normalize(&t.nx,&t.ny,&t.nz);
  t.u=u*127.0f;t.v=v*127.0f;
  return t;
}
static void bb_build_head(void){
  n_head=0;
  for(int i=0;i<NEW_HEAD_RINGS;i++){
    float ya=1.14f-(2.16f*i)/(float)NEW_HEAD_RINGS;
    float yb=1.14f-(2.16f*(i+1))/(float)NEW_HEAD_RINGS;
    for(int j=0;j<NEW_HEAD_SLICES;j++){
      float ua=(float)j/(float)NEW_HEAD_SLICES;
      float ub=(float)(j+1)/(float)NEW_HEAD_SLICES;
      float aa=(ua-.5f)*2.0f*PI,ab=(ub-.5f)*2.0f*PI;
      TexturedVertex a=bb_head_vert(ya,aa,ua,(float)i/NEW_HEAD_RINGS);
      TexturedVertex b=bb_head_vert(ya,ab,ub,(float)i/NEW_HEAD_RINGS);
      TexturedVertex c=bb_head_vert(yb,aa,ua,(float)(i+1)/NEW_HEAD_RINGS);
      TexturedVertex d=bb_head_vert(yb,ab,ub,(float)(i+1)/NEW_HEAD_RINGS);
      if(n_head+6>MAX_HEAD_VERTS)return;
      head[n_head++]=a;head[n_head++]=c;head[n_head++]=b;
      head[n_head++]=b;head[n_head++]=c;head[n_head++]=d;
    }
  }
  for(int j=0;j<NEW_HEAD_SLICES;j++){
    float ua=(float)j/(float)NEW_HEAD_SLICES;
    float ub=(float)(j+1)/(float)NEW_HEAD_SLICES;
    float aa=(ua-.5f)*2.0f*PI,ab=(ub-.5f)*2.0f*PI;
    TexturedVertex crown=bb_head_vert(1.14f,aa,ua,0.0f);
    TexturedVertex next=bb_head_vert(1.14f,ab,ub,0.0f);
    TexturedVertex center=crown;
    center.x=0.0f;center.y=1.17f;center.z=-.045f;
    center.nx=0;center.ny=1;center.nz=0;
    center.u=(ua+ub)*63.5f;
    head[n_head++]=center;head[n_head++]=crown;head[n_head++]=next;
  }
}
static unsigned short bb_rgb565(int r,int g,int b){
  r=CLAMP(r,0,255);g=CLAMP(g,0,255);b=CLAMP(b,0,255);
  return (unsigned short)(((b>>3)<<11)|((g>>2)<<5)|(r>>3));
}
static void bb_paint(float cx,float cy,float rx,float ry,int r,int g,int b,float opacity){
  int l=CLAMP((int)(cx-rx-2),0,255),h=CLAMP((int)(cx+rx+2),0,255);
  int top=CLAMP((int)(cy-ry-2),0,255),bottom=CLAMP((int)(cy+ry+2),0,255);
  for(int y=top;y<=bottom;y++)for(int x=l;x<=h;x++){
    float d=fsq((x-cx)/rx)+fsq((y-cy)/ry);
    if(d>1.0f)continue;
    float a=opacity*(1.0f-d*.30f);
    unsigned short c=skin_tex[y*256+x];
    int oldr=(c&31)*255/31,oldg=((c>>5)&63)*255/63,oldb=((c>>11)&31)*255/31;
    skin_tex[y*256+x]=bb_rgb565((int)(oldr*(1-a)+r*a),
       (int)(oldg*(1-a)+g*a),(int)(oldb*(1-a)+b*a));
  }
}
static void bb_make_skin(void){
  for(int y=0;y<256;y++)for(int x=0;x<256;x++){
    float u=(x-128.0f)/128.0f,v=(float)y/255.0f;
    float frontal=1.0f-CLAMP(fabsf(u)*1.1f,0.0f,1.0f);
    int grain=((x*37+y*53+(x*y)%17)%11)-5;
    int r=(int)(219.0f-24.0f*fabsf(u)+10.0f*bb_exp((v-.36f)/.20f)*frontal)+grain/2;
    int g=(int)(178.0f-16.0f*fabsf(u)+6.0f*bb_exp((v-.33f)/.23f)*frontal)+grain/2;
    int b=(int)(147.0f-11.0f*fabsf(u))+grain/2;
    skin_tex[y*256+x]=bb_rgb565(r,g,b);
  }
  bb_paint(100,156,16,30,174,119,104,.22f);
  bb_paint(156,156,16,30,174,119,104,.22f);
  bb_paint(107,137,23,16,225,155,145,.12f);
  bb_paint(149,137,23,16,225,155,145,.12f);
  for(int side=-1;side<=1;side+=2){
    float cx=128.0f+side*16.0f;
    bb_paint(cx,109,14,8,143,101,89,.27f);
    bb_paint(cx,115,11,5,242,235,217,.98f);
    bb_paint(cx,116,4.5f,5.0f,98,154,164,.96f);
    bb_paint(cx,116,2.25f,3.45f,22,32,38,1.0f);
    bb_paint(cx-1.2f,114,1.2f,1.1f,248,247,242,1.0f);
    bb_paint(cx,109.5f,13,2,94,63,54,.65f);
    bb_paint(cx,122,11,1.2f,141,104,89,.30f);
    bb_paint(cx,99,13,2.6f,68,47,39,.83f);
  }
  bb_paint(128,141,4.0f,23,244,210,182,.20f);
  bb_paint(122,159,3.0f,2.1f,101,62,55,.62f);
  bb_paint(134,159,3.0f,2.1f,101,62,55,.62f);
  bb_paint(128,161,11,3,221,168,138,.18f);
  bb_paint(128,192,16,3.4f,153,93,86,.56f);
  bb_paint(128,197,15.5f,4.2f,178,107,98,.61f);
  bb_paint(128,194,14,1.0f,90,58,55,.46f);
  bb_paint(128,220,11,6,155,112,95,.10f);
  for(int y=176;y<240;y+=3)for(int x=84;x<173;x+=4){
    if((x*31+y*17)%9!=0)continue;
    float d=fsq((x-128.0f)/49.0f)+fsq((y-204.0f)/43.0f);
    if(d<1.0f && !(y>188&&y<204&&abs(x-128)<20))
      bb_paint((float)x,(float)y,1.0f,1.0f,102,83,71,.12f);
  }
}
#endif
