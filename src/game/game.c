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
    fprintf(stderr,"map:%d not found\n",mapid);//TODO should be our "game over" trigger
    return -1;
  }
  struct map_res res;
  if (map_res_decode(&res,serial,serialc)<0) return -1;
  fprintf(stderr,"map:%d: %dx%d cmdc=%d\n",mapid,res.w,res.h,res.cmdc);
  if ((res.w!=NS_sys_mapw)||(res.h!=NS_sys_maph)) return -1;
  memcpy(g.cellv,res.v,NS_sys_mapw*NS_sys_maph);
  
  /* Reset some globals.
   */
  g.mapid=mapid;
  g.windowc=0;
  g.sunp=0.0;
  g.leveltime=g.levelclock=20.0;
  sprites_nuke();
  
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
          if ((window->w<1)||(window->h<1)||(window->x+window->w>NS_sys_mapw)||(window->y+window->h>NS_sys_maph)) {
            fprintf(stderr,"map:%d invalid window (%d,%d,%d,%d)\n",mapid,window->x,window->y,window->w,window->h);
            return -1;
          }
        } break;
    
      case CMD_map_sprite: {
          int x=cmd.arg[0];
          int y=cmd.arg[1];
          int spriteid=(cmd.arg[2]<<8)|cmd.arg[3];
          uint32_t arg=(cmd.arg[4]<<24)|(cmd.arg[5]<<16)|(cmd.arg[6]<<8)|cmd.arg[7];
          struct sprite *sprite=sprite_spawn(x+0.5,y+0.5,spriteid,arg);
        } break;
        
    }
  }
  
  /* Find each window's floor.
   * There must be a solid or oneway cell at (x,y+h) or below.
   */
  struct window *window=g.windowv;
  int i=g.windowc;
  for (;i-->0;window++) {
    window->floory=window->y+window->h;
    for (;;) {
      if (window->floory>=NS_sys_maph) {
        fprintf(stderr,"map:%d, window at %d,%d has no floor below.\n",mapid,window->x,window->y);
        return -1;
      }
      uint8_t physics=g.physics[g.cellv[window->floory*NS_sys_mapw+window->x]];
      if ((physics==NS_physics_solid)||(physics==NS_physics_oneway)) break;
      window->floory++;
    }
    // (beaml,beamr) should be overwritten before the first render. But if not, they're straight down.
    window->beaml=window->x;
    window->beamr=window->x+window->w;
    // Confirm that that floor extends to the diagonals spreading from the window's top.
    // Not looking for occlusions above -1, because that starts to get complicated.
    // This is only validation; in theory we could be doing it at build time instead.
    int dy=window->floory-window->y;
    int xlo=window->x-dy; // inclusive
    int xhi=window->x+window->w+dy; // exclusive
    if ((xlo<0)||(xhi>NS_sys_mapw)) {
      fprintf(stderr,"map:%d, window at %d,%d exceeds horizontal map edges\n",mapid,window->x,window->y);
      return -1;
    }
    const uint8_t *mbtm=g.cellv+window->floory*NS_sys_mapw+xlo;
    const uint8_t *mtop=mbtm-NS_sys_mapw;
    int x=xlo;
    for (;x<xhi;x++,mbtm++,mtop++) {
      uint8_t phtop=g.physics[*mtop];
      uint8_t phbtm=g.physics[*mbtm];
      if (phtop!=NS_physics_vacant) {
        fprintf(stderr,"map:%d, window at %d,%d beam is occluded at %d,%d\n",mapid,window->x,window->y,x,window->floory-1);
        return -1;
      }
      if ((phbtm!=NS_physics_solid)&&(phbtm!=NS_physics_oneway)) {
        fprintf(stderr,"map:%d, window at %d,%d beam runs off a cliff around %d,%d\n",mapid,window->x,window->y,x,window->floory);
        return -1;
      }
    }
  }
  
  return 0;
}
