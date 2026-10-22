// Slice s0052f250: the map editor's per-frame object placement update (0x52f250, 3049 bytes).
// Each frame it repeats the scroll keys into adjust_mode (0x4bb260), rebuilds the preview entity
// (clear, a temporary 0x1c8 placed-object record, its meshes cloned into the entity, a camera frame
// built from the editor's direction), lets 0x52fe40 nudge the placement, picks a world position with
// 0x52bdb0, and finally either just moves the preview or commits a placed object through
// Mission::add_placed (0x44f870) + the map-editor dialog.  Names and layouts are provisional; every
// offset here is read off this function's disassembly (see work/match/scratch_s0052f250_dis.txt).
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) -------------------------------------------------------------------
struct tstring {
    int pos;                                    // +0
    char* buf;                                  // +4
    int cap;                                    // +8
    int len;                                    // +0xc
    tstring();                                  // 0x42bbd0
    ~tstring();                                 // 0x42bc40
};

extern int g_tick_lo;                           // 0x5e2958  (g_time[2] low)
extern int g_tick_hi;                           // 0x5e295c  (g_time[2] high)
extern __int64 g_time[];                        // 0x5e2948  clock, 1e-5 s ticks (g_time[0] here)

// ---- 0x1c8-byte placed-object record (layout from s0044f3c0 / s00435490, vtable 0x590b84) --------
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Quad {
    int a, b, c, d;
    Quad() : a(2), c(g_tick_lo), d(g_tick_hi) {}
};
struct PlacedObj : IfaceB {
    int m10, m14;                               // +0x10
    matrix a18, a58, a98, ad8, a118;            // +0x18 .. +0x157
    vector4 v158;                               // +0x158
    int m168;                                   // +0x168
    float f16c;                                 // +0x16c
    Quad q170;                                  // +0x170
    int m180, m184, m188, m18c;                 // +0x180 (m188 = kind)
    tstring name;                               // +0x190
    int m1a0, m1a4, m1a8, m1ac, m1b0;           // +0x1a0 (m1a0 = slot number)
    void* f1b4;                                 // +0x1b4  spawn handle, virtual +0x5c
    int m1b8, m1bc, m1c0, m1c4;                 // +0x1b8
    PlacedObj() { reset(); }                    // 0x437830 (inlined here)
    void reset();                               // 0x4352f0
};

// ---- the non-virtual slot at +0x5c of the object behind record->f1b4 ------------------------------
struct Prop {
    virtual void p00(); virtual void p01(); virtual void p02(); virtual void p03();
    virtual void p04(); virtual void p05(); virtual void p06(); virtual void p07();
    virtual void p08(); virtual void p09(); virtual void p10(); virtual void p11();
    virtual void p12(); virtual void p13(); virtual void p14(); virtual void p15();
    virtual void p16(); virtual void p17(); virtual void p18(); virtual void p19();
    virtual void p20(); virtual void p21(); virtual void p22();
    virtual void apply(void* frame, void* pos); // slot 23 (+0x5c)
};

// ---- scene mesh + the 16-byte {pad, first, last, end} container ----------------------------------
struct mesh {
    mesh* clone();                              // 0x41c6d0
    void set_color(vector4 c);                  // 0x47af30 (16-byte argument passed by value)
};
struct MeshVec {
    int alloc;                                  // +0 (unused by this function)
    mesh** first;                               // +4
    mesh** last;                                // +8
    mesh** end;                                 // +0xc
    unsigned size() const { return first ? (unsigned)(last - first) : 0u; }
    unsigned capacity() const { return first ? (unsigned)(end - first) : 0u; }
    void _Insert_n(mesh** pos, unsigned n, mesh** val);          // 0x410610 (ICF'd vector<int>::_Insert_n)
    void push_back(mesh* x)
    {
        if (size() < capacity()) {
            *last = x;
            last = last + 1;
        } else {
            _Insert_n(last, 1, &x);
        }
    }
};

