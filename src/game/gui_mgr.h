// gui_mgr.h: the gui manager object (global 0x5d7080) as the screen-building and main-loop TUs call into it. The full
// object is gui_manager (strat/gui_manager.h, 0xb350 bytes, which those TUs cannot include next to their own widget
// types); this is the member-function view they share. One definition for what the TUs called GuiManager / GuiMgr:
// inventory_panel/{game_app,char_sheet,note_window,loading_screen}.cpp, agent5/agent_hud.h, char_sheet/screen_init.h.
// The views agree on the address of every function they share: reset 0x42d1b0 (also called `init`) and the
// ret-4 "make this window the active root" 0x49b120 (called push / add_screen / add_root; takes the address of a
// pointer to the window), all reached through the one global 0x5d7080.
#ifndef GAME_GUI_MGR_H
#define GAME_GUI_MGR_H
#include "tstring.h"

struct Widget;
struct GuiManager {
    void reset();                               // 0x42d1b0
    void add_root(void* window);                // 0x49b120 (ret 4: the address of a pointer to the window)
    void frame(float dt);                       // 0x4ebfd0
    void add_log(tstring s, int type);          // 0x452c00 (tstring by value, then the type)
    void set_active(Widget* w);                 // 0x50d0e0
    void build_frames();                        // 0x50f930
    void build_quad();                          // 0x50f8a0
};
#endif
