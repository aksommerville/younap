/* modal.c
 * Hello and Game Over.
 * Level intro and outtro are not managed here.
 */

#include "younap.h"

/* Begin Hello.
 */
 
void hello_begin() {
  g.level_intro=0;
  g.level_report=0;
  g.hello=1;
  g.gameover=0;
  play_song(RID_song_noodlecat);
}

/* Render Hello.
 */
 
void hello_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0xffded1ff);
  
  graf_set_image(&g.graf,RID_image_titlebg);
  graf_decal(&g.graf,0,FBH-300,0,0,FBW,300);
  
  graf_set_image(&g.graf,RID_image_titletext);
  graf_decal(&g.graf,(FBW>>1)-135,30,0,0,270,240);
  
  // I'd like to render the cats and snores as tiles so we can animate them. But the static ones are scaled up.
  
  /* Show best score and time if present.
   */
  graf_set_image(&g.graf,RID_image_fonttiles);
  graf_set_tint(&g.graf,0x000000ff);
  int y=320;
  if (memcmp(g.hiscore,"000000",6)) {
    render_kv(y,"High score",10,g.hiscore,sizeof(g.hiscore));
    y+=20;
  }
  if (memcmp(g.hitime_any,"00:00:00.000",12)) {
    int trimc=trim_time(g.hitime_any);
    render_kv(y,"Best time",9,g.hitime_any+trimc,12-trimc);
    y+=20;
  }
  if (memcmp(g.hitime_100,"00:00:00.000",12)) {
    int trimc=trim_time(g.hitime_100);
    render_kv(y,"Full clear",10,g.hitime_100+trimc,12-trimc);
    y+=20;
  }
  graf_set_tint(&g.graf,0);
}

/* Begin Game Over.
 */
 
void gameover_begin() {
  g.level_intro=0;
  g.level_report=0;
  g.hello=0;
  g.gameover=1;
  g.gameover_clock=0.0;
  g.gameover_headt=0.0;
  play_song(RID_song_fishie_fishie);
  
  score_finalize();
}

/* Render Game Over.
 */
 
void gameover_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  graf_set_image(&g.graf,RID_image_fonttiles);
  
  int y=150;
  render_string_centered(y,"Game Over",9);
  y+=40;
  
  render_kv(y,"Score",5,g.rptscore,sizeof(g.rptscore)); y+=20;
  render_kv(y,"Time",4,g.rpttime,g.rpttimec); y+=20;
  render_kv_int(y,"Death",5,g.deathc_total); y+=20;
  if (g.fishc_possible) { render_kv_int2(y,"Fish",4,g.fishc_total,g.fishc_possible); y+=20; }
  if (g.bonusc_possible) { render_kv_int2(y,"Bonus",5,g.bonusc_total,g.bonusc_possible); y+=20; }
  y+=20;
  
  if (g.new_hi_score) {
    render_string_centered(y,"New high score!",15);
    y+=20;
  } else if (memcmp(g.hiscore,"000000",6)) {
    render_kv(y,"High score",10,g.hiscore,6);
    y+=20;
  }
  if (g.new_hi_time_any||g.new_hi_time_100) {
    render_string_centered(y,"New best time!",14);
    y+=20;
  }
  if (memcmp(g.hitime_any,"00:00:00.000",12)) {
    int trimc=trim_time(g.hitime_any);
    render_kv(y,"Best time",9,g.hitime_any+trimc,12-trimc);
    y+=20;
  }
  if (memcmp(g.hitime_100,"00:00:00.000",12)) {
    int trimc=trim_time(g.hitime_100);
    render_kv(y,"Best full clear",9,g.hitime_100+trimc,12-trimc);
    y+=20;
  }
  
  /* Bobblehead cats.
   */
  graf_set_image(&g.graf,RID_image_bobblehead);
  graf_set_filter(&g.graf,1);
  int bodyy_final=FBH-128;
  int bodyy_initial=FBH+300;
  double t=g.gameover_clock/5.0;
  if (t<0.0) t=0.0; else if (t>1.0) t=1.0;
  int bodyy=(int)((bodyy_initial*(1.0-t))+(bodyy_final*t));
  int heady=bodyy-128;
  int xl=180;
  int xr=FBW-xl;
  double cost=cos(g.gameover_headt);
  double sint=sin(g.gameover_headt);
  graf_decal(&g.graf,xl-128,bodyy-128,0,256,256,256);
  graf_decal_rotate(&g.graf,xl,heady,0,0,256,cost,sint,1.0);
  graf_decal(&g.graf,xr-128,bodyy-128,256,256,256,256);
  graf_decal_rotate(&g.graf,xr,heady,256,0,256,cost,sint,1.0);
  graf_set_filter(&g.graf,0);
}

