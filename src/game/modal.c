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
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  graf_set_image(&g.graf,RID_image_fonttiles);
  render_string_centered(FBH>>1,"hello",5);
  //TODO
}

/* Begin Game Over.
 */
 
void gameover_begin() {
  g.level_intro=0;
  g.level_report=0;
  g.hello=0;
  g.gameover=1;
  play_song(RID_song_fishie_fishie);
}

/* Render Game Over.
 */
 
void gameover_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  graf_set_image(&g.graf,RID_image_fonttiles);
  render_string_centered(FBH>>1,"game over",9);
  //TODO
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
        if (game_start_level(g.mapid+1)<0) {
          gameover_begin();
        }
      } break;
    case MODAL_UPDATE_START_GAME: {
        if (game_start_level(1)<0) {
          egg_terminate(1);
        }
      } break;
    case MODAL_UPDATE_FULL_RESET: {
        hello_begin();
      } break;
  }
}
/*
  if (g.level_intro==1) {
    if (!(g.input[0]&important_buttons)) g.level_intro=2;
  } else if (g.level_intro==2) {
    if (g.input[0]&EGG_BTN_SOUTH) {
      SND(uiactivate)
      g.level_intro=3;
    }
  } else if (g.level_intro==3) {
    if (!(g.input[0]&important_buttons)) g.level_intro=0;
    
  } else if (g.level_report==1) {
    if (!(g.input[0]&important_buttons)) g.level_report=2;
  } else if (g.level_report==2) {
    if (g.input[0]&EGG_BTN_SOUTH) {
      SND(uiactivate)
      g.level_report=3;
    }
  } else if (g.level_report==3) {
    if (!(g.input[0]&important_buttons)) {
      g.level_report=0;
      if (game_start_level(g.mapid+1)<0) {
        //TODO game over, you win
        if (game_start_level(1)<0) {
          egg_terminate(1);
          return;
        }
      }
    }
  }
}
/**/
