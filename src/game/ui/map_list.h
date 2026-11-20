// map_list.h: the game-log / map-text list widget MWidget (vtable 0x5995c0, size 0x1d0, constructor 0x4fd150): a MissionUi that
// lays its text out with the tag-open rectangles of the map spots and answers clicks on them. ONE definition for
// src/game/agent5 (the game-log window, agent_hud.cpp) and src/game/inventory_panel (note_window.cpp: the report and link lists).
// pages / page_args / rect_links are std::vectors; the two rectangle lists are still the V4List view of a VC7.1 vector<vector4>.
#ifndef GAME_UI_MAP_LIST_H
#define GAME_UI_MAP_LIST_H
#include <stddef.h>
#include "../../include/layout32.h"
#include <vector>
#include "./mission_ui.h"
#include "../../rgl/rgl_math.h"
#include "./note_page.h"                                         // NotePage: the element of pages

struct V4List {                                                   // a VC7.1 vector<vector4>
    int alloc;
    rgl::vector4* first;
    rgl::vector4* last;
    rgl::vector4* end;
    struct iterator { rgl::vector4* p; iterator() {} };
    V4List() { first = 0; last = 0; end = 0; }                    // the inlined vector() (_Buy(0))
    ~V4List() { free_block(); }                                   // the inlined ~vector (_Tidy)
    __declspec(noinline) void push_back(const rgl::vector4& v);   // 0x4fdff0 (ret 4; called out of line by 0x4fd700)
    __declspec(noinline) iterator insert(iterator where, const rgl::vector4& val);   // 0x4fe060 (ret 0xc)
    void free_block() { if (first) operator delete(first); first = 0; last = 0; end = 0; }
};

struct MWidget : MissionUi {
    V4List rects_a;                                               // +0x178 tag-open rectangles (0x4fd1d0 scans them; 0x4fd700 fills them)
    V4List rects_b;                                               // +0x188
    std::vector<NotePage> pages;                                  // +0x198 the page each link opens (0x4fd700 fills it; _Buy 0x4fd490, ~vector 0x41f4b0)
    std::vector<int> page_args;                                   // +0x1a8 its argument: the <tag=value> id (troop / faction / party / quest); _Insert_n 0x43e7b0, push_back 0x440b30
    std::vector<int> rect_links;                                  // +0x1b8 the link index of every rectangle of rects_a
    int cur;                                                      // +0x1c8 selected spot (-1 = none)
    int sel;                                                      // +0x1cc
    MWidget();                                                    // 0x4fd150
    virtual ~MWidget();                                           // +0x00 0x4fd3d0 (the deleting destructor is 0x4fd3b0): the members' destructors only
    virtual char key(unsigned ev);                                // +0x8c 0x4fd1d0
    virtual V2 get_xy();                                          // +0x94 0x4fd4f0 (the text rect, by value)
};
MB_LAYOUT32_ASSERT(mwidget_cur_at_1c8, (offsetof(MWidget, cur) == 0x1c8));
MB_LAYOUT32_ASSERT(mwidget_size_check, (sizeof(MWidget) == 0x1d0));

#endif // GAME_UI_MAP_LIST_H
