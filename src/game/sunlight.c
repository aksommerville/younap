#include "younap.h"

/* Manifold for describing a ray's collision with the map.
 */

#define SIDE_LEFT   1
#define SIDE_RIGHT  2
#define SIDE_TOP    3
#define SIDE_BOTTOM 4
 
struct rayend {
  double x,y; // Collision point, map meters.
  int qx,qy; // Cell position. May be OOB by one.
  int side; // SIDE_*
};

/* Extend a ray from (x,y) along (nx,ny) until it touches a solid cell.
 * Returns nonzero if valid, or zero if anything goes wrong, eg runs off map edge.
 * (x,y) must be within the map.
 * (nx,ny) must have magnitude 1.
 */
 
static int find_rayend(struct rayend *rayend,double x,double y,double nx,double ny) {

  /* If either normal component is very close to zero, treat it as axis-aligned.
   * Zeroes will break the math below. And axis-aligned ray tracing is easy, to say the least.
   */
  const double SMALL=0.001;
  if ((nx>-SMALL)&&(nx<SMALL)) { // Vertical line.
    int qx=(int)x;
    if ((qx<0)||(qx>=NS_sys_mapw)) return 0;
    int qy=(int)y;
    if (ny<0.0) {
      qy--;
      for (;;qy--) {
        if (qy<0) return 0;
        switch (g.physics[g.cellv[qy*NS_sys_mapw+qx]]) {
          case NS_physics_solid: {
              rayend->x=x;
              rayend->y=qy+1.0;
              rayend->qx=qx;
              rayend->qy=qy;
              rayend->side=SIDE_BOTTOM;
            } return 1;
        }
      }
    } else {
      for (;;qy++) {
        if (qy>=NS_sys_maph) return 0;
        switch (g.physics[g.cellv[qy*NS_sys_mapw+qx]]) {
          case NS_physics_solid: {
              rayend->x=x;
              rayend->y=qy;
              rayend->qx=qx;
              rayend->qy=qy;
              rayend->side=SIDE_TOP;
            } return 1;
        }
      }
    }
  } else if ((ny>-SMALL)&&(ny<SMALL)) { // Horizontal line.
    int qy=(int)y;
    if ((qy<0)||(qy>=NS_sys_maph)) return 0;
    int qx=(int)x;
    if (nx<0.0) {
      qx--;
      for (;;qx--) {
        if (qx<0) return 0;
        switch (g.physics[g.cellv[qy*NS_sys_mapw+qx]]) {
          case NS_physics_solid: {
              rayend->x=qx+1.0;
              rayend->y=y;
              rayend->qx=qx;
              rayend->qy=qy;
              rayend->side=SIDE_RIGHT;
            } return 1;
        }
      }
    } else {
      for (;;qx++) {
        if (qx>=NS_sys_mapw) return 0;
        switch (g.physics[g.cellv[qy*NS_sys_mapw+qx]]) {
          case NS_physics_solid: {
              rayend->x=qx;
              rayend->y=y;
              rayend->qx=qx;
              rayend->qy=qy;
              rayend->side=SIDE_LEFT;
            } return 1;
        }
      }
    }
  }

  // Diagonals, the usual case.
  for (;;) {
    
    // Which grid lines are the next that we cross?
    int nextx=(nx<0.0)?((int)x-1):((int)x+1);
    int nexty=(ny<0.0)?((int)y-1):((int)y+1);
    
    // Find each crossing.
    double xdx=nextx-x;
    double xdy=(xdx*ny)/nx;
    double ydy=nexty-y;
    double ydx=(ydy*nx)/ny;
    
    // The square magnitudes of those crossing deltas tell us which is nearer.
    double xd2=xdx*xdx+xdy*xdy;
    double yd2=ydx*ydx+ydy*ydy;
    if (xd2<yd2) { // X crossing is nearer.
      rayend->qx=nexty;
      rayend->qy=(int)y;
      x=nextx;
      y+=xdy;
      if ((rayend->qx<0)||(rayend->qx>=NS_sys_mapw)||(rayend->qy<0)||(rayend->qy>=NS_sys_maph)) return 0;
      switch (g.physics[g.cellv[rayend->qy*NS_sys_mapw+rayend->qx]]) {
        case NS_physics_solid: {
            rayend->x=x;
            rayend->y=y;
            rayend->side=(nx<0.0)?SIDE_RIGHT:SIDE_LEFT;
          } return 1;
      }
    } else { // Y crossing is nearer.
      rayend->qx=(int)x;
      rayend->qy=nexty;
      x+=ydx;
      y=nexty;
      if ((rayend->qx<0)||(rayend->qx>=NS_sys_mapw)||(rayend->qy<0)||(rayend->qy>=NS_sys_maph)) return 0;
      switch (g.physics[g.cellv[rayend->qy*NS_sys_mapw+rayend->qx]]) {
        case NS_physics_solid: {
            rayend->x=x;
            rayend->y=y;
            rayend->side=(ny<0.0)?SIDE_BOTTOM:SIDE_TOP;
          } return 1;
        case NS_physics_oneway: if (ny>0.0) {
            rayend->x=x;
            rayend->y=y;
            rayend->side=SIDE_TOP;
            return 1;
          } break;
      }
    }
  }
}

