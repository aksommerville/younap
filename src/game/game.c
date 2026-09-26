#include "younap.h"

/* Score validation.
 */
 
static int all_digits(const char *src,int srcc) {
  for (;srcc-->0;src++) if ((*src<'0')||(*src>'9')) return 0;
  return 1;
}

static int time_valid(const char *src,int srcc) {
  // Read back to front. Any suffix of the full valid time "00:00:00.000" is valid.
  #define DIGIT { \
    if (srcc<=0) return 1; \
    srcc--; \
    if ((src[srcc]<'0')||(src[srcc]>'9')) return 0; \
  }
  #define LITERAL(ch) { \
    if (srcc<=0) return 1; \
    if (src[--srcc]!=ch) return 0; \
  }
  DIGIT DIGIT DIGIT
  LITERAL('.')
  DIGIT DIGIT
  LITERAL(':')
  DIGIT DIGIT
  LITERAL(':')
  DIGIT DIGIT
  #undef DIGIT
  #undef LITERAL
  if (srcc>0) return 0;
  return 1;
}

/* Nonzero if (a) is better than (b). Not equal.
 */
 
static int time_better(const char *a,int ac,const char *b,int bc) {
  if (!a) ac=0; else if (ac<0) { ac=0; while (a[ac]) ac++; }
  if (!b) bc=0; else if (bc<0) { bc=0; while (b[bc]) bc++; }
  // Trim zero, colon, and dot from the start of each.
  while (ac&&((*a=='0')||(*a==':')||(*a=='.'))) { ac--; a++; }
  while (bc&&((*b=='0')||(*b==':')||(*b=='.'))) { bc--; b++; }
  // If one is empty, the other is better.
  if (!bc) return ac?1:0;
  // Shorter is better.
  if (ac<bc) return 1;
  if (ac>bc) return 0;
  // Compare lexically. Earlier is better.
  return (memcmp(a,b,ac)<0);
}

/* Finalize session score.
 */
 
void score_finalize() {

  /* Parameters for computing score.
   * We're not going to read anything dynamically off the maps; just hard-code the parameters here.
   */
  const double time_min=((( 1.0 )))*60.0+((( 0.0 ))); // ((( M ))):((( S ))) ; At or below this time you get a perfect score.
  const double time_max=((( 5.0 )))*60.0+((( 0.0 ))); // ((( M ))):((( S ))) ; Above this time you get no time points.
  const int death_max=5; // Above this you get no death points.
  const int time_weight= 300000; // Portion of the million awarded per time.
  const int bonus_weight=200000; // '' per bonuses.
  const int fish_weight= 200000; // Portion of the million awarded per fish.
  const int death_weight=200000; // Portion of the million awarded per death count.
  const int pity_points= 100000; // If they completed the game at all, a minimum score. (I like 100k exactly, so there's never a leading zero).
  if (time_weight+bonus_weight+fish_weight+death_weight+pity_points!=1000000) {
    fprintf(stderr,"%s:%d: hey doofus, the score parameters don't add up\n",__FILE__,__LINE__);
  }
  
  /* Compute score.
   */
  double time_score=1.0-((g.time_total-time_min)/(time_max/time_min));
  if (time_score<0.0) time_score=0.0; else if (time_score>1.0) time_score=1.0;
  double bonus_score=1.0; // If we forgot to assign any, give them the points.
  if (g.bonusc_possible) {
    bonus_score=(double)g.bonusc_total/(double)g.bonusc_possible;
    if (bonus_score>1.0) bonus_score=1.0;
  }
  double fish_score=1.0;
  if (g.fishc_possible) {
    fish_score=(double)g.fishc_total/(double)g.fishc_possible;
    if (fish_score>1.0) fish_score=1.0;
  }
  double death_score=1.0-(double)g.deathc_total/(double)death_max;
  if (death_score<0.0) death_score=0.0;
  int score=(int)(time_score*time_weight+bonus_score*bonus_weight+fish_score*fish_weight+death_score*death_weight+pity_points);
  if (score<pity_points) score=pity_points;
  else if (score>999999) score=999999;
  
  /* Serialize time and score for reporting.
   */
  g.rpttimec=time_repr(g.rpttime,sizeof(g.rpttime),g.time_total,0);
  g.rptscore[0]='0'+(score/100000)%10;
  g.rptscore[1]='0'+(score/10000 )%10;
  g.rptscore[2]='0'+(score/1000  )%10;
  g.rptscore[3]='0'+(score/100   )%10;
  g.rptscore[4]='0'+(score/10    )%10;
  g.rptscore[5]='0'+(score/1     )%10;
  
  /* Check whether score or time sets a new record. They compare independently.
   */
  if (memcmp(g.rptscore,g.hiscore,sizeof(g.rptscore))>0) {
    g.new_hi_score=1;
  } else {
    g.new_hi_score=0;
  }
  if (time_better(g.rpttime,g.rpttimec,g.hitime,sizeof(g.hitime))) {
    g.new_hi_time=1;
  } else {
    g.new_hi_time=0;
  }
  
  /* If a new record was set, encode and save it.
   */
  if (g.new_hi_score||g.new_hi_time) {
    char tmp[20];
    int tmpc=0;
    if (g.new_hi_score) {
      memcpy(tmp,g.rptscore,sizeof(g.rptscore));
      memcpy(g.hiscore,g.rptscore,sizeof(g.rptscore));
    } else {
      memcpy(tmp,g.hiscore,sizeof(g.hiscore));
    }
    tmpc=sizeof(g.rptscore);
    tmp[tmpc++]=';';
    if (g.new_hi_time) {
      memcpy(tmp+tmpc,g.rpttime,g.rpttimec);
      tmpc+=g.rpttimec;
      memcpy(g.hitime,"00:00:00.000",12);
      memcpy(g.hitime+12-g.rpttimec,g.rpttime,g.rpttimec);
    } else {
      memcmp(tmp+tmpc,g.hitime,sizeof(g.hitime));
      tmpc+=sizeof(g.hitime);
    }
    egg_store_set("hiscore",7,tmp,tmpc);
  }
}

