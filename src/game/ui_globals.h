// ui_globals.h: the engine-core globals the UI / camera / face-generator modules read (viewport size, window-dirty flag,
// mission clock, developer mode, input key and axis state). A leaf header: game_globals.h includes it, and the modules whose
// own declarations of other globals clash with game_globals.h include only this one. One definition each (the owner is named).
#ifndef GAME_UI_GLOBALS_H
#define GAME_UI_GLOBALS_H

extern int g_dev_mode; // 0x5a9fb0 developer/edit-mode flag (text resources .trf, text log, no terrain camera)
extern int g_screen_w; // 0x5d75c8 viewport width
extern int g_screen_h; // 0x5d75cc viewport height
extern unsigned char g_gui_dirty; // 0x5d8788 set whenever a window was installed
extern int g_mission_clock; // 0x5d8790 mission time; 0 = no mission
extern float g_fov_k; // 0x5e20a8
extern int g_5e28c8; // 0x5e28c8
extern int g_skill_clamp; // 0x8ee3ac4 upper clamp of the skill offset

#endif // GAME_UI_GLOBALS_H