// ---- scene object at g_map+0x65e4: 64-byte frame at +0x58, mesh vector at +0x118, shown at +0x140 ---
struct mat3 { vector4 s, f, u; };
struct smat : matrix {
    void rotate_sf(const float* angle);         // 0x40e770
};
struct SceneObj {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void recalc();                      // slot 4  (+0x10)
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8();
    virtual void s9(); virtual void s10(); virtual void s11(); virtual void s12();
    virtual void s13(); virtual void s14(); virtual void s15(); virtual void s16();
    virtual void s17(); virtual void s18();
    virtual void clear();                       // slot 19 (+0x4c)
    char pad14[0x58 - 4];                       // vptr is the first dword
    smat frame;                                 // +0x58  (s, f, u, o with o at +0x88)
    char pad98[0x118 - 0x98];
    MeshVec meshes;                             // +0x118
    char pad128[0x140 - 0x128];
    char f140;                                  // +0x140  preview shown
};

// ---- map world (0x5e292c) and frame state (0x5e2930) ---------------------------------------------
struct Map {
    char pad0[0x65e4];
    SceneObj* object;                           // +0x65e4  the edited scene object
    char pad65e8[0x668c - 0x65e8];
    PlacedObj* records;                         // +0x668c  the 0x1c8 record array
    void init_placed(PlacedObj*, const int* kind, const int* id, const int* value, const matrix*);   // 0x435860
    void build_meshes(PlacedObj*, MeshVec*);    // 0x5302f0
    int add_placed(const int* kind, const int* id, const int* value, const matrix*, int* spawn);     // 0x44f870
};
struct IntVec {
    int* first;                                 // +0
    int* last;                                  // +4
    int* end;                                   // +8
    void resize(unsigned n, int v);             // 0x43f310
    void push_back(const int& v);               // 0x440b30
};
struct Frame_ {
    char pad0[0x1a09c];
    vector4 color;                              // +0x1a09c  (placement colour / eye)
    float f1a0ac;                               // +0x1a0ac  placement z offset
    char pad1a0b0[0x1a0b8 - 0x1a0b0];
    int mode;                                   // +0x1a0b8  kind being placed
    int id;                                     // +0x1a0bc  record id
    int prev_mode;                              // +0x1a0c0
    int prev_id;                                // +0x1a0c4
    char commit;                                // +0x1a0c8  force-place flag
    char pad1a0c9[3];
    int rand_rot;                               // +0x1a0cc
    int rand_scale;                             // +0x1a0d0
    char pad1a0d4[0x1a164 - 0x1a0d4];
    matrix cam;                                 // +0x1a164
    char pad1a1a4[0x1a1b4 - 0x1a1a4];
    int selected;                               // +0x1a1b4
    IntVec sel;                                 // +0x1a1b8
    char pad1a1c4[0x1a1cc - 0x1a1c4];
    int dirty;                                  // +0x1a1cc
};
struct ObjRec20 {                               // 0x20-byte record at 0x5e28f8
    char pad0[0x18];
    int variants;                               // +0x18
    char pad1c[4];
};

// ---- editor objects -----------------------------------------------------------------------------
struct EditorView {
    char pad0[0x2f9];
    char f2f9;                                  // +0x2f9
    void pick_entity();                         // 0x4ed8d0
};
struct MapEditorDialog {
    void update_objects();                      // 0x4c7e20
    void refresh();                             // 0x4c77c0
};

// ---- globals (all offsets/addresses from the disassembly) ----------------------------------------
extern Map* g_map;                              // 0x5e292c
extern Frame_* g_game;                          // 0x5e2930
extern ObjRec20* g_objrecs;                     // 0x5e28f8
extern int g_key_inc;                           // 0xa17414
extern int g_key_inc_prev;                      // 0xa17814
extern int g_key_dec;                           // 0xa173f4
extern int g_key_dec_prev;                      // 0xa177f4
extern int g_key_place;                         // 0xa17458
extern int g_key_place_prev;                    // 0xa17858
extern int g_cursor_x;                          // 0x5d70b4
extern int g_cursor_y;                          // 0x5d70b8
extern int g_saved_x;                           // 0x5e2130
extern int g_saved_y;                           // 0x5e2134
extern HWND g_editor_hwnd;                      // 0x8ee3d30
extern EditorView* g_editor_view;               // 0xa739b8
extern MapEditorDialog g_editor_dialog;         // 0x8ee3c48

