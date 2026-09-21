#include "game/younap.h"

#define GRAVITY_ACCEL 30.0 /* m/s**2 */
#define GRAVITY_LIMIT 10.0 /* m/s */
#define JUMP_CHARGE   10.0 /* m/s**2 */
#define JUMP_LIMIT    10.0 /* m/s */
#define JUMP_MIN       4.0 /* m/s ; what you get if there's exactly one frame of charge */
#define JUMP_DECEL    20.0 /* m/s**2 */

// Enumerate the various faces, so we can detect changes without checking a bunch of different state every time.
#define FACE_IDLE 0
#define FACE_WALK 1
#define FACE_CHARGE 2
#define FACE_JUMP 3
#define FACE_FALL 4
#define FACE_CLIMB 5
#define FACE_SLEEP 6

struct sprite_cat {
  struct sprite hdr;
  int input; // <0 if i'm not being controlled
  int face;
  int sleeping;
  int seated;
  int jumpok; // Can start charging a jump.
  int jumping; // True during jump, goes false when crested.
  int charging; // Preparing a jump.
  int walking;
  int climbing;
  double gravity; // m/s
  double jump_power; // m/s, rises during charge
  double animclock;
  int animframe;
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
  SPRITE->jumpok=1;
  SPRITE->face=FACE_IDLE;
  return 0;
}

/* Set face and update animation.
 */

static void cat_animate(struct sprite *sprite,double elapsed) {

  int nface=SPRITE->face;
       if (SPRITE->climbing) nface=FACE_CLIMB;
  else if (SPRITE->sleeping) nface=FACE_SLEEP;
  else if (SPRITE->charging) nface=FACE_CHARGE;
  else if (SPRITE->jumping) nface=FACE_JUMP;
  else if (!SPRITE->seated) nface=FACE_FALL;
  else if (SPRITE->walking) nface=FACE_WALK;
  else nface=FACE_IDLE;
  if (nface!=SPRITE->face) {
    SPRITE->face=nface;
    SPRITE->animclock=0.0;
    SPRITE->animframe=0;
  }

  if ((SPRITE->animclock-=elapsed)<=0.0) {
    #define MONOTONIC(tag,sperframe,framec) case FACE_##tag: SPRITE->animclock+=sperframe; if (++(SPRITE->animframe)>=framec) SPRITE->animframe=0; break;
    #define STILL(tag) case FACE_##tag: SPRITE->animclock+=1.000; break;
    switch (SPRITE->face) {
      MONOTONIC(CLIMB,0.150,4)
      MONOTONIC(SLEEP,0.250,2)
      MONOTONIC(CHARGE,0.250,2)
      STILL(JUMP)
      STILL(FALL)
      MONOTONIC(WALK,0.200,4)
      MONOTONIC(IDLE,0.200,4)
    }
    #undef MONOTONIC
    #undef STILL
  }
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
      cat_animate(sprite,elapsed);
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
  if (SPRITE->jumping) {
    SPRITE->jumpok=0;
    SPRITE->jump_power-=JUMP_DECEL*elapsed;
    if (SPRITE->jump_power<=0.0) {
      // peaked
      SPRITE->jumping=0;
    } else {
      sprite_move(sprite,0.0,-SPRITE->jump_power*elapsed);
      SPRITE->seated=0;
    }
  } else if (SPRITE->charging) {
    if (!injump) {
      // Charge=>Jump.
      SPRITE->charging=0;
      SPRITE->jumping=1;
    } else {
      SPRITE->jump_power+=JUMP_CHARGE*elapsed;
      if (SPRITE->jump_power>JUMP_LIMIT) SPRITE->jump_power=JUMP_LIMIT;
    }
  } else if (SPRITE->jumpok&&injump) {
    // Begin charging.
    SPRITE->charging=1;
    SPRITE->jump_power=JUMP_MIN;
  } else {
    SPRITE->gravity+=elapsed*GRAVITY_ACCEL;
    if (SPRITE->gravity>GRAVITY_LIMIT) SPRITE->gravity=GRAVITY_LIMIT;
    if (sprite_move(sprite,0.0,SPRITE->gravity*elapsed)) {
      SPRITE->seated=0;
      SPRITE->jump_power=0.0;
    } else {
      SPRITE->gravity=0.0;
      SPRITE->seated=1;
      SPRITE->jumpok=!injump;
    }
  }
  
  /* Walking.
   */
  if (indx) {
    sprite->xform=(indx<0)?EGG_XFORM_XREV:0;
    if (!SPRITE->charging) {
      SPRITE->walking=1;
      sprite_move(sprite,6.000*elapsed*indx,0.0);
    } else {
      SPRITE->walking=0;
    }
  } else {
    SPRITE->walking=0;
  }
  
  /* Animation.
   * This also takes care of setting (face). Do it last.
   */
  cat_animate(sprite,elapsed);
}

/* Render.
 */
 
static void _cat_render(struct sprite *sprite,int x,int y) {
  graf_set_image(&g.graf,sprite->imageid);
  
  /* Frames: 0..3:idle, 4..5:charge, 6:jump, 7:fall, 8..9:sleep, 10..12:walk, 13..14:climb, 15:name
   * Climbing will use xform differently; the source xform is irrelevant.
   * Face and animframe are set for us, but animframe is interpretted differently for each face.
   */
  uint8_t tileid=sprite->tileid;
  uint8_t xform=sprite->xform;
  switch (SPRITE->face) {
    case FACE_CLIMB: switch (SPRITE->animframe) {
        case 0: tileid+=13; xform=0; break;
        case 1: tileid+=14; xform=0; break;
        case 2: tileid+=13; xform=EGG_XFORM_XREV; break;
        case 3: tileid+=14; xform=EGG_XFORM_XREV; break;
      } break;
    case FACE_SLEEP: tileid+=8+SPRITE->animframe; break;
    case FACE_CHARGE: tileid+=4+SPRITE->animframe; break;
    case FACE_JUMP: tileid+=6; break;
    case FACE_FALL: tileid+=7; break;
    case FACE_WALK: switch (SPRITE->animframe) {
        case 0: tileid+=10; break;
        case 1: tileid+=11; break;
        case 2: tileid+=12; break;
        case 3: tileid+=11; break;
      } break;
    case FACE_IDLE: tileid+=SPRITE->animframe; break;
  }
  graf_tile(&g.graf,x,y,tileid,xform);
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
