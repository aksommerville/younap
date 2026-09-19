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
  if (rid!=RID_image_scratch) return 0;
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
  
  if (game_start_level(1)<0) return -1;
  
  egg_play_song(1,RID_song_noodlecat,1,1.0,0.0);

  return 0;
}

/* Notification.
 */

void egg_client_notify(int k,int v) {
}

/* Update.
 */

void egg_client_update(double elapsed) {
  //TODO
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  render_far_bg();
  render_map();
  render_sunbeams();
  render_sprites();
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
