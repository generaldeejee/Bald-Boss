/* Bald Elite - portable 3DS arcade rules and hair hit detection.
 * Pure C99, testable on a desktop without a Nintendo SDK.
 */
#ifndef BALD_ELITE_3DS_GAME_H
#define BALD_ELITE_3DS_GAME_H
#include "mesh.h"

enum {BE_TITLE=0,BE_PLAY=1,BE_WIN=2,BE_FAIL=3};
enum {BE_CHALLENGE=0,BE_ZEN=1};
#define BE_FPS 60
#define BE_TIME_LIMIT (90*BE_FPS)
typedef struct {
    BeMesh *mesh;
    int candidate, mode, phase, paused;
    int frames, score, combo, best[BE_CANDIDATES][2];
    int last_cut, last_hits, celebration, total_shaves;
    float yaw,pitch,cursor_x,cursor_y;
    int zoom, previous_active;
    float previous_x,previous_y;
} BeGame;

void be_game_init(BeGame *g,BeMesh *mesh);
void be_game_select(BeGame *g,int candidate);
void be_game_start(BeGame *g);
void be_game_tick(BeGame *g);
int be_game_shave(BeGame *g,float cursor_x,float cursor_y);
/* Removes every hair crossed between consecutive clipper positions. */
int be_game_shave_sweep(BeGame *g,float x0,float y0,float x1,float y1);
float be_game_distance(const BeGame *g);
int be_game_project(const BeGame *g,const BeHair *hair,float *x,float *y);
int be_game_locate(BeGame *g); /* accessible last-hair assist */
void be_game_next(BeGame *g);
void be_game_title(BeGame *g);
int be_game_percentage(const BeGame *g);
int be_game_stars(const BeGame *g);
int be_game_time_seconds(const BeGame *g);
#endif
