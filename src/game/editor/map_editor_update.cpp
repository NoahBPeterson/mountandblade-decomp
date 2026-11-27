//-----------------------------------------------------------------------------
// File: map_editor_update.cpp  (0x5305b0..0x531590, TU 118)
//
// Desc: the map editor's per-frame update (MissionBlocker::update_editor), the matrix advance helper it calls, the prop
//       list removal (remove_prop and the out-of-line vector erase) and the selection-marker append (add_selected).
//       "T regime" region (docs/tu_map.md): binary order, helpers after their caller stay out of line.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <math.h>
#include "game/config/core.h"
#include "game/ui/window_mgr.h"      // g_windows slots (slot_as<T>(i))
#include "game/editor/map_editor_update.h"
#include "game/data/mission_item_recs.h"   // MissionBlocker (slot 14): pick_entity 0x4ed8d0, f2f9
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "game/editor/editor_selection_and_translation.h"
#include "engine/scene/world.h"
#include "engine/math/mat4_include.h"
#include "game/mission/placed_obj.h"
#include "game/data/game_data.h"
#include "game/mission/mission.h"
#include "game/mission/placed_obj.h"
#include <vector>
#include "engine/core/std_vec_out_of_line.h"   // vector<int>::_Insert_n 0x43e7b0 (game/agent/agent_logic.cpp): out of line
#include "game/input/key_names.h"
#include "game/ui/window_mgr.h"
#include "game/input/slot.h"
#include "game/ui/ui_globals.h"      // UI / core globals



void __stdcall select_entity(rgl::mesh* m, PlacedObj* o, unsigned color);   // 0x4ea0c0 (game/ui/screens/save_panel.cpp)

// the map's records (0x1c8 bytes each) are Mission::props
inline int record_count()
{
    return (int)g_mission->props.size();
}

inline PlacedObj* rec_at(int i)
{
    return &g_mission->props[i];
}

inline matrix& frame_of(rgl::entity* e) { return e->local; }


// the original cross / normalize are inlined into update()
static __forceinline void cross3(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
}

