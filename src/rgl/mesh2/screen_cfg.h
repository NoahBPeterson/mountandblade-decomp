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
extern IDirect3DDevice9* g_dev_renderer;         // 0xa17f28 (the Direct3D device)
extern int g_windowed_setting;                   // 0xa17144
extern int g_windowed_other;                     // 0xa17544
extern unsigned char g_5d75e0;                   // 0x5d75e0

struct screen_cfg {
    __declspec(noinline) void apply();           // 0x4ca600
    dev_item* items[1];                          // +0x00: array of records, indexed by the queue
    char pad4[0x74];
    std::deque<int> req;                         // +0x78 the request queue (VC7.1 deque of 4-int blocks)
};
extern screen_cfg g_screen;                      // 0xa73980

// a gadget with the current-mode flag at +0x0c and the name text at +0x24
struct screen_gadget {
    char pad0[0xc];
    int* flag;                                   // +0x0c
    char pad10[0x14];
    tstring* text;                               // +0x24
};

// the screen-text widget
struct screen_text {
    void refresh(int handle);                    // 0x4ca550
    char pad0[0x38];
    void* win;                                   // +0x38: the window record; a tstring at +0x128
    dev_rec* dev1;                               // +0x3c
    dev_rec* dev2;                               // +0x40
    char pad44[0x8];
    screen_gadget* g;                            // +0x4c
    tstring* text;                               // +0x50
};

// a list of device records: +0x38 .. +0x50 (the last slot holds the selected index)
struct dev_list {
    void select_windowed(int a);                 // 0x4ca6c0
    char pad0[0x38];
    dev_rec* dev[7];                             // +0x38 .. +0x50
    int nreg;                                    // +0x54
};

#endif
