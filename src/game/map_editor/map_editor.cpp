//-----------------------------------------------------------------------------
// File: map_editor.cpp  (0x5305b0..0x531590, TU 118)
//
// Desc: the map editor's per-frame update (MapEditorDialog::update), the matrix advance helper it calls, the prop
//       list removal (remove_prop and the out-of-line vector erase) and the selection-marker append (add_selected).
//       "T regime" region (docs/tu_map.md): binary order, helpers after their caller stay out of line.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <math.h>
#include "./map_editor.h"
#include "../editor_entity.h"     // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../rgl/mesh2/gui_edit.h"
#include "../../rgl/scene/world.h"
#include "../../include/mat4.h"
#include "../../include/rec1c8.h"
#include "../../include/game_data.h"
#include "../mission.h"
#include "../placed_obj.h"
#include "../int_vec.h"
extern MapEditorDialog g_editor_dialog;          // 0x8ee3c48 (the application object seen as the map editor dialog)
#include "../prop_vec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

struct skin_owner;
void __stdcall select_entity(rgl::mesh* m, skin_owner* o, unsigned color);   // 0x4ea0c0 (char_sheet/save_panel.cpp)
struct GuiCtx;
void __fastcall pick_entity(GuiCtx* self);                                 // 0x4ed8d0 (char_sheet/pick_entity.cpp)

// the map's records (0x1c8 bytes each) are Mission::props (+0x668c / +0x6690)
inline int record_count()
{
    char* b = (char*)g_mission->props._Myfirst;
    return b ? (int)((char*)g_mission->props._Mylast - b) / 0x1c8 : 0;
}

inline PlacedObj* rec_at(int i)
{
    return (PlacedObj*)((char*)g_mission->props._Myfirst + i * 0x1c8);
}

inline matrix& frame_of(rgl::entity* e) { return e->local; }

// std::vector<int>::push_back(const int&) inlined by 0x5305b0 at 0x53086e: the slow path hands _Insert_n the address of the
// REFERENCED int (the game's selected-record dword), not of a by-value copy (IntVec::push_back(int) copies to the stack)
static __forceinline void push_back_ref(IntVec& v, const int& x)
{
    unsigned size = v._Myfirst == 0 ? 0 : (unsigned)(v._Mylast - v._Myfirst);
    if (v._Myfirst && size < (unsigned)(v._Myend - v._Myfirst)) {
        int* last = v._Mylast;
        *last = x;
        v._Mylast = last + 1;
    } else {
        v.insert_n(v._Mylast, 1, &x);
    }
}

