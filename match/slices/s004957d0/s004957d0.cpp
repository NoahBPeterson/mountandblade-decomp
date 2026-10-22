// Slice s004957d0: rgl mesh helpers shared by the map-legend and slider widgets.
//   0x4957d0  spread a party's earned experience over its hero stacks (contains two post-link
//             marker gaps, see partial.txt),
//   0x4958f0  rgl_mesh::clone,
//   0x495970  build the horizontal slider widget and append it to a list,
//   0x495b20  build the map-legend mesh,
//   0x495ba0  rgl_mesh::transform (position / uv / vertex lists),
//   0x495f00  rgl_mesh::set_colour_all,
//   0x495fa0  rgl_mesh::normalize_uv.
// Names are provisional; every layout below is cited from the disassembly.
// Layouts reused from verified slices: rgl::vector4 / rgl::matrix (rgl.h, s00407c10), tstring
// (s0042aaf0), Party / PartyMgr / GameData (s004947a0, s00494e40).
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <vector>
#include <deque>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- rgl string (16 bytes) ----------------------------------------------------------------------------
struct tstring {
    int p0;             // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring() {}                              // 0x42bbd0
    tstring(const char* s) throw();           // 0x42bbe0
    ~tstring() throw();                       // 0x42bc40
    void operator=(const tstring& o) throw(); // 0x42bc50
};

// one named vertex list: a VC7.1 std::vector<rgl::vector4> (0x20 bytes).
struct vert_rec { char b[0x54]; unsigned color; int b58; char b5c[4]; };   // 0x60: packed colour at +0x54
static char size_check_vert[(sizeof(vert_rec) == 0x60) ? 1 : -1];
struct uv_rec { rgl::vector4 v; rgl::vector4 u; };   // 0x20: position/uv pair

struct vertex_list {                              // 0x10: std::vector<vector4> at +4 (two per named list)
    void* vptr;                                   // +0
    rgl::vector4* _Myfirst;                       // +4
    rgl::vector4* _Mylast;                        // +8
    rgl::vector4* _Myend;                         // +0xc
};

struct rgl_mesh;

