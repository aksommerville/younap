#include "game/younap.h"

/* Move sprite, with map collisions.
 * Might not hold forever, but I'm thinking today we won't need sprite-on-sprite collisions.
 */
 
int sprite_move(struct sprite *sprite,double dx,double dy) {

  /* If they requested two axes of motion, resolve them independently.
   * Or if both axes are zero, we're done.
   */
  if ((dx<-0.0)||(dx>0.0)) {
    if ((dy<-0.0)||(dy>0.0)) {
      int xresult=sprite_move(sprite,dx,0.0);
      int yresult=sprite_move(sprite,0.0,dy);
      if (xresult||yresult) return 1;
      return 0;
    }
  } else if ((dy<-0.0)||(dy>0.0)) {
  } else {
    return 0;
  }
  
  /* Hitbox size.
   * For now it's always a perfect meter square.
   */
  const double hbl=-0.5;
  const double hbr= 0.5;
  const double hbt=-0.250;
  const double hbb= 0.5;
  
  /* Eagerly take the entire step.
   * If really large steps ever become possible, we might have to do something more complicated.
   */
  double nx=sprite->x+dx;
  double ny=sprite->y+dy;
  double bl,br,bt,bb;
  #define REBOX { \
    bl=nx+hbl; \
    br=nx+hbr; \
    bt=ny+hbt; \
    bb=ny+hbb; \
    /* Squeeze the off-axis in just a smidgeon. */ \
    if ((dx<-0.0)||(dx>0.0)) { \
      bt+=0.001; \
      bb-=0.001; \
    } else { \
      bl+=0.001; \
      br-=0.001; \
    } \
  }
  REBOX
  #define VALIDATE { \
    if (dx<0.0) { \
      if (nx>=sprite->x) return 0; \
    } else if (dx>0.0) { \
      if (nx<=sprite->x) return 0; \
    } else if (dy<0.0) { \
      if (ny>=sprite->y) return 0; \
    } else { \
      if (ny<=sprite->y) return 0; \
    } \
    REBOX \
  }
  
  // Not checking edges. I'm thinking we'll design maps without any exposed edges.
  
  /* Check the map where our provisional hitbox intersects it.
   */
  int cola=(int)(bl); if (cola<0) cola=0;
  int colz=(int)(br); if (colz>=NS_sys_mapw) colz=NS_sys_mapw-1;
  int rowa=(int)(bt); if (rowa<0) rowa=0;
  int rowz=(int)(bb); if (rowz>=NS_sys_maph) rowz=NS_sys_maph-1;
  int col=cola; for (;col<=colz;col++) {
    int row=rowa; for (;row<=rowz;row++) {
      uint8_t ph=g.physics[g.cellv[row*NS_sys_mapw+col]];
      if (ph==NS_physics_solid) {
             if (dx<0.0) nx=col+1.0-hbl;
        else if (dx>0.0) nx=col-hbr;
        else if (dy<0.0) ny=row+1.0-hbt;
        else if (dy>0.0) ny=row-hbb;
        VALIDATE
      } else if (ph==NS_physics_oneway) {
        if (dy>0.0) {
          int prow=(int)(sprite->y+hbb-0.001);
          if (prow<row) { // Oneway and our toes just crossed into it.
            ny=row-hbb;
            VALIDATE
          }
        }
      }
    }
  }
  
  /* Accept the move.
   */
  #undef REBOX
  #undef VALIDATE
  sprite->x=nx;
  sprite->y=ny;
  return 1;
}
