/* Run on Linux/macOS using a normal C compiler, without any Nintendo SDK.
 * Tests real 3DS port mesh generation, matching candidate selection from PSP.
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../source/mesh.h"

static BeMesh mesh; /* don't put a ~0.5 MB mesh on a tiny test thread stack */

int main(void){
    assert(sizeof(BeVertex)==9*sizeof(float));
    for(int candidate=0;candidate<BE_CANDIDATES;candidate++){
        be_mesh_generate(&mesh,candidate);
        assert(mesh.candidate==candidate);
        assert(mesh.head_count==BE_HEAD_VERTS);
        assert(mesh.head_count>BE_RINGS*BE_SLICES*5);
        assert(mesh.hair_count>300 && mesh.hair_count<=BE_MAX_HAIR);
        for(int j=0;j<mesh.head_count;j++){
            const BeVertex *v=&mesh.head[j];
            assert(isfinite(v->x)&&isfinite(v->y)&&isfinite(v->z));
            assert(isfinite(v->r)&&isfinite(v->g)&&isfinite(v->b));
            assert(v->r>=0 && v->r<=1 && v->g>=0 && v->g<=1);
            assert(v->u>=0 && v->u<=1 && v->v>=0 && v->v<=1.0001f);
        }
        for(int j=0;j<mesh.hair_count;j++){
            const BeHair *h=&mesh.strands[j];
            assert(isfinite(h->x)&&isfinite(h->length));
            assert(h->u>=0 && h->u<=1 && h->v>=0 && h->v<=1);
            assert(h->length>.02f && h->length<.21f);
            if(candidate==1)assert(fabsf(h->x)<=.281f);
            if(candidate==2){
                assert(!(h->y>.86f && fabsf(h->x)<.58f));
            }
        }
        BeHair first=mesh.strands[0];
        int count=mesh.hair_count;
        be_mesh_generate(&mesh,candidate);
        assert(mesh.hair_count==count);
        assert(mesh.strands[0].x==first.x);
        assert(mesh.strands[0].length==first.length);
    }
    be_mesh_generate(&mesh,0);
    assert(mesh.hair_count==900);
    be_mesh_generate(&mesh,1);
    assert(mesh.hair_count==BE_MAX_HAIR);
    be_mesh_generate(&mesh,2);
    assert(mesh.hair_count==760);
    puts("PASS: 3DS detailed PSP face atlas, scalp vertices and three hairstyles.");
    return 0;
}
