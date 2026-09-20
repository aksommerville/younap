#include "game/younap.h"

#define GRAVITY_ACCEL 30.0 /* m/s**2 */
#define GRAVITY_LIMIT 10.0 /* m/s */
#define JUMP_DECEL    20.0 /* m/s**2 */
#define JUMP_INITIAL  10.0 /* m/s */

struct sprite_cat {
  struct sprite hdr;
  int input; // <0 if i'm not being controlled
  int sleeping;
  int seated;
  int jump_poison; // If nonzero, you must hit the ground again before jumping.
  double gravity; // m/s
  double jump_power; // m/s
};

#define SPRITE ((struct sprite_cat*)sprite)

/* Cleanup.
 */
 
static void _cat_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _cat_init(struct sprite *sprite) {
  SPRITE->input=-1;
  SPRITE->seated=1;
  SPRITE->jump_power=JUMP_INITIAL;
  return 0;
}

/* Update.
 */
 
static void _cat_update(struct sprite *sprite,double elapsed) {

  /* If I'm standing in a sunbeam, I fall asleep and if not, I wake up.
   * Regardless of whether I'm currently bound to an input.
   * And if I'm sleeping, no need for further activity.
   */
  if (SPRITE->seated) {
    int nsleeping=0;
    int floory=(int)(sprite->y+1.0);
    struct window *window=g.windowv;
    int i=g.windowc;
    for (;i-->0;window++) {
      if (floory!=window->floory) continue;
      if (sprite->x<window->beaml) continue;
      if (sprite->x>window->beamr) continue;
      nsleeping=1;
      break;
    }
    if (nsleeping) {
      SPRITE->input=-1;
      SPRITE->sleeping=1;
      return;
    }
    if (SPRITE->sleeping) {
      SPRITE->sleeping=0;
    }
  } else if (SPRITE->sleeping) {
    SPRITE->sleeping=0;
  }
  
  /* Gather input.
   */
  int indx=0;
  int indy=0;
  int injump=0;
  if ((SPRITE->input>=0)&&(SPRITE->input<INPUT_LIMIT)) {
    switch (g.input[SPRITE->input]&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
      case EGG_BTN_LEFT: indx=-1; break;
      case EGG_BTN_RIGHT: indx=1; break;
    }
    switch (g.input[SPRITE->input]&(EGG_BTN_UP|EGG_BTN_DOWN)) {
      case EGG_BTN_UP: indy=-1; break;
      case EGG_BTN_DOWN: indy=1; break;
    }
    injump=(g.input[SPRITE->input]&EGG_BTN_SOUTH)?1:0;
  }
  
  /* Gravity or jumping.
   */
  //XXX nope, remember? we're doing charged jumps
  if (injump&&(SPRITE->jump_power>0.0)) {
    SPRITE->jump_power-=JUMP_DECEL*elapsed;
    if (SPRITE->jump_power<=0.0) {
      // peaked
    } else {
      sprite_move(sprite,0.0,-SPRITE->jump_power*elapsed);
      SPRITE->seated=0;
      SPRITE->jump_poison=1;
    }
  } else {
    SPRITE->gravity+=elapsed*GRAVITY_ACCEL;
    if (SPRITE->gravity>GRAVITY_LIMIT) SPRITE->gravity=GRAVITY_LIMIT;
    if (sprite_move(sprite,0.0,SPRITE->gravity*elapsed)) {
      SPRITE->seated=0;
      SPRITE->jump_power=0.0;
    } else {
      SPRITE->gravity=0.0;
      SPRITE->seated=1;
      SPRITE->jump_poison=0;
      SPRITE->jump_power=JUMP_INITIAL;
    }
  }
  
  /*XXX move per dpad, just getting things to happen, not real
   */
  if (indx) {
    sprite->xform=(indx<0)?EGG_XFORM_XREV:0;
    sprite_move(sprite,6.000*elapsed*indx,0.0);
  }
  if (indy) {
    sprite_move(sprite,0.0,6.000*elapsed*indy);
  }
  
  //TODO
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

/* Public: Examine all cat sprites and ensure inputs are assigned sensibly.
 * I'm writing with an eye toward local multiplayer, tho we're not actually doing that today.
 */
 
void require_cat_inputs() {
  int assigned[INPUT_LIMIT]={0}; // index is playerid
  struct sprite *available=0; // Null or an awake cat with no input assigned.
  struct sprite **spritep=spritev;
  int spritei=spritec;
  for (;spritei-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (sprite->type!=&sprite_type_cat) continue;
    
    if ((SPRITE->input>=0)&&(SPRITE->input<INPUT_LIMIT)) {
      if (assigned[SPRITE->input]) { // Collision! Drop this one.
        SPRITE->input=-1;
      } else {
        assigned[SPRITE->input]=1;
      }
    } else {
      if (!available) {
        if (!SPRITE->sleeping) {
          available=sprite;
        }
      }
    }
  }
  // If we didn't get a [1] or [0], assign [1] to anyone available.
  if (!assigned[0]&&!assigned[1]) {
    if (available) {
      struct sprite *sprite=available;
      SPRITE->input=1;
    }
  }
}
