#include "younap.h"

/* Advance the sun.
 */
 
void advance_sun(double elapsed) {
  if ((g.levelclock-=elapsed)<=0.0) {
    fprintf(stderr,"%s:%d:TODO: End of level.\n",__FILE__,__LINE__);//TODO interlevel fanfare, check game over, etc
    if (game_start_level(g.mapid)<0) {
      egg_terminate(1);
      return;
    }
  }
  g.sunp=1.0-g.levelclock/g.leveltime;
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
