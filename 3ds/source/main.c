/* Bald Elite 3DS - milestone 1: real PICA200 head + hairstyle renderer.
 * The existing PSP game is untouched. First target: original Nintendo 3DS,
 * single top screen and bottom console. Not yet full shaving gameplay.
 */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "mesh.h"
#include "vshader_shbin.h"

#define BG_COLOR 0x142537FF
#define DISPLAY_FLAGS (GX_TRANSFER_FLIP_VERT(0) | \
 GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
 GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
 GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
 GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

/* Static data lives in the app's BSS, not on the small 3DS thread stack. */
static BeMesh scene;
static BeVertex *head_vbo=NULL;
static BeVertex *hair_vbo=NULL;
static DVLB_s *vshader=NULL;
static shaderProgram_s program;
static bool program_ready=false;
static C3D_Mtx projection;
static int loc_projection=-1,loc_modelview=-1;

static float yaw=0.0f,pitch=0.0f;
static int candidate=0,frames=0;
static bool auto_spin=false;

static void upload_mesh(void){
    be_mesh_generate(&scene,candidate);
    memcpy(head_vbo,scene.head,scene.head_count*sizeof(BeVertex));
    memcpy(hair_vbo,scene.hair,scene.hair_count*6*sizeof(BeVertex));
    GSPGPU_FlushDataCache(head_vbo,scene.head_count*sizeof(BeVertex));
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
    AttrInfo_AddLoader(attribs,0,GPU_FLOAT,3); /* v0: XYZ */
    AttrInfo_AddLoader(attribs,1,GPU_FLOAT,4); /* v1: RGBA */

    C3D_TexEnv *env=C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env,C3D_Both,GPU_PRIMARY_COLOR,0,0);
    C3D_TexEnvFunc(env,C3D_Both,GPU_REPLACE);

    /* Standard single-eye 3DS top-screen perspective. */
    Mtx_PerspTilt(&projection,C3D_AngleFromDegrees(50.0f),
                  C3D_AspectRatioTop,.1f,40.0f,false);
    upload_mesh();
    return true;
}

static void scene_draw(void){
    C3D_Mtx view;
    Mtx_Identity(&view);
    Mtx_Translate(&view,0.0f,-.06f,-3.8f,true);
    Mtx_RotateX(&view,pitch,true);
    Mtx_RotateY(&view,yaw,true);

    C3D_BindProgram(&program);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,loc_projection,&projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,loc_modelview,&view);

    C3D_BufInfo *buffers=C3D_GetBufInfo();
    BufInfo_Init(buffers);
    BufInfo_Add(buffers,head_vbo,sizeof(BeVertex),2,0x10);
    C3D_DrawArrays(GPU_TRIANGLES,0,scene.head_count);

    /* Draw separate candidate hair mesh through the same shader. */
    BufInfo_Init(buffers);
    BufInfo_Add(buffers,hair_vbo,sizeof(BeVertex),2,0x10);
    C3D_DrawArrays(GPU_TRIANGLES,0,scene.hair_count*6);
}

static void scene_shutdown(void){
    if(program_ready)shaderProgramFree(&program);
    if(vshader)DVLB_Free(vshader);
    if(head_vbo)linearFree(head_vbo);
    if(hair_vbo)linearFree(hair_vbo);
}

static void display_status(void){
    /* Bottom console refresh only every ~1/4 second. Top remains smooth. */
    if(frames%15!=0)return;
    printf("\x1b[1;1H\x1b[2J");
    printf("       BALD ELITE 3DS\n");
    printf("    --------------------\n\n");
    printf(" PORT MILESTONE 1\n");
    printf(" Native PICA200 head / hair\n\n");
    printf(" APPLICANT %d / %d\n",candidate+1,BE_CANDIDATES);
    printf(" %s\n",be_candidate_names[candidate]);
    printf(" Visible hair: %d\n\n",scene.hair_count);
    printf(" LEFT/RIGHT  change applicant\n");
    printf(" L / R       rotate head\n");
    printf(" UP/DOWN     tilt head\n");
    printf(" X           automatic spin\n");
    printf(" START       exit to Homebrew\n\n");
    printf(" Work in progress!\n");
    printf(" No clipping, scores or audio yet.\n");
    fflush(stdout);
}

int main(void){
    gfxInitDefault();
    consoleInit(GFX_BOTTOM,NULL);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)){
        printf("Citro3D initialization failed.\n");
        gfxExit();return 1;
    }
    C3D_RenderTarget *target=C3D_RenderTargetCreate(
        240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    if(!target){
        printf("Unable to create the 3DS render target.\n");
        C3D_Fini();gfxExit();return 1;
    }
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,DISPLAY_FLAGS);
    if(!scene_init()){
        printf("Unable to initialize Bald Elite mesh/shader.\n");
        scene_shutdown();C3D_Fini();gfxExit();return 1;
    }
    while(aptMainLoop()){
        hidScanInput();
        u32 down=hidKeysDown(),held=hidKeysHeld();
        if(down&KEY_START)break;
        if(down&KEY_LEFT){candidate=(candidate+BE_CANDIDATES-1)%BE_CANDIDATES;upload_mesh();}
        if(down&KEY_RIGHT){candidate=(candidate+1)%BE_CANDIDATES;upload_mesh();}
        if(down&KEY_X)auto_spin=!auto_spin;
        if(held&KEY_L)yaw-=.035f;
        if(held&KEY_R)yaw+=.035f;
        if(held&KEY_UP)pitch+=.025f;
        if(held&KEY_DOWN)pitch-=.025f;
        if(pitch>.9f)pitch=.9f;
        if(pitch<-.6f)pitch=-.6f;
        if(auto_spin)yaw+=.008f;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target,C3D_CLEAR_ALL,BG_COLOR,0);
        C3D_FrameDrawOn(target);
        scene_draw();
        C3D_FrameEnd(0);
        frames++;
        display_status();
    }
    scene_shutdown();
    C3D_Fini();
    gfxExit();
    return 0;
}
