/* Bald Elite 3DS head geometry. Standalone, portable C99 (no PSP APIs). */
#ifndef BALD_ELITE_3DS_MESH_H
#define BALD_ELITE_3DS_MESH_H

#define BE_RINGS 64
#define BE_SLICES 96
#define BE_HEAD_VERTS (BE_RINGS*BE_SLICES*6 + BE_SLICES*3)
#define BE_MAX_HAIR 1250
#define BE_HAIR_VERTS (BE_MAX_HAIR*6)
#define BE_CANDIDATES 3

typedef struct {float x,y,z, r,g,b,a, u,v;} BeVertex;
typedef struct {
    float x,y,z, nx,ny,nz, length, u,v;
    unsigned char gone;
} BeHair;
typedef struct {
    BeVertex head[BE_HEAD_VERTS];
    BeVertex hair[BE_HAIR_VERTS];
    BeHair strands[BE_MAX_HAIR];
    /* PSP-style 256x256 face atlas and scalp polish coverage. */
    unsigned short skin_base[256*256];
    unsigned char shaved_coverage[256*256];
    int head_count, hair_count, removed, candidate;
} BeMesh;

extern const char *be_candidate_names[BE_CANDIDATES];

void be_mesh_generate(BeMesh *m, int candidate);
void be_mesh_rebuild_hair(BeMesh *m);
void be_mesh_polish_hair(BeMesh *m,const BeHair *h);
void be_mesh_recolor_head(BeMesh *m);
#endif
