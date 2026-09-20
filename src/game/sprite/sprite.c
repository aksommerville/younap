#include "game/younap.h"

/* Globals.
 */
 
#define SPRITE_LIMIT 64

static struct sprite *spritev_static[SPRITE_LIMIT];
struct sprite **spritev=spritev_static;
int spritec=0;

/* Delete.
 * Private. Consumers must set (defunct) and eventually call sprites_reap().
 */

static void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* New.
 * Private. Consumers must use sprite_spawn().
 * This does not append to the global list.
 */
 
static struct sprite *sprite_new(double x,double y,const struct sprite_type *type,uint32_t arg,int rid,const void *res,int resc) {
  if (!type) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  
  sprite->x=x;
  sprite->y=y;
  sprite->type=type;
  sprite->arg=arg;
  sprite->rid=rid;
  sprite->res=res;
  sprite->resc=resc;
  
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,res,resc)>=0) {
    struct cmdlist_entry cmd;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case CMD_sprite_image: sprite->imageid=(cmd.arg[0]<<8)|cmd.arg[1]; break;
        case CMD_sprite_tile: sprite->tileid=cmd.arg[0]; sprite->xform=cmd.arg[1]; break;
        //TODO other generic sprite commands
      }
    }
  }
  
  if (type->init) {
    if ((type->init(sprite)<0)||sprite->defunct) {
      sprite_del(sprite);
      return 0;
    }
  }
  
  return sprite;
}

/* Spawn.
 */
 
struct sprite *sprite_spawn(double x,double y,int rid,uint32_t arg) {
  
  // If the list is full, look for a defunct sprite we can replace.
  int replacep=-1;
  if (spritec>=SPRITE_LIMIT) {
    int i=spritec;
    while (i-->0) {
      if (spritev[i]->defunct) {
        replacep=i;
        break;
      }
    }
    if (replacep<0) return 0;
  }
  
  // Resolve type.
  const struct sprite_type *type=0;
  const void *res;
  int resc=res_get(&res,EGG_TID_sprite,rid);
  if (resc<1) {
    fprintf(stderr,"sprite:%d not found\n",rid);
    return 0;
  }
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,res,resc)<0) return 0;
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode==CMD_sprite_type) {
      int id=(cmd.arg[0]<<8)|cmd.arg[1];
      if (!(type=sprite_type_by_id(id))) {
        fprintf(stderr,"sprtype:%d not found, referred by sprite:%d\n",id,rid);
        return 0;
      }
    }
  }
  if (!type) {
    fprintf(stderr,"sprite:%d does not name a type\n",rid);
    return 0;
  }
  
  // Create.
  struct sprite *sprite=sprite_new(x,y,type,arg,rid,res,resc);
  if (!sprite) return 0;
  
  // Install.
  if (replacep<0) {
    spritev[spritec++]=sprite;
  } else {
    sprite_del(spritev[replacep]);
    spritev[replacep]=sprite;
  }
  
  return sprite;
}

/* Drop all defunct sprites.
 */
 
void sprites_reap() {
  int i=spritec;
  struct sprite **p=spritev+i-1;
  for (;i-->0;p--) {
    struct sprite *sprite=*p;
    if (!sprite->defunct) continue;
    spritec--;
    memmove(p,p+1,sizeof(void*)*(spritec-i));
    sprite_del(sprite);
  }
}

/* Drop all sprites.
 */
 
void sprites_nuke() {
  while (spritec>0) {
    spritec--;
    sprite_del(spritev[spritec]);
  }
}

/* Type by ID.
 */
 
const struct sprite_type *sprite_type_by_id(int sprtype) {
  switch (sprtype) {
    #define _(tag) case NS_sprtype_##tag: return &sprite_type_##tag;
    FOR_EACH_SPRTYPE
    #undef _
  }
  return 0;
}
