// map_icon.h: the map-icon table g_map_icons (0x5e29f0, count g_map_icon_count 0x5e29f4) and its 0x48-byte record MapIcon, one
// definition for the module loader (inventory_panel/module_files.cpp: `new MapIcon[n]`, the inline constructor), the party icon
// builder (party2/party_2.cpp Party::update_flags 0x4b7840, the icon sound), the tableau spawner (world_data/obj_tbl.cpp 0x499540)
// the teardown (cls_map/cls_map.cpp: `delete[]`) and the reader 0x4c20c0 (triggers/trigger_vec.cpp), which fills the trigger list
// through SimpleTriggerVec's resize / stamp (trigger_vecs.h: the same three words as SimpleTriggerVec, with 0x30-byte trigger_entry records).
#ifndef GAME_MAP_ICON_H
#define GAME_MAP_ICON_H
#include "./tstring.h"
#include "../rgl/rgl_math.h"            // rgl::vector4
#include "trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list
struct stream_base;
struct MapIcon {
    tstring name;                       // +0x00 the icon id ("player", "town", ...)
    int flags;                          // +0x10 bit 0: plain model, no fade ramp (0x4b7840)
    tstring mesh;                       // +0x14 the mesh resource (0x4b7840, 0x499540)
    float scale;                        // +0x24 (0x4b7840, 0x4b7e50)
    int sound;                          // +0x28 the moving sound (party_2.cpp)
    rgl::vector4 offset;                // +0x2c the rider mesh translation: x, y, z read by 0x4c20c0, w unused (0x4b7840)
    SimpleTriggerVec triggers;                 // +0x3c the icon's trigger entries (0x4b7840 runs them: update(-0x46))
    MapIcon() {}
    void read(stream_base& s);          // 0x4c20c0 (ret 4; triggers/trigger_vec.cpp)
};
extern MapIcon* g_map_icons;            // 0x5e29f0 (defined in inventory_panel/module_files.cpp)
#endif // GAME_MAP_ICON_H
