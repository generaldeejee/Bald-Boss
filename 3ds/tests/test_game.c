/* Bald Elite Nintendo 3DS arcade gameplay tests; runs under ordinary GCC.
 * Exercises the *same* game.c/mesh.c used by the Homebrew Launcher build.
 */
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../source/game.h"

static BeMesh mesh;
static BeGame game;
int main(void){
    be_game_init(&game,&mesh);
    assert(game.phase==BE_TITLE && game.candidate==0);
    assert(mesh.hair_count==900);
    game.mode=BE_CHALLENGE;
    be_game_select(&game,1);
    assert(mesh.hair_count==BE_MAX_HAIR && game.mode==BE_CHALLENGE);
    be_game_start(&game);
    assert(game.phase==BE_PLAY && game.score==0 && game.frames==0);
    assert(be_game_percentage(&game)==0);
    assert(be_game_time_seconds(&game)==90);
    game.paused=1;be_game_tick(&game);
    assert(game.frames==0);
    assert(be_game_shave(&game,200,105)==0);
    game.paused=0;
    /* Find one actually projectable strand and shave it via screen-space
       targeting after rotating the candidate. */
    int found=0;
    for(int i=0;i<mesh.hair_count && !found;i++){
        BeHair *h=&mesh.strands[i];
        for(int a=0;a<48 && !found;a++){
            game.yaw=(float)a*6.28318530718f/48.0f;
            float x,y;
            if(!be_game_project(&game,h,&x,&y))continue;
            int hits=be_game_shave(&game,x,y);
            if(hits>0)found=1;
        }
    }
    assert(found && game.score>=20 && mesh.removed>0);
    int oldscore=game.score,oldremoved=mesh.removed;
    assert(be_game_shave(&game,-1000,-1000)>=0);
    assert(game.score>=oldscore&&mesh.removed>=oldremoved);
    /* Force a controlled one-hair endgame to exercise victory and record. */
    for(int i=0;i<mesh.hair_count;i++)mesh.strands[i].gone=1;
    mesh.strands[0].gone=0;
    mesh.removed=mesh.hair_count-1;
    assert(be_game_locate(&game)==1);
    assert(game.phase==BE_WIN);
    assert(mesh.removed==mesh.hair_count);
    assert(be_game_percentage(&game)==100);
    assert(game.total_shaves==1);
    assert(game.best[1][BE_CHALLENGE]==game.score);
    be_game_next(&game);
    assert(game.candidate==2&&game.phase==BE_PLAY);
    assert(game.mode==BE_CHALLENGE);
    assert(mesh.hair_count==760);
    for(int i=0;i<BE_TIME_LIMIT;i++)be_game_tick(&game);
    assert(game.phase==BE_FAIL);
    be_game_title(&game);
    assert(game.phase==BE_TITLE && game.candidate==2);
    game.mode=BE_ZEN;be_game_start(&game);
    for(int i=0;i<BE_TIME_LIMIT+240;i++)be_game_tick(&game);
    assert(game.phase==BE_PLAY);
    assert(be_game_time_seconds(&game)>90);
    assert(be_game_stars(&game)==3);
    /* Zoom must affect both the GPU camera distance and projected hit
       coordinates. Sweeping across one projected tuft must clip it. */
    be_game_select(&game,0);
    be_game_start(&game);
    assert(fabsf(be_game_distance(&game)-3.8f)<.001f);
    int target=-1;
    float sx=0,sy=0;
    for(int i=0;i<mesh.hair_count;i++){
        float x,y;
        if(be_game_project(&game,&mesh.strands[i],&x,&y) &&
           x>40.0f && x<360.0f && fabsf(x-200.0f)>20.0f){
            target=i;sx=x;sy=y;break;
        }
    }
    assert(target>=0);
    game.zoom=1;
    float zx=0,zy=0;
    assert(be_game_distance(&game)<3.8f);
    assert(be_game_project(&game,&mesh.strands[target],&zx,&zy));
    assert(fabsf(zx-200.0f)>fabsf(sx-200.0f));
    game.zoom=0;
    assert(be_game_shave_sweep(&game,sx-38.0f,sy,sx+38.0f,sy)>0);
    assert(mesh.strands[target].gone && mesh.removed>0);
    assert(game.score>=20);
    /* Hair removal must also mark atlas polish; the scalp is no longer
       merely hair-free geometry. */
    int polished=0;
    for(int i=0;i<256*256;i++)if(mesh.shaved_coverage[i])polished++;
    assert(polished>0);
    /* Original PSP's last-hair protection: a single inaccessible strand
       must not fail the timed challenge during cleanup. */
    game.mode=BE_CHALLENGE;
    be_game_start(&game);
    for(int i=1;i<mesh.hair_count;i++)mesh.strands[i].gone=1;
    mesh.removed=mesh.hair_count-1;
    game.frames=BE_TIME_LIMIT-1;
    for(int i=0;i<3*BE_FPS;i++)be_game_tick(&game);
    assert(game.phase==BE_PLAY && game.frames==BE_TIME_LIMIT-1);
    puts("PASS: 3DS arcade + sweep shaving + synchronized camera zoom.");
    return 0;
}
