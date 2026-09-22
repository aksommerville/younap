#include "younap.h"

/* Far background. The sky.
 */
 
void render_far_bg() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x80c0ffff);//TODO
}

/* Map cells.
 */
 
void render_map() {
  graf_set_image(&g.graf,RID_image_sprites);
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

  // XXX Probably no need to draw highlights on the floor.
  uint32_t floor_color=0xffffffff;
  graf_set_input(&g.graf,0);
  struct window *window=g.windowv;
  int i=g.windowc;
  /**
  for (;i-->0;window++) {
    graf_line(&g.graf,
      (int)(window->beaml*NS_sys_tilesize),window->floory*NS_sys_tilesize+2,floor_color,
      (int)(window->beamr*NS_sys_tilesize),window->floory*NS_sys_tilesize+2,floor_color
    );
  }
  /**/
  
  uint32_t ray_color=0xffff0080;
  for (window=g.windowv,i=g.windowc;i-->0;window++) {
    if (window->beaml<window->x) {
      graf_triangle_strip_begin(&g.graf,
        window->x*NS_sys_tilesize,window->y*NS_sys_tilesize,ray_color,
        (int)(window->beaml*NS_sys_tilesize),window->floory*NS_sys_tilesize,ray_color,
        window->x*NS_sys_tilesize,(window->y+window->h)*NS_sys_tilesize,ray_color
      );
      graf_triangle_strip_more(&g.graf,(int)(window->beamr*NS_sys_tilesize),window->floory*NS_sys_tilesize,ray_color);
      graf_triangle_strip_more(&g.graf,(window->x+window->w)*NS_sys_tilesize,(window->y+window->h)*NS_sys_tilesize,ray_color);
    } else {
      graf_triangle_strip_begin(&g.graf,
        window->x*NS_sys_tilesize,(window->y+window->h)*NS_sys_tilesize,ray_color,
        (int)(window->beaml*NS_sys_tilesize),window->floory*NS_sys_tilesize,ray_color,
        (window->x+window->w)*NS_sys_tilesize,(window->y+window->h)*NS_sys_tilesize,ray_color
      );
      graf_triangle_strip_more(&g.graf,(int)(window->beamr*NS_sys_tilesize),window->floory*NS_sys_tilesize,ray_color);
      graf_triangle_strip_more(&g.graf,(window->x+window->w)*NS_sys_tilesize,window->y*NS_sys_tilesize,ray_color);
    }
  }
}

/* Sprites.
 */

void render_sprites() {
  struct sprite **spritep=spritev;
  int spritei=spritec;
  for (;spritei-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    int x=(int)(sprite->x*NS_sys_tilesize);
    int y=(int)(sprite->y*NS_sys_tilesize);
    if (sprite->type->render) {
      sprite->type->render(sprite,x,y);
    } else {
      graf_set_image(&g.graf,sprite->imageid);
      graf_tile(&g.graf,x,y,sprite->tileid,sprite->xform);
    }
  }
}

/* Overlay: Clock, score, etc.
 */
 
void render_overlay() {
  graf_set_image(&g.graf,RID_image_fonttiles);
  
  /* Clock in the middle top.
   */
  {
    int y=13;
    int s=(int)(g.levelclock+0.999);
    if (s<0) s=0; else if (s>99) s=99;
    if (s>=10) {
      graf_tile(&g.graf,(FBW>>1)-6,y,'0'+s/10,0);
      graf_tile(&g.graf,(FBW>>1)+6,y,'0'+s%10,0);
    } else {
      graf_tile(&g.graf,(FBW>>1),y,'0'+s,0);
    }
  }
  
  /* Score readout at the bottom.
   */
  {
    double range=g.leveltime*g.catc;
    range*=0.750; // Don't show the full technically-possible range; it's not actually possible to fill that, ever.
    double n=g.score/range;
    int spacing=14;
    int zc=FBW/spacing;
    int y=FBH-13;
    int x=10;
    int hotc=(int)(zc*n);
    if (hotc<0) hotc=0;
    else if (hotc>zc) hotc=zc;
    int coldc=zc-hotc;
    for (;hotc-->0;x+=spacing) graf_tile(&g.graf,x,y,'Z',0);
    graf_set_tint(&g.graf,0x404040ff);
    for (;coldc-->0;x+=spacing) graf_tile(&g.graf,x,y,'Z',0);
    graf_set_tint(&g.graf,0);
  }
}
