// item_kind.h: the carry-frame helper (pos2, identity_pos) and the resource lookup item_kind.cpp (0x433bf0..0x434b20) uses
// beside the item definition record. The record itself (ItemKind, ItemMesh, g_items, itp_type_*, imod_*) is the shared
// src/include/item_kind.h, included here. Names provisional.
#ifndef AGENT_ITEM_KIND_H
#define AGENT_ITEM_KIND_H
#include <vector>
#include "rgl.h"
#include "../tstring.h"
#include "../../include/item_kind.h"

using rgl::vector4;

// the resource manager's object lookup (0x419540, ret 4) as item_kind.cpp uses it; the class itself is migrated
// with the 0x419420 object (src/rgl/resource.h does not declare get_object yet, and defines g_resmgr with its own
// type, so that header cannot be included here).
namespace rgl { struct mesh; }
struct res_shape;
struct resmgr {
    rgl::mesh* get_object(const rgl::string* name);                 // 0x419540, as the definition (src/rgl/core/resmgr.cpp) declares it
    // a tstring and an rgl::string are the same 16 bytes (pos, buf, cap, len); the resource is an rgl::mesh (res_obj is its opaque view here)
    res_obj* get_object(const tstring* name) { return (res_obj*)get_object((const rgl::string*)name); }
    std::vector<res_obj*> find_meshes(const tstring* prefix);       // 0x41b960 (agent4: the meshes whose names start with prefix)
    void* get_material(const tstring* name);                        // 0x418a70 (agent4: a material by name)
    res_shape* get_object60(const tstring* name);                   // 0x418c10 (agent4: a body resource)
};
extern resmgr g_resmgr;                                         // 0x5d6fa0

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
