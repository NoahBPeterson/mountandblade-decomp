// field_panel.h: the battle-scene field panel (a MissionPanel, src/game/ui/mission_ui.h) and the objects it drives, used by
// mission_3.cpp (0x48d020..0x48d470) and mission_4.cpp (0x48f020, 0x48f120). Names provisional; every offset cites the
// function that reads it.
#ifndef GAME_MISSION_UI2_FIELD_PANEL_H
#define GAME_MISSION_UI2_FIELD_PANEL_H
#include <stddef.h>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh_x.h"
#include "../ui/mission_ui.h"

// The panel is a MissionPanel (src/game/ui/mission_ui.h: draw 0x488d50 is its tail call): its five child meshes at +0x108 are
// the bound shapes, its two draw children at +0x480 are the shape mirror (slot 1 is the mirror panel) and `host` (+0x490) is the
// scene widget. Only the members of other types are declared here.
struct FieldV2 { float x, y; FieldV2(float a, float b) { x = a; y = b; } };
typedef mesh_x FieldShape;                  // a child mesh as the bounds code sees it: the AABB at +0x22c is x0, y0 / x1, y1 (compute_bounds 0x419300: mesh_x.h)
struct FieldVisual {                        // the view object (a camera): colour at +0x14c, floats at +0xf0/+0xf4
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54();
    virtual void set_pos(float* p);         // +0x58
    virtual void v5c(); virtual void v60();
    virtual void set_xy(float* v);          // +0x64
    virtual void v68();
    virtual void set_size(float* v);        // +0x6c
    virtual void v70(); virtual void v74(); virtual void v78();
    virtual void v7c(); virtual void v80(); virtual void v84(); virtual void v88();
    virtual void v8c(); virtual void v90();
    virtual void get_state(float* out);     // +0x94
    virtual void set_color(unsigned* c);    // +0x98
    virtual void v9c(); virtual void va0();
    virtual void text_size(float* out, const tstring* s);   // +0xa4 (the hidden return slot first: MissionUi::get_text_size)
    char pad4[0xf0 - 4];
    float f0;                               // +0xf0
    float f4;                               // +0xf4
    char padf8[0x13c - 0xf8];
    float f13c;                             // +0x13c (0x48f020)
    char pad140[0x14c - 0x140];
    unsigned color;                         // +0x14c
};
struct FieldHost : MissionUi {              // the scene widget (MissionPanel::host): the view slot at +0x17c
    int f178;                               // +0x178
    FieldVisual** view;                     // +0x17c
};
struct FieldPanel : MissionPanel {      // MissionPanel is 0x498 bytes: its __int64 clocks align it to 8 (host +0x490, then 4 bytes of tail padding)
    char pad498[0x49c - 0x498];
    float f49c;                             // +0x49c
    float f4a0;                             // +0x4a0
    FieldHost* scene() const { return (FieldHost*)host; }
    FieldPanel* mirror() const { return (FieldPanel*)slots[1]; }
    FieldShape* shape(int i) const { return (FieldShape*)tracks[i]; }

    void update_bounds();                   // 0x48d020 (the union of the shapes' bounds)
    void set_color(unsigned rgb);           // 0x48d350
    void set_alpha(unsigned a);             // 0x48d3c0
    void update();                          // 0x48d470
    void relayout(const tstring* text);     // 0x48f020 (ret 4: re-places the scene's view after a text change)
};
typedef char assert_field_panel_f49c[(offsetof(FieldPanel, f49c) == 0x49c) ? 1 : -1];

#endif // GAME_MISSION_UI2_FIELD_PANEL_H
