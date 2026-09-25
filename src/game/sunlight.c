#include "younap.h"

/* Advance the sun.
 */
 
void advance_sun(double elapsed) {
  g.levelclock+=elapsed;
  g.sunp+=g.sundp*elapsed;
  if (g.sunp<0.0) {
    g.sunp=0.0;
    if (g.sundp<0.0) g.sundp=-g.sundp;
  } else if (g.sunp>1.0) {
    g.sunp=1.0;
    if (g.sundp>0.0) g.sundp=-g.sundp;
  }
}

/* Regenerate spots.
 */
 
void regenerate_spots() {
  // Throughout the day, the sun's angle shifts from 3/4 pi to 5/4 pi, ie one downward diagonal to the other.
  const double tlo=M_PI*0.750;
  const double thi=M_PI*1.250;
  double t=tlo*(1.0-g.sunp)+thi*g.sunp;
  double nx=sin(t);
  double ny=-cos(t);
  struct window *window=g.windowv;
  int i=g.windowc;
  for (;i-->0;window++) {
    double ax=window->x;
    double bx=window->x+window->w;
    double ay,by;
    if (nx<0.0) {
      ay=window->y;
      by=window->y+window->h;
    } else {
      ay=window->y+window->h;
      by=window->y;
    }
    double cy=window->floory;
    window->beaml=ax+((cy-ay)*nx)/ny;
    window->beamr=bx+((cy-by)*nx)/ny;
  }
}

/* Poll for completion.
 */
 
void check_level_completion(double elapsed) {
  int win=0;
  
  int all_sleep=1;
  struct sprite **p=spritev;
  int i=spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct||(sprite->type!=&sprite_type_cat)) continue;
    if (!sprite_cat_is_sleeping(sprite)) {
      all_sleep=0;
      break;
    }
  }
  if (all_sleep) {
    g.all_sleep_time+=elapsed;
    if (g.all_sleep_time>=1.0) win=1;
  } else {
    g.all_sleep_time=0.0;
  }
  
  if (win) {
    g.level_report=1;
  }
}
