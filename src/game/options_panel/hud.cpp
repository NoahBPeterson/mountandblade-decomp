// hud.cpp: the mesh editor's selection handler (the last editor function), the mission object's destructor
// wrapper, and the mission screen's button handler and mission-item callbacks (0x4d4b80..0x4d5ba0). Names
// provisional.
#include <windows.h>
#include <math.h>
#include <new>
#include "./mesh_edit.h"
#include "./hud.h"
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"                               // Mission::destroy 0x4d51d0 (the world object 0x5e292c is the mission)
#include "../mission_source.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "quest_page.h"        // QuestPage: Hud is the quest page screen (QuestPage::sel is Hud::f250), fill() 0x4d5ba0

// @ 0x4d4b80
bool EditMesh::on_selection_changed()
{
    int idx[4];
    int cur = 0;
    for (unsigned i = 0; i < c.verts.size(); i++) {
        if (c.verts[i].pad14 != 0) {
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
    for (cur = 0; (unsigned)cur < c.faces.size(); cur++) {
        int n = c.faces[cur].n;
        int cnt = 0;
        const int* p = c.faces[cur].vid;
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
        MPoly& f = c.faces[found[i]];
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

    const MVert* v = &c.verts[0];   // the original indexes from the first vertex (verts begin loaded once)
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
    unsigned nfaces = c.faces.size();
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
                    if (nfaces < c.faces.size()) {
                        graph(this)->remove_face((int)c.faces.size() - 1);
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
        for (unsigned i = 0; i < c.edges.size(); i++) {
            MEdge& e = c.edges[i];
            if ((e.v0 == idx[0] && e.v1 == sel) || (e.v1 == idx[0] && e.v0 == sel))
                break;
        }
    }
    return true;
}


// The item source as handle_buttons sees it: the callee 0x4d5410 (MissionSource::pick, defined below) without its body, so it is an
// ordinary external call (cl would otherwise see that &arg does not escape and turn the set_state calls into tail jumps). The
// linker binds the view's symbol to the definition.
struct ItemSource {
    void pick(int* arg);   // 0x4d5410 (MissionSource::pick)
};
#pragma comment(linker, "/alternatename:?pick@ItemSource@@QAEXPAH@Z=?pick@MissionSource@@QAEXPAH@Z")

// @ 0x4d5290
void Hud::handle_buttons(int arg)
{
    ((ItemSource*)((char*)(void*)this + 0x30))->pick(&arg);
    if (f60->changed(0x10000)) {
        g_menu_w1[6] = 4;
        g_menu_w1[1] = 3;
        g_menu_w1[0x8c / 4] = 0;
        g_main_menu.set_state(3);
        return;
    }
    if (f64->changed(0x10000)) {
        g_menu_w1[1] = 0;
        g_menu_w1[0x8c / 4] = f88;
        g_main_menu.set_state(3);
        return;
    }
    if (f68->changed(0x10000)) {
        g_menu_w5[6] = 4;
        g_main_menu.set_state(0x15);
        return;
    }
    if (f6c != 0 && f6c->changed(0x10000)) {
        g_menu_w1[6] = 4;
        g_menu_w1[1] = 2;
        g_menu_w1[0x8c / 4] = 0;
        g_main_menu.set_state(3);
        return;
    }
    if (f70->changed(0x10000)) {
        g_menu_w4[6] = 4;
        g_main_menu.set_state(0x12);
        return;
    }
    if (f74->changed(0x10000)) {
        g_menu_w6[6] = 4;
        g_main_menu.set_state(0x1c);
        return;
    }
    if (f78->changed(0x10000))
        g_saved = 1;
}

// ---- the world object (0x5e292c) -------------------------------------------------------------------------
#include "../../include/crt.h"   // canonical: operator delete (0x535712)
// clip_table (config/config.h): that header redefines stream_base (dialog_manager.h, included through hud.h, has its own), so the
// destructor the world teardown calls is repeated under the same class name.
struct clip_table {
    ~clip_table();   // 0x4be6f0
};
#include "../../rgl/mesh2/graph_obj.h"   // graph_obj (clear_vecs 0x4cb780)
// MapDialog (rgl/mesh2/gui_edit.h): that header cannot be included here (its mesh2_common.h declares another g_strtable), so the
// one member the destructor needs is repeated under the same class name.
// mesh_node (rgl/xmlcfg/game_util.h, whose shape.h clashes with this TU's headers): the animation node's time setter.
struct mesh_node {
    void set_time(const float* t);   // 0x433190
};
struct MapDialog {
    void clear_objects();   // 0x4c6e30
};
#include "../../rgl/xmlcfg/records.h"    // Item (std::vector<Item>::_Tidy 0x431a60)
// the vector's out-of-line _Tidy, kept a call as in the original
namespace std { template<> void vector<Item, allocator<Item> >::_Tidy(); }   // 0x431a60
struct ItemVec : std::vector<Item> { using std::vector<Item>::_Tidy; };

struct WorldVec {                                       // an STL vector triple
    void* begin; void* end; void* cap;
    void release() { if (begin) operator delete(begin); begin = 0; end = 0; cap = 0; }
};

// @ 0x4d51d0
void Mission::destroy()
{
    char* p = (char*)(void*)this;
    reset();                                            // 0x479160 (the Mission part of the teardown)
    ((tstring*)(p + 0x6814))->~tstring();               // 0x42bc40 (16-byte string at +0x6814)
    ((graph_obj*)(p + 0x6698))->clear_vecs();           // 0x4cb780: Mission::nav is the editor's mesh graph (same tables)
    ((ItemVec*)(p + 0x6688))->_Tidy();                  // 0x431a60 (Mission::props)
    ((WorldVec*)(p + 0x667c))->release();
    ((clip_table*)(p + 0x63d0))->~clip_table();         // 0x4be6f0
    ((WorldVec*)(p + 0x63c4))->release();
    ((WorldVec*)(p + 0x5e00))->release();
    // 0x4c6e30 (MapDialog::clear_objects) is shared by the two vector-bearing sub-objects at +0x30 and +0x4: the code only
    // empties the vectors at +0x8.. / +0x18.. of whatever object it is handed (identical-code folding)
    ((MapDialog*)(p + 0x30))->clear_objects();
    ((MapDialog*)(p + 0x4))->clear_objects();
}

// @ 0x4d5940
__declspec(noinline) void ValueSource::on_value_changed(float arg)
{
    ((UiD*)(void*)this)->set_value((step / slot[idx] * arg) * g_char_scale + ((UiD*)(void*)this)->value());
}

// @ 0x4d5980
void Hud::on_mission_start(int arg)
{
    *(int*)((char*)(void*)((void*)g_mission) + 0x5270) = g_u32;
    g_u32 = g_u32 + 1;
    g_mission->start_setup();
    g_screen->v4();                                 // +0x10 (0x4d59ad)
    g_screen->f18 = f18;
    g_menu_w3[1] = 0;
    g_menu_w3[0x18 / 4] = f18;
    g_main_menu.set_state(0xe);
}

// @ 0x4d59f0
void Hud::on_leave(int arg)
{
    if (f34->changed(0x10000))
        g_main_menu.set_state(7);
}

// @ 0x4d5a20
void Hud::on_update(int arg)
{
    if (g_mission_clock) {
        ValueWidget* u = item_value(f3c);
        float v = (u->f168 - u->f16c) / (u->f170 - u->f16c) * (u->slot[u->idx] - u->step);
        u->set_value((float)g_mission_clock / u->slot[u->idx] * u->step * g_char_scale + v);
        item_value(f3c)->set_key_flags(0x10000);
    }
    for (int i = 0; i < f24c; i++) {
        if (wi[i]->changed(0x10000)) {
            f250 = i;
            ((QuestPage*)(void*)this)->fill();
        }
    }
    if (f34->changed(0x10000)) {
        if (f04 == 0) {
            f34->activate(1);
            return;
        }
        f04 = 0;
        g_main_menu.set_state(0xd);
    } else if (f38->changed(0x10000)) {
        if (f04 == 1) {
            f38->activate(1);
            return;
        }
        f04 = 1;
        g_main_menu.set_state(0xd);
    } else if (f30->changed(0x10000) || (!g_a170d8 && g_a174d8)) {
        g_main_menu.set_state(f18);
    }
}

// @ 0x4d5410
void MissionSource::pick(int* arg)
{
    float t;
    if (family < 0)                     // 0x4d541b: jl to the epilogue -- nothing else runs without a family
        return;
    {
        __int64 now = *(__int64*)(g_ticks_ + slot * 2);
        __int64 d = now - *(__int64*)&tick_lo;      // 0x4d5433: sub/sbb with the 64-bit tick at +0x20
        g_spawn_lo = (__int64)(int)((double)d * g_day_scale * g_tick_scale);
        play_family_sound(family, -0x3d);         // (char)0xc3: the original pushes the ids sign-extended
        t = (float)(fmod((double)*(__int64*)g_ticks_ * g_tick_scale, g_day_period) * g_hour_scale
                    + g_hour_base);             // 0x4d5498: fmul, fadd, fstp -- no integer truncation
    }
    if (g_mission_clock) {
        float best_d = 1.0e5f;
        int best = -1;
        for (unsigned i = 0; i < (unsigned)size(); i++) {
            MissionItem* it = begin[i];
            if (it->kind() != 7)
                continue;
            // inside [org + near, org + far) on an axis: 0, else the distance to the nearer edge (0x4d54f5..0x4d55f0)
            float dx;
            if (it->ext[2] + it->org[0] <= g_mouse_x && g_mouse_x < it->ext[0] + it->org[0]) {
                dx = 0.0f;
            } else {
                dx = ABS(g_mouse_x - (it->ext[0] + it->org[0]));
                float alt = ABS(g_mouse_x - (it->ext[2] + it->org[0]));
                if (alt < dx)
                    dx = alt;
            }
            float dy;
            if (it->ext[3] + it->org[1] <= g_mouse_y && g_mouse_y < it->ext[1] + it->org[1]) {
                dy = 0.0f;
            } else {
                dy = ABS(g_mouse_y - (it->ext[1] + it->org[1]));
                float alt = ABS(g_mouse_y - (it->org[1] + it->ext[3]));
                if (alt < dy)
                    dy = alt;
            }
            float dist = (float)sqrt(dx * dx + dy * dy);
            if (dist < best_d) {
                best = i;
                best_d = dist;
            }
        }
        if (best >= 0) {
            int idx = best;
            ValueSource* u = item_value(begin[idx]);
            u->on_value_changed((float)g_mission_clock);
            ValueWidget* w = item_value(begin[best]);
            w->set_key_flags(0x10000);
        }
    }

    for (unsigned i = 0; i < (unsigned)size(); i++) {
        // every use re-reads the item from the vector (the original goes through its iterator, 0x534160, each
        // time): a callee may move the storage, so a pointer to begin[i] taken once can go stale
        for (int n = 0; n < 5; n++) {
            if (begin[i]->nodes[n])
                ((mesh_node*)begin[i]->nodes[n])->set_time(&t);
        }
        if (begin[i]->changed(0x10000)) {
            g_spawn_lo = i;
            g_spawn_hi = 0;
            int k = begin[i]->kind();
            if (k == 2 || k == 3) {
                g_spawn_hi = (__int64)(int)begin[i]->get_float();
                play_family_sound(family, -0x3e);
            } else if (k == 5) {
                g_spawn_hi = (__int64)begin[i]->get_value();
                play_family_sound(family, -0x3e);
            } else if (k == 6) {
                g_script.str[0] = begin[i]->name;
                play_family_sound(family, -0x3e);
            } else if (k != 8 || begin[i]->f184 != 1) {
                play_family_sound(family, -0x3e);
            } else {
                g_spawn_hi = begin[i]->f180 != 0;
                play_family_sound(family, -0x3e);
            }
        } else if (begin[i]->changed(2)) {
            g_spawn_hi = 0;
            g_spawn_lo = i;
            play_family_sound(family, -0x3f);
        } else if (begin[i]->changed(4)) {
            g_spawn_hi = 1;
            g_spawn_lo = i;
            play_family_sound(family, -0x3f);
        }
    }
}

// ItemRec slot 3 (0x4d5290 Hud::handle_buttons).
#include "../triggers/item_rec.h"
void ItemRec::scale_3(const float* a) { ((Hud*)this)->handle_buttons((int)a); }
