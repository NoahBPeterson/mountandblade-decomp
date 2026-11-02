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
extern int g_a1711c; // 0xa1711c
extern int g_key_171fc; // 0xa171fc zoom out
extern int g_key_17204; // 0xa17204 pitch down
extern int g_key_1720c; // 0xa1720c zoom in
extern int g_key_17214; // 0xa17214 pitch up
extern int g_key_17454; // 0xa17454 roll with the vertical axis
extern int g_key_17458; // 0xa17458 turn (heading)
extern int g_a1751c; // 0xa1751c
extern int g_a17854; // 0xa17854
extern float g_axis_x; // 0xa178d4 first input axis (mouse dx)
extern float g_axis_vert; // 0xa178d8 second input axis (mouse dy)
extern int g_skill_clamp; // 0x8ee3ac4 upper clamp of the skill offset

#endif // GAME_UI_GLOBALS_H
