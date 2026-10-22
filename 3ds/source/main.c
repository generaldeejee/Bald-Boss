/* Bald Elite for the Nintendo 3DS - playable arcade edition.
 * Original 3DS (not New 3DS only), citro3d top screen, stable bottom console.
 * PSP build is a separate, unchanged target.
 */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "mesh.h"
#include "game.h"
#include "audio.h"
#include "vshader_shbin.h"

#define BG_COLOR 0x142537FF
#define DISPLAY_FLAGS (GX_TRANSFER_FLIP_VERT(0) | \
 GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
 GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
 GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
 GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static BeMesh scene;                  /* static: avoid 3DS stack overflow */
static BeGame game;
static BeVertex *head_vbo=NULL,*hair_vbo=NULL;
static DVLB_s *vshader=NULL;
static shaderProgram_s program;
static bool program_ready=false;
static C3D_Mtx projection,overlay_projection;
static int loc_projection=-1,loc_modelview=-1;
static bool sound_available=false;
static bool sound_enabled=true;
static int last_audio_phase=BE_TITLE;

/* Stable bottom LCD: no \x1b[2J each frame, and no whole-screen repaint.
 * Only changed text rows are redrawn, eliminating the old console flicker. */
#define STATUS_ROWS 29
#define STATUS_COLS 38
static char old_row[STATUS_ROWS+1][STATUS_COLS+1];
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
}
static void status_display(void){
    const char *name=be_candidate_names[game.candidate];
    status_row(1,"  =================================");
    status_row(2,"            BALD ELITE");
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
        status_row(16,"");
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
        status_row(13,"");
        status_row(14,"  TOUCH + drag  SHAVE on top");
        status_row(15,"  CIRCLE PAD    MOVE CLIPPERS");
        status_row(16,"  HOLD A        SHAVE HAIR");
        status_row(17,"  L/R           ROTATE HEAD");
        status_row(18,"  UP / DOWN     TILT HEAD");
        status_row(19,"  Y             FINAL HAIR HELP");
        status_row(20,"  B             PAUSE");
        status_row(21,"  START         LEVEL SELECT");
        status_row(22,remaining<=8?"  Y ASSIST READY: %d LEFT": "  Keep shaving!",remaining);
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
    status_row(28,sound_available?"  Bald Elite 3DS  |  DSP audio":"  Bald Elite 3DS  |  silent mode");
    fflush(stdout);
}

/* One-time VBO allocation, candidate swaps regenerate only their hair. */
static void upload_mesh(bool generate){
    if(generate)be_mesh_generate(&scene,game.candidate);
    if(generate){
        memcpy(head_vbo,scene.head,scene.head_count*sizeof(BeVertex));
        GSPGPU_FlushDataCache(head_vbo,scene.head_count*sizeof(BeVertex));
    }
    memcpy(hair_vbo,scene.hair,scene.hair_count*6*sizeof(BeVertex));
    GSPGPU_FlushDataCache(hair_vbo,scene.hair_count*6*sizeof(BeVertex));
}
static bool scene_init(void){
    head_vbo=linearAlloc(sizeof(scene.head));
    hair_vbo=linearAlloc(sizeof(scene.hair));
    if(!head_vbo||!hair_vbo)return false;
    vshader=DVLB_ParseFile((u32*)vshader_shbin,vshader_shbin_size);
    if(!vshader)return false;
    shaderProgramInit(&program);
    program_ready=true;
    shaderProgramSetVsh(&program,&vshader->DVLE[0]);
    C3D_BindProgram(&program);
    loc_projection=shaderInstanceGetUniformLocation(program.vertexShader,"projection");
    loc_modelview=shaderInstanceGetUniformLocation(program.vertexShader,"modelView");
    if(loc_projection<0||loc_modelview<0)return false;

    C3D_AttrInfo *attribs=C3D_GetAttrInfo();
    AttrInfo_Init(attribs);
    AttrInfo_AddLoader(attribs,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(attribs,1,GPU_FLOAT,4);

    C3D_TexEnv *env=C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env,C3D_Both,GPU_PRIMARY_COLOR,0,0);
    C3D_TexEnvFunc(env,C3D_Both,GPU_REPLACE);
    Mtx_PerspTilt(&projection,C3D_AngleFromDegrees(50.0f),
                  C3D_AspectRatioTop,.1f,40.0f,false);
    Mtx_OrthoTilt(&overlay_projection,0.0f,400.0f,240.0f,0.0f,
                   0.0f,1.0f,true);
    upload_mesh(true);
    return true;
}

/* Immediate-mode screen overlay. Shares the same color shader and avoids
 * external textures/font dependencies on the original 3DS. */
