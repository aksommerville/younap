#include "younap.h"

/* Prerender map.
 */
 
void prerender_map() {
  egg_texture_clear(g.bgtexid);
  graf_flush(&g.graf);
  graf_reset(&g.graf);
  graf_set_output(&g.graf,g.bgtexid);
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
  graf_set_output(&g.graf,1);
}

/* Far background. The sky.
 */
 
void render_far_bg() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x80c0ffff);//TODO
}

/* Map cells.
 */
 
void render_map() {
  graf_set_input(&g.graf,g.bgtexid);
  graf_decal(&g.graf,0,0,0,0,FBW,FBH);
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
    int ms=(int)(g.levelclock*1000.0);
    int s=ms/1000; ms%=1000;
    int m=s/60; s%=60;
    if (m>99) { // If you take an hour and a half to play one level, I'm not really worried about what we show on the clock.
      m=99;
      s=99;
      ms=0;
    }
    int x=(FBW>>1)-24;
    if (m>=10) { // Minute tens digit shouldn't come up most of the time. It's off center, whatever.
      graf_tile(&g.graf,x,y,'0'+m/10,0); x+=12;
    }
    graf_tile(&g.graf,x,y,'0'+m%10,0); x+=12;
    if (ms<800) graf_tile(&g.graf,x,y,':',0); x+=12;
    graf_tile(&g.graf,x,y,'0'+s/10,0); x+=12;
    graf_tile(&g.graf,x,y,'0'+s%10,0);
  }
  
  /* Score readout at the bottom.
   */
  {
    double range=1.0;
    double n=g.all_sleep_time;
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
