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

static void fish_animate(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=FRAME_INTERVAL;
    if (++(SPRITE->animframe)>=FRAMEC) SPRITE->animframe=0;
    sprite->tileid=SPRITE->tileid0+FRAMEC;
  }
}

static void _fish_update(struct sprite *sprite,double elapsed) {
  fish_animate(sprite,elapsed);
  
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

static void _fish_update_bg(struct sprite *sprite,double elapsed) {
  fish_animate(sprite,elapsed);
}

//XXX Delete this once tiles are available.
static void _fish_render(struct sprite *sprite,int x,int y) {
  const uint32_t colorv[4]={0x800000ff,0x804000ff,0x808000ff,0x804000ff};
  graf_fill_rect(&g.graf,x-4,y-4,8,8,colorv[SPRITE->animframe]);
}

const struct sprite_type sprite_type_fish={
  .name="fish",
  .objlen=sizeof(struct sprite_fish),
  .init=_fish_init,
  .update=_fish_update,
  .update_bg=_fish_update_bg,
  .render=_fish_render,
};