// ---- party bookkeeping layouts (see s004947a0) -------------------------------------------------------
struct Stack {                                   // 0x20 bytes, array at party+0xa8
    int count;                                   // +0x00
    int troop;                                   // +0x04
    int flags;                                   // +0x08 (bit 0 = end-of-list marker)
    int f0c;                                     // +0x0c
    int f10;                                     // +0x10
    int f14;                                     // +0x14
    int f18;                                     // +0x18
    int f1c;                                     // +0x1c
};
struct Party {                                   // 0x5ac8 bytes
    char pad0[0x4];
    int state;                                   // +0x04 (1 = in mission)
    char pad8[0xa8 - 0x8];
    Stack stacks[64];                            // +0xa8 .. (hero troop ids at +0xcc, stride 0x20)
    int nstacks;                                 // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};
struct PartyMgr {                                // embedded in the game data at +0xf8
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0 (index >> 4 -> page of 16)
    int index_of(int id);                        // 0x48ad00 (thiscall, ret 4, -1 on miss)
};
struct Troop {                                   // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    unsigned flags;                              // +0x30 (0x10 = hero)
    char pad34[0x508 - 0x34];
};

// ---- callees / globals --------------------------------------------------------------------------------
rgl_mesh* get_mesh(const tstring& name);            // 0x419540 (ret 4)
void* get_object(rgl_mesh* m);                      // 0x41c6d0
struct res_mgr {
    void* get_material(const tstring& name);        // 0x418a70 (ret 4)
};
extern res_mgr g_res;                               // 0x5d6fa0
struct colour_owner {
    void set_colour_on(rgl_mesh* m);                // 0x442110 (ret 4)
};
rgl::vector4 from_packed_colour(unsigned c);        // 0x40e4e0
unsigned to_packed_colour(const rgl::vector4* v);   // 0x40e440

// The party helper lives on the game data object at 0x5e2930.
struct GameData {
    char pad0[0x78];
    int f78;                                     // +0x78 (default party)
    char pad7c[0x8c - 0x7c];
    int f8c;                                     // +0x8c
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                            // +0xf8 (pages pointer at +0x139c8)
    char pad139c8[0x13a00 - (0xf8 + 0x138d4)];
    Troop* troops;                               // +0x13a00
    int add_party_exp(int party, int amount);    // 0x4428c0 (ret 8)
    void spread_party_experience(int amount);    // 0x4957d0 (ret 4)
};
extern GameData g_game;                          // 0x5e2930

static Party* get_party(PartyMgr* mgr, int id);

// ---- rgl_mesh (0x260 bytes; the first 0x1c bytes are the shared resource header) ---------------------
struct rgl_mesh {
    void* vptr;                                   // +0
    tstring name;                                 // +4
    char pad14[0x18 - 0x14];
    void* owner;                                  // +0x18  (adopting parent)
    char pad1c[0x2c - 0x1c];
    void* material;                               // +0x2c  (refcount at +0x64)
    int f30;                                      // +0x30
    int f34;                                      // +0x34
    char pad38[0x3c - 0x38];
    int f3c;                                      // +0x3c
    int f40;                                      // +0x40
    char pad44[0xf0 - 0x44];
    float uv_x0;                                  // +0xf0  (uv rectangle)
    float uv_y0;                                  // +0xf4
    float uv_x1;                                  // +0xf8
    float uv_y1;                                  // +0xfc
    char pad100[0x228 - 0x100];
    int f228;                                     // +0x228
    char pad22c[0x260 - 0x22c];
    rgl_mesh();                                   // 0x412a90 (returns `this`)
    void ensure_local();                          // 0x412ee0 (returns `this`, plain `ret`)
    void reupload();                              // 0x419300
    rgl_mesh* clone();                            // 0x4958f0
    void transform(const rgl::matrix& mtx);       // 0x495ba0 (ret 4, one 0x30-byte 3x4 frame)
    void set_alpha(float a);                      // 0x495f00 (ret 4)
    void normalize_uv(rgl_mesh* src);             // 0x495fa0 (ret 4)
};

// Members at 0x54/0x58/0x74/0x78/0xa4/0xac/0x138 are also touched by the methods below; they were never
// given a type by the original (raw offsets), so keep them as macros on the mesh pointer.
#define MESH_VERT(m)      (*(char**)((char*)(m) + 0x54))             // array of 0x60-byte vertex records
#define MESH_VERT_END(m)  (*(char**)((char*)(m) + 0x58))
#define MESH_POS(m)       (*(rgl::vector4**)((char*)(m) + 0x74))     // array of 0x20-byte records
#define MESH_POS_END(m)   (*(rgl::vector4**)((char*)(m) + 0x78))
#define MESH_LISTS(m)     (*(vertex_list**)((char*)(m) + 0xa4))      // vertex_list[8], stride 0x100
#define MESH_NLISTS(m)    (*(int*)((char*)(m) + 0xac))
#define MESH_DIRTY(m)     (*(unsigned char*)((char*)(m) + 0x138))

static char size_check_mesh[(sizeof(rgl_mesh) == 0x260) ? 1 : -1];
static char size_check_uv[(offsetof(rgl_mesh, uv_x0) == 0xf0 && offsetof(rgl_mesh, f228) == 0x228) ? 1 : -1];
static char size_check_party[(sizeof(Party) == 0x5ac8 && offsetof(Party, nstacks) == 0x8a8) ? 1 : -1];

// ---- party table lookup (inlined in the original: pages[index >> 4][index & 0xf]) --------------------
static Party* get_party(PartyMgr* mgr, int id)
{
    int idx = mgr->index_of(id);
    return (Party*)((char*)mgr->pages[idx >> 4] + (idx & 0xf) * 0x5ac8);
}

// ---- 0x4957d0: share `amount` earned experience between a party and its hero stacks -------------------
// The party stack list (+0xa8, stride 0x20, flags at +8) is walked to count stacks before the
// end-of-list marker (flags bit 0). With at least two live stacks the hero stacks (`troop` at +4,
// stride 0x20; hero flag 0x10 in troop->flags) from index 1 up are counted, and each then receives
// amount / heroes. `this` is the game data (ecx); the single stack argument is the amount.
// @ 0x4957d0
void GameData::spread_party_experience(int amount)
{
    add_party_exp(f8c, amount);
    Party* p = get_party(&parties, f78);
    int live = 0;
    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        live++;
    }
    int heroes = 0;
    if (live <= 1)
        return;
    for (int i = 1; i < live; i++) {
        int troop = p->stacks[i].troop;
        if (troops[troop].flags & 0x10)
            heroes++;
    }
    if (!heroes)
        return;
    for (int i = 1; i < live; i++) {
        int troop = p->stacks[i].troop;
        if (troops[troop].flags & 0x10)
            add_party_exp(troop, amount / heroes);
    }
}

