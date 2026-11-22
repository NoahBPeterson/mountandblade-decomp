// map_list.h: the game-log / map-text list widget MWidget (vtable 0x5995c0, size 0x1d0, constructor 0x4fd150): a MissionUi that
// lays its text out with the tag-open rectangles of the map spots and answers clicks on them. ONE definition for
// src/game/agent5 (the game-log window, agent_hud.cpp) and src/game/inventory_panel (notes_window.cpp: the report and link lists).
// pages / page_args / rect_links are std::vectors; the two rectangle lists are std::vector<vector4>.
#ifndef GAME_UI_MAP_LIST_H
#define GAME_UI_MAP_LIST_H
#include <stddef.h>
#include "engine/core/layout32.h"
#include <vector>
#include "game/ui/widgets/widget_base.h"
#include "engine/math/rgl_math.h"
#include "game/ui/widgets/note_page.h"                                         // NotePage: the element of pages

struct MWidget : MissionUi {
    std::vector<rgl::vector4> rects_a;                                               // +0x178 tag-open rectangles (0x4fd1d0 scans them; 0x4fd700 fills them)
    std::vector<rgl::vector4> rects_b;                                               // +0x188
    std::vector<NotePage> pages;                                  // +0x198 the page each link opens (0x4fd700 fills it; _Buy 0x4fd490, ~vector 0x41f4b0)
    std::vector<int> page_args;                                   // +0x1a8 its argument: the <tag=value> id (troop / faction / party / quest); _Insert_n 0x43e7b0, push_back 0x440b30
    std::vector<int> rect_links;                                  // +0x1b8 the link index of every rectangle of rects_a
    int cur;                                                      // +0x1c8 selected spot (-1 = none)
    int sel;                                                      // +0x1cc
    MWidget();                                                    // 0x4fd150
    virtual ~MWidget();                                           // +0x00 0x4fd3d0 (the deleting destructor is 0x4fd3b0): the members' destructors only
    virtual char key(unsigned ev);                                // +0x8c 0x4fd1d0
    virtual V2 get_xy();                                          // +0x94 0x4fd4f0 (the text rect, by value)
    __declspec(noinline) void get3(int* page, int* arg, int* spot);   // 0x533a60 (game/ui/screens/scroll_list_screen.cpp, ret 0xc): the selected spot's page / argument
};
MB_LAYOUT32_ASSERT(mwidget_cur_at_1c8, (offsetof(MWidget, cur) == 0x1c8));
MB_LAYOUT32_ASSERT(mwidget_size_check, (sizeof(MWidget) == 0x1d0));

#endif // GAME_UI_MAP_LIST_H
