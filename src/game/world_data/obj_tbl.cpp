//-----------------------------------------------------------------------------
// File: obj_tbl.cpp  (0x499050..0x499830)
//
// Desc: resource record -> mesh -> entity helpers of the scene editor and the tableau spawners: the placed-mesh builder of the
//       scene table, the tableau table's mesh/entity spawners, and the two mesh-widget factories. Translation unit 87
//       (obj_tbl) of the TU map. Names provisional. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40; listed directly for the resolution audit)
#include "../../include/image_addrs.h"
#include "./obj_tbl.h"
#include "./scene_object.h"      // SceneObject: TableauTbl::worlds' scene
#include "./mesh_hud.h"      // MeshWidget
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../config/widget_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../ui/mission_ui.h"
#include "./tableau.h"
#include "./widget_mgr_w.h"
#include "../../rgl/mesh.h"
#include "../../rgl/rgl_math.h"
#include "../../include/mesh_parts.h"
#include "../../include/mesh_prims.h"
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)
#include "../../include/render_flags.h"


// @ 0x499050
rgl::mesh* MeshTable::placed(int index)
{
    rgl::mesh* m = ((rgl::mesh*)g_resmgr.get_object(&((SceneRec48*)records)[index].name))->clone();
    if (((SceneRec48*)records)[index].flags & 1)
        m->sort_order += 0x2000;
    // the position is read through a reference bound before pos's block opens: a pointer defined outside a local's
    // scope does not get the local in its points-at set, so the x/y loads are free to cross the pos.x store
    SceneRec48& q = ((SceneRec48*)records)[index];
    {
    vector4 sc;
    vector4 pos;
    pos.x = q.px;
    pos.y = q.py;
    pos.z = q.pz;
    SceneRec48* r = &((SceneRec48*)records)[index];
    Frame3 frame;
    identity3(&frame);
    float a = r->rx * 0.017453292f;
    ((rgl::matrix*)&frame)->rotate_about_s(&a);
    a = r->ry * 0.017453292f;
    ((rgl::matrix*)&frame)->rotate_about_f(&a);
    a = r->rz * 0.017453292f;
    ((rgl::matrix*)&frame)->rotate_about_u(&a);
    sc.x = r->sx;
    sc.y = r->sy;
    sc.z = r->sz;
    if (sc.x != g_one || sc.y != g_one || sc.z != g_one)
        m->set_color(sc);
    m->set_part(*(rgl::matrix*)&frame);
    if (pos.z * pos.z + pos.y * pos.y + pos.x * pos.x != 0.0f)
        m->translate(&pos);
    }
    return m;
}
// @ 0x4991d0
rgl::mesh* TableauTbl::make_mesh_from(const int* index, const void* from)
{
    rgl::mesh* m = new rgl::mesh;
    int i = *index;
    unsigned color;
    // as in placed(): the record reference is bound before the block that holds a and b, so the loads through it can
    // cross the stores to a and b (the first read keeps the plain indexed spelling: its base load is a scratch pick)
    E68& q = recs[i];
    {
    vector4 b, a;
    b.x = recs[i].b8;
    b.y = q.b9;
    a.x = q.b6;
    a.y = q.b7;
    color = 0xffffffff;
    vec2 uv1, uv0;
    uv1.x = 1.0f;
    uv1.y = 1.0f;
    uv0.x = 0.0f;
    uv0.y = 0.0f;
    b.z = 0.0f;
    a.z = 0.0f;
    m->add_quad(&a, &b, &uv0, &uv1, &color);
    }
    skin* nr = add_spawned(index, (const int*)from);   // 0x4b14b0
    if (m->res)
        --m->res->refcount;
    m->res = nr;
    if (nr)
        ++nr->refcount;
    return m;
}

// @ 0x4992b0
void TableauTbl::spawn(int index, int from, const rgl::matrix* frame, const rgl::vector4* scale)
{
    rgl::world* w = worlds[worlds.size() - 1];   // the tableau's scene (SceneObject)
    rgl::entity* e = w->create_entity(9);
    rgl::mesh* m = make_mesh_from(&index, &from);
    if (!(0.99999f <= scale->x && scale->x < 1.00001f
       && 0.99999f <= scale->y && scale->y < 1.00001f
       && 0.99999f <= scale->z && scale->z < 1.00001f))
        m->set_color(*scale);
    e->add_mesh(m);
    (rgl::matrix&)e->local = *frame;
}

// @ 0x499390
void TableauTbl::spawn_scaled(int index, const rgl::matrix* frame, const rgl::vector4* scale,
                              const float* alpha, const unsigned* color)
{
    rgl::world* w = worlds[worlds.size() - 1];   // the tableau's scene (SceneObject)
    rgl::entity* e = w->create_entity(9);
    rgl::mesh* m = g_meshes.placed(index);
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
        ((rgl::mesh*)(void*)m)->set_bone_index((int)rgl::pack_color(&c));
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
    e->add_mesh(m);
    (rgl::matrix&)e->local = *frame;
}

// @ 0x499540
void TableauTbl::spawn_scaled2(int index, const rgl::matrix* frame, const float* scale)
{
    rgl::world* w = worlds[worlds.size() - 1];   // the tableau's scene (SceneObject)
    rgl::entity* e = w->create_entity(9);
    rgl::mesh* m = (((rgl::mesh*)g_resmgr.get_object(&g_map_icons[index].mesh)))->copy_of();
    if (!(0.99999f <= *scale && *scale < 1.00001f))
        m->scale(*scale);
    e->add_mesh(m);
    (rgl::matrix&)e->local = *frame;
}

// @ 0x4995e0
int WidgetMgrW::create_mesh_overlay(int index)
{
    MeshWidget* w = new MeshWidget;
    w->move_a(V2(0.0f, 0.0f));
    w->set148(0x700);
    w->set_track(0, g_meshes.placed(index));
    w->update_bounds();
    scene->add_child(w);
    widgets.push_back(w);
    return (int)widgets.size() - 1;
}

// @ 0x4996e0
int WidgetMgrW::create_mesh_overlay_with_tableau(int a, int b, float c)
{
    MeshWidget* w = new MeshWidget;
    w->move_a(V2(0.0f, 0.0f));
    w->set148(0x700);
    if (a >= 0) {
        rgl::mesh* m = g_meshes.placed(a);
        skin* nr = g_tableaus.add_spawned(&b, (const int*)&c);   // 0x4b14b0
        if (m->res)
            --m->res->refcount;
        m->res = nr;
        if (nr)
            ++nr->refcount;
        w->set_track(0, m);
    } else {
        w->set_track(0, g_tableaus.make_mesh_from(&b, &c));
    }
    w->update_bounds();
    scene->add_child(w);
    widgets.push_back(w);
    return (int)widgets.size() - 1;
}
