// Slice s005305b0: MapEditorDialog::update() (0x5305b0) -- the map editor's per-frame update.
//
// One frame does, in order:
//   1. when the list-modifier key is held, add/remove the picked record (f2cc) to/from the
//      multi-selection list (g_game->sel_list) and rebuild the editor object list;
//   2. when the select-all modifier is held (with the second key), select every map record;
//   3. build the editor camera frame from the dialog's camera direction and push it to g_game;
//   4. sync the cursor / selection markers with the selected record;
//   5. on the delete key, delete every selected record;
//   6. on the mouse drag, move / rotate every selected record and update its visual transforms;
//   7. refresh the editor dialogs and the cached matrices.
//
// Layouts come from the verified neighbouring slices s004c7290 / s004c7bf0 (MapEditorDialog, record
// stride 0x1c8), s0050ec40 (Game camera fields) and s0040e060 / s00432130 (pos / pos3 / mat3 helpers).
#include "rgl.h"
#include <windows.h>
#include <vector>

using rgl::matrix;
using rgl::vector4;

// ---- engine globals -------------------------------------------------------------------------------
extern int g_a17458;                    // 0xa17458
extern int g_a17858;                    // 0xa17858
extern int g_a17148;                    // 0xa17148  select-all modifier
extern int g_a17348;                    // 0xa17348
extern int g_a1714c;                    // 0xa1714c
extern int g_a1754c;                    // 0xa1754c
extern int g_a17420;                    // 0xa17420  delete key
extern int g_a17820;                    // 0xa17820
extern int g_a1712c;                    // 0xa1712c
extern int g_a1752c;                    // 0xa1752c
extern HWND g_editor_hwnd;             // 0x8ee3d30

// ---- small engine types ---------------------------------------------------------------------------

// 0x30-byte rotation basis (three rows, no origin)
struct mat3 { vector4 s, f, u; };
mat3* identity3(mat3* out);                                     // 0x40cd60

// a frame whose origin is expressed in this frame / composed with another frame
struct pos : matrix {
    matrix relative(const matrix& b) const;                     // 0x4329d0
};
struct pos3 : matrix {
    matrix mul(const matrix& m) const;                          // 0x40e870
    void move_forward(const float& d);                          // 0x531380
};

// the engine mesh (only the one method this function needs)
struct mesh {
    void set_offset(const vector4& offset);                     // 0x4fb8e0
};

struct MapRecord;
struct MapEditorDialog;

// the per-record scene object (virtual slot +0x5c updates a record's visual frame)
struct MapEntity {
    char pad0[0x13c];                                           // +0x004
    char f140;                                                  // +0x140
    char f141;                                                  // +0x141
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22();
    virtual void set_frame(matrix* m, vector4* dir);            // +0x5c (slot 23)
};

// a scene frame object: matrix at +0x58, "changed" flag at +0x140
struct SceneFrame {
    char pad0[0x58];
    matrix m;                                                   // +0x58
    char pad98[0xa8];
    char f140;                                                  // +0x140
};

// a placed map object, stride 0x1c8
struct MapRecord {
    char pad0[0x18];
    matrix m;                                                   // +0x18  (origin at +0x48)
    matrix vis;                                                 // +0x58
    matrix prev;                                                // +0x98
    char padD8[0x158 - 0xd8];
    vector4 dir;                                                // +0x158
    char pad168[0x1b4 - 0x168];
    MapEntity* obj;                                             // +0x1b4
};

// the mission / map object (0x5e292c)
struct Mission {
    char pad0[0x65d0];
    SceneFrame* cursor;                                         // +0x65d0
    char pad65d4[4];
    mesh* mesh_a;                                               // +0x65d8
    SceneFrame* frame_b;                                        // +0x65dc
    char pad65e0[0x668c - 0x65e0];
    char* records;                                              // +0x668c  begin
    char* records_end;                                          // +0x6690  end
    void remove_record(int index);                              // 0x5313c0
};
extern Mission* g_mission;                                      // 0x5e292c

// the current game frame (0x5e2930)
struct Game {
    char pad0[0x1a164];
    matrix editor_m;                                            // 0x1a164
    vector4 editor_pos;                                         // 0x1a1a4
    int selected;                                               // 0x1a1b4
    std::vector<int> sel_list;                                  // 0x1a1b8
    int f1a1c8;                                                 // 0x1a1c8
    int f1a1cc;                                                 // 0x1a1cc
    char pad1a1d0[0x1a20c - 0x1a1d0];
    int f1a20c;                                                 // 0x1a20c
};
extern Game* g_game;                                            // 0x5e2930

