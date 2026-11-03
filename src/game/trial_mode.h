// trial_mode.h: the trial-build flag (g_trial_mode 0x5e29c4: set when the game runs as the trial; the download button, the trial
// limits). Its own header so the TUs that cannot take game_globals.h next to their own table declarations (rgl/mesh2/gui_edit.h) name it too.
#ifndef GAME_TRIAL_MODE_H
#define GAME_TRIAL_MODE_H
extern int g_trial_mode; // 0x5e29c4 (defined in options_panel/world_app.cpp)
#endif
