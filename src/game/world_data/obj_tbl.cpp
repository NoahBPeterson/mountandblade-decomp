//-----------------------------------------------------------------------------
// File: obj_tbl.cpp  (0x499050..0x499830)
//
// Desc: resource record -> mesh -> entity helpers of the scene editor and the tableau spawners: the placed-mesh builder of the
//       scene table, the tableau table's mesh/entity spawners, and the two mesh-widget factories. Translation unit 87
//       (obj_tbl) of the TU map. Names provisional. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./obj_tbl.h"
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../config/widget_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../ui/mission_ui.h"
#include "./tableau.h"
#include "./widget_mgr_w.h"
#include "../../rgl/mesh.h"
#include "../../rgl/rgl_math.h"
#include "../../include/mesh_parts.h"
#include "../../include/mesh_prims.h"

// the mesh widget (MissionUi-derived, vtable 0x5922e8, 0x178 bytes): the slots the factories call
struct MeshWidgetVt {
    virtual void w0(); virtual void w1(); virtual void w2();
    virtual void w3();                                          // +0x0c
    virtual void w4(); virtual void w5(); virtual void w6(); virtual void w7(); virtual void w8(); virtual void w9();
    virtual void w10(); virtual void w11(); virtual void w12(); virtual void w13(); virtual void w14(); virtual void w15();
    virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19(); virtual void w20(); virtual void w21();
    virtual void w22(void* a);                                  // +0x58
    virtual void w23(); virtual void w24(); virtual void w25(); virtual void w26(); virtual void w27(); virtual void w28();
    virtual void w29(void* a);                                  // +0x74
    virtual void w30(); virtual void w31();
    virtual void w32(int a, rgl::mesh* m);                      // +0x80
};
struct MeshWidget : MissionUi {};

// @ 0x499050
rgl::mesh* scene_tbl::placed(int index)
{
    SceneRec48* recs = (SceneRec48*)this->recs;
    MeshY* m = (MeshY*)((gui_mesh*)((resmgr*)(void*)&g_resmgr_full)->get_object(&recs[index].name))->clone();
    if (recs[index].flags & 1)
        m->f214 += 0x2000;
    vector4 pos;
    SceneRec48* r = &recs[index];
    vector4 sc(recs[index].sx, recs[index].sy, recs[index].sz);
    pos.x = r->px;
    pos.y = r->py;
    pos.z = r->pz;
    Frame3 frame;
    identity3(&frame);
    float a = r->rx * 0.017453292f;
    ((rgl::matrix*)&frame)->rotate_about_s(&a);
    a = r->ry * 0.017453292f;
    ((rgl::matrix*)&frame)->rotate_about_f(&a);
    a = r->rz * 0.017453292f;
    ((rgl::matrix*)&frame)->rotate_about_u(&a);
    if (r->sx != g_one || r->sy != g_one || r->sz != g_one)
        m->set_color(sc);
    m->set_part(*(rgl::matrix*)&frame);
    if (pos.z * pos.z + pos.y * pos.y + pos.x * pos.x != g_zero)
        m->translate(&pos);
    return m;
}

// @ 0x4991d0
rgl::mesh* TableauTbl::make_mesh_from(const int* index, const void* from)
{
    MeshY* m = (MeshY*)new rgl::mesh;
    int i = *index;
    vector4 b, a;
    vec2 uv1, uv0;
    unsigned color;
    b.x = recs[i].f30;
    b.y = recs[i].f34;
    a.x = recs[i].f28;
    a.y = recs[i].f2c;
    color = 0xffffffff;
    uv1.x = 1.0f;
    uv1.y = 1.0f;
    uv0.x = 0.0f;
    uv0.y = 0.0f;
    b.z = 0.0f;
    a.z = 0.0f;
    m->add_quad(&a, &b, &uv0, &uv1, &color);
    skin* nr = TAB_ADD_SPAWNED(this, index, (const int*)from);   // 0x4b14b0
    if (m->res)
        --*(int*)((char*)m->res + 0x64);
    m->res = nr;
    if (nr)
        ++*(int*)((char*)nr + 0x64);
    return m;
}

// @ 0x4992b0
void TableauTbl::spawn(int index, int from, const rgl::matrix* frame, const rgl::vector4* scale)
{
    TableauWorld* w = worlds[worlds.size() - 1];
    TableauEntity* e = (TableauEntity*)((rgl::world*)(void*)w)->create_entity(9);
    MeshY* m = (MeshY*)make_mesh_from(&index, &from);
    if (!(0.99999f <= scale->x && scale->x < 1.00001f
       && 0.99999f <= scale->y && scale->y < 1.00001f
       && 0.99999f <= scale->z && scale->z < 1.00001f))
        m->set_color(*scale);
    ((Poser*)(void*)e)->add_extra((int)m);
    e->local = *frame;
}

