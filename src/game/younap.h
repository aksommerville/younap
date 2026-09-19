#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/font/font.h"
#include "util/res/res.h"
#include "util/text/text.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"

#define FBW 1024
#define FBH 576

extern struct g {
  void *rom;
  int romc;
  struct rom_entry *resv;
  int resc,resa;
  struct graf graf;
  
  /* Single tilesheet, and just one interesting table.
   */
  uint8_t physics[256];
  
  int mapid;
  uint8_t cellv[NS_sys_mapw*NS_sys_maph];
  
} g;

int res_search(int tid,int rid);
int res_get(const void *dstpp,int tid,int rid);

void render_far_bg(); // Fills framebuffer.
void render_map();
void render_sunbeams();
void render_sprites();

int game_start_level(int mapid);

#endif
