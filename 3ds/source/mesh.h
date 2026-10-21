/* Bald Elite 3DS head geometry. Standalone, portable C99 (no PSP APIs). */
#ifndef BALD_ELITE_3DS_MESH_H
#define BALD_ELITE_3DS_MESH_H

#define BE_RINGS 32
#define BE_SLICES 48
#define BE_HEAD_VERTS (BE_RINGS*BE_SLICES*6 + BE_SLICES*3)
#define BE_MAX_HAIR 1250
#define BE_HAIR_VERTS (BE_MAX_HAIR*6)
#define BE_CANDIDATES 3

typedef struct {float x,y,z, r,g,b,a;} BeVertex;
typedef struct {
    float x,y,z, nx,ny,nz, length;
    unsigned char gone;
} BeHair;
typedef struct {
    BeVertex head[BE_HEAD_VERTS];
    BeVertex hair[BE_HAIR_VERTS];
    BeHair strands[BE_MAX_HAIR];
    int head_count, hair_count, removed, candidate;
} BeMesh;

extern const char *be_candidate_names[BE_CANDIDATES];

void be_mesh_generate(BeMesh *m, int candidate);
void be_mesh_rebuild_hair(BeMesh *m);
#endif
