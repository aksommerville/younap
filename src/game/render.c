#include "younap.h"

/* Far background. The sky.
 */
 
void render_far_bg() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x80c0ffff);//TODO
}

/* Map cells.
 */
 
void render_map() {
  graf_set_image(&g.graf,RID_image_scratch);
  int dstx0=NS_sys_tilesize>>1;
  int dsty=NS_sys_tilesize>>1;
  const uint8_t *src=g.cellv;
  int yi=NS_sys_maph;
  for (;yi-->0;dsty+=NS_sys_tilesize) {
    int dstx=dstx0;
    int xi=NS_sys_mapw;
    for (;xi-->0;dstx+=NS_sys_tilesize,src++) {
      graf_tile(&g.graf,dstx,dsty,*src,0);
    }
  }
}

/* Sunbeams.
 * After map, before sprites.
 */
 
void render_sunbeams() {
  //TODO
}

/* Sprites.
 */

void render_sprites() {
  //TODO
}
