// map_list.h: the game-log / map-text list widget MWidget (vtable 0x5995c0, size 0x1d0, constructor 0x4fd150): a MissionUi that
// lays its text out with the tag-open rectangles of the map spots and answers clicks on them. ONE definition for
// src/game/agent5 (the game-log window, agent_hud.cpp) and src/game/inventory_panel (note_window.cpp: the report and link lists).
// The three member lists are the VC7.1 vector layout (allocator word, first, last, end) as the layout code fills them.
#ifndef GAME_UI_MAP_LIST_H
#define GAME_UI_MAP_LIST_H
#include <stddef.h>
#include <vector>
#include "./mission_ui.h"
#include "../../rgl/rgl_math.h"
#include "../../include/vec_xlen.h"

struct IntList {                                                  // a VC7.1 vector<int> (alloc slot, first, last, end)
    int alloc;
    int* first;
    int* last;
    int* end;
    __declspec(noinline) bool buy(unsigned n);                    // 0x4fd490 (ret 4: the allocation of the vector<int>'s first block)
    void xlen() { VEC_XLEN_CALL(this); }                          // 0x43e2a0 (std::vector<int>::_Xlen, agent3/agent_combat.cpp)
    void free_block() { if (first) operator delete(first); first = 0; last = 0; end = 0; }   // the inlined vector release of ~MWidget
};
struct V4List {                                                   // a VC7.1 vector<vector4>
    int alloc;
    rgl::vector4* first;
    rgl::vector4* last;
    rgl::vector4* end;
    struct iterator { rgl::vector4* p; iterator() {} };
    __declspec(noinline) void push_back(const rgl::vector4& v);   // 0x4fdff0 (ret 4; called out of line by 0x4fd700)
    __declspec(noinline) iterator insert(iterator where, const rgl::vector4& val);   // 0x4fe060 (ret 0xc)
    void free_block() { if (first) operator delete(first); first = 0; last = 0; end = 0; }
};
struct MapText { int a; float w; };                               // the 8-byte layout result (width, height)

struct MWidget : MissionUi {
    V4List rects_a;                                               // +0x178 tag-open rectangles (0x4fd1d0 scans them; 0x4fd700 fills them)
    V4List rects_b;                                               // +0x188
    IntList list_c;                                               // +0x198 (0x4fd490 builds it; 0x4fd3d0 clears it)
    IntList list_d;                                               // +0x1a8
    IntList list_e;                                               // +0x1b8 the tag index of every rectangle of rects_a
    int cur;                                                      // +0x1c8 selected spot (-1 = none)
    int sel;                                                      // +0x1cc
    MWidget();                                                    // 0x4fd150
    virtual ~MWidget();                                           // +0x00 0x4fd3d0 (the deleting destructor is 0x4fd3b0)
    virtual char key(unsigned ev);                                // +0x8c 0x4fd1d0
    virtual void get_xy(float* out);                              // +0x94 0x4fd4f0 (the text rect, through the hidden result pointer)
};
typedef char mwidget_cur_at_1c8[(offsetof(MWidget, cur) == 0x1c8) ? 1 : -1];
typedef char mwidget_size_check[(sizeof(MWidget) == 0x1d0) ? 1 : -1];

#endif // GAME_UI_MAP_LIST_H
