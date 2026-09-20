#include "younap.h"

/* Start level.
 */
 
int game_start_level(int mapid) {
  fprintf(stderr,"%s(%d)\n",__func__,mapid);
  
  /* Acquire resource, validation dimensions, copy cells.
   */
  const void *serial;
  int serialc=res_get(&serial,EGG_TID_map,mapid);
  if (serialc<1) {
    fprintf(stderr,"map:%d not found\n",mapid);
    return -1;
  }
  struct map_res res;
  if (map_res_decode(&res,serial,serialc)<0) return -1;
  fprintf(stderr,"map:%d: %dx%d cmdc=%d\n",mapid,res.w,res.h,res.cmdc);
  if ((res.w!=NS_sys_mapw)||(res.h!=NS_sys_maph)) return -1;
  memcpy(g.cellv,res.v,NS_sys_mapw*NS_sys_maph);
  
  /* Reset some globals.
   */
  g.windowc=0;
  g.sunmidx=NS_sys_mapw*0.5;
  g.sunmidy=NS_sys_maph;
  g.sunr=50.0; // not to scale :)
  g.sunt=0.0;
  
  /* Read commands.
   */
  struct cmdlist_reader reader={.v=res.cmd,.c=res.cmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
    
      case CMD_map_window: if (g.windowc<WINDOW_LIMIT) {
          struct window *window=g.windowv+g.windowc++;
          window->x=cmd.arg[0];
          window->y=cmd.arg[1];
          window->w=cmd.arg[2];
          window->h=cmd.arg[3];
        } break;
    
      case CMD_map_sprite: {
          int x=cmd.arg[0];
          int y=cmd.arg[1];
          int spriteid=(cmd.arg[2]<<8)|cmd.arg[3];
          const uint8_t *arg=cmd.arg+4;
          fprintf(stderr,"TODO spawn sprite:%d at %d,%d with arg: %02x %02x %02x %02x\n",spriteid,x,y,arg[0],arg[1],arg[2],arg[3]);//TODO
        } break;
        
    }
  }
  
  return 0;
}

/* Advance the sun.
 */
 
void game_advance_sun(double elapsed) {
  g.sunt+=elapsed*0.200;
  if (g.sunt>=M_PI) {
    fprintf(stderr,"END OF DAY\n");//TODO
    game_start_level(1);
  }
}

/* Regenerate spots, recursive entry point.
 * It proceeds, examining each row, until eliminated or offscreen.
 */
 
static void game_regenerate_spots_inner(int y,double xa,double xz,double xpery) {
  if ((y<0)||(y>=NS_sys_maph)) return;
  if (xa>=xz) return;
  
  if (g.spotc>=SPOT_LIMIT) return;
  g.spotv[g.spotc++]=(struct spot){y,xa,xz};//TODO
}

/* Regenerate spots.
 */
 
void game_regenerate_spots() {
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
      game_regenerate_spots_inner(window->y+window->h,window->x+xpery*window->h,window->x+window->w,xpery);
    }
  } else {
    for (;i-->0;window++) {
      game_regenerate_spots_inner(window->y+window->h,window->x,window->x+window->w+xpery*window->h,xpery);
    }
  }
}
