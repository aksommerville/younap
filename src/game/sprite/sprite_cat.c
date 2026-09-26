#include "game/younap.h"

#define GRAVITY_ACCEL 30.0 /* m/s**2 */
#define GRAVITY_LIMIT 10.0 /* m/s */
#define JUMP_CHARGE   10.0 /* m/s**2 */
#define JUMP_LIMIT    10.0 /* m/s */
#define JUMP_MIN       4.0 /* m/s ; what you get if there's exactly one frame of charge */
#define JUMP_DECEL    20.0 /* m/s**2 */
#define STAMINA_MAX    2.0 /* s */
#define SLEEP_TIME_MIN 1.000 /* s */

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
  double stamina;
  double jumpdx;
  double sleeptime; // s, this nap. So we can enforce the minimum.
  double wall_damage_clock;
  double zanimclock;
  int zanimframe;
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
  SPRITE->stamina=STAMINA_MAX;
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
  
  if (SPRITE->sleeping) {
    if ((SPRITE->zanimclock-=elapsed)<=0.0) {
      SPRITE->zanimclock+=0.125;
      if (++(SPRITE->zanimframe)>=16) SPRITE->zanimframe=0;
    }
  }
}

/* Down-jump, thru a one-way.
 */
 
static int cat_can_downjump(const struct sprite *sprite) {
  int y=(int)(sprite->y+0.501);
  if ((y<0)||(y>=NS_sys_maph)) return 0;
  int xa=(int)(sprite->x-0.500);
  int xz=(int)(sprite->x+0.499);
  if (xa<0) xa=0;
  if (xz>=NS_sys_mapw) xz=NS_sys_mapw-1;
  const uint8_t *src=g.cellv+y*NS_sys_mapw+xa;
  int i=xz-xa+1;
  for (;i-->0;src++) {
    uint8_t ph=g.physics[*src];
    switch (ph) {
      case NS_physics_vacant:
      case NS_physics_slippy:
      case NS_physics_oneway:
        continue;
      default: return 0;
    }
  }
  return 1;
}

static void cat_downjump(struct sprite *sprite) {
  SPRITE->jumpok=0;
  sprite->y+=0.010; // Just a wee kick to put him below the one-way's upper edge.
  SND(downjump)
}

/* Check slippage.
 */
 
static int cat_should_slip(const struct sprite *sprite) {
  int x=(int)sprite->x;
  int y=(int)sprite->y;
  if ((x<0)||(y<0)||(x>=NS_sys_mapw)||(y>=NS_sys_maph)) return 0;
  if (g.physics[g.cellv[y*NS_sys_mapw+x]]==NS_physics_slippy) return 1;
  return 0;
}

/* Periodically add a scratch mark to the background, when climbing.
 * Using this timing for the scratch sound too.
 */
 
static uint32_t random_scratch_color() {
  uint8_t r=0x20+rand()%0x40;
  uint8_t g=r>>1;
  uint8_t b=r>>1;
  return (r<<24)|(g<<16)|(b<<8)|0x80;
}
 
static void cat_update_wall_damage(struct sprite *sprite,double elapsed) {
  if ((SPRITE->wall_damage_clock-=elapsed)>0.0) return;
  SND(scratch)
  SPRITE->wall_damage_clock+=0.150;
  double dx=0.250;
  if (rand()&1) dx=-dx;
  int x=(int)((sprite->x+dx)*NS_sys_tilesize);
  int y=(int)((sprite->y-0.250)*NS_sys_tilesize);
  int h=4;
  uint32_t color=random_scratch_color();
  x+=rand()%9-4;
  y+=rand()%6;
  h+=rand()%4-1;
  if ((x<0)||(x>=FBW)) return;
  graf_flush(&g.graf);
  graf_reset(&g.graf);
  graf_set_output(&g.graf,g.bgtexid);
  graf_set_input(&g.graf,0);
  graf_line(&g.graf,x,y,color,x,y+h,(color&0xffffff00));
  graf_set_output(&g.graf,1);
  graf_flush(&g.graf);
}

/* Update.
 */
 