/* Trace one rayend against the map until interrupted (ei

/* Given two corners of a window and a normal for the sunbeam's direction.
 * Trace a closed polygon along the floor that meets both rays.
 * (a) must be left of (b).
 */
 
static void regenerate_spots_from_corners(
  double ax,double ay,
  double bx,double by,
  double nx,double ny
) {
  // Abort if either endpoint is outside the map, or on the edge.
  if ((ax<=0.0)||(ax>=NS_sys_mapw)||(ay<=0.0)||(ay>=NS_sys_maph)) return;
  if ((bx<=0.0)||(bx>=NS_sys_mapw)||(by<=0.0)||(by>=NS_sys_maph)) return;
  if (ax>=bx) return;
  if (ny<0.100) return; // Also a sanity check, we assume that (ny) is positive, and substantially so.
  
  // Trace both corners to their map collision.
  struct rayend rea,reb;
  if (!find_rayend(&rea,ax,ay,nx,ny)) return;
  if (!find_rayend(&reb,bx,by,nx,ny)) return;
  
  /* Now trace the floor from (rea) rightward to (reb).
   */
  int panic=100;
  while (panic-->0) {
    // If this cross-product goes positive or zero, (rea) has passed (reb) and we're done.
    double cp=(rea.x-bx)*(reb.y-by)-(rea.y-by)*(reb.x-bx);
    if (cp>=0.0) break;
    
    // Trace whatever we landed on until it corners away or smacks a wall.
    struct rayend rea_next;
    if (!trace_rayend_against_map(&rea_next,&rea)) break;
    
    // If (rea_next) crosses (reb), shorten it and note that we're done after this. We don't need the quantized positions.
    int term=0;
    if (rea_next.side==SIDE_TOP) {
      if (rea_next.x>=reb.x) {
        rea_next.x=reb.x;
        term=1;
      }
    } else {
      if (rea_next.y<=reb.y) {
        rea_next.y=reb.y;
        term=1;
      }
    }
    
    // If it's a SIDE_TOP collision, produce a spot.
    if (rea.side==SIDE_TOP) {
      if (g.spotc<SPOT_LIMIT) {
        g.spotv[g.spotc++]=(struct spot){rea.y,rea.x,rea_next.x};
      }
    }
    
    // Terminate if we reached the other side.
    if (term) break;
    
    // Move (rea) to (rea_next). It's the right position now, but the axis needs to swap.
    rea=rea_next;
    ...
  }
}

/* Regenerate spots.
 */
 
void regenerate_spots() {
  g.spotc=0;
  
  /* Arguably, we should project a unique ray for each outside window corner, from the sun's center.
   * But I think it might be even better to do it naively: All sunbeams have exactly the same angle.
   * Because in real life, the sun is far away.
   */
  double ny=sin(g.sunt);
  if (ny<0.100) return; // Negative or very slanted beams, don't even bother.
  double nx=cos(g.sunt);
  double xpery=nx/ny;
  struct window *window=g.windowv;
  int i=g.windowc;
  if (nx<0.0) {
    for (;i-->0;window++) {
      regenerate_spots_from_corners(
        window->x,window->y,
        window->x+window->w,window->y+window->h,
        nx,ny
      );
    }
  } else {
    for (;i-->0;window++) {
      regenerate_spots_from_corners(
        window->x,window->y+window->h,
        window->x+window->w,window->y,
        nx,ny
      );
    }
  }
}