// the original cross / normalize are inlined into update()
static __forceinline void cross3(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
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

// @ 0x5305b0
void MapEditorDialog::update()
{
    // 1. list-modifier: toggle the picked record in the multi-selection list
    if (g_a17458 != 0 && g_a17858 == 0) {
        bool found = false;
        unsigned pos_ = 0;

        if (g_a17148 == 0 && g_a17348 == 0) {
            ed_sel_list(g_game).resize(0, 0);
            ed_selected(g_game) = -1;
            f2f9 = 1;
        } else if (picked >= 0) {
            if (picked == ed_selected(g_game)) {
                for (unsigned i = 0; i < ed_sel_list(g_game).size(); i++)
                    if (ed_sel_list(g_game)[i] == ed_selected(g_game)) {
                        ed_sel_list(g_game)[i] = ed_sel_list(g_game).back();
                        ed_sel_list(g_game).pop_back();
                    }
                if (ed_sel_list(g_game).size() == 0)
                    ed_selected(g_game) = -1;
                else
                    ed_selected(g_game) = ed_sel_list(g_game).back();
                f2f9 = 1;
                picked = ed_selected(g_game);
                goto refresh_list;
            }

            pos_ = -1;
            for (unsigned i = 0; i < ed_sel_list(g_game).size(); i++)
                if (ed_sel_list(g_game)[i] == picked) { found = true; pos_ = i; }
            if (!found)
                goto add_current;

            for (unsigned i = pos_ + 1; i < ed_sel_list(g_game).size(); i++)
                ed_sel_list(g_game)[i - 1] = ed_sel_list(g_game)[i];
            ed_sel_list(g_game).pop_back();
            f2f9 = 1;
            picked = ed_selected(g_game);
            goto refresh_list;
        }

add_current:
        if (picked >= 0) {
            ed_selected(g_game) = picked;
            for (unsigned i = 0; i < ed_sel_list(g_game).size(); i++)
                if (ed_sel_list(g_game)[i] == ed_selected(g_game)) {
                    ed_sel_list(g_game)[i] = ed_sel_list(g_game).back();
                    ed_sel_list(g_game).pop_back();
                }
            push_back_ref((IntVec&)ed_sel_list(g_game), ed_selected(g_game));
            f2f9 = 1;
        }

refresh_list:
        ed_dirty_1c8(g_game) = 1;
        g_editor_dialog.refresh();
    }

    // 2. select-all modifier: select every record
    if ((g_a17148 != 0 || g_a17348 != 0) && g_a1714c != 0 && g_a1754c == 0) {
        ed_sel_list(g_game).resize(0, 0);
        ed_selected(g_game) = -1;
        for (unsigned i = 0; i < (unsigned)record_count(); i++)
            ((IntVec&)ed_sel_list(g_game)).push_back(i);
        if (ed_sel_list(g_game).size() != 0)
            ed_selected(g_game) = ed_sel_list(g_game)[0];
        f2f9 = 1;
        ed_dirty_1c8(g_game) = 1;
        g_editor_dialog.refresh();
    }

    // 3. the editor camera frame
    {
        vector4 dir(-cam_x, -cam_y, -cam_z);
        pos3 m;
        rgl::mat3 idtmp;
        *(rgl::mat3*)&m = *::identity3(&idtmp);
        m.o = ed_pos(g_game);
        cross3(m.s, dir, m.u);
        normalize_vec(m.s);
        cross3(m.f, m.u, m.s);
        normalize_vec(m.f);
        m.move_forward(1.0f);
        ed_frame(g_game) = m;
    }

    // 4. sync the cursor / selection markers with the selected record
    if (ed_selected(g_game) >= 0) {
        if (g_game->f1a20c == 0)
            g_map_mission()->cursor()->f140 = 1;
        frame_of(g_map_mission()->cursor()) = rec_at(ed_selected(g_game))->m18;

        if (f2f9) {
            f2f9 = 0;
            ::select_entity(g_map_mission()->mesh_a(), (skin_owner*)rec_at(ed_selected(g_game)), 0xa0dd7711);   // 0x4ea0c0 (a free stdcall function, save_panel.cpp)
            if (ed_sel_list(g_game).size() > 0)   // 0x530d43..0x530d5e: an empty list skips the loop (size()-1 would wrap)
                for (unsigned i = 0; i < ed_sel_list(g_game).size() - 1; i++)
                    add_selected(g_map_mission()->mesh_a(), &frame_of(g_map_mission()->cursor()),
                                 rec_at(ed_sel_list(g_game)[i]), 0x70886611);
            vector4 off;
            off.x = 0.01f;
            g_map_mission()->mesh_a()->set_offset(off);
        }
    }

    // 5. delete every selected record
    if (g_a17420 != 0 && g_a17820 == 0 && ed_selected(g_game) >= 0) {
        while (ed_sel_list(g_game).size() > 0) {
            int max = -1;
            int idx = -1;
            for (unsigned i = 0; i < ed_sel_list(g_game).size(); i++)
                if (ed_sel_list(g_game)[i] > max) { max = ed_sel_list(g_game)[i]; idx = i; }
            ed_sel_list(g_game)[idx] = ed_sel_list(g_game).back();
            ed_sel_list(g_game).pop_back();
            g_map_mission()->remove_prop(max);
        }
        ed_dirty_1cc(g_game) = 1;
        ed_selected(g_game) = -1;
        ed_sel_list(g_game).resize(0, 0);
        g_editor_dialog.update_objects();
        pick_entity((GuiCtx*)(void*)g_editor);   // 0x4ed8d0 (char_sheet/pick_entity.cpp: the editor entity is the GuiCtx it works on)
        g_editor->f2f9 = 1;
        SetFocus(g_editor_hwnd);
    }

    // 6. drag / rotate every selected record
    if (ed_selected(g_game) >= 0) {
        PlacedObj* rec = rec_at(ed_selected(g_game));
        f2f8 = 0;

        vector4 dragged = rec->m18.o;
        pos old;
        *(matrix*)&old = rec->m18;
        rgl::mat3 frame;
        *(rgl::mat3*)&frame = *(const rgl::mat3*)&rec->m18;
        vector4 dir = rec->v158;

        bool moved;
        bool rotated;
        bool drag;
        apply(moved, dragged, rotated, frame, drag, dir, f2f8);

        pos3 basis;
        basis.o = dragged;
        *(rgl::mat3*)&basis = frame;

        for (unsigned i = 0; i < ed_sel_list(g_game).size(); i++) {
            PlacedObj* r = rec_at(ed_sel_list(g_game)[i]);

            if (moved) {
                vector4 delta;
                delta.x = dragged.x - rec->m18.o.x;
                delta.y = dragged.y - rec->m18.o.y;
                delta.z = dragged.z - rec->m18.o.z;
                r->m18.o.x += delta.x;
                r->m18.o.y += delta.y;
                r->m18.o.z += delta.z;
                ent_of(rec)->f141 = 1;
            }

            if (rotated) {
                matrix rel = ((const rgl::mat4*)&old)->relative((const rgl::mat4&)r->m18);   // 0x4329d0
                matrix res = (const matrix&)basis * rel;   // 0x40e870 (rgl::matrix::operator*)
                r->m18 = res;
                r->m18.orthonormalize();
                ent_of(r)->f141 = 1;
            }

            r->m98 = r->m18;
            if (ent_of(r))
                ent_of(r)->set_local_scaled_dirty(&r->m18, &r->v158);
        }

        if (drag) {
            rec_at(ed_selected(g_game))->v158 = dir;
            PlacedObj* p = rec_at(ed_selected(g_game));   // 0x5311e2: re-read after the store
            if (ent_of(p))
                ent_of(p)->set_local_scaled_dirty(&p->m18, &p->v158);
        }

        // 7. push the visual frames back into the scene objects
        if (g_a17148 != 0 && g_a1712c != 0 && g_a1752c == 0)
            for (unsigned i = 0; i < (unsigned)record_count(); i++) {
                PlacedObj* r = rec_at(i);
                *(matrix*)((char*)ent_of(r) + 0x58) = r->m58;
            }

        if (drag || rotated || moved) {
            ed_dirty_1cc(g_game) = 1;
            g_editor_dialog.refresh();
        }

        rec_at(ed_selected(g_game))->m98 = rec_at(ed_selected(g_game))->m18;
        frame_of(g_map_mission()->cursor()) = rec_at(ed_selected(g_game))->m18;
        if (picked >= 0)
            frame_of(g_map_mission()->frame_b()) = rec_at(picked)->m18;
    }
}

// @ 0x531380
void pos3::move_forward(const float& k)
{
    vector4 d;
    d.x = f.x * k;
    d.y = f.y * k;
    d.z = f.z * k;
    o.x += d.x;
    o.y += d.y;
    o.z += d.z;
}

// @ 0x5313c0
void MapMission::remove_prop(int index)
{
#define pv ((PropVec&)props)
    if (pv[index].ent)
    {
        ((rgl::world*)scene_world)->remove_entity(((rgl::entity*)pv[index].ent)->id);
        pv[index].ent = 0;
    }
    pv.erase(pv.begin() + index);
    for (unsigned i = 0; i < pv.size(); i++)
    {
        rgl::entity* e = (rgl::entity*)pv[i].ent;
        if (e)
            e->fc = i;
    }
#undef pv
}

// @ 0x531470
std::vector<Rec1c8>::iterator PropVec::erase(std::vector<Rec1c8>::iterator where)
{
    Rec1c8* p = where._Myptr;
    Rec1c8* end = _Mylast;
    Rec1c8* q = p + 1;
    for (; q != end; ++q)
        *p++ = *q;
    end = _Mylast;
    for (q = end - 1; q != end; ++q)
        q->~Rec1c8();
    --_Mylast;
    return where;
}

// @ 0x5314e0
void MapEditorDialog::add_selected(rgl::mesh* target, const rgl::matrix* frame, void* rec, unsigned color)
{
    PlacedObj* prop = (PlacedObj*)rec;
    if (!prop->m1b4)
        return;
    char flip = 0;
    if (prop->v158.z * prop->v158.y * prop->v158.x < 0.0f)
        flip = 1;
    for (unsigned i = 0; i < ent_of(prop)->meshes.size(); i++)
    {
        rgl::mesh* msh = ent_of(prop)->meshes[i];
        matrix m = ((const rgl::mat4*)frame)->relative((const rgl::mat4&)ent_of(prop)->local);   // 0x4329d0
        target->append_skinned(msh, (const rgl::mat4*)&m, -1, color, flip);   // 0x44eb00
    }
}
