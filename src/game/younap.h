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
#include "sprite/sprite.h"

#define FBW 960
#define FBH 480
#define WINDOW_LIMIT 8
#define INPUT_LIMIT 3 /* One more than the max player count. */

extern struct g {
  void *rom;
  int romc;
  struct rom_entry *resv;
  int resc,resa;
  struct graf graf;
  int input[INPUT_LIMIT],pvinput[INPUT_LIMIT];
  
  /* Single tilesheet, and just one interesting table.
   */
  uint8_t physics[256];
  
  int mapid;
  uint8_t cellv[NS_sys_mapw*NS_sys_maph];
  
  double sunp; // 0..1
  double levelclock; // s, counts down
  double leveltime; // s, constant during level
  
  /* Windows come straight off the map: CMD_map_window.
   */
  struct window {
    uint8_t x,y,w,h; // In map meters, from the map command.
    int floory; // >=y+h, meters, where my sunbeams land.
    double beaml,beamr; // Meters, left and right extents of my sunbeam. Highly volatile.
  } windowv[WINDOW_LIMIT];
  int windowc;
} g;

// main.c
int res_search(int tid,int rid);
int res_get(const void *dstpp,int tid,int rid);

// render.c
void render_far_bg(); // Fills framebuffer.
void render_map();
void render_sunbeams();
void render_sprites();
void render_overlay();

// game.c
int game_start_level(int mapid);

// sunlight.c
void advance_sun(double elapsed); // May start a new level.
void regenerate_spots();

#endif
