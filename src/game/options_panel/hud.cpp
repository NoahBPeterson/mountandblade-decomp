// hud.cpp: the mesh editor's selection handler (the last editor function), the mission object's destructor
// wrapper, and the mission screen's button handler and mission-item callbacks (0x4d4b80..0x4d5ba0). Names
// provisional.
#include <windows.h>
#include "../game_globals.h"
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): scene_counter
#include <math.h>
#include <new>
#include "./mesh_edit.h"
#include "./hud.h"
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"                               // Mission::destroy 0x4d51d0 (the world object 0x5e292c is the mission)
#include "../mission_source.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../screen_records.h"   // Rec38 / Rec44 / Rec454: the records whose slot 3 these are (vtables 0x595828 / 0x5957e0 / 0x595894)
#include "../triggers/item_rec.h"   // ItemRec: slot 3 is handle_buttons (vtable 0x596124)
#include "quest_page.h"        // QuestPage: Hud is the quest page screen (QuestPage::sel is Hud::f250), fill() 0x4d5ba0
#include "../shared_vars.h"
#include "../key_names.h"
#include "../ui_globals.h"      // UI / core globals

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

#define W(x) ((MissionUi*)(void*)(x))
// @ 0x4d5290
void ItemRec::scale_3(float arg)
{
    ((ItemSource*)((char*)(void*)this + 0x30))->pick((int*)&arg);
    if (W(row_tutorial)->clear_mask(0x10000)) {
        slot_as<int>(3)[6] = 4;
        slot_as<int>(3)[1] = 3;
        slot_as<int>(3)[0x8c / 4] = 0;
        g_main_menu.set_state(3);
        return;
    }
    if (W(row_new_game)->clear_mask(0x10000)) {
        slot_as<int>(3)[1] = 0;
        slot_as<int>(3)[0x8c / 4] = first_free;
        g_main_menu.set_state(3);
        return;
    }
    if (W(row_restore)->clear_mask(0x10000)) {
        slot_as<int>(21)[6] = 4;
        g_main_menu.set_state(0x15);
        return;
    }
    if (row_custom != 0 && W(row_custom)->clear_mask(0x10000)) {
        slot_as<int>(3)[6] = 4;
        slot_as<int>(3)[1] = 2;
        slot_as<int>(3)[0x8c / 4] = 0;
        g_main_menu.set_state(3);
        return;
    }
    if (W(row_options)->clear_mask(0x10000)) {
        slot_as<int>(18)[6] = 4;
        g_main_menu.set_state(0x12);
        return;
    }
    if (W(row_credits)->clear_mask(0x10000)) {
        slot_as<int>(28)[6] = 4;
        g_main_menu.set_state(0x1c);
        return;
    }
    if (W(row_exit)->clear_mask(0x10000))
        g_saved = 1;
}

#undef W

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
    ((UiD*)(void*)this)->set_value((step / slot[idx] * arg) * 0.0005f + ((UiD*)(void*)this)->value());
}

// @ 0x4d5980
void Rec44::scale_3(float)
{
    *(int*)((char*)(void*)((void*)g_mission) + 0x5270) = g_mission_ctx.scene_counter;
    g_mission_ctx.scene_counter = g_mission_ctx.scene_counter + 1;
    g_mission->start_setup();
    slot_as<MenuScreen>(14)->v4();                                 // +0x10 (0x4d59ad)
    slot_as<MenuScreen>(14)->f18 = w18;
    slot_as<int>(11)[1] = 0;
    slot_as<int>(11)[0x18 / 4] = w18;
    g_main_menu.set_state(0xe);
}

// @ 0x4d59f0
void Rec38::scale_3(float)
{
    if (((MissionUi*)(void*)f34)->clear_mask(0x10000))
        g_main_menu.set_state(7);
}