static void screen_vertex(float x,float y,float r,float g,float b){
    C3D_ImmSendAttrib(x,y,.5f,0);
    C3D_ImmSendAttrib(r,g,b,1);
}
static void screen_rect(float x,float y,float w,float h,float r,float g,float b){
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    screen_vertex(x,y,r,g,b);screen_vertex(x+w,y,r,g,b);screen_vertex(x,y+h,r,g,b);
    screen_vertex(x+w,y,r,g,b);screen_vertex(x+w,y+h,r,g,b);screen_vertex(x,y+h,r,g,b);
    C3D_ImmDrawEnd();
}
static void overlay_draw(void){
    C3D_Mtx identity;
    Mtx_Identity(&identity);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,loc_projection,&overlay_projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,loc_modelview,&identity);
    C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_ALL);
    screen_rect(0,0,400,12,.05f,.10f,.15f);
    screen_rect(0,0,400,3,.75f,.57f,.20f);
    if(game.phase==BE_PLAY){
        float p=(float)be_game_percentage(&game)*4.0f;
        screen_rect(0,232,400,8,.07f,.13f,.20f);
        screen_rect(0,232,p,8,.26f,.82f,.63f);
        float x=game.cursor_x,y=game.cursor_y;
        screen_rect(x-14,y-1,28,2,.97f,.80f,.30f);
        screen_rect(x-1,y-14,2,28,.97f,.80f,.30f);
        screen_rect(x-3,y-3,6,6,.18f,.85f,.76f);
        if(game.paused){
            screen_rect(90,85,220,7,.65f,.51f,.22f);
            screen_rect(90,155,220,7,.65f,.51f,.22f);
        }
    }else if(game.phase==BE_WIN){
        screen_rect(25,194,350,28,.72f,.52f,.15f);
        screen_rect(31,200,338,16,.92f,.75f,.32f);
        int stars=be_game_stars(&game);
        for(int i=0;i<stars;i++)screen_rect(164+i*30,205,17,8,.99f,.98f,.75f);
    }else if(game.phase==BE_FAIL){
        screen_rect(45,207,310,12,.66f,.14f,.14f);
    }else{
        screen_rect(78,225,244,5,.74f,.54f,.20f);
    }
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
}

static void scene_draw(void){
    C3D_Mtx view;
    Mtx_Identity(&view);
    Mtx_Translate(&view,0.0f,-.06f,-3.8f,true);
    Mtx_RotateX(&view,game.pitch,true);
    Mtx_RotateY(&view,game.yaw,true);
    C3D_BindProgram(&program);
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,loc_projection,&projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,loc_modelview,&view);
    C3D_BufInfo *buffers=C3D_GetBufInfo();
    BufInfo_Init(buffers);
    BufInfo_Add(buffers,head_vbo,sizeof(BeVertex),2,0x10);
    C3D_DrawArrays(GPU_TRIANGLES,0,scene.head_count);
    BufInfo_Init(buffers);
    BufInfo_Add(buffers,hair_vbo,sizeof(BeVertex),2,0x10);
    C3D_DrawArrays(GPU_TRIANGLES,0,scene.hair_count*6);
    overlay_draw();
}
static void scene_shutdown(void){
    if(program_ready)shaderProgramFree(&program);
    if(vshader)DVLB_Free(vshader);
    if(head_vbo)linearFree(head_vbo);
    if(hair_vbo)linearFree(hair_vbo);
}

static float clip(float x,float low,float high){
    return x<low?low:(x>high?high:x);
}

/* Buttons map follows the 3DS's actual dual-screen layout:
 * title LEFT/RIGHT select; play L/R rotate; circle pad or touchscreen
 * moves the cursor and touch/A clips hair at that exact on-screen location. */
static bool process_input(void){
    hidScanInput();
    u32 down=hidKeysDown(),held=hidKeysHeld();
    if(down&KEY_SELECT)sound_enabled=!sound_enabled;
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
        if(down&KEY_UP)game.mode=BE_CHALLENGE;
        if(down&KEY_DOWN)game.mode=BE_ZEN;
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
    if(down&KEY_B)game.paused=!game.paused;
    if(game.paused){
        if(down&KEY_A){be_game_start(&game);upload_mesh(true);}
        return true;
    }
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
        if(be_game_shave(&game,game.cursor_x,game.cursor_y)>0)
            upload_mesh(false);
    }
    be_game_tick(&game);
    return true;
}
int main(void){
    gfxInitDefault();
    consoleInit(GFX_BOTTOM,NULL);
    be_game_init(&game,&scene);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)){
        printf("Citro3D could not initialize.\n");
        gfxExit();return 1;
    }
    C3D_RenderTarget *target=C3D_RenderTargetCreate(
        240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    if(!target){
        printf("Unable to create 3DS GPU target.\n");
        C3D_Fini();gfxExit();return 1;
    }
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,DISPLAY_FLAGS);
    if(!scene_init()){
        printf("Failed to initialize Bald Elite 3DS.\n");
        scene_shutdown();C3D_Fini();gfxExit();return 1;
    }
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
        last_audio_phase=game.phase;
        status_display();
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target,C3D_CLEAR_ALL,BG_COLOR,0);
        C3D_FrameDrawOn(target);
        scene_draw();
        C3D_FrameEnd(0);
    }
    if(sound_available)be_audio_shutdown();
    scene_shutdown();
    C3D_Fini();
    gfxExit();
    return 0;
}