// @ 0x499390
void TableauTbl::spawn_scaled(int index, const rgl::matrix* frame, const rgl::vector4* scale,
                              const float* alpha, const unsigned* color)
{
    TableauWorld* w = worlds[worlds.size() - 1];
    TableauEntity* e = (TableauEntity*)((rgl::world*)(void*)w)->create_entity(9);
    MeshY* m = (MeshY*)g_meshes.placed(index);
    if (!(0.99999f <= scale->x && scale->x < 1.00001f
       && 0.99999f <= scale->y && scale->y < 1.00001f
       && 0.99999f <= scale->z && scale->z < 1.00001f))
        m->set_color(*scale);
    if (*color != 0xffffffffu) {
        vector4 c = rgl::unpack_color(color);
        if (g_use_shaders) {
            c.x = pow((double)c.x, (double)2.2f);
            c.y = pow((double)c.y, (double)2.2f);
            c.z = pow((double)c.z, (double)2.2f);
        }
        ((mesh_x*)(void*)m)->set_bone_index((int)rgl::pack_color(&c));
    }
    if (*alpha < 0.99f) {
        float a;
        if (*alpha < 0.0f)
            a = 0.0f;
        else if (*alpha >= 1.0f)
            a = 1.0f;
        else
            a = *alpha;
        m->set_alpha(a);
    }
    ((Poser*)(void*)e)->add_extra((int)m);
    e->local = *frame;
}

// @ 0x499540
void TableauTbl::spawn_scaled2(int index, const rgl::matrix* frame, const float* scale)
{
    TableauWorld* w = worlds[worlds.size() - 1];
    TableauEntity* e = (TableauEntity*)((rgl::world*)(void*)w)->create_entity(9);
    MeshY* m = (MeshY*)((MeshX*)((gui_mesh*)((resmgr*)(void*)&g_resmgr_full)->get_object((rgl::string*)((char*)g_map_icons + index * 0x48 + 0x14))))->copy_of();
    if (!(0.99999f <= *scale && *scale < 1.00001f))
        m->scale(*scale);
    ((Poser*)(void*)e)->add_extra((int)m);
    e->local = *frame;
}

// @ 0x4995e0
int WidgetMgrW::create_mesh_overlay(int index)
{
    MeshWidget* w = (MeshWidget*)operator new(0x178);
    if (w) {
        new ((void*)w, raw_place()) MissionUi;                  // 0x48ae20
        *(void**)w = (void*)vtbl_5922e8;
    }
    vec2 v;
    v.x = 0.0f;
    v.y = 0.0f;
    ((MeshWidgetVt*)w)->w22(&v);
    *(int*)&v.x = 0x700;
    ((MeshWidgetVt*)w)->w29(&v);
    ((MeshWidgetVt*)w)->w32(0, g_meshes.placed(index));
    ((MeshWidgetVt*)w)->w3();
    scene->d50(w);
    widgets.push_back(w);
    return (int)widgets.size() - 1;
}

// @ 0x4996e0
int WidgetMgrW::create_mesh_overlay_with_tableau(int a, int b, float c)
{
    MeshWidget* w = (MeshWidget*)operator new(0x178);
    if (w) {
        new ((void*)w, raw_place()) MissionUi;                  // 0x48ae20
        *(void**)w = (void*)vtbl_5922e8;
    }
    vec2 v;
    v.x = 0.0f;
    v.y = 0.0f;
    ((MeshWidgetVt*)w)->w22(&v);
    *(int*)&v.x = 0x700;
    ((MeshWidgetVt*)w)->w29(&v);
    if (a >= 0) {
        MeshY* m = (MeshY*)g_meshes.placed(a);
        skin* nr = TAB_ADD_SPAWNED(&g_tableaus, &a, &b);   // 0x4b14b0
        if (m->res)
            --*(int*)((char*)m->res + 0x64);
        m->res = nr;
        if (nr)
            ++*(int*)((char*)nr + 0x64);
        ((MeshWidgetVt*)w)->w32(0, m);
    } else {
        ((MeshWidgetVt*)w)->w32(0, g_tableaus.make_mesh_from(&a, &b));
    }
    ((MeshWidgetVt*)w)->w3();
    scene->d50(w);
    widgets.push_back(w);
    return (int)widgets.size() - 1;
}
