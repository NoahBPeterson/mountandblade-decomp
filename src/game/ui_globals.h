// ui_globals.h: the engine-core globals the UI / camera / face-generator modules read (viewport size, window-dirty flag,
// mission clock, field-of-view factor). A leaf header: game_globals.h includes it, and the modules whose own declarations of
// other globals clash with game_globals.h include only this one. The developer-mode flag is g_cfg.enable_edit_mode
// (config/config_block.h), not a global of its own.
// None of these is DEFINED: each is a label inside the gui manager g_gui (0x5d7080, 0xb350 bytes), which the glue lays out once
// from the original's bytes and Core::update_view_rect (0x4bace0) / gui_manager write through their own members: a definition
// here is a second storage that those writers never reach (g_screen_w / g_screen_h read 0 in the save/load screen, whose
// SetViewport and Clear then covered nothing and every frame stayed on screen). tools/relink/data_size_audit.py fails the
// build on a definition inside another object's range.
#ifndef GAME_UI_GLOBALS_H
#define GAME_UI_GLOBALS_H

extern int g_screen_w; // 0x5d75c8 viewport width
extern int g_screen_h; // 0x5d75cc viewport height
extern unsigned char g_gui_dirty; // 0x5d8788 set whenever a window was installed
extern int g_mission_clock; // 0x5d8790 mission time; 0 = no mission
extern float g_fov_k; // 0x5e20a8
extern int g_5e28c8; // 0x5e28c8

#endif // GAME_UI_GLOBALS_H
