/* Bald Elite 3DS v1.2 - stability recovery for ORIGINAL 3DS.
 * Keep gameplay, touch input and audio, but avoid all PICA200 shader draws.
 * Use libctru's double-buffered top CPU framebuffer instead.
 */
#include <3ds.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "mesh.h"
#include "game.h"
#include "audio.h"
#include "save.h"
#include "render_soft.h"

static BeMesh scene;
static BeGame game;
static bool sound_available=false;
static bool sound_enabled=true;
static int last_audio_phase=BE_TITLE;

/* Stable bottom LCD: no \x1b[2J each frame, and no whole-screen repaint.
 * Only changed text rows are redrawn, eliminating the old console flicker. */
#define STATUS_ROWS 29
#define STATUS_COLS 38
static char old_row[STATUS_ROWS+1][STATUS_COLS+1];
/* Coalesce console changes to ten refreshes/sec instead of doing formatted
   text output on every GPU frame. Phase/selection switches remain instant. */
static int status_epoch=0;
static bool status_dirty=false;
/* Console writes pixels into CPU memory. Flush the BOTTOM framebuffer after
 * the complete batch of changed rows so the LCD never sees stale cache lines.
 * This is independent of the Citro3D TOP-screen render target. */
static void status_present(void){
    if(!status_dirty)return;
    fflush(stdout);
    u16 width=0,height=0;
    u8 *framebuffer=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
    if(framebuffer){
        u32 bpp=(gfxGetScreenFormat(GFX_BOTTOM)==GSP_BGR8_OES)?3u:2u;
        GSPGPU_FlushDataCache(framebuffer,(u32)width*(u32)height*bpp);
    }
    status_dirty=false;
}
static void status_invalidate(void){
    memset(old_row,0,sizeof(old_row));
    status_epoch++;
}
static void status_row(int line,const char *format,...){
    if(line<1||line>STATUS_ROWS)return;
    char tmp[160],display[STATUS_COLS+1];
    va_list args;
    va_start(args,format);
    vsnprintf(tmp,sizeof(tmp),format,args);
    va_end(args);
    snprintf(display,sizeof(display),"%-37.37s",tmp);
    if(strcmp(old_row[line],display)==0)return;
    strcpy(old_row[line],display);
    printf("\x1b[%d;1H%s",line,display);
    status_dirty=true;
}
static void status_display(void){
    /* Update console text at 10 Hz, and immediately on a mode/phase change.
       Crucially we NEVER clear the whole bottom LCD while playing. */
    static int previous_layout=-1,previous_candidate=-1,previous_epoch=-1;
    static unsigned refresh_frame=0;
    int layout=game.phase+(game.phase==BE_PLAY && game.paused?10:0);
    refresh_frame++;
    if(layout==previous_layout && previous_candidate==game.candidate &&
       previous_epoch==status_epoch && refresh_frame%6!=0)return;
    if(layout!=previous_layout || previous_epoch!=status_epoch){
        for(int line=5;line<=23;line++)status_row(line,"");
        previous_layout=layout;
    }
    previous_candidate=game.candidate;
    previous_epoch=status_epoch;
    const char *name=be_candidate_names[game.candidate];
    status_row(1,"  =================================");
    status_row(2,"       BALD ELITE 3DS ARCADE");
    status_row(3,"  ===== THE ELITE BALD CLUB =======");
    status_row(4,"");
    if(game.phase==BE_TITLE){
        status_row(6,"          CHOOSE APPLICANT");
        status_row(7,"");
        status_row(8,"  %d/3  %s",game.candidate+1,name);
        status_row(9,"  Hair: %d strands",scene.hair_count);
        status_row(10,"");
        status_row(11,"  %s",game.mode==BE_ZEN?"> ZEN / NO TIMER":"> CHALLENGE / 90 SEC");
        status_row(12,"");
        status_row(13,"  LEFT/RIGHT  another applicant");
        status_row(14,"  UP / DOWN  change game mode");
        status_row(15,"  A or X     START SHAVING!");
        status_row(16,"  SELECT     toggle music");
        status_row(17,"  L/R        preview rotation");
        status_row(18,"  START      quit Homebrew");
        status_row(19,"");
        status_row(20,"  NO HAIR. NO FEAR.");
        status_row(21,"  ONLY THE BALD MAY JOIN.");
    }else if(game.phase==BE_PLAY && game.paused){
        status_row(6,"             PAUSED");
        status_row(7,"");
        status_row(8,"  %s",name);
        status_row(9,"  Score %d",game.score);
        status_row(10,"");
        status_row(11,"  B             RESUME");
        status_row(12,"  A             RETRY LEVEL");
        status_row(13,"  START         LEVEL SELECT");
        status_row(14,"");
        status_row(15,"  Time and clippers are paused.");
        for(int line=16;line<=22;line++)status_row(line,"");
    }else if(game.phase==BE_PLAY){
        int remaining=scene.hair_count-scene.removed;
        char bar[21];
        int done=20*scene.removed/(scene.hair_count?scene.hair_count:1);
        for(int i=0;i<20;i++)bar[i]=i<done?'#':'-';
        bar[20]=0;
        status_row(6,"  %s",name);
        status_row(7,"  %s",game.mode==BE_ZEN?"ZEN MODE":"90-SECOND CHALLENGE");
        status_row(8,"");
        status_row(9,"  SHAVED    %3d%%  [%s]",be_game_percentage(&game),bar);
        status_row(10,"  REMAINING %-4d  SCORE %-7d",remaining,game.score);
        status_row(11,"  %s %02d:%02d",game.mode==BE_ZEN?"ELAPSED":"TIME LEFT",
                   be_game_time_seconds(&game)/60,be_game_time_seconds(&game)%60);
        status_row(12,"  COMBO %d",game.combo);
        status_row(13,"  X  ZOOM %d/3   SELECT %s",game.zoom+1,sound_enabled?"MUTE":"UNMUTE");
        status_row(14,"  TOUCH + drag  SHAVE on top");
        status_row(15,"  CIRCLE PAD    MOVE CLIPPERS");
        status_row(16,"  HOLD A        SHAVE HAIR");
        status_row(17,"  L/R           ROTATE HEAD");
        status_row(18,"  UP / DOWN     TILT HEAD");
        status_row(19,"  Y             FINAL HAIR HELP");
        status_row(20,"  B             PAUSE");
        status_row(21,"  START         LEVEL SELECT");
        status_row(22,remaining<=8?"  Y ASSIST READY: %d LEFT":
                   (game.cleanup_bonus_given?"  +30 SEC CLEANUP BONUS!":"  Keep shaving!"),remaining);
    }else if(game.phase==BE_WIN){
        int stars=be_game_stars(&game);
        status_row(6,"  +------------------------------+");
        status_row(7,"  |  THE ELITE BALD MAN CLUB     |");
        status_row(8,"  |                              |");
        status_row(9,"  |    MEMBERSHIP APPROVED!      |");
        status_row(10,"  |                              |");
        status_row(11,"  |  %s",name);
        status_row(12,"  |  RANK: BALD BOSS             |");
        status_row(13,"  |  %s",stars==3?"* * *  GOLD SHAVE":stars==2?"* *    SILVER SHAVE":"*      BRONZE SHAVE");
        status_row(14,"  |                              |");
        status_row(15,"  +------------------------------+");
        status_row(16,"");
        status_row(17,"  SCORE %d  BEST %d",game.score,game.best[game.candidate][game.mode]);
        status_row(18,"  ANOTHER BALD BOSS!");
        status_row(19,"");
        status_row(20,"  A RETRY    X NEXT APPLICANT");
        status_row(21,"  B / START BACK TO LEVEL SELECT");
        status_row(22,"");
    }else if(game.phase==BE_FAIL){
        status_row(6,"");
        status_row(7,"        MEMBERSHIP DENIED!");
        status_row(8,"");
        status_row(9,"  THE CLOCK HAS RUN OUT.");
        status_row(10,"  %s",name);
        status_row(11,"  SHAVED %d%%  SCORE %d",be_game_percentage(&game),game.score);
        status_row(12,"");
        status_row(13,"  A / X    RETRY");
        status_row(14,"  B / START   SELECT ANOTHER");
        for(int line=15;line<=22;line++)status_row(line,"");
    }
    status_row(24,"  ---------------------------------");
    status_row(25,"  TOUCHSCREEN = CLIPPER PAD");
    status_row(26,"  PSP AND 3DS EDITIONS ARE SEPARATE");
    status_row(27,"");
    status_row(28,sound_available?"  SAFE v1.2  |  DSP AUDIO":"  SAFE v1.2  |  SILENT MODE");
    status_present();
}