// ---- 0x4958f0: clone a mesh with the same owner, material, flags and name ----------------------------
// @ 0x4958f0
rgl_mesh* rgl_mesh::clone()
{
    rgl_mesh* m = new rgl_mesh;
    if (owner)
        m->owner = owner;
    else
        m->owner = this;
    m->material = material;
    if (m->material)
        ++*(int*)((char*)m->material + 0x64);     // material reference count
    m->f30 = f30;
    m->f3c = f3c;
    m->f40 = f40;
    *(unsigned char*)((char*)m + 0xa0) = 0;
    *(unsigned char*)((char*)m + 0xa1) = 0;
    m->name = name;
    m->f228 = f228;
    return m;
}

// ---- ui widget (0x188 bytes, base ctor 0x48ae20) -----------------------------------------------------
struct ui_widget {
    void* vptr;                                   // +0
    char pad4[0x2c - 0x4];
    ui_widget* parent;                            // +0x2c
    char pad30[0xf4 - 0x30];
    int fF4;                                      // +0xf4
    char padF8[0x168 - 0xf8];
    float f168;                                   // +0x168
    float f16c;                                   // +0x16c
    float f170;                                   // +0x170
    char pad174[0x178 - 0x174];
    int f178;                                     // +0x178
    float f17c;                                   // +0x17c
    int f180;                                     // +0x180
    int f184;                                     // +0x184
    ui_widget();                                  // 0x48ae20
};

static char size_check_ui[(sizeof(ui_widget) == 0x188) ? 1 : -1];

// A vtable slot is called as `obj->slot(args...)`: `this` goes in ecx and the pushed arguments start at
// esp+4 on entry. The slots are __thiscall, so the pointer types below declare the instance as a leading
// stack argument purely so the call site matches (it is never read from the stack).
typedef void (__stdcall* ui_slot_c)(void*);
typedef void (__stdcall* ui_slot_34)(void*, int);
typedef void (__stdcall* ui_slot_64)(void*, const void*);
typedef void (__stdcall* ui_slot_80)(void*, int, void*);
typedef void (__stdcall* ui_slot_c8)(void*, ui_widget*);

// ---- 0x495970: build the horizontal slider widget and append it to this->children ---------------------
// Returns the index of the appended widget. `this` is the owning panel (parent at +0x2c, list at +4).
// @ 0x495970

struct slider_panel {
    char pad0[0x4];
    std::vector<ui_widget*> children;             // +4
    char pad18[0x2c - 0x18];
    ui_widget* parent;                            // +0x2c
    int build_slider(int vmin, int vmax);         // 0x495970 (ret 8)
};

