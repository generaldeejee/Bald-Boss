/* Portable Bald Elite Nintendo 3DS arcade game rules.
 * The projection mirrors the Citro3D view (50-degree vertical FOV,
 * 400x240 top screen, translated camera 3.8 units from the head).
 */
#include "game.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f
#define CLAMP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define BE_FOCAL (120.0f/0.466307658f) /* tan(25 degrees) */

void be_game_init(BeGame *g,BeMesh *mesh){
    memset(g,0,sizeof(*g));
    g->mesh=mesh;
    g->cursor_x=200.0f;g->cursor_y=104.0f;
    be_game_select(g,0);
}

void be_game_select(BeGame *g,int candidate){
    if(candidate<0)candidate=(candidate%BE_CANDIDATES+BE_CANDIDATES)%BE_CANDIDATES;
    g->candidate=candidate%BE_CANDIDATES;
    be_mesh_generate(g->mesh,g->candidate);
    g->phase=BE_TITLE;g->paused=0;
    g->cursor_x=200;g->cursor_y=105;
    g->yaw=0;g->pitch=0;g->zoom=0;g->previous_active=0;
    g->score=0;g->frames=0;g->combo=0;g->last_hits=0;
    g->cleanup_bonus_given=0;
}
void be_game_start(BeGame *g){
    be_mesh_generate(g->mesh,g->candidate);
    g->phase=BE_PLAY;g->paused=0;g->score=0;g->frames=0;
    g->combo=0;g->last_cut=-999;g->last_hits=0;g->celebration=0;
    g->cleanup_bonus_given=0;
    g->yaw=0;g->pitch=0;g->zoom=0;g->previous_active=0;
    g->cursor_x=200;g->cursor_y=105;
}
void be_game_title(BeGame *g){
    g->phase=BE_TITLE;g->paused=0;
    be_game_select(g,g->candidate);
}
void be_game_next(BeGame *g){
    g->candidate=(g->candidate+1)%BE_CANDIDATES;
    be_game_start(g);
}
int be_game_percentage(const BeGame *g){
    return g->mesh->hair_count ? 100*g->mesh->removed/g->mesh->hair_count:0;
}
int be_game_time_seconds(const BeGame *g){
    int seconds=g->frames/BE_FPS;
    if(g->mode==BE_ZEN)return seconds;
    return CLAMP(90-seconds,0,90);
}
int be_game_stars(const BeGame *g){
    if(g->mode==BE_ZEN)return 3;
    return g->frames<=50*BE_FPS?3:(g->frames<=75*BE_FPS?2:1);
}
static void finish_run(BeGame *g){
    g->phase=BE_WIN;
    g->paused=0;
    g->celebration=0;
    g->zoom=0;g->previous_active=0;
    int *record=&g->best[g->candidate][g->mode];
    if(g->score>*record)*record=g->score;
    g->total_shaves++;
}
void be_game_tick(BeGame *g){
    if(g->phase==BE_WIN){g->celebration++;return;}
    if(g->phase!=BE_PLAY||g->paused)return;
    /* The PSP edition freezes the clock for the very last strand. */
    if(g->mode==BE_ZEN || g->mesh->hair_count-g->mesh->removed>1)g->frames++;
    if(g->mode==BE_CHALLENGE&&g->frames>=BE_TIME_LIMIT&&
       g->mesh->hair_count-g->mesh->removed>1){
        g->phase=BE_FAIL;
        g->paused=0;
    }
}

float be_game_distance(const BeGame *g){
    static const float distances[3]={3.8f,3.1f,2.65f};
    int level=CLAMP(g->zoom,0,2);
    return distances[level];
}

/* Hair face visibility and screen coordinates (top screen, not bottom touch).
 * A small threshold permits the crown to be shaved while rotating the head.
 * The thick hair root is projected, not the tip, which is more stable. */
/* One projection formula for both rendering and hit-testing. Hot swipe
 * paths reuse frame trig rather than evaluating 4 sin/cos per hair. */