/* Load high score.
 */
 
void hiscore_load() {
  char src[32];
  int srcc=egg_store_get(src,sizeof(src),"hiscore",7);
  if ((srcc<0)||(srcc>sizeof(src))) srcc=0;
  const char *srcscore=src;
  int srcscorec=0,srcp=0;
  while ((srcscorec<srcc)&&(src[srcp++]!=';')) srcscorec++;
  const char *srctime=src+srcp;
  int srctimec=srcc-srcp;
  
  // Score must be six digits. Default "000000".
  if ((srcscorec==sizeof(g.hiscore))&&all_digits(srcscore,sizeof(g.hiscore))) {
    memcpy(g.hiscore,srcscore,sizeof(g.hiscore));
  } else {
    memset(g.hiscore,'0',sizeof(g.hiscore));
  }
  
  // Time may be short at the front. We pad to the full length.
  memcpy(g.hitime,"00:00:00.000",12);
  if (time_valid(srctime,srctimec)) {
    memcpy(g.hitime+sizeof(g.hitime)-srctimec,srctime,srctimec);
  }
}

/* Reset session scores.
 */
 
void game_reset_scores() {
  g.time_total=0.0;
  g.deathc_total=0;
  g.fishc_total=0;
  g.bonusc_total=0;
}

/* Time or integer as a string.
 * NB: We're accepting the output vector length but not checking it. Caller should always provide sufficient space.
 */
 
int time_repr(char *text,int texta,double f,int full) {
  int ms=(int)(f*1000.0);
  if (ms<0) ms=0;
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  int hour=min/60; min%=60;
  if (hour>99) { // um, seriously?
    hour=min=sec=99;
    ms=999;
  }
  int textc=0;
  // Hours only if nonzero; I can't imagine they will ever go above zero.
  if (full||(hour>=10)) text[textc++]='0'+hour/10;
  if (full||(hour>0)) {
    text[textc++]='0'+hour%10;
    text[textc++]=':';
  }
  // Minutes always present but trim the high digit if zero (mind the hours too).
  if (full||hour||(min>=10)) text[textc++]='0'+min/10;
  text[textc++]='0'+min%10;
  text[textc++]=':';
  text[textc++]='0'+sec/10;
  text[textc++]='0'+sec%10;
  // Do milliseconds matter? Might as well show I guess.
  text[textc++]='.';
  text[textc++]='0'+ms/100;
  text[textc++]='0'+(ms/10)%10;
  text[textc++]='0'+ms%10;
  return textc;
}
 
int decsint_repr(char *text,int texta,int v) {
  int textc=0;
  if (v<0) {
    text[textc++]='-';
    v=-v;
    if (v<0) v=INT_MAX; // was INT_MIN
  }
  int limit=10,digitc=1;
  while (v>=limit) { digitc++; if (limit>INT_MAX/10) break; limit*=10; }
  int i=digitc;
  for (;i-->0;v/=10) text[textc+i]='0'+v%10;
  textc+=digitc;
  return textc;
}

