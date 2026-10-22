// Slice s0047dd90: the animation-set machinery of the troop/skin owner (EChar) and the scatter-set
// generator that builds one mesh per (map record, grid cell) and attaches it to the terrain's entities.
//
//   0x47dd90  lod_mesh::apply_anim_all  - push a time into every mesh of every list
//   0x47ddf0  EChar::blend_all          - blend one ArrSet into the first mesh of each list
//   0x47de60  EChar::use_carry          - look up/create the ArrSet named like the lod_mesh, then blend
//   0x47df80  gen_set_init              - scatter-set generator (per-map-record + per-grid-cell pass)
//
// Layouts (verified against the disassembly):
//   std::vector<T> is 16 bytes: {allocator@0, _Myfirst@4, _Mylast@8, _Myend@0xc}.
//   EChar: skin data at +0, std::vector<ArrSet> at +0x68.
//   ArrSet (0x38): tstring name@0x00, int cnt[5]@0x10, int* arr[5]@0x24.
//   lod_mesh: std::vector<mesh*> lists[5]@0x34, count@0x84; its TroopRec name is at +0xd0.
#include <vector>
#include <stddef.h>
#include "tstring.h"
#include "mesh.h"

using rgl::mesh;
using rgl::lod_mesh;
using rgl::mat4;
using rgl::vector4;
using rgl::matrix;
using rgl::scene;
using rgl::vat;

// ---- shared helpers ---------------------------------------------------------------------------------------
struct mat3 { vector4 s, f, u; };                               // 0x30
mat3* identity3(mat3* out);                                     // 0x40cd60
struct pos : matrix { mat3 rot3(const matrix& m) const; };       // 0x40a920

// 0x38-byte animation set: the element of the owner's std::vector<ArrSet>.
struct ArrSet {
    tstring name;        // 0x00
    int     cnt[5];      // 0x10
    int*    arr[5];      // 0x24
    ArrSet() { for (int i = 0; i < 5; i++) { arr[i] = 0; cnt[i] = 0; } }
};

// the troop/skin owner (this): the skin at +0, the ArrSet vector at +0x68.
struct EChar {
    char pad0[0x68];
    std::vector<ArrSet> sets;                                             // 0x68
    void blend_indexed(mesh* m, void* set, int* index, const float* w);   // 0x47c810
    void blend_all(lod_mesh* m, void* set, const float* w);               // 0x47ddf0
    void use_carry(lod_mesh* l, const float* w);                          // 0x47de60
    void apply_group(tstring* dst, int idx, int first);                   // 0x470db0
    void copy_troop(tstring* dst, const void* r);                         // 0x473570
};

