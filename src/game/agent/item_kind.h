// item_kind.h: the carry-frame helper (pos2, identity_pos) and the resource lookup item_kind.cpp (0x433bf0..0x434b20) uses
// beside the item definition record. The record itself (ItemKind, ItemMesh, g_items, itp_type_*, imod_*) is the shared
// src/include/item_kind.h, included here. Names provisional.
#ifndef AGENT_ITEM_KIND_H
#define AGENT_ITEM_KIND_H
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/rgl_string.h"
#include "../tstring.h"
#include "../../include/item_kind.h"

using rgl::vector4;

// local frame of a carried item: matrix plus the three rotation members of 0x40e570 / 0x40e670 / 0x40e770
// (rgl::matrix::rotate_about_s / _f / _u, declared in match/include/rgl.h)
struct pos2 : rgl::matrix {
    void rotate_fu(const float* angle) { rotate_about_s(angle); }     // 0x40e570 rotates rows f and u
    void rotate_su(const float* angle) { rotate_about_f(angle); }     // 0x40e670 rotates rows s and u
    void rotate_sf(const float* angle) { rotate_about_u(angle); }     // 0x40e770 rotates rows s and f
};
struct pos;                                 // the global-namespace twin of rgl::matrix that the definition of 0x40cd00 names (src/include/mat4.h)
pos2 identity_pos();                        // 0x40cd00 (cdecl, hidden return pointer: identity rotation, zero origin; item_kind.cpp calls the definition's own pos* form)

#endif // AGENT_ITEM_KIND_H