// ---- callees / helpers not local to this slice ---------------------------------------------------
void adjust_mode(int dir);                      // 0x4bb260 (cdecl)
matrix* identity(matrix* out);                  // 0x40cd00 (cdecl, returns out)
mat3* identity3(mat3* out);                     // 0x40cd60 (cdecl, returns out)

// The original inlines a component-wise cross (0x407dc0's body); keep the same evaluation order.
static __forceinline vector4 cross_inline(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}
// Inlined vector4::normalize (0x407d60): (0,1,0) when the length is not > 1e-10.
static __forceinline void normalize_inline(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float s = 1.0f / len;
        v.x *= s;
        v.y *= s;
        v.z *= s;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
}

// ---- the map editor screen object (this) ----------------------------------------------------------
struct MapEditor {
    char pad0[0x60];
    void* view;                                 // +0x60  3D view, used by 0x52bdb0
    char pad64[0x248 - 0x64];
    float dir_x, dir_y, dir_z;                  // +0x248  editor look direction
    char pad254[0x268 - 0x254];
    int f268;                                   // +0x268
    char pad26c[0x2f8 - 0x26c];
    char f2f8;                                  // +0x2f8  placement adjusted
    char pad2f9[0x304 - 0x2f9];
    vector4 f304;                               // +0x304  last picked position
    int f314;                                   // +0x314  down-repeat timer
    int f318;                                   // +0x318  up-repeat timer
    int f31c;                                   // +0x31c  selected variant
    int f320;                                   // +0x320  previous variant
    void find_position(char* ok, vector4* pos); // 0x52bdb0
    void adjust(char* a, vector4* off, char* b, mat3* frame, char* c, vector4* col, char* out); // 0x52fe40
    void update();                              // 0x52f250
};

