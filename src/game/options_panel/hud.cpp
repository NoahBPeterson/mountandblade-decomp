// hud.cpp: the mesh editor's selection handler (the last editor function), the mission object's destructor
// wrapper, and the mission screen's button handler and mission-item callbacks (0x4d4b80..0x4d5410). Names
// provisional.
#include <windows.h>
#include "../game_globals.h"
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): scene_counter
#include <math.h>
#include <new>
#include "./mesh_edit.h"
#include "./hud.h"
#include "../window_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"                               // Mission::destroy 0x4d51d0 (the world object 0x5e292c is the mission)
#include "../presentation.h"      // widget_mgr::pick 0x4d5410
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../screen_records.h"   // Rec38 / Rec44 / Rec454: the records whose slot 3 these are (vtables 0x595828 / 0x5957e0 / 0x595894)
#include "../triggers/item_rec.h"   // ItemRec: slot 3 is handle_buttons (vtable 0x596124)
#include "../shared_vars.h"
#include "../key_names.h"
#include "../ui_globals.h"      // UI / core globals

// @ 0x4d4b80
bool PolyMesh::on_selection_changed()
{
    int idx[4];
    int cur = 0;
    for (unsigned i = 0; i < verts.size(); i++) {
        if (verts[i].mark != 0) {
            if (cur < 4)
                idx[cur] = (int)i;
            cur++;
        }
    }
    if (cur == 3)
        return add_triangle(idx);
    if (cur != 4)
        return false;

    OwnedIntVec found;
    for (cur = 0; (unsigned)cur < faces.size(); cur++) {
        int n = faces[cur].n;
        int cnt = 0;
        const int* p = faces[cur].vid;
        for (int j = 0; j < n; j++) {
            int v = *p++;
            if (v == idx[0]) cnt++;
            if (v == idx[1]) cnt++;
            if (v == idx[2]) cnt++;
            if (v == idx[3]) cnt++;
        }
        if (cnt > 0)
            found.push_back(cur);
        if (cnt > 2)
            return false;
    }

    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    for (unsigned i = 0; i < found.size(); i++) {
        MPoly& f = faces[found[i]];
        nx = nx + f.normal.x;
        ny = ny + f.normal.y;
        nz = nz + f.normal.z;
    }
    nx = nx / (float)found.size();
    ny = ny / (float)found.size();
    nz = nz / (float)found.size();
    float len = (float)sqrt(nz * nz + ny * ny + nx * nx);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        nx *= inv; ny *= inv; nz *= inv;
    } else {
        nx = 0.0f; ny = 1.0f; nz = 0.0f;
    }

    const MVert* v = &verts[0];   // the original indexes from the first vertex (verts begin loaded once)
    Vec4 a;
    a = Vec4(v[idx[1]].pos.x - v[idx[0]].pos.x, v[idx[1]].pos.y - v[idx[0]].pos.y, v[idx[1]].pos.z - v[idx[0]].pos.z);
    Vec4 b;
    b = Vec4(v[idx[2]].pos.x - v[idx[0]].pos.x, v[idx[2]].pos.y - v[idx[0]].pos.y, v[idx[2]].pos.z - v[idx[0]].pos.z);
    Vec4 cc;
    cc = Vec4(v[idx[3]].pos.x - v[idx[0]].pos.x, v[idx[3]].pos.y - v[idx[0]].pos.y, v[idx[3]].pos.z - v[idx[0]].pos.z);

    int cls[3];
    cls[0] = cls[1] = cls[2] = 0;
    if ((a.z * b.y - a.y * b.z) * nx +
        ((a.x * b.z - b.x * a.z) * ny +
         (b.x * a.y - a.x * b.y) * nz) > 0.0f)
        cls[0] = 1;
    else
        cls[1] = 1;
    if ((cc.y * b.z - cc.z * b.y) * nx +
        ((cc.z * b.x - cc.x * b.z) * ny +
         (cc.x * b.y - cc.y * b.x) * nz) > 0.0f)
        cls[1] = cls[1] + 1;
    else
        cls[2] = 1;
    if ((cc.z * a.y - cc.y * a.z) * nx +
        ((cc.x * a.z - cc.z * a.x) * ny +
         (cc.y * a.x - cc.x * a.y) * nz) > 0.0f)
        cls[2] = cls[2] + 1;
    else
        cls[0] = cls[0] + 1;

    if (cls[0] == 1 && cls[1] == 1 && cls[2] == 1)
        return false;

    char ok = 1;
    unsigned nfaces = faces.size();
    for (int i = 0; i < 3; i++) {
        if (cls[i] != 1)
            continue;
        for (int j = 0; j < 3; j++) {
            if (cls[j] == 1)
                continue;
            if (ok) {
                int tri[3];
                tri[0] = idx[0];
                tri[1] = idx[i + 1];
                tri[2] = idx[j + 1];
                ok = add_triangle(tri);
                if (!ok) {
                    if (nfaces < faces.size()) {
                        remove_face((int)faces.size() - 1);
                        build_adjacency();
                    }
                    return false;
                }
            }
        }
    }
    if (ok) {
        int& sel = cur;
        if (cls[0] == 1)
            sel = idx[1];
        else if (cls[1] == 1)
            sel = idx[2];
        else
            sel = idx[3];
        for (unsigned i = 0; i < edges.size(); i++) {
            MEdge& e = edges[i];
            if ((e.v0 == idx[0] && e.v1 == sel) || (e.v1 == idx[0] && e.v0 == sel))
                break;
        }
    }
    return true;
}


