// widget_mgr_w.h: the members of the editor / presentation widget manager (config/widget_mgr.h widget_mgr: widgets vector at +0,
// scene container at +0x2c) that sit in the 0x4940xx..0x4996xx range, as a derived view with no data members. Shared by
// world_data.cpp (0x494110), mesh_hud.cpp (0x495970) and obj_tbl.cpp (0x4995e0, 0x4996e0). Names provisional.
#ifndef GAME_WORLD_DATA_WIDGET_MGR_W_H
#define GAME_WORLD_DATA_WIDGET_MGR_W_H
#include "../config/widget_mgr.h"

struct WidgetMgrW : widget_mgr {
    int add_button(const tstring* text, unsigned flags);        // 0x494110 (ret 8)
    int add_slider(int vmin, int vmax);                         // 0x495970 (ret 8)
    int add_mesh_widget(int index);                             // 0x4995e0 (ret 4)
    int add_mesh_widget2(int a, int b, float c);                // 0x4996e0 (ret 0xc)
};

#endif // GAME_WORLD_DATA_WIDGET_MGR_W_H
