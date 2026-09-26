#include "younap.h"

struct g g={0};

/* Quit.
 */

void egg_client_quit(int status) {
}

/* Receive tilesheet resource.
 * There's just one.
 */
 
static int receive_tilesheet(int rid,const void *v,int c) {
  if (rid!=RID_image_sprites) return 0;
  struct tilesheet_reader reader;
  if (tilesheet_reader_init(&reader,v,c)<0) return -1;
  struct tilesheet_entry entry;
  while (tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid!=NS_tilesheet_physics) continue;
    memcpy(g.physics+entry.tileid,entry.v,entry.c);
  }
  return 0;
}

/* Init.
 */

int egg_client_init() {

  // Validate framebuffer.
  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size mismatch! metadata=%dx%d header=%dx%d\n",fbw,fbh,FBW,FBH);
    return -1;
  }
  if ((FBW!=NS_sys_tilesize*NS_sys_mapw)||(FBH!=NS_sys_tilesize*NS_sys_maph)) {
    fprintf(stderr,"Expected framebuffer to fit one map exactly. Please check FBW,FBH,NS_sys_tilesize,NS_sys_mapw,NS_sys_maph\n");
    return -1;
  }

  // Acquire ROM.
  g.romc=egg_rom_get(0,0);
  if (!(g.rom=malloc(g.romc))) return -1;
  egg_rom_get(g.rom,g.romc);
  text_set_rom(g.rom,g.romc);
  
  // Index ROM.
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_entry res;
  while (rom_reader_next(&res,&reader)>0) {
    int keep=0;
    switch (res.tid) {
      case EGG_TID_map: keep=1; break;
      case EGG_TID_tilesheet: if (receive_tilesheet(res.rid,res.v,res.c)<0) return -1; break;
      case EGG_TID_sprite: keep=1; break;
    }
    if (!keep) continue;
    if (g.resc>=g.resa) {
      int na=g.resa+64;
      if (na>INT_MAX/sizeof(struct rom_entry)) return -1;
      void *nv=realloc(g.resv,sizeof(struct rom_entry)*na);
      if (!nv) return -1;
      g.resv=nv;
      g.resa=na;
    }
    g.resv[g.resc++]=res;
  }

  srand_auto();
  
  // Texture for map images.
  if ((g.bgtexid=egg_texture_new())<1) return -1;
  if (egg_texture_load_raw(g.bgtexid,FBW,FBH,FBW<<2,0,0)<0) return -1;
  
  hiscore_load();
  
  hello_begin();

  return 0;
}

/* Notification.
 */

void egg_client_notify(int k,int v) {
}

/* Mode.
 */
 
static int game_interactive() {
  if (g.level_intro) return 0;
  if (g.level_report) return 0;
  if (g.hello) return 0;
  if (g.gameover) return 0;
  return 1;
}

/* Update sprites.
 */
 
static void update_sprites(int fg,double elapsed) {
  struct sprite **spritep=spritev;
  int spritei=spritec;
  for (;spritei-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (fg) {
      if (sprite->type->update) sprite->type->update(sprite,elapsed);
    } else {
      if (sprite->type->update_bg) sprite->type->update_bg(sprite,elapsed);
    }
  }
  sprites_reap();
}

/* Update.
 */

void egg_client_update(double elapsed) {

  memcpy(g.pvinput,g.input,sizeof(g.input));
  egg_input_get_all(g.input,INPUT_LIMIT);
  
  /* L1/R1 to change cats.
   * Also mirror L2/R2, in case the automapper screws up, which it does. We don't want two pairs.
   */
  int playerid=1;
  for (;playerid<INPUT_LIMIT;playerid++) {
    if (g.input[playerid]&EGG_BTN_L2) g.input[playerid]|=EGG_BTN_L1;
    if (g.input[playerid]&EGG_BTN_R2) g.input[playerid]|=EGG_BTN_R1;
    if (game_interactive()) {
           if ((g.input[playerid]&EGG_BTN_L1)&&!(g.pvinput[playerid]&EGG_BTN_L1)) cat_shuffle_input(playerid,-1);
      else if ((g.input[playerid]&EGG_BTN_R1)&&!(g.pvinput[playerid]&EGG_BTN_R1)) cat_shuffle_input(playerid,1);
    }
  }
  
  /* Reset due to death?
   */
  if (g.mrrrdr) {
    g.deathc_total++;
    if (game_start_level(g.mapid)<0) {
      egg_terminate(1);
      return;
    }
  }
  
  /* Dismiss modal?
   */
  modal_update(elapsed);

  /* Normal stuff when game is running.
   */
  if (game_interactive()) {
    g.time_total+=elapsed;
    advance_sun(elapsed);
    regenerate_spots();
    require_cat_inputs();
    update_sprites(1,elapsed);
    check_level_completion(elapsed);
  } else {
    update_sprites(0,elapsed);
  }
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  if (g.hello) {
    hello_render();
  } else if (g.gameover) {
    gameover_render();
  } else {
    render_far_bg();
    render_map();
    render_sunbeams();
    render_sprites();
    render_overlay();
    if (g.level_intro) {
      render_level_intro();
    } else if (g.level_report) {
      render_level_report();
    }
  }
  graf_flush(&g.graf);
}

/* ROM TOC.
 */
 
int res_search(int tid,int rid) {
  int lo=0,hi=g.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_entry *res=g.resv+ck;
         if (tid<res->tid) hi=ck;
    else if (tid>res->tid) lo=ck+1;
    else if (rid<res->rid) hi=ck;
    else if (rid>res->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int res_get(const void *dstpp,int tid,int rid) {
  int p=res_search(tid,rid);
  if (p<0) return 0;
  const struct rom_entry *res=g.resv+p;
  *(const void**)dstpp=res->v;
  return res->c;
}

/* Audio.
 */
 
void play_song(int rid) {
  if (rid==g.songid) return;
  g.songid=rid;
  egg_play_song(1,rid,1,0.500,0.0);
}