/* Generic update.
 * Call with a nonzero (*state). We'll update that as needed.
 * Returns one of MODAL_UPDATE_*
 */
 
#define MODAL_UPDATE_NOOP 0
#define MODAL_UPDATE_DONE 1
#define MODAL_UPDATE_NEXT_LEVEL 2
#define MODAL_UPDATE_START_GAME 3
#define MODAL_UPDATE_FULL_RESET 4
 
static int modal_update_1(int *state,double elapsed) {
  const int important_buttons=(EGG_BTN_SOUTH|EGG_BTN_WEST);
  
  // 1: Wait for input to clear.
  if (*state==1) {
    if (!(g.input[0]&important_buttons)) (*state)=2;
    
  // 2: Wait for SOUTH.
  } else if (*state==2) {
    if (g.input[0]&EGG_BTN_SOUTH) {
      SND(uiactivate)
      (*state)=3;
    }
    
  // 3: Wait for input to clear again.
  } else if (*state==3) {
    if (!(g.input[0]&important_buttons)) {
      (*state)=0;
      if (state==&g.level_intro) return MODAL_UPDATE_DONE;
      if (state==&g.level_report) return MODAL_UPDATE_NEXT_LEVEL;
      if (state==&g.hello) return MODAL_UPDATE_START_GAME;
      if (state==&g.gameover) return MODAL_UPDATE_FULL_RESET;
      return MODAL_UPDATE_DONE;
    }
  }
  return MODAL_UPDATE_NOOP;
}

/* Update, for all modals.
 */
 
void modal_update(double elapsed) {
  int result=0;
  if (g.level_intro) result=modal_update_1(&g.level_intro,elapsed);
  else if (g.level_report) result=modal_update_1(&g.level_report,elapsed);
  else if (g.hello) result=modal_update_1(&g.hello,elapsed);
  else if (g.gameover) {
    result=modal_update_1(&g.gameover,elapsed);
    g.gameover_clock+=elapsed;
    if (g.gameover_tempo>0.0) {
      double ph=egg_song_get_playhead(1);
      double t=fmod(ph,g.gameover_tempo)/g.gameover_tempo;
      g.gameover_headt=t*M_PI;
      if (g.gameover_headt>M_PI*0.5) {
        g.gameover_headt=M_PI-g.gameover_headt;
      }
      g.gameover_headt+=M_PI*0.25;
    }
  }
  switch (result) {
    case MODAL_UPDATE_NEXT_LEVEL: {
        g.fishc_total+=g.fishc_level;
        g.fishc_level=0;
        if (g.bonus) {
          if (g.bonus_ok) g.bonusc_total++;
        }
        if (game_start_level(g.mapid+1)<0) {
          gameover_begin();
        }
      } break;
    case MODAL_UPDATE_START_GAME: {
        game_reset_scores();
        if (game_start_level(1)<0) {
          egg_terminate(1);
        }
      } break;
    case MODAL_UPDATE_FULL_RESET: {
        hello_begin();
      } break;
  }
}
