#include "game/younap.h"

struct sprite_cat {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_cat*)sprite)

/* Cleanup.
 */
 
static void _cat_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _cat_init(struct sprite *sprite) {
  return 0;
}

/* Update.
 */
 
static void _cat_update(struct sprite *sprite,double elapsed) {
}

/* Render.
 */
 
static void _cat_render(struct sprite *sprite,int x,int y) {
  graf_set_image(&g.graf,sprite->imageid);
  graf_tile(&g.graf,x,y,sprite->tileid,sprite->xform);
}

/* Type definition.
 */

const struct sprite_type sprite_type_cat={
  .name="cat",
  .objlen=sizeof(struct sprite_cat),
  .del=_cat_del,
  .init=_cat_init,
  .update=_cat_update,
  .render=_cat_render,
};
