// screen_cfg.h: the screen/device settings objects of 0x4ca550..0x4ca6c0: the window-mode toggle widget, the
// device-state request queue of the GUI (screen_cfg) and the device list. Shared by gui_edit.cpp (0x4ca550 only) and
// mesh_2.cpp. Slot numbers are from the call sites; only the slots used here are modelled.
// Merged from slice s004ca550.
#ifndef MESH2_SCREEN_CFG_H
#define MESH2_SCREEN_CFG_H
#include <deque>
#include <windows.h>
#include <d3d9.h>
#include "../../game/tstring.h"
#include "../../game/strat/gui_manager.h"
#include "../../game/window_mgr.h"

// the render-device record's vtable
struct dev_rec {
    virtual void s00() = 0;
    virtual void s04() = 0;
    virtual void s08() = 0;
    virtual void s0c() = 0;
    virtual void s10() = 0;
    virtual void s14() = 0;
    virtual void s18() = 0;
    virtual void s1c() = 0;
    virtual void s20() = 0;
    virtual void s24() = 0;
    virtual void s28() = 0;
    virtual void s2c() = 0;
    virtual void s30() = 0;
    virtual void s34() = 0;
    virtual void s38() = 0;
    virtual void s3c() = 0;
    virtual void s40() = 0;
    virtual bool is_fullscreen(int which) = 0;   // +0x44
    virtual void s48() = 0;
};

// slots 0x04 (no-arg restore) and 0x20 (the state index stored in g_5d75e0)
struct dev_item {
    virtual void s00() = 0;
    virtual void restore() = 0;                  // +0x04
    virtual void s08() = 0;
    virtual void s0c() = 0;
    virtual void s10() = 0;
    virtual void s14() = 0;
    virtual void s18() = 0;
    virtual void s1c() = 0;
    virtual int index() = 0;                     // +0x20
};

// the render device seen through slot 0x30 (set_index)
struct dev_index {
    virtual void s00() = 0;
    virtual void s04() = 0;
    virtual void s08() = 0;
    virtual void s0c() = 0;
    virtual void s10() = 0;
    virtual void s14() = 0;
    virtual void s18() = 0;
    virtual void s1c() = 0;
    virtual void s20() = 0;
    virtual void s24() = 0;
    virtual void s28() = 0;
    virtual void s2c() = 0;
    virtual void set_index(unsigned char i) = 0; // +0x30
};

extern gui_manager g_gui;                        // 0x5d7080
#include "../../include/renderer.h"   // canonical: g_render.dev (0xa17f28)
#include "../../game/key_names.h"      // g_keyboard (0xa170c8)

// the window manager (game/window_mgr.h, g_windows 0xa73980: 30 screen objects, then the request queue at +0x78) with the
// apply step of this module; the screens are read as dev_item here
struct screen_cfg : WindowMgrW {
    __declspec(noinline) void apply();           // 0x4ca600
};
inline screen_cfg& windows_cfg() { return static_cast<screen_cfg&>(g_windows); }

// a gadget with the current-mode flag at +0x0c and the name text at +0x24
struct screen_gadget {
    char pad0[0xc];
    int* flag;                                   // +0x0c
    char pad10[0x14];
    tstring* text;                               // +0x24
};

// a list of device records: +0x38 .. +0x50 (the last slot holds the selected index)
struct dev_list {
    void select_windowed(int a);                 // 0x4ca6c0
    char pad0[0x38];
    dev_rec* dev[7];                             // +0x38 .. +0x50
    int nreg;                                    // +0x54
};

#endif
