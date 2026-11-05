// rot2.h: a 2x2 rotation matrix (four floats). rotate_2x2 (0x4f3b40, thiscall, ret 4: the angle) turns it in place; defined in
// game/char_sheet/terrain_page_input.cpp. One class for every caller (char_sheet, inventory_panel/mission_ctx, mission5/mesh_3).
#ifndef MB_ROT2_H
#define MB_ROT2_H
struct rot2 { float m[4]; void rotate_2x2(float angle); };  // 0x4f3b40
#endif