// @ 0x52f250
void MapEditor::update()
{
    g_map->object->f140 = 0;

    // repeat the two scroll keys every 35000 ticks
    if (g_key_inc != 0) {
        if (this->f318 < g_time[0] - 35000)
            adjust_mode(1);
    }
    if (g_key_dec != 0) {
        if (this->f314 < g_time[0] - 35000)
            adjust_mode(-1);
    }
    if (g_key_inc != 0 && g_key_inc_prev == 0)
        this->f318 = (int)g_time[0];
    if (g_key_dec != 0 && g_key_dec_prev == 0)
        this->f314 = (int)g_time[0];
    if (g_key_inc == 0 && g_key_inc_prev != 0)
        this->f318 = 0;
    if (g_key_dec == 0 && g_key_dec_prev != 0)
        this->f314 = 0;

    // rebuild the preview when the kind / record / variant changed
    if (g_game->mode != g_game->prev_mode ||
        g_game->id != g_game->prev_id ||
        (g_game->mode == 4 && this->f31c != this->f320))
    {
        g_map->object->clear();

        PlacedObj rec;
        MeshVec list;
        int value = 0;
        if (g_game->mode == 4)
            value = this->f31c;

        matrix m;
        identity(&m);
        g_map->init_placed(&rec, &g_game->mode, &g_game->id, &value, &m);
        g_map->build_meshes(&rec, &list);

        for (unsigned i = 0; i < list.size(); i++) {
            mesh* c = list.first[i]->clone();
            g_map->object->meshes.push_back(c);
            g_map->object->recalc();
        }

        g_game->prev_mode = g_game->mode;
        g_game->prev_id = g_game->id;
        this->f320 = this->f31c;

        // preview colour / scale
        float v = 1.0f;
        if (g_game->rand_scale != 0)
            v = (float)(rand() % 15785) * 4.741733573609963e-05f + 0.75f;
        g_game->color = vector4(v, v, v);
        g_game->f1a0ac = 0.0f;

        // frame: s = cross(-dir, up) normalised, f = cross(up, s), u = up
        mat3 m3;
        m3.u = vector4(0.0f, 0.0f, 1.0f);
        vector4 d(-this->dir_x, -this->dir_y, -this->dir_z);
        vector4 s = cross_inline(d, m3.u);
        m3.s = s;
        normalize_inline(m3.s);
        vector4 f = cross_inline(m3.u, m3.s);
        m3.f = f;
        *(mat3*)&g_map->object->frame = m3;

        if (g_game->rand_rot != 0) {
            float angle = (float)(rand() % 15785) * 0.00039724254747852683f;
            g_map->object->frame.rotate_sf(&angle);
        }
        if (g_game->mode == 4) {
            int n = g_objrecs[g_game->id].variants;
            this->f31c = (n > 1) ? rand() % n : 0;
        }
    }

    // let the placement helper nudge the preview
    this->f2f8 = 0;
    vector4 off(0.0f, 0.0f, g_game->f1a0ac);
    mat3 m3 = *(mat3*)&g_map->object->frame;
    vector4 col = g_game->color;
    char fA, fB, fC;
    this->adjust(&fA, &off, &fB, &m3, &fC, &col, &this->f2f8);

    if (fA)
        g_game->f1a0ac = off.z;
    if (fB)
        *(mat3*)&g_map->object->frame = m3;
    if (fC) {
        vector4 old = g_game->color;
        g_game->color = col;
        for (unsigned i = 0; i < g_map->object->meshes.size(); i++) {
            mesh* c = g_map->object->meshes.first[i];
            c->set_color(vector4(1.0f / old.x, 1.0f / old.y, 1.0f / old.z));
            c->set_color(g_game->color);
        }
    }
    if (fC != 0 || fB != 0 || fA != 0)
        goto set_cursor;

    {
        char ok;
        vector4 pos;
        this->find_position(&ok, &pos);
        if (ok) {
            this->f304 = pos;
            g_map->object->frame.o = pos;
            g_map->object->frame.o.z += g_game->f1a0ac;
            g_map->object->f140 = 1;
            if (g_key_place != 0 && g_key_place_prev == 0)
                goto place;
        }
        if (g_game->commit == 0)
            goto refresh;
place:
        if (g_game->mode >= 0 && g_game->id >= 0) {
                matrix cm;
                if (g_game->commit != 0) {
                    g_game->commit = 0;
                    cm = g_game->cam;
                } else {
                    cm = g_map->object->frame;
                }

                int pv = 0;
                if (g_game->mode == 4)
                    pv = this->f31c;
                int spawn = -1;
                int idx = g_map->add_placed(&g_game->mode, &g_game->id, &pv, &cm, &spawn);

                g_game->selected = idx;
                g_game->sel.resize(0, 0);
                g_game->sel.push_back(g_game->selected);

                PlacedObj* p = &g_map->records[g_game->selected];
                p->v158 = g_game->color;
                if (p->f1b4 != 0)
                    ((Prop*)p->f1b4)->apply(&p->a18, &p->v158);

                g_game->dirty = 1;
                g_editor_dialog.update_objects();
                g_editor_view->pick_entity();
                g_editor_view->f2f9 = 1;
                SetFocus(g_editor_hwnd);

                if (g_game->rand_scale != 0) {
                    float v2 = (float)(rand() % 15785) * 4.741733573609963e-05f + 0.75f;
                    g_game->color = vector4(v2, v2, v2);
                }
                if (g_game->rand_rot != 0) {
                    mat3 im;
                    identity3(&im);
                    *(mat3*)&g_map->object->frame = im;
                    float angle = (float)(rand() % 15785) * 0.00039724254747852683f;
                    g_map->object->frame.rotate_sf(&angle);
                }
                if (g_game->mode == 4) {
                    int n = g_objrecs[g_game->id].variants;
                    this->f31c = (n > 1) ? rand() % n : 0;
                }
        }
    }
    goto refresh;

set_cursor:
    SetCursorPos(g_cursor_x, g_cursor_y);
    g_saved_x = g_cursor_x;
    g_saved_y = g_cursor_y;
    g_map->object->frame.o = this->f304;
    g_map->object->frame.o.z += g_game->f1a0ac;
    g_map->object->f140 = 1;

refresh:
    g_editor_dialog.refresh();
}