/* Keep compatibility with the existing gameplay input controller. Mesh
 * mutations happen in game.c and appear on the top LCD the next frame;
 * no GPU VBO upload or command-buffer synchronisation is needed. */
static void upload_mesh(bool ignored){(void)ignored;}
static void shaving_emit(float x,float y,int count){be_soft_emit(x,y,count);}
static float clip(float x,float low,float high){
    return x<low?low:(x>high?high:x);
}

/* Buttons map follows the 3DS's actual dual-screen layout:
 * title LEFT/RIGHT select; play L/R rotate; circle pad or touchscreen
 * moves the cursor and touch/A clips hair at that exact on-screen location. */
static bool process_input(void){
    hidScanInput();
    u32 down=hidKeysDown(),held=hidKeysHeld();
    if(down&KEY_SELECT){sound_enabled=!sound_enabled;status_invalidate();}
    if(game.phase==BE_TITLE){
        if(down&KEY_START)return false;
        if(down&KEY_LEFT){
            be_game_select(&game,(game.candidate+BE_CANDIDATES-1)%BE_CANDIDATES);
            upload_mesh(true);
        }
        if(down&KEY_RIGHT){
            be_game_select(&game,game.candidate+1);
            upload_mesh(true);
        }
        if(down&KEY_UP){game.mode=BE_CHALLENGE;status_invalidate();}
        if(down&KEY_DOWN){game.mode=BE_ZEN;status_invalidate();}
        if(down&(KEY_A|KEY_X)){
            be_game_start(&game);
            upload_mesh(true);
        }
        if(held&KEY_L)game.yaw-=.035f;
        if(held&KEY_R)game.yaw+=.035f;
        return true;
    }
    if(game.phase==BE_WIN||game.phase==BE_FAIL){
        if(down&(KEY_B|KEY_START)){
            be_game_title(&game);upload_mesh(true);
        }else if(down&KEY_A || (down&KEY_X && game.phase==BE_FAIL)){
            be_game_start(&game);upload_mesh(true);
        }else if((down&KEY_X)&&game.phase==BE_WIN){
            be_game_next(&game);upload_mesh(true);
        }
        if(game.phase==BE_WIN){
            game.yaw+=.01f;game.celebration++;
        }
        return true;
    }
    if(down&KEY_START){
        be_game_title(&game);upload_mesh(true);return true;
    }
    if(down&KEY_B){game.paused=!game.paused;game.previous_active=0;}
    if(game.paused){
        if(down&KEY_A){be_game_start(&game);upload_mesh(true);}
        return true;
    }
    if(down&KEY_X){game.zoom=(game.zoom+1)%3;game.previous_active=0;status_invalidate();}
    if(held&KEY_L)game.yaw-=.034f;
    if(held&KEY_R)game.yaw+=.034f;
    if(held&KEY_UP)game.pitch=clip(game.pitch+.027f,-.7f,.8f);
    if(held&KEY_DOWN)game.pitch=clip(game.pitch-.027f,-.7f,.8f);

    circlePosition pad;
    hidCircleRead(&pad);
    if(pad.dx>18 || pad.dx< -18)
        game.cursor_x=clip(game.cursor_x+(float)pad.dx/40,5,395);
    if(pad.dy>18 || pad.dy< -18)
        game.cursor_y=clip(game.cursor_y-(float)pad.dy/40,5,235);
    if(held&KEY_TOUCH){
        touchPosition touch;
        hidTouchRead(&touch);
        game.cursor_x=clip((float)touch.px*400.0f/320.0f,5,395);
        game.cursor_y=clip((float)touch.py,5,235);
    }
    if((down&KEY_Y)&&be_game_locate(&game))upload_mesh(false);
    if(game.phase==BE_PLAY && ((held&KEY_A)||(held&KEY_TOUCH))){
        float x0=game.previous_active?game.previous_x:game.cursor_x;
        float y0=game.previous_active?game.previous_y:game.cursor_y;
        int cut=be_game_shave_sweep(&game,x0,y0,game.cursor_x,game.cursor_y);
        if(cut>0){
            shaving_emit(game.cursor_x,game.cursor_y,cut);
            upload_mesh(false);
        }
        game.previous_x=game.cursor_x;
        game.previous_y=game.cursor_y;
        game.previous_active=1;
    }else game.previous_active=0;
    be_game_tick(&game);
    return true;
}