#define W(x) ((MissionUi*)(void*)(x))
// The button handler sees the source's pick as a declaration only (it is in hud_source.cpp). With the real body visible cl
// knows pick keeps nothing of &arg and turns set_state into a tail call that overwrites arg, which the original does not.
// @ 0x4d5290
void ItemRec::frame_update(float arg)
{
    pres.pick((int*)&arg);
    if (W(row_tutorial)->clear_mask(0x10000)) {
        slot_as<head_rec>(3)->w18 = 4;
        slot_as<head_rec>(3)->w04 = 3;
        slot_as<C4c2fb0>(3)->f8c = 0;
        g_windows.set_state(3);
    } else if (W(row_new_game)->clear_mask(0x10000)) {
        slot_as<head_rec>(3)->w04 = 0;
        slot_as<C4c2fb0>(3)->f8c = first_free;
        g_windows.set_state(3);
    } else if (W(row_restore)->clear_mask(0x10000)) {
        slot_as<head_rec>(21)->w18 = 4;
        g_windows.set_state(0x15);
    } else if (row_custom != 0 && W(row_custom)->clear_mask(0x10000)) {
        slot_as<head_rec>(3)->w18 = 4;
        slot_as<head_rec>(3)->w04 = 2;
        slot_as<C4c2fb0>(3)->f8c = 0;
        g_windows.set_state(3);
    } else if (W(row_options)->clear_mask(0x10000)) {
        slot_as<head_rec>(18)->w18 = 4;
        g_windows.set_state(0x12);
    } else if (W(row_credits)->clear_mask(0x10000)) {
        slot_as<head_rec>(28)->w18 = 4;
        g_windows.set_state(0x1c);
    } else if (W(row_exit)->clear_mask(0x10000))
        g_saved = 1;
}

#undef W

// ---- the world object (0x5e292c) -------------------------------------------------------------------------
#include "../../include/crt.h"   // canonical: operator delete (0x535712)
#include "../placed_obj.h"                         // PlacedObj: Mission::props
#include "../../include/pool.h"                  // Pool<T>::~Pool (0x4c6e30): the mission's two pools
#include "../missile.h"   // Missile, the record of Pool<Missile> (Mission::poolJ)
// the vector's out-of-line _Tidy (rgl/xmlcfg/records.cpp), kept a call as in the original
namespace std { template<> void vector<PlacedObj, allocator<PlacedObj> >::_Tidy(); }   // 0x431a60

// @ 0x4d51d0 (reset, then the members' destructors: str, nav 0x4cb780, props 0x431a60, dead, terrain 0x4be6f0, v63c0, v5dfc,
// the two pools 0x4c6e30)
Mission::~Mission()
{
    reset();                                            // 0x479160
}