#define W(x) ((MissionUi*)(void*)(x))
// @ 0x4d5a20
void Rec454::scale_3(float)
{
    if (g_mission_clock) {
        ValueWidget* u = item_value((MissionUi*)(void*)list);
        float v = (u->f168 - u->f16c) / (u->f170 - u->f16c) * (u->slot[u->idx] - u->step);
        u->set_value((float)g_mission_clock / u->slot[u->idx] * u->step * 0.0005f + v);
        item_value((MissionUi*)(void*)list)->set_key_flags(0x10000);
    }
    for (int i = 0; i < count; i++) {
        if (W(rows[i])->clear_mask(0x10000)) {
            f250 = i;
            ((QuestPage*)(void*)this)->fill();
        }
    }
    if (W(active_tab)->clear_mask(0x10000)) {
        if (w04 == 0) {
            W(active_tab)->set15c(1);
            return;
        }
        w04 = 0;
        g_main_menu.set_state(0xd);
    } else if (W(finished_tab)->clear_mask(0x10000)) {
        if (w04 == 1) {
            W(finished_tab)->set15c(1);
            return;
        }
        w04 = 1;
        g_main_menu.set_state(0xd);
    } else if (W(done)->clear_mask(0x10000) || (!g_keyboard.down[0x1] && g_keyboard.prev[0x1])) {
        g_main_menu.set_state(w18);
    }
}
#undef W

// @ 0x4d5410
void MissionSource::pick(int* arg)
{
    float t;
    if (family < 0)                     // 0x4d541b: jl to the epilogue -- nothing else runs without a family
        return;
    {
        __int64 now = g_ticks[slot];
        __int64 d = now - *(__int64*)&tick_lo;      // 0x4d5433: sub/sbb with the 64-bit tick at +0x20
        g_script.param1 = (__int64)(int)((double)d * 1000.0f * 1e-5f);
        play_family_sound(family, -0x3d);         // (char)0xc3: the original pushes the ids sign-extended
        t = (float)(fmod((double)g_ticks[0] * 1e-5f, 1.0) * 40.0f
                    + 100.0f);             // 0x4d5498: fmul, fadd, fstp -- no integer truncation
    }
    if (g_mission_clock) {
        float best_d = 1.0e5f;
        int best = -1;
        for (unsigned i = 0; i < (unsigned)size(); i++) {
            MissionUi* it = begin[i];
            if (it->kind() != 7)
                continue;
            // inside [org + near, org + far) on an axis: 0, else the distance to the nearer edge (0x4d54f5..0x4d55f0)
            float dx;
            if (it->screen[2] + it->e[0] <= g_gui.mouse[0] && g_gui.mouse[0] < it->screen[0] + it->e[0]) {
                dx = 0.0f;
            } else {
                dx = ABS(g_gui.mouse[0] - (it->screen[0] + it->e[0]));
                float alt = ABS(g_gui.mouse[0] - (it->screen[2] + it->e[0]));
                if (alt < dx)
                    dx = alt;
            }
            float dy;
            if (it->screen[3] + it->e[1] <= g_gui.mouse[1] && g_gui.mouse[1] < it->screen[1] + it->e[1]) {
                dy = 0.0f;
            } else {
                dy = ABS(g_gui.mouse[1] - (it->screen[1] + it->e[1]));
                float alt = ABS(g_gui.mouse[1] - (it->e[1] + it->screen[3]));
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
            if (begin[i]->tracks[n])
                ((mesh_node*)begin[i]->tracks[n])->set_time(&t);
        }
        if (begin[i]->clear_mask(0x10000)) {
            g_script.param1 = i;                 // 0x4d56eb: only the low pair is written here; g_spawn_hi stays stale unless a kind sets it
            int k = begin[i]->kind();
            if (k == 2 || k == 3) {
                g_script.param2 = (__int64)(int)begin[i]->get168();
                play_family_sound(family, -0x3e);
            } else if (k == 5) {
                g_script.param2 = (__int64)begin[i]->get15c();
                play_family_sound(family, -0x3e);
            } else if (k == 6) {
                g_script.str[0] = begin[i]->name;
                play_family_sound(family, -0x3e);
            } else if (k != 8 || ((UiA*)begin[i])->f184 != 1) {
                play_family_sound(family, -0x3e);
            } else {
                g_script.param2 = ((UiA*)begin[i])->f180 != 0;
                play_family_sound(family, -0x3e);
            }
        } else if (begin[i]->clear_mask(2)) {
            g_script.param2 = 0;
            g_script.param1 = i;
            play_family_sound(family, -0x3f);
        } else if (begin[i]->clear_mask(4)) {
            g_script.param2 = 1;
            g_script.param1 = i;
            play_family_sound(family, -0x3f);
        }
    }
}