/* Start level.
 */
 
int game_start_level(int mapid) {
  
  /* Acquire resource, validation dimensions, copy cells.
   */
  const void *serial;
  int serialc=res_get(&serial,EGG_TID_map,mapid);
  if (serialc<1) return -1;
  struct map_res res;
  if (map_res_decode(&res,serial,serialc)<0) return -1;
  if ((res.w!=NS_sys_mapw)||(res.h!=NS_sys_maph)) return -1;
  memcpy(g.cellv,res.v,NS_sys_mapw*NS_sys_maph);
  
  /* Reset some globals.
   */
  g.mapid=mapid;
  g.mapmsg=0;
  g.mapmsgc=0;
  g.windowc=0;
  g.sunp=0.0;
  g.sundp=0.250;
  g.levelclock=0.0;
  g.leveltime=10.0;
  g.score=0.0;
  g.catc=0;
  g.all_sleep_time=0.0;
  g.mrrrdr=0;
  g.fishc_level=0;
  g.jumpc_level=0;
  g.climbtime_level=0.0;
  g.bonus=NS_bonus_none;
  g.bonus_ok=0;
  g.level_intro=1;
  g.level_report=0;
  g.hello=0;
  g.gameover=0;
  sprites_nuke();
  
  /* Read commands.
   */
  struct cmdlist_reader reader={.v=res.cmd,.c=res.cmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
    
      case CMD_map_song: play_song((cmd.arg[0]<<8)|cmd.arg[1]); break;
      case CMD_map_sunrate: g.sundp=((cmd.arg[0]<<8)|cmd.arg[1])/65535.0; break;
      case CMD_map_score: g.leveltime=((cmd.arg[0]<<8)|cmd.arg[1])/256.0; break;
      case CMD_map_bonus: g.bonus=(cmd.arg[0]<<8)|cmd.arg[1]; break;
    
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
          if (sprite) {
            if (sprite->type==&sprite_type_cat) g.catc++;
          }
        } break;
      
      case CMD_map_mapmsg: {
          g.mapmsg=(const char*)cmd.arg;
          g.mapmsgc=cmd.argc;
        } break;
    }
  }
  if (!g.catc) {
    fprintf(stderr,"map:%d no cats\n",mapid);
    return -1;
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
  
  prerender_map();
  regenerate_spots();
  
  return 0;
}

/* Which sunbeam is each cat napping in?
 * Skip awake cats.
 */
 
static int identify_sunbeams_by_cat(int *dstv,int dsta) {
  int dstc=0;
  struct sprite **spritep=spritev;
  int spritei=spritec;
  for (;spritei-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct||(sprite->type!=&sprite_type_cat)) continue;
    if (!sprite_cat_is_sleeping(sprite)) continue;
    if (dstc>=dsta) break;
    const struct window *window=g.windowv;
    int windowi=g.windowc;
    for (;windowi-->0;window++) {
      if (sprite->x<window->beaml-0.5) continue;
      if (sprite->x>window->beamr+0.5) continue;
      if (sprite->y>window->floory) continue;
      if (sprite->y<window->floory-1.0) continue;
      dstv[dstc++]=windowi+1;
      break;
    }
  }
  return dstc;
}

/* Check bonus condition, at end of level.
 */
 
void check_bonus() {
  switch (g.bonus) {
  
    case NS_bonus_different_sunbeams: {
        int v[8];
        int c=identify_sunbeams_by_cat(v,8);
        if (c>0) {
          g.bonus_ok=1;
          int ai=c; while (ai-->0) {
            int bi=ai; while (bi-->0) {
              if (v[ai]==v[bi]) {
                g.bonus_ok=0;
              }
            }
          }
        }
      } break;
      
    case NS_bonus_same_sunbeam: {
        int v[8];
        int c=identify_sunbeams_by_cat(v,8);
        if (c>0) {
          g.bonus_ok=1;
          const int *p=v;
          for (;c-->0;p++) if (*p!=v[0]) g.bonus_ok=0;
        }
      } break;
      
    case NS_bonus_no_climb: {
        if (g.climbtime_level<=0.0) g.bonus_ok=1;
      } break;
      
    case NS_bonus_no_jump: {
        if (!g.jumpc_level) g.bonus_ok=1;
      } break;
  }
}