// the current editor entity (0xa739b8)
struct EditorEntity {
    void pick_entity();                                         // 0x4ed8d0
    char pad2f9[0x2f9];
    char f2f9;                                                  // 0x2f9
};
extern EditorEntity* g_editor;                                  // 0xa739b8

// the map editor's object dialog
struct MapEditorDialog {
    char pad0[0x248];
    float f248;                                                 // 0x248
    float f24c;                                                 // 0x24c
    float f250;                                                 // 0x250
    char pad254[0x2cc - 0x254];
    int f2cc;                                                   // 0x2cc  picked record
    char pad2d0[0x2f8 - 0x2d0];
    char f2f8;                                                  // 0x2f8
    char f2f9;                                                  // 0x2f9
    void refresh();                                             // 0x4c77c0
    void update_objects();                                      // 0x4c7e20
    void select_entity(mesh* m, MapRecord* rec, int id);        // 0x4ea0c0
    void add_selected(mesh* m, matrix* frame, MapRecord* rec, int id);  // 0x5314e0
    void handle_input(bool& moved, vector4& pos, bool& rotated, mat3& frame,
                      bool& drag, vector4& dir, char& flag);    // 0x52fe40
    void update();                                              // 0x5305b0
};

// ---- helpers --------------------------------------------------------------------------------------

// number of records in the map (char pointers, 0x1c8 apart)
inline int record_count()
{
    char* b = g_mission->records;
    return b ? (int)(g_mission->records_end - b) / 0x1c8 : 0;
}

