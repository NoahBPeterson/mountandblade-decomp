// shadow_map.h: the shadow map as far as the light-camera attach (0x4711e0) and the HUD meter table (0x470d80) go.
#ifndef TERRAIN_B_SHADOW_MAP_H
#define TERRAIN_B_SHADOW_MAP_H
#include "./cell_grid.h"

extern int g_shadow_quality;         // 0x5aa000
extern int g_shadow_quality_req;     // 0x5aa004

struct ShadowMap {
    char pad0[0x1f0];
    CellGrid* cam;                   // 0x1f0 the light camera (a CellGrid's view block)
    void release_all();              // 0x463b70
    void attach_camera(CellGrid* c); // 0x4711e0
};

// a HUD element table: one widget pointer per slot at +0x180
struct Meter { void set_value(float* v); };          // 0x439000 (thiscall, ret 4; ICF-folded with node_groups::cap_all)
struct MeterBar {
    char pad0[0x180];
    Meter* slots[16];                                // 0x180
    void set_slot(int idx, int value);               // 0x470d80
};

#endif // TERRAIN_B_SHADOW_MAP_H
