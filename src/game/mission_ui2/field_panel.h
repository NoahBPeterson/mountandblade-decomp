// field_panel.h: the battle-scene field panel (a MissionPanel, src/game/ui/mission_ui.h) and the objects it drives, used by
// mission_3.cpp (0x48d020..0x48d470) and mission_4.cpp (0x48f020, 0x48f120). Names provisional; every offset cites the
// function that reads it.
#ifndef GAME_MISSION_UI2_FIELD_PANEL_H
#define GAME_MISSION_UI2_FIELD_PANEL_H
#include <stddef.h>
#include <vector>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh.h"
#include "../ui/mission_ui.h"
#include "../party2/camera_class.h"      // Camera: the panel is a list/scene view of that layout

// The panel is a MissionPanel (src/game/ui/mission_ui.h: draw 0x488d50 is its tail call): its five child meshes at +0x108 are
// the bound shapes, its two draw children at +0x480 are the shape mirror (slot 1 is the mirror panel) and `host` (+0x490) is the
// scene widget. Only the members of other types are declared here.
struct FieldV2 { float x, y; FieldV2(float a, float b) { x = a; y = b; } };
typedef rgl::mesh FieldShape;                  // a child mesh as the bounds code sees it: the AABB at +0x22c is x0, y0 / x1, y1 (compute_bounds 0x419300: mesh.h)
struct FieldHost : MissionUi {              // the scene widget (MissionPanel::host)
    // +0x178: the view slot is element 0. The original reads it through operator[] at every use (base -> _Myfirst ->
    // element chained in one scratch register: 0x48d350, 0x48d3c0, 0x48f020, 0x48f120).
    std::vector<MissionUi*> views;     // the view widget: its colour (color14c), position (screen), f13c
};
struct FieldPanel : Camera {
    FieldHost* scene() const { return (FieldHost*)target; }
    FieldPanel* mirror() const { return (FieldPanel*)follow[1]; }
    FieldShape* shape(int i) const { return (FieldShape*)tracks[i]; }

    void set_color(unsigned rgb);           // 0x48d350
    void set_alpha(unsigned a);             // 0x48d3c0
    void update();                          // 0x48d470
    void relayout(const tstring* text);     // 0x48f020 (ret 4: re-places the scene's view after a text change)
};

#endif // GAME_MISSION_UI2_FIELD_PANEL_H
