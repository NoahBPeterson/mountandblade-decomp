// cs_res.h: the resource manager g_alloc (0x5d6fa0) and mesh_x, for the char_sheet modules that do not use game/res_mgr.h (their mesh
// class is their own: the return type of get_object is part of the declaration, here the definition's rgl::mesh).
#ifndef CHAR_SHEET_CS_RES_H
#define CHAR_SHEET_CS_RES_H
// resource manager g_alloc (0x5d6fa0): get_object returns the mesh resource, which the callers clone (mesh_x::clone 0x41c6d0)
namespace rgl { struct mesh; struct string; struct matrix; }
struct mesh_x {                                                    // rgl/mesh_x.h
    mesh_x* clone();                                               // 0x41c6d0
    void compute_bounds();                                         // 0x419300
    void transform(rgl::mesh* src, const rgl::matrix* m, unsigned color);   // 0x47aa40 (ret 0xc)
};
struct resmgr {
    rgl::mesh* get_object(const rgl::string* name);   // 0x419540 (core/resmgr.cpp; the name is an rgl::string, tstring is its twin)
};
extern resmgr g_alloc;                                             // 0x5d6fa0
#endif
