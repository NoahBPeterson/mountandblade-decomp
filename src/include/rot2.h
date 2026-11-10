// rot2.h: a 2x2 rotation matrix (two rows of two floats). rotate_2x2 (0x4f3b40, thiscall, ret 4: the angle) turns it in
// place; defined in game/char_sheet/terrain_page_input.cpp. One class for every caller (char_sheet,
// inventory_panel/mission_ctx, mission5/mesh_3, mission5/mesh_poly_ray).
#ifndef MB_ROT2_H
#define MB_ROT2_H
// The row type with the engine's inline 2-D operators: rotate_2x2 is byte-exact only through them (each returned
// temporary keeps x on the x87 stack and spills y; the scalar is taken by const reference, as in mission_ctx's Vec2f).
// Kept here rather than on vec.h's shared Vec2: adding inline members to a header 112 sources include shifted cl's
// symbol ids and swapped two stack slots in MapPanel::draw_party_line 0x51c090 (docs/codegen.md 6.2).
struct rot2_row {
    float x, y;
    rot2_row() {}
    rot2_row(float a, float b) : x(a), y(b) {}
    rot2_row operator*(const float& f) const { return rot2_row(x * f, y * f); }
    rot2_row operator+(const rot2_row& v) const { return rot2_row(x + v.x, y + v.y); }
    rot2_row operator-(const rot2_row& v) const { return rot2_row(x - v.x, y - v.y); }
};
struct rot2 { rot2_row r[2]; void rotate_2x2(float angle); };  // 0x4f3b40
#endif
