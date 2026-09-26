/* sprite_fish.c
 * Bonus collectible.
 */
 
#include "game/younap.h"

#define FRAMEC 4
#define FRAME_INTERVAL 0.250
#define PICKUP_RADIUS 0.750 /* m */
#define PICKUP_RADIUS_2 (PICKUP_RADIUS*PICKUP_RADIUS)

struct sprite_fish {
  struct sprite hdr;
  uint8_t tileid0;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_fish*)sprite)

static int _fish_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  SPRITE->animframe=rand()%FRAMEC;
  SPRITE->animclock=((rand()&0xffff)*FRAME_INTERVAL)/65535.0;
  return 0;
}

static void _fish_update(struct sprite *sprite,double elapsed) {
  struct sprite **catp=spritev; // ew cat pee
  int cati=spritec;
  for (;cati-->0;catp++) {
    struct sprite *cat=*catp;
    if (cat->defunct||(cat->type!=&sprite_type_cat)) continue;
    double dx=cat->x-sprite->x;
    double dy=cat->y-sprite->y;
    double d2=dx*dx+dy*dy;
    if (d2<PICKUP_RADIUS_2) {
      SND(bonus)
      sprite->defunct=1;
      g.fishc_level++;
      return;
    }
  }
}

const struct sprite_type sprite_type_fish={
  .name="fish",
  .objlen=sizeof(struct sprite_fish),
  .init=_fish_init,
  .update=_fish_update,
};