int slider_panel::build_slider(int vmin, int vmax)
{
    ui_widget* w = new ui_widget;                 // 0x188, ctor 0x48ae20 (then vptr 0x591fb0)
    w->f178 = 0;
    w->f17c = 0.3f;
    w->f168 = 1.0f;
    w->f180 = 0;
    w->f184 = 0;
    {
        vector4 range[2];
        ((void (__stdcall*)(void*, void*, void*))(*(void***)w)[0x58 / 4])(w, &range[0], &range[1]);
    }
    {
        tstring n("slider_hor");
        rgl_mesh* m = get_mesh(n);
        void* object = get_object(m);
        ((ui_slot_80)(*(void***)w)[0x80 / 4])(w, 0, object);
    }
    {
        tstring n("handle_hor");
        rgl_mesh* m = get_mesh(n);
        void* object = get_object(m);
        ((ui_slot_80)(*(void***)w)[0x80 / 4])(w, 2, object);
    }
    ((ui_slot_c)(*(void***)w)[0xc / 4])(w);
    {
        vector4 v;
        v.x = 0.25f;
        v.y = (float)w->fF4;
        v.z = 0.0f;
        v.w = 0.0f;
        ((ui_slot_64)(*(void***)w)[0x64 / 4])(w, &v);
    }
    float fmin = (float)vmin;
    w->f16c = fmin;
    w->f170 = (float)vmax;
    ((ui_slot_34)(*(void***)w)[0x34 / 4])(w, vmin);
    ((ui_slot_c8)(*(void***)parent)[0xc8 / 4])(parent, w);
    children.push_back(w);
    return (int)(children.size() - 1);
}

// ---- 0x495b20: build the map-legend mesh -------------------------------------------------------------
// @ 0x495b20
rgl_mesh* build_legend_mesh(void* parent)
{
    rgl_mesh* m = new rgl_mesh;
    m->f34 = 2;
    {
        tstring n("map_legend");
        m->material = g_res.get_material(n);
    }
    *(int*)((char*)m + 0x10c) = 1;
    *(int*)((char*)m + 0x114) = 1;
    *(int*)((char*)m + 0x140) = 1;
    ((colour_owner*)parent)->set_colour_on(m);
    return m;
}

// ---- 0x495ba0: transform the position list, the vertex list and every named vertex list --------------
// Takes a 3x4 transform (0x30 bytes: s/f/u rows) by reference -- the single stack argument, ret 4.
// Every 16-byte vector4 in place is rebuilt from mtx.s/f/u dotted with its own x/y/z and the caller
// copies the whole rebuilt record back, so the w lane is whatever the uninitialised local carry (the
// original never writes the local's w; see the sibling rgl::mesh::transform at 0x44f3c0).
// @ 0x495ba0
void rgl_mesh::transform(const rgl::matrix& mtx)
{
    ensure_local();
    {
        std::vector<uv_rec>& pos = *(std::vector<uv_rec>*)((char*)this + 0x70);
        for (unsigned i = 0; i < pos.size(); i++) {
            vector4 a;
            a.x = mtx.u.x * pos[i].v.z + mtx.f.x * pos[i].v.y + mtx.s.x * pos[i].v.x;
            a.y = mtx.u.y * pos[i].v.z + mtx.f.y * pos[i].v.y + mtx.s.y * pos[i].v.x;
            a.z = mtx.u.z * pos[i].v.z + mtx.f.z * pos[i].v.y + mtx.s.z * pos[i].v.x;
            pos[i].v = a;
            vector4 b;
            b.x = mtx.u.x * pos[i].u.z + mtx.f.x * pos[i].u.y + mtx.s.x * pos[i].u.x;
            b.y = mtx.u.y * pos[i].u.z + mtx.f.y * pos[i].u.y + mtx.s.y * pos[i].u.x;
            b.z = mtx.u.z * pos[i].u.z + mtx.f.z * pos[i].u.y + mtx.s.z * pos[i].u.x;
            pos[i].u = b;
        }
    }
    {
        std::vector<vert_rec>& verts = *(std::vector<vert_rec>*)((char*)this + 0x50);
        for (unsigned i = 0; i < verts.size(); i++) {
            vector4* p = (vector4*)((char*)&verts[i] + 0x30);
            vector4 a;
            a.x = mtx.u.x * p->z + mtx.f.x * p->y + mtx.s.x * p->x;
            a.y = mtx.u.y * p->z + mtx.f.y * p->y + mtx.s.y * p->x;
            a.z = mtx.u.z * p->z + mtx.f.z * p->y + mtx.s.z * p->x;
            *p = a;
        }
    }
    for (int i = 0; i < MESH_NLISTS(this); i++) {
        vertex_list* l = MESH_LISTS(this) + (i << 1);
        {
            std::vector<rgl::vector4>& vl = *(std::vector<rgl::vector4>*)((char*)&l[0] + 4);
            for (unsigned j = 0; j < vl.size(); j++) {
                vector4 a;
                a.x = mtx.u.x * vl[j].z + mtx.f.x * vl[j].y + mtx.s.x * vl[j].x;
                a.y = mtx.u.y * vl[j].z + mtx.f.y * vl[j].y + mtx.s.y * vl[j].x;
                a.z = mtx.u.z * vl[j].z + mtx.f.z * vl[j].y + mtx.s.z * vl[j].x;
                vl[j] = a;
            }
        }
        {
            std::vector<rgl::vector4>& vl = *(std::vector<rgl::vector4>*)((char*)&l[1] + 4);
            for (unsigned j = 0; j < vl.size(); j++) {
                vector4 a;
                a.x = mtx.u.x * vl[j].z + mtx.f.x * vl[j].y + mtx.s.x * vl[j].x;
                a.y = mtx.u.y * vl[j].z + mtx.f.y * vl[j].y + mtx.s.y * vl[j].x;
                a.z = mtx.u.z * vl[j].z + mtx.f.z * vl[j].y + mtx.s.z * vl[j].x;
                vl[j] = a;
            }
        }
    }
    MESH_DIRTY(this) = 1;
    reupload();
}