static __forceinline vector4 crossv(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

static __forceinline float normalize_vec(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv;
        v.y *= inv;
        v.z *= inv;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
    return len;
}

// Lane device (I1-C): std::vector<int>::push_back expanded in place at the 0x53086e site (cl 13.10 weighs the call v 76 there, the
// original inlined it: fast path store, else _Insert_n 0x43e7b0), written as the vector's own push_back body.
struct sel_vec : std::vector<int> {
    __forceinline void push(const int& _Val)
    {
        if (size() < capacity())
            _Mylast = _Ufill(_Mylast, 1, _Val);
        else
            insert(end(), _Val);
    }
};
// @ 0x5305b0
void MissionBlocker::update_editor()
{
    // 1. list-modifier: toggle the picked record (sel_target) in the multi-selection list
    if (g_keyboard.down[0xe1] != 0 && g_keyboard.prev[0xe1] == 0) {
        bool found = false;
        unsigned pos_ = 0;

        if (g_keyboard.down[0x1d] == 0 && g_keyboard.down[0x9d] == 0) {
            g_game->editor.sel.resize(0, 0);
            g_game->editor.sel_index = -1;
            f2f9 = 1;
        } else if (sel_target >= 0) {
            if (sel_target == g_game->editor.sel_index) {
                for (unsigned i = 0; i < g_game->editor.sel.size(); i++)
                    if (g_game->editor.sel[i] == g_game->editor.sel_index) {
                        g_game->editor.sel[i] = g_game->editor.sel[g_game->editor.sel.size() - 1];
                        g_game->editor.sel.pop_back();
                    }
                if (g_game->editor.sel.size() > 0)
                    g_game->editor.sel_index = g_game->editor.sel[g_game->editor.sel.size() - 1];
                else
                    g_game->editor.sel_index = -1;
                f2f9 = 1;                sel_target = g_game->editor.sel_index;
                goto refresh_list;
            }

            pos_ = -1;
            for (unsigned i = 0; i < g_game->editor.sel.size(); i++)
                if (g_game->editor.sel[i] == sel_target) { found = true; pos_ = i; }
            if (!found)
                goto add_current;

            for (unsigned j = pos_ + 1; j < g_game->editor.sel.size(); j++)
                g_game->editor.sel[j - 1] = g_game->editor.sel[j];
            g_game->editor.sel.pop_back();
            f2f9 = 1;
            sel_target = g_game->editor.sel_index;
            goto refresh_list;
        }

add_current:
        if (sel_target >= 0) {
            g_game->editor.sel_index = sel_target;
            for (unsigned i = 0; i < g_game->editor.sel.size(); i++)
                if (g_game->editor.sel[i] == g_game->editor.sel_index) {
                    g_game->editor.sel[i] = g_game->editor.sel[g_game->editor.sel.size() - 1];
                    g_game->editor.sel.pop_back();
                }
            ((sel_vec&)g_game->editor.sel).push(g_game->editor.sel_index);   // 0x53086e: _Insert_n gets the address of the referenced dword
            f2f9 = 1;
        }
refresh_list:
        g_game->editor.list_dirty = 1;
        g_app_obj.refresh_editor_dialog();
    }

    // 2. select-all modifier: select every record
    if ((g_keyboard.down[0x1d] != 0 || g_keyboard.down[0x9d] != 0) && g_keyboard.down[0x1e] != 0 && g_keyboard.prev[0x1e] == 0) {   // Ctrl+A press (0x5308f0: [0xa1714c] = down[0x1e])
        g_game->editor.sel.resize(0, 0);
        g_game->editor.sel_index = -1;
        for (unsigned i = 0; i < (unsigned)record_count(); i++)
            g_game->editor.sel.push_back(i);
        if (g_game->editor.sel.size() > 0)
            g_game->editor.sel_index = g_game->editor.sel[0];
        f2f9 = 1;
        g_game->editor.list_dirty = 1;
        g_app_obj.refresh_editor_dialog();
    }

    // 3. the editor camera frame
    {
        vector4 dir;
        dir.x = 0.0f; dir.y = 0.0f; dir.z = 0.0f;
        pos3 m;
        rgl::mat3 idtmp;
        static_cast<rgl::mat3&>(m) = *rgl::identity3(&idtmp);
        m.o = dir;
        dir.x = -k.u.x; dir.y = -k.u.y; dir.z = -k.u.z;
        m.o = g_game->editor.cam_pos;
        m.f = dir;
        m.s = crossv(m.f, m.u);
        normalize_vec(m.s);
        m.f = crossv(m.u, m.s);
        normalize_vec(m.f);
        m.move_forward(1.0f);
        g_game->editor.cam_frame = m;
    }

    // 4. sync the cursor / selection markers with the selected record
    if (g_game->editor.sel_index >= 0) {
        if (g_game->f1a20c == 0)
            g_map_mission()->cursor->visible = 1;
        frame_of(g_map_mission()->cursor) = rec_at(g_game->editor.sel_index)->frame;

        if (f2f9) {
            f2f9 = 0;
            ::select_entity(g_map_mission()->cursor_fill, rec_at(g_game->editor.sel_index), 0xa0dd7711);   // 0x4ea0c0 (a free stdcall function, save_panel.cpp)
            if (g_game->editor.sel.size() > 0)   // 0x530d43..0x530d5e: an empty list skips the loop (size()-1 would wrap)
                for (unsigned i = 0; i < g_game->editor.sel.size() - 1; i++)
                    add_selected(g_map_mission()->cursor_fill, &frame_of(g_map_mission()->cursor),
                                 rec_at(g_game->editor.sel[i]), 0x70886611);
            vector4 off;
            off.x = 0.01f;
            g_map_mission()->cursor_fill->set_offset(off);
        }
    }

    // 5. delete every selected record
    if (g_keyboard.down[0xd3] != 0 && g_keyboard.prev[0xd3] == 0 && g_game->editor.sel_index >= 0) {
        while (g_game->editor.sel.size() > 0) {
            int max = -1;
            int idx = -1;
            for (unsigned i = 0; i < g_game->editor.sel.size(); i++)
                if (g_game->editor.sel[i] > max) { max = g_game->editor.sel[i]; idx = i; }
            g_game->editor.sel[idx] = g_game->editor.sel[g_game->editor.sel.size() - 1];
            g_game->editor.sel.pop_back();
            g_map_mission()->remove_prop(max);
        }
        g_game->editor.dirty = 1;
        g_game->editor.sel_index = -1;
        g_game->editor.sel.resize(0, 0);
        g_app_obj.refresh_editor_objects();
        slot_as<MissionBlocker>(14)->pick_entity();   // 0x4ed8d0 (game/mission/pick_entity.cpp)
        slot_as<MissionBlocker>(14)->f2f9 = 1;
        SetFocus(g_app_obj.m_hWnd);
    }

    // 6. drag / rotate every selected record
    if (g_game->editor.sel_index >= 0) {
        PlacedObj* rec = rec_at(g_game->editor.sel_index);
        placement_adjusted = 0;

        vector4 dragged = rec->frame.o;
        pos old;
        *(matrix*)&old = rec->frame;
        rgl::mat3 frame;
        frame = rec->frame;
        vector4 dir = rec->scale;

        bool moved;
        bool rotated;
        bool drag;
        apply(moved, dragged, rotated, frame, drag, dir, placement_adjusted);

        pos3 basis;
        basis.o = dragged;
        static_cast<rgl::mat3&>(basis) = frame;

        for (unsigned i = 0; i < g_game->editor.sel.size(); i++) {
            PlacedObj* r = rec_at(g_game->editor.sel[i]);

            if (moved) {
                vector4 delta;
                delta.x = dragged.x - rec->frame.o.x;
                delta.y = dragged.y - rec->frame.o.y;
                delta.z = dragged.z - rec->frame.o.z;
                r->frame.o.x += delta.x;
                r->frame.o.y += delta.y;
                r->frame.o.z += delta.z;
                ent_of(rec)->dirty = 1;
            }

            if (rotated) {
                matrix rel = ((const rgl::mat4*)&old)->relative((const rgl::mat4&)r->frame);   // 0x4329d0
                matrix res = (const matrix&)basis * rel;   // 0x40e870 (rgl::matrix::operator*)
                r->frame = res;
                r->frame.orthonormalize();
                ent_of(r)->dirty = 1;
            }

            r->start_frame = r->frame;
            if (ent_of(r))
                ent_of(r)->set_local_scaled_dirty(&r->frame, &r->scale);
        }

        if (drag) {
            rec_at(g_game->editor.sel_index)->scale = dir;
            PlacedObj* p = rec_at(g_game->editor.sel_index);   // 0x5311e2: re-read after the store
            if (ent_of(p))
                ent_of(p)->set_local_scaled_dirty(&p->frame, &p->scale);
        }

        // 7. push the visual frames back into the scene objects
        if (g_keyboard.down[0x1d] != 0 && g_keyboard.down[0x16] != 0 && g_keyboard.prev[0x16] == 0)   // Ctrl+U press (0x531235: [0xa1712c] = down[0x16])
            for (unsigned i = 0; i < (unsigned)record_count(); i++) {
                PlacedObj* r = rec_at(i);
                static_cast<matrix&>(ent_of(r)->local) = r->m58;
            }

        if (drag || rotated || moved) {
            g_game->editor.dirty = 1;
            g_app_obj.refresh_editor_dialog();
        }

        rec_at(g_game->editor.sel_index)->start_frame = rec_at(g_game->editor.sel_index)->frame;
        frame_of(g_map_mission()->cursor) = rec_at(g_game->editor.sel_index)->frame;
        if (sel_target >= 0)
            frame_of(g_map_mission()->sel_marker) = rec_at(sel_target)->frame;
    }
}

// @ 0x531380
void pos3::move_forward(const float& k)
{
    vector4& dst = o;        // the original adds through a reference to o (it fences where cl schedules the parameter load)
    vector4 d;
    d.x = f.x * k;
    d.y = f.y * k;
    d.z = f.z * k;
    dst.x += d.x;
    dst.y += d.y;
    dst.z += d.z;
}

namespace std { template<> vector<PlacedObj>::iterator vector<PlacedObj>::erase(vector<PlacedObj>::iterator); }
// @ 0x5313c0
void MapMission::remove_prop(int index)
{
#define pv props
    if (pv[index].ent)
    {
        ((rgl::world*)scene)->remove_entity(((rgl::entity*)pv[index].ent)->id);
        pv[index].ent = 0;
    }
    pv.erase(pv.begin() + index);
    for (unsigned i = 0; i < pv.size(); i++)
    {
        rgl::entity* e = (rgl::entity*)pv[i].ent;
        if (e)
            e->game_id = i;
    }
#undef pv
}

// @ 0x531470 (Mission::props.erase(where): the library's erase body, with the std::copy of the tail expanded in place as the original did)
namespace std {
template<> vector<PlacedObj>::iterator vector<PlacedObj>::erase(vector<PlacedObj>::iterator _Where)
{
    PlacedObj* last = _Mylast;
    if (_Where + 1 != last) {
        PlacedObj* d = _Where._Myptr;
        for (PlacedObj* s = d + 1; s != last; ++s, ++d)
            *d = *s;
    }
    _Destroy(_Mylast - 1, _Mylast);
    --_Mylast;
    return _Where;
}
}

// @ 0x5314e0
void MissionBlocker::add_selected(rgl::mesh* target, const rgl::matrix* frame, PlacedObj* prop, unsigned color)
{
    if (!prop->ent)
        return;
    char flip = 0;
    if (prop->scale.z * prop->scale.y * prop->scale.x < 0.0f)
        flip = 1;
    for (unsigned i = 0; i < ent_of(prop)->meshes.size(); i++)
    {
        rgl::mesh*& msh = ent_of(prop)->meshes[i];
        target->append_skinned(msh, (const rgl::mat4*)&((const rgl::mat4*)frame)->relative((const rgl::mat4&)ent_of(prop)->local), -1, color, flip);   // 0x4329d0, 0x44eb00
    }
}
