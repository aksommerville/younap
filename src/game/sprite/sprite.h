/* sprite.h
 * We'll do generic sprites because I'm not sure how complex the game is going to be.
 * At a minimum, there are multiple cats.
 * But a single global list with a fixed size limit.
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;

struct sprite {
  const struct sprite_type *type;
  int defunct;
  double x,y; // Center in map meters.
  uint32_t arg; // From spawn point, big-endianly.
  int rid; // Zero if not applicable.
  const void *res; // The entire resource, if applicable.
  int resc;
  int imageid;
  uint8_t tileid;
  uint8_t xform;
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  
  /* Do not spawn other sprites during init.
   * OK to return <0 to reject construction; setting (defunct) has the same effect.
   */
  int (*init)(struct sprite *sprite);
  
  void (*update)(struct sprite *sprite,double elapsed);
  void (*render)(struct sprite *sprite,int x,int y);
};

/* Consumers are allowed to read the global sprite list directly.
 * Please use responsibly. Read only.
 * Sprites may get added to the end at any time, but will only be removed during sprites_reap().
 */
extern struct sprite **spritev;
extern int spritec;

/* Create a sprite, add to the global list, and return WEAK.
 * (arg) comes off the spawn command big-endianly.
 */
struct sprite *sprite_spawn(double x,double y,int rid,uint32_t arg);

/* Drop all defunct sprites.
 * Call me every update, when we know the list isn't being iterated.
 */
void sprites_reap();

/* Delete every sprite, defunct or not.
 * Obviously don't do this during iteration.
 */
void sprites_nuke();

const struct sprite_type *sprite_type_by_id(int sprtype);

#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_SPRTYPE
#undef _

/* For sprites participating in physics.
 * Returns nonzero if we move at all.
 * Won't correct in the opposite direction.
 */
int sprite_move(struct sprite *sprite,double dx,double dy);

/* Specific types.
 ***************************************************************************/
 
// sprite_cat.c
void require_cat_inputs();
void cat_shuffle_input(int playerid,int d);
int sprite_cat_is_sleeping(const struct sprite *sprite);

#endif