static int project_fast(const BeGame *g,const BeHair *h,float *x,float *y,
                        float cy,float sy,float cp,float sp){
    float xx=cy*h->x+sy*h->z;
    float zz=cy*h->z-sy*h->x;
    float yy=cp*h->y-sp*zz;
    float zr=sp*h->y+cp*zz;
    float nz=cy*h->nz-sy*h->nx;
    float front=sp*h->ny+cp*nz;
    float distance=be_game_distance(g);
    if(front<-.03f || zr>=distance-.8f)return 0;
    float inv=BE_FOCAL/(distance-zr);
    *x=200.0f+xx*inv;
    *y=120.0f-(yy-.06f)*inv;
    return *x>=1 && *x<=399 && *y>=1 && *y<=239;
}
int be_game_project(const BeGame *g,const BeHair *h,float *x,float *y){
    return project_fast(g,h,x,y,cosf(g->yaw),sinf(g->yaw),
                        cosf(g->pitch),sinf(g->pitch));
}

/* Segment / swept-circle collision means fast touchscreen strokes remove the
 * intervening hairs instead of sampling only one point per video frame.
 * Elliptical contact approximates the physical clipper width on the scalp. */
int be_game_shave_sweep(BeGame *g,float x0,float y0,float x1,float y1){
    if(g->phase!=BE_PLAY||g->paused)return 0;
    x0=CLAMP(x0,5.0f,395.0f);y0=CLAMP(y0,4.0f,235.0f);
    x1=CLAMP(x1,5.0f,395.0f);y1=CLAMP(y1,4.0f,235.0f);
    g->cursor_x=x1;
    g->cursor_y=y1;
    const float vx=(x1-x0)/16.0f,vy=(y1-y0)/14.0f;
    const float len2=vx*vx+vy*vy;
    int hits=0;
    const float cy=cosf(g->yaw),sinyaw=sinf(g->yaw);
    const float cp=cosf(g->pitch),sp=sinf(g->pitch);
    for(int i=0;i<g->mesh->hair_count;i++){
        BeHair *h=&g->mesh->strands[i];
        if(h->gone)continue;
        float sx,sy;
        if(!project_fast(g,h,&sx,&sy,cy,sinyaw,cp,sp))continue;
        float dx=(sx-x0)/16.0f,dy=(sy-y0)/14.0f;
        float t=len2>1.0e-5f?CLAMP((dx*vx+dy*vy)/len2,0.0f,1.0f):0.0f;
        float ex=dx-t*vx,ey=dy-t*vy;
        if(ex*ex+ey*ey>1.0f)continue;
        h->gone=1;
        g->mesh->removed++;
        be_mesh_polish_hair(g->mesh,h);
        hits++;
    }
    if(hits){
        if(g->frames-g->last_cut>20)g->combo=0;
        g->combo=CLAMP(g->combo+hits,0,100);
        g->score+=hits*20+CLAMP(g->combo/3,0,60);
        g->last_cut=g->frames;
        g->last_hits=hits;
        be_mesh_rebuild_hair(g->mesh);
        be_mesh_recolor_head(g->mesh);
        if(g->mode==BE_CHALLENGE && !g->cleanup_bonus_given &&
           g->mesh->hair_count-g->mesh->removed<=10 &&
           g->mesh->hair_count-g->mesh->removed>0){
            g->cleanup_bonus_given=1;
            g->frames=g->frames>30*BE_FPS?g->frames-30*BE_FPS:0;
        }
        if(g->mesh->removed>=g->mesh->hair_count)finish_run(g);
    }
    return hits;
}
int be_game_shave(BeGame *g,float x,float y){
    return be_game_shave_sweep(g,x,y,x,y);
}

/* Y assist: never force the player to search indefinitely for a few hairs.
 * Orient the camera toward a remaining hair and center the clipper.
 * Last remaining inaccessible hair can be removed directly, as in PSP.
 */
int be_game_locate(BeGame *g){
    if(g->phase!=BE_PLAY||g->paused)return 0;
    int remaining=g->mesh->hair_count-g->mesh->removed;
    if(remaining>8)return 0;
    for(int i=0;i<g->mesh->hair_count;i++){
        BeHair *h=&g->mesh->strands[i];
        if(h->gone)continue;
        g->yaw=atan2f(h->nx,h->nz);
        g->pitch=CLAMP(-atan2f(h->ny,sqrtf(h->nx*h->nx+h->nz*h->nz))*.7f,-.70f,.6f);
        float sx,sy;
        if(be_game_project(g,h,&sx,&sy)){
            be_game_shave(g,sx,sy);
        }else{
            h->gone=1;g->mesh->removed++;g->score+=20;
            be_mesh_polish_hair(g->mesh,h);
            be_mesh_rebuild_hair(g->mesh);
            be_mesh_recolor_head(g->mesh);
            if(g->mesh->removed>=g->mesh->hair_count)finish_run(g);
        }
        return 1;
    }
    return 0;
}