int main(void){
    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP,true);
    /* consoleInit sets bottom RGB565 single-buffered and keeps its framebuffer. */
    consoleInit(GFX_BOTTOM,NULL);
    be_game_init(&game,&scene);
    be_save_load(&game);
    status_invalidate();
    /* Draw a real top frame even before the first gameplay/input iteration. */
    be_soft_draw(&scene,&game);
    gfxFlushBuffers();
    gfxScreenSwapBuffers(GFX_TOP,false);
    gspWaitForVBlank();
    sound_available=be_audio_init();
    while(aptMainLoop()){
        if(!process_input())break;
        if(sound_available){
            if(last_audio_phase!=BE_WIN && game.phase==BE_WIN)be_audio_victory();
            u32 held=hidKeysHeld();
            bool shaving=game.phase==BE_PLAY && !game.paused &&
                        ((held&KEY_A)||(held&KEY_TOUCH));
            be_audio_update(sound_enabled,shaving);
        }
        if(last_audio_phase!=BE_WIN && game.phase==BE_WIN)be_save_write(&game);
        last_audio_phase=game.phase;
        status_display();
        be_soft_draw(&scene,&game);
        gfxFlushBuffers();
        /* Swap TOP ONLY. Never swap the bottom console's backing buffer. */
        gfxScreenSwapBuffers(GFX_TOP,false);
        gspWaitForVBlank();
    }
    if(sound_available)be_audio_shutdown();
    gfxExit();
    return 0;
}