// ---- 0x495f00: set the alpha of every 0x60-byte vertex record on a mesh -----------------------------
// Each vertex record keeps its packed colour at +0x54; that colour is unpacked into floats, the alpha
// lane is replaced by `a`, and it is packed back. The single stack argument is a *float* (the original
// copies [esp+0x40] straight into the unpacked vector's w lane); the helper builds the vector in the
// frame and the source reads it back through a plain binding, so no temporary is named and cl keeps a
// local copy of the returned record.
// @ 0x495f00
void rgl_mesh::set_alpha(float a)
{
    ensure_local();
    std::vector<vert_rec>& verts = *(std::vector<vert_rec>*)((char*)this + 0x50);
    for (unsigned i = 0; i < verts.size(); i++) {
        vert_rec* base = &verts[0];
        unsigned packed = base[i].color;
        rgl::vector4 v = from_packed_colour(packed);
        v.w = a;
        base[i].color = to_packed_colour(&v);
    }
}

// ---- 0x495fa0: rescale `src`'s uv records into this mesh's uv rectangle ------------------------------
// `src` is the mesh in edi; the target rectangle comes from *this* (f0/f4/f8/fc = x0/y0/x1/y1).
// @ 0x495fa0
void rgl_mesh::normalize_uv(rgl_mesh* src)
{
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    src->ensure_local();
    uv_rec* p = (uv_rec*)MESH_POS(src);
    uv_rec* end = (uv_rec*)MESH_POS_END(src);
    if (p != end) {
        do {
            if (p->v.x > u0)
                u0 = p->v.x;
            if (p->v.x < u1)
                u1 = p->v.x;
            if (p->v.y > v0)
                v0 = p->v.y;
            if (p->v.y < v1)
                v1 = p->v.y;
            ++p;
        } while (p != end);
    }
    if (u0 == 0.0f || v0 == 0.0f)
        return;
    float su = (uv_x0 - uv_x1) / (u0 - u1);
    float sv = (uv_y0 - uv_y1) / (v0 - v1);
    float sm = (sv + su) * 0.5f;
    uv_rec* q = (uv_rec*)MESH_POS(src);
    if (q == end)
        return;
    for (;;) {
        float x = q->v.x, y = q->v.y, z = q->v.z;
        q->v.x = x * su;
        q->v.y = y * sv;
        q->v.z = z * sm;
        q->u = q->v;
        ++q;
        if (q == (uv_rec*)MESH_POS_END(src))
            break;
    }
}