static void _cat_update(struct sprite *sprite,double elapsed) {

  /* Y below some level, neutralize and report death.
   */
  if (sprite->y>NS_sys_maph+2.0) {
    g.mrrrdr=1;
    SPRITE->climbing=0;
    return;
  }

  /* If I'm standing in a sunbeam, I fall asleep and if not, I wake up.
   * Regardless of whether I'm currently bound to an input.
   * And if I'm sleeping, no need for further activity.
   */
  if (SPRITE->seated) {
    int nsleeping=0;
    if (SPRITE->sleeping&&(SPRITE->sleeptime<SLEEP_TIME_MIN)) {
      // If we only just fell asleep, stay that way for at least some tasteful interval, don't even check.
      nsleeping=1;
    } else {
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
    }
    if (nsleeping) {
      if (!SPRITE->sleeping) {
        SND(sleep)
        SPRITE->sleeping=1;
        SPRITE->sleeptime=0.0;
      }
      g.score+=elapsed;
      SPRITE->sleeptime+=elapsed;
      SPRITE->input=-1;
      SPRITE->sleeping=1;
      SPRITE->charging=0;
      SPRITE->jumping=0;
      SPRITE->walking=0;
      cat_animate(sprite,elapsed);
      return;
    }
    if (SPRITE->sleeping) {
      SND(wake)
      SPRITE->sleeping=0;
    }
  } else if (SPRITE->sleeping) {
    SND(wake)
    SPRITE->sleeping=0;
  }
  
  /* Gather input.
   */
  int indx=0;
  int indy=0;
  int injump=0;
  int inclimb=0;
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
    inclimb=(g.input[SPRITE->input]&EGG_BTN_WEST)?1:0;
  }
  
  /* Climbing is kind of a different thing.
   */
  if (!SPRITE->charging&&!SPRITE->climbing&&inclimb&&(SPRITE->stamina>0.0)) {
    if (!SPRITE->seated||(indy<0)) { // Start in the air, or while holding Up.
      SPRITE->climbing=1;
      SPRITE->seated=0;
      SPRITE->gravity=0.0;
      SPRITE->jumpdx=0.0;
      SPRITE->jump_power=0.0;
      SPRITE->jumping=0;
    }
  }
  if (SPRITE->climbing) {
    if (!inclimb) {
      SPRITE->climbing=0;
    } else {
      if (cat_should_slip(sprite)) SPRITE->stamina=0.0;
      if ((SPRITE->stamina-=elapsed)<=0.0) {
        SPRITE->climbing=0;
      } else {
        if (indx) sprite_move(sprite,4.000*elapsed*indx,0.0);
        double dy=1.000+indy*3.000;
        if (!sprite_move(sprite,0.0,dy*elapsed)&&(dy>0.0)) {
          SPRITE->climbing=0;
        } else {
          cat_update_wall_damage(sprite,elapsed);
          cat_animate(sprite,elapsed);
          return;
        }
      }
    }
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
      SND(jump)
      SPRITE->charging=0;
      SPRITE->jumping=1;
      SPRITE->jumpdx=5.0;
      if (sprite->xform) SPRITE->jumpdx*=-1.0;
    } else {
      SPRITE->jump_power+=JUMP_CHARGE*elapsed;
      if (SPRITE->jump_power>JUMP_LIMIT) SPRITE->jump_power=JUMP_LIMIT;
    }
  } else if (SPRITE->jumpok&&SPRITE->seated&&injump) {
    if ((indy>0)&&cat_can_downjump(sprite)) {
      cat_downjump(sprite);
    } else {
      // Begin charging.
      SND(charge)
      SPRITE->charging=1;
      SPRITE->jump_power=JUMP_MIN;
    }
  } else {
    SPRITE->gravity+=elapsed*GRAVITY_ACCEL;
    if (SPRITE->gravity>GRAVITY_LIMIT) SPRITE->gravity=GRAVITY_LIMIT;
    if (sprite_move(sprite,0.0,SPRITE->gravity*elapsed)) {
      SPRITE->seated=0;
      SPRITE->jump_power=0.0;
    } else {
      if (SPRITE->gravity>=2.0) SND(land)
      SPRITE->gravity=0.0;
      SPRITE->seated=1;
      SPRITE->jumpok=!injump;
      SPRITE->stamina=STAMINA_MAX;
    }
  }
  
  /* Jump X motion plays out even after cresting.
   * But stops cold if you hit the floor. (or grab the wall; that's handled above).
   */
  if (SPRITE->seated&&!SPRITE->jumping) {
    SPRITE->jumpdx=0.0;
  } else if (SPRITE->jumpdx<0.0) {
    if ((SPRITE->jumpdx+=5.000*elapsed)>=0.0) SPRITE->jumpdx=0.0;
    else sprite_move(sprite,SPRITE->jumpdx*elapsed,0.0);
  } else if (SPRITE->jumpdx>0.0) {
    if ((SPRITE->jumpdx-=5.000*elapsed)<=0.0) SPRITE->jumpdx=0.0;
    else sprite_move(sprite,SPRITE->jumpdx*elapsed,0.0);
  }
  
  /* Walking.
   */
  if (indx) {
    sprite->xform=(indx<0)?EGG_XFORM_XREV:0;
    if (!SPRITE->charging) {
      SPRITE->walking=1;
      double speed=6.000;
      if (SPRITE->jumpdx>0.0) speed-=SPRITE->jumpdx; else speed+=SPRITE->jumpdx;
      if (speed>0.0) sprite_move(sprite,speed*elapsed*indx,0.0);
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

static void _cat_update_bg(struct sprite *sprite,double elapsed) {
  cat_animate(sprite,elapsed);
}

/* Render artificial power meters.
 */
 
static void render_bar(int x,int y,double v,uint32_t fg) {
  const int barw=40;
  const int barh=3;
  const int framew=barw+2;
  const int frameh=barh+2;
  const uint32_t bg=0x000000ff;
  int fillw=(int)(v*barw);
  if (fillw<0) fillw=0;
  else if (fillw>barw) fillw=barw;
  x-=framew>>1; // Translate (x,y) to the frame's top left corner.
  y-=NS_sys_tilesize; // (y) is initially the cat's center, so we want much lower.
  graf_fill_rect(&g.graf,x,y,framew,frameh,bg);
  graf_fill_rect(&g.graf,x+1,y+1,fillw,barh,fg);
}
 
static void render_charge_indicator(struct sprite *sprite,int x,int y) {
  render_bar(x,y,(SPRITE->jump_power-JUMP_MIN)/(JUMP_LIMIT-JUMP_MIN),0xffff00ff);
}

static void render_stamina_indicator(struct sprite *sprite,int x,int y) {
  render_bar(x,y,SPRITE->stamina/STAMINA_MAX,0x00ff00ff);
}

/* Render.
 */
 
static void _cat_render(struct sprite *sprite,int x,int y) {
  graf_set_image(&g.graf,sprite->imageid);
  
  /* Cheat it down so we can draw a surface into the tiles.
   */
  y+=3;
  
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
  
  /* Highlight cats receiving input.
   */
  uint32_t hilitecolor=0;
  switch (SPRITE->input) {
    case 1: hilitecolor=0xffff00ff; break;
    case 2: hilitecolor=0xff0000ff; break;
  }
  if (hilitecolor) {
    // Draw the tile four times, offset by a pixel in each of the cardinal directions, tinted full to the highlight color.
    graf_set_tint(&g.graf,hilitecolor);
    graf_set_alpha(&g.graf,0xc0);
    graf_tile(&g.graf,x+2,y,tileid,xform);
    graf_tile(&g.graf,x-2,y,tileid,xform);
    //graf_tile(&g.graf,x,y+2,tileid,xform); // er, maybe not the downward offset, it interferes with the floor
    graf_tile(&g.graf,x,y-2,tileid,xform);
    graf_set_tint(&g.graf,0);
    graf_set_alpha(&g.graf,0xff);
  }
  
  /* Draw my main tile.
   */
  graf_tile(&g.graf,x,y,tileid,xform);
  
  /* Artificial indicators while charging or climbing.
   */
  switch (SPRITE->face) {
    case FACE_CHARGE: render_charge_indicator(sprite,x,y); break;
    case FACE_CLIMB: render_stamina_indicator(sprite,x,y); break;
  }
  
  /* Zs when sleeping.
   */
  if (SPRITE->sleeping) {
    graf_tile(&g.graf,x,y-20,0x80+SPRITE->zanimframe,0);
  }
}

/* Type definition.
 */

const struct sprite_type sprite_type_cat={
  .name="cat",
  .objlen=sizeof(struct sprite_cat),
  .del=_cat_del,
  .init=_cat_init,
  .update=_cat_update,
  .update_bg=_cat_update_bg,
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

/* Public: Find a cat with the given playerid, and focus whoever's next or previous in the list.
 * If no cat has this playerid, pick any.
 */
 
void cat_shuffle_input(int playerid,int d) {
  #define HARD_CAT_LIMIT 10
  struct sprite *catv[HARD_CAT_LIMIT]; // Only record those unassigned and awake, plus the current one.
  int catc=0;
  int cat_focusp=-1; // Index in (catv) where input currently resides, or -1.
  struct sprite **spritep=spritev;
  int spritei=spritec;
  for (;spritei-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (sprite->type!=&sprite_type_cat) continue;
    if (SPRITE->sleeping) continue; // We can ignore the sleeping cats entirely.
    if (SPRITE->input<0) { // Record unassigned awake cats.
      catv[catc++]=sprite;
    } else if (SPRITE->input==playerid) { // Also record me.
      cat_focusp=catc;
      catv[catc++]=sprite;
    }
    if (catc>=HARD_CAT_LIMIT) break;
  }
  #undef HARD_CAT_LIMIT
  if (cat_focusp>=0) { // Step in the list.
    if (catc<2) return; // ...nope! It's just me.
    int np=cat_focusp+d;
    if (np<0) np=catc-1;
    else if (np>=catc) np=0;
    ((struct sprite_cat*)catv[cat_focusp])->input=-1;
    ((struct sprite_cat*)catv[np])->input=playerid;
  } else { // Pick any.
    if (catc<1) return; // ...nope!
    ((struct sprite_cat*)catv[0])->input=playerid;
  }
  SND(choosecat)
}

/* Cat's state.
 */
 
int sprite_cat_is_sleeping(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_cat)) return 0;
  return SPRITE->sleeping;
}