inline MapRecord* record_at(int i)
{
    return (MapRecord*)(g_mission->records + i * 0x1c8);
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
        unsigned pos = 0;

        if (g_a17148 == 0 && g_a17348 == 0) {
            g_game->sel_list.resize(0, 0);
            g_game->selected = -1;
            f2f9 = 1;
        } else if (f2cc >= 0) {
            if (f2cc == g_game->selected) {
                for (unsigned i = 0; i < g_game->sel_list.size(); i++)
                    if (g_game->sel_list[i] == g_game->selected) {
                        g_game->sel_list[i] = g_game->sel_list.back();
                        g_game->sel_list.pop_back();
                    }
                if (g_game->sel_list.size() == 0)
                    g_game->selected = -1;
                else
                    g_game->selected = g_game->sel_list.back();
                f2f9 = 1;
                f2cc = g_game->selected;
                goto refresh_list;
            }

            pos = -1;
            for (unsigned i = 0; i < g_game->sel_list.size(); i++)
                if (g_game->sel_list[i] == f2cc) { found = true; pos = i; }
            if (!found)
                goto add_current;

            for (unsigned i = pos + 1; i < g_game->sel_list.size(); i++)
                g_game->sel_list[i - 1] = g_game->sel_list[i];
            g_game->sel_list.pop_back();
            f2f9 = 1;
            f2cc = g_game->selected;
            goto refresh_list;
        }

add_current:
        if (f2cc >= 0) {
            g_game->selected = f2cc;
            for (unsigned i = 0; i < g_game->sel_list.size(); i++)
                if (g_game->sel_list[i] == g_game->selected) {
                    g_game->sel_list[i] = g_game->sel_list.back();
                    g_game->sel_list.pop_back();
                }
            g_game->sel_list.push_back(g_game->selected);
            f2f9 = 1;
        }

refresh_list:
        g_game->f1a1c8 = 1;
        ((MapEditorDialog*)0x8ee3c48)->refresh();
    }

    // 2. select-all modifier: select every record
    if ((g_a17148 != 0 || g_a17348 != 0) && g_a1714c != 0 && g_a1754c == 0) {
        g_game->sel_list.resize(0, 0);
        g_game->selected = -1;
        for (unsigned i = 0; i < (unsigned)record_count(); i++)
            g_game->sel_list.push_back(i);
        if (g_game->sel_list.size() != 0)
            g_game->selected = g_game->sel_list[0];
        f2f9 = 1;
        g_game->f1a1c8 = 1;
        ((MapEditorDialog*)0x8ee3c48)->refresh();
    }

    // 3. the editor camera frame
    {
        vector4 dir(-f248, -f24c, -f250);
        pos3 m;
        mat3 idtmp;
        *(mat3*)&m = *identity3(&idtmp);
        m.o = g_game->editor_pos;
        cross3(m.s, dir, m.u);
        normalize_vec(m.s);
        cross3(m.f, m.u, m.s);
        normalize_vec(m.f);
        m.move_forward(1.0f);
        g_game->editor_m = m;
    }

    // 4. sync the cursor / selection markers with the selected record
    if (g_game->selected >= 0) {
        if (g_game->f1a20c == 0)
            g_mission->cursor->f140 = 1;
        g_mission->cursor->m = record_at(g_game->selected)->m;

        if (f2f9) {
            f2f9 = 0;
            select_entity(g_mission->mesh_a, record_at(g_game->selected), 0xa0dd7711);
            for (unsigned i = 0; i < g_game->sel_list.size() - 1; i++)
                add_selected(g_mission->mesh_a, &g_mission->cursor->m,
                             record_at(g_game->sel_list[i]), 0x70886611);
            vector4 off;
            off.x = 0.01f;
            g_mission->mesh_a->set_offset(off);
        }
    }

    // 5. delete every selected record
    if (g_a17420 != 0 && g_a17820 == 0 && g_game->selected >= 0) {
        while (g_game->sel_list.size() > 0) {
            int max = -1;
            int idx = -1;
            for (unsigned i = 0; i < g_game->sel_list.size(); i++)
                if (g_game->sel_list[i] > max) { max = g_game->sel_list[i]; idx = i; }
            g_game->sel_list[idx] = g_game->sel_list.back();
            g_game->sel_list.pop_back();
            g_mission->remove_record(max);
        }
        g_game->f1a1cc = 1;
        g_game->selected = -1;
        g_game->sel_list.resize(0, 0);
        ((MapEditorDialog*)0x8ee3c48)->update_objects();
        g_editor->pick_entity();
        g_editor->f2f9 = 1;
        SetFocus(g_editor_hwnd);
    }

    // 6. drag / rotate every selected record
    if (g_game->selected >= 0) {
        MapRecord* rec = record_at(g_game->selected);
        f2f8 = 0;

        vector4 dragged = rec->m.o;
        pos old;
        *(matrix*)&old = rec->m;
        mat3 frame;
        *(mat3*)&frame = *(const mat3*)&rec->m;
        vector4 dir = rec->dir;

        bool moved;
        bool rotated;
        bool drag;
        handle_input(moved, dragged, rotated, frame, drag, dir, f2f8);

        pos3 basis;
        basis.o = dragged;
        *(mat3*)&basis = frame;

        for (unsigned i = 0; i < g_game->sel_list.size(); i++) {
            MapRecord* r = record_at(g_game->sel_list[i]);

            if (moved) {
                vector4 delta;
                delta.x = dragged.x - rec->m.o.x;
                delta.y = dragged.y - rec->m.o.y;
                delta.z = dragged.z - rec->m.o.z;
                r->m.o.x += delta.x;
                r->m.o.y += delta.y;
                r->m.o.z += delta.z;
                rec->obj->f141 = 1;
            }

            if (rotated) {
                matrix rel = old.relative(r->m);
                matrix res = basis.mul(rel);
                r->m = res;
                r->m.orthonormalize();
                r->obj->f141 = 1;
            }

            r->prev = r->m;
            if (r->obj)
                r->obj->set_frame(&r->m, &r->dir);
        }

        if (drag) {
            MapRecord* p = record_at(g_game->selected);
            p->dir = dir;
            if (p->obj)
                p->obj->set_frame(&p->m, &p->dir);
        }

        // 7. push the visual frames back into the scene objects
        if (g_a17148 != 0 && g_a1712c != 0 && g_a1752c == 0)
            for (unsigned i = 0; i < (unsigned)record_count(); i++) {
                MapRecord* r = record_at(i);
                *(matrix*)((char*)r->obj + 0x58) = r->vis;
            }

        if (drag || rotated || moved) {
            g_game->f1a1cc = 1;
            ((MapEditorDialog*)0x8ee3c48)->refresh();
        }

        record_at(g_game->selected)->prev = record_at(g_game->selected)->m;
        g_mission->cursor->m = record_at(g_game->selected)->m;
        if (f2cc >= 0)
            g_mission->frame_b->m = record_at(f2cc)->m;
    }
}
