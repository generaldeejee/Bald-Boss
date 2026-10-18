/* Real host-side geometry/gameplay regression test using C source itself.
 * Build with tools/syntax_check.py's generated PSP header shims and section GC.
 * No PSP graphics emulator is involved. */
#define main game_psp_main
#include "../src/main.c"
#undef main
#include <assert.h>
int main(void){
  make_textures();make_head();make_hairs();
  assert(n_head==HR*HS*6);
  assert(n_hairs>700&&n_hairs<=HAIRS);
  for(int i=0;i<n_head;i++){
    assert(isfinite(head[i].x)&&isfinite(head[i].y)&&isfinite(head[i].z));
    assert(isfinite(head[i].nx)&&isfinite(head[i].ny)&&isfinite(head[i].nz));
  }
  Hair *h=&hairs[0];PosVertex a[3],b[3];
  hair_vertices(a,h,0);hair_vertices(b,h,0);
  assert(memcmp(a,b,sizeof(a))==0);
  reset_game();assert(removed==0&&score==0&&phase==1);
  for(int deg=0;deg<360;deg+=30){
    yaw=deg*(PI/180.0f);
    float s=sinf(yaw),c=cosf(yaw);
    int hits_before=removed;
    for(int i=0;i<n_hairs;i++){
      Hair *item=&hairs[i];if(item->gone)continue;
      float z=item->z*c-item->x*s,x=item->x*c+item->z*s;
      if(item->nz*c-item->nx*s<0.40f)continue;
      cursor_x=240.0f+303.0f*x/(3.55f-z);
      cursor_y=136.0f-303.0f*(item->y-.03f)/(3.55f-z);
      shave_at_cursor();break;
    }
    assert(removed>hits_before);
  }
  assert(score==removed*20);
  reset_game();assert(removed==0&&score==0);
  puts("PASS: closed mesh count, finite normals, stable hair, 360-degree shave, reset.");
  return 0;
}
