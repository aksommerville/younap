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

#define FBW 960
#define FBH 480
#define WINDOW_LIMIT 8
#define SPOT_LIMIT 32

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
  
  double sunmidx,sunmidy; // Sun's center of rotation in map meters. Probably center of map?
  double sunr; // Sun's distance from center of ration in meters.
  double sunt; // Sun's position along its orbit: 0..pi = dawn..dusk
  
  /* Windows come straight off the map: CMD_map_window.
   */
  struct window {
    uint8_t x,y,w,h; // In map meters.
  } windowv[WINDOW_LIMIT];
  int windowc;
  
  /* Spots are horizontal lines on the floor, warmed by the sun.
   * Regenerated each update.
   */
  struct spot {
    double y,xa,xz;
  } spotv[SPOT_LIMIT];
  int spotc;
  
} g;

// main.c
int res_search(int tid,int rid);
int res_get(const void *dstpp,int tid,int rid);

// render.c
void render_far_bg(); // Fills framebuffer.
void render_map();
void render_sunbeams();
void render_sprites();

// game.c
int game_start_level(int mapid);
void advance_sun(double elapsed); // May start a new level.

// sunlight.c
void regenerate_spots();

#endif