// std::vector push_back with the fast path guarded exactly as the original does.
template<class T> __forceinline void vpush_g(std::vector<T>& v, const T& x)
{
    if (v._Myfirst != 0 && v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}

// @ 0x47dd90
void lod_mesh::apply_anim_all(const float* t)
{
    for (int i = 0; i < count; i++) {
        std::vector<mesh*>& v = lists[i].v;
        mesh** p = v._Myfirst;
        if (p < v._Mylast) {
            do {
                (*p)->apply_anim(t);
                p++;
            } while (p < v._Mylast);
        }
    }
}

// @ 0x47ddf0
void EChar::blend_all(lod_mesh* m, void* set, const float* w)
{
    int i;
    for (i = 0; i < m->count; i++) {
        std::vector<mesh*>& v = m->lists[i].v;
        if (v._Myfirst == 0)
            continue;
        if (v.size() == 0)
            continue;
        blend_indexed(*v._Myfirst, set, &i, w);
    }
}

// @ 0x47de60
void EChar::use_carry(lod_mesh* l, const float* w)
{
    tstring* lname = (tstring*)((char*)l + 0xd0);
    int idx = -1;
    for (int i = 0; i < (int)sets.size(); i++) {
        if (vat(sets, i).name.equals(*lname)) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        idx = (int)sets.size();
        sets.resize(sets.size() + 1, ArrSet());
        copy_troop(&vat(sets, idx).name, l);
    }
    blend_all(l, &vat(sets, idx), w);
}

// ---- 0x47df80: the scatter-set generator ----------------------------------------------------------------
struct GmRec {                        // 0x48 bytes: a per-map record list
    char pad0[0xc];
    int n;                            // 0x0c
    std::vector<mesh*> items;         // 0x10
    char pad1[0x48 - 0x20];
};
struct GmEntry {                      // g_map[i], stride 0x20
    char pad0[0x10];
    int f10;                          // 0x10
    char pad14[0x18 - 0x14];
    int f18;                          // 0x18
    GmRec* recs;                      // 0x1c
};
extern GmEntry* g_map;                // 0x5e28f8 (pointer to the entry array, stride 0x20)
extern int g_dinput_enabled;          // 0x5e28fc

struct GenRec {                       // 0xa0 bytes, records of the scatter set
    int map_idx;                      // 0x00
    int x;                            // 0x04
    int y;                            // 0x08
    char pad0c[8];
    pos p;                            // 0x14 (0x40)
    float f54, f58, f5c;              // 0x54
    char pad60[4];
    int val;                          // 0x64
    char pad68[4];
    mesh* meshes[4];                  // 0x6c
    int counts2[4];                   // 0x7c
    int counts3[4];                   // 0x8c
    int count;                        // 0x9c
};
struct scatter_set { int count; };    // records at +0x04, stride 0xa0

struct entity {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    char pad[0x118 - 4];
    std::vector<mesh*> meshes;        // 0x118
};

struct GenObj {
    char pad0[0xc3b0dc];
    void* sc;                         // 0xc3b0dc: the scene registry
    char pad1[0x843bd70 - 0xc3b0e0];
    int blk;                          // 0x843bd70
    char pad2[0x845d984 - 0x843bd74];
    int nx;                           // 0x845d984
    int ny;                           // 0x845d988
    entity** ents;                    // 0x845d98c
    void gen_set_init(scatter_set* s);    // 0x47df80
};

// @ 0x47df80
void GenObj::gen_set_init(scatter_set* s)
{
    std::vector<mesh*> grid[25];

    for (int i = 0; i < s->count; i++) {
        GenRec* rec = (GenRec*)((char*)s + 0x04 + i * 0xa0);
        GmEntry* ge = &g_map[rec->map_idx];
        rec->val = (rec->val + ge->f18 * 100) % ge->f18;

        std::vector<mesh*> list;
        GmRec* gr = &ge->recs[rec->val];
        if (gr->n > 0)
            list = gr->items;

        mat3 sm;
        identity3(&sm);
        sm.s.x = rec->f54;
        sm.f.y = rec->f58;
        sm.u.z = rec->f5c;
        mat3 r = rec->p.rot3(*(const matrix*)&sm);

        mat4 mat;
        mat.s = r.s;
        mat.f = r.f;
        mat.u = r.u;
        mat.o = rec->p.o;

        rec->count = 0;
        for (size_t k = 0; k < list.size(); k++) {
            mesh* src = list[k];

            int cx = rec->x / blk;
            if (cx < 0)
                cx = 0;
            else if (cx >= nx)
                cx = nx - 1;
            int cy = rec->y / blk;
            if (cy < 0 || cy >= ny)
                cy = ny - 1;
            int cell = cy + cx * 5;

            std::vector<mesh*>& g = grid[cell];
            int found = -1;
            for (size_t j = 0; j < g.size(); j++) {
                if (g[j]->res == src->res) {
                    found = (int)j;
                    break;
                }
            }
            mesh* dst;
            if (found >= 0 && !(g[found]->v50._Myfirst != 0 && g[found]->v50.size() > 32000)) {
                dst = g[found];
            } else {
                dst = new mesh();
                dst->res = src->res;
                dst->mask = 0x1ffff;
                dst->flags |= 0x10000000;
                found = (int)g.size();
                vpush_g(g, dst);
            }

            mesh* cur = g[found];
            if (rec->count < 4) {
                rec->meshes[rec->count] = cur;
                rec->counts2[rec->count] = cur->v50._Myfirst ? (int)cur->v50.size() : 0;
            }
            if ((ge->f10 & 0x20000) == 0)
                cur->append_skinned(src, &mat, -1, -1, 0);
            else
                cur->FUN_0047aa40(src, &mat, -1);
            if (rec->count < 4) {
                rec->counts3[rec->count] = cur->v50._Myfirst ? (int)cur->v50.size() : 0;
                rec->count++;
            }
        }
    }

    for (int y = 0; y < ny; y++) {
        int cell = y;
        for (int x = 0; x < nx; x++) {
            std::vector<mesh*>& g = grid[cell];
            for (size_t j = 0; j < g.size(); j++) {
                mesh* m = g[j];
                if (g_dinput_enabled == 0 && (*(unsigned char*)((char*)m->res + 0x1c) & 0x40) == 0) {
                    mat3 im;
                    identity3(&im);
                    mat4 id;
                    id.s = im.s;
                    id.f = im.f;
                    id.u = im.u;
                    id.o = vector4(0.0f, 0.0f, 0.0f);
                    ((scene*)sc)->place_mesh(m, &id, 1);
                }
                m->finish_load();
                entity* e = ents[cell];
                vpush_g(e->meshes, m);
                e->v4();
            }
            cell += 5;
        }
    }
}
