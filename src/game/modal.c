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
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x002090ff);
  //TODO Incorporate graphics.
  graf_set_image(&g.graf,RID_image_fonttiles);
  render_string_centered(200,"You Could Use A Nap!",20);
  render_string_centered(440,"GDEX Game Jam 2026",18);
  render_string_centered(460,"By AK Sommerville, Alex Hansen, Aster Kanke",43);
  
  /* Show best score and time if present.
   */
  if (memcmp(g.hiscore,"000000",6)||memcmp(g.hitime,"00:00:00.000",12)) {
    char msg[128];
    memcpy(msg,"Record: ",8);
    int msgc=8;
    memcpy(msg+msgc,g.hiscore,sizeof(g.hiscore));
    msgc+=sizeof(g.hiscore);
    msg[msgc++]=' ';
    msg[msgc++]='/';
    msg[msgc++]=' ';
    int trimc=0;
    if (!memcmp(g.hitime,"00:0",4)) trimc=4;
    else if (!memcmp(g.hitime,"00",2)) trimc=3;
    else if (g.hitime[0]=='0') trimc=1;
    memcpy(msg+msgc,g.hitime+trimc,sizeof(g.hitime)-trimc);
    msgc+=sizeof(g.hitime)-trimc;
    render_string_centered(360,msg,msgc);
  }
}

/* Begin Game Over.
 */
 
void gameover_begin() {
  g.level_intro=0;
  g.level_report=0;
  g.hello=0;
  g.gameover=1;
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
  render_kv_int(y,"Fish",4,g.fishc_total); y+=20;
  if (g.bonusc_possible) { render_kv_int2(y,"Bonus",5,g.bonusc_total,g.bonusc_possible); y+=20; }
  y+=20;
  
  if (g.new_hi_score) {
    render_string_centered(y,"New high score!",15);
    y+=20;
  } else if (memcmp(g.hiscore,"000000",6)) {
    render_kv(y,"High score",10,g.hiscore,6);
    y+=20;
  }
  if (g.new_hi_time) {
    render_string_centered(y,"New best time!",14);
    y+=20;
  } else if (memcmp(g.hitime,"00:00:00.000",12)) {
    int trimc=0;
    if (!memcmp(g.hitime,"00:0",4)) trimc=4;
    else if (!memcmp(g.hitime,"00",2)) trimc=3;
    else if (g.hitime[0]=='0') trimc=1;
    render_kv(y,"Best time",9,g.hitime+trimc,12-trimc); y+=20;
  }
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
  else if (g.gameover) result=modal_update_1(&g.gameover,elapsed);
  switch (result) {
    case MODAL_UPDATE_NEXT_LEVEL: {
        g.fishc_total+=g.fishc_level;
        g.fishc_level=0;
        if (g.bonus) {
          if (g.bonus_ok) g.bonusc_total++;
          g.bonusc_possible++;
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
