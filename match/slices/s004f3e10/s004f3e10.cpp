// Slice s004f3e10: the "dust grid" particle pass of the mission object (same giant object as s0047a0c0:
// dust list at +0x5c3b8f4, cell scene objects at +0x843bd78, extents +0x845d97c / +0x845d980).
//
//   0x4f3e10  update_particles(view): for every grid cell (outer a < +0x845d980, inner b < +0x845d97c):
//             - the cell's 0x34-byte record holds (x, y) (+0/+4) and a reach (+0x10); when the cell has not been
//               emitted yet (word at +0x843e178 + (a * 0x30 + b) * 4) and the view origin is within reach + 36
//               in xy, emit_cell(b, a) re-emits its dust;
//             - the cell's scene object (when its +0x11c list is non-empty): hide it (byte +0x140 = 0) when its
//               animation length + 36 is below the distance to the view origin; otherwise show it and rewrite the
//               matrix at list[0] + 0x1d4: a copy of the view frame with the 4th column of side/forward/up cleared
//               and the 1/20-scaled axes, a (0.5, 0.5, 0.5, 1) colour at +0x204.
//   0x4f4060  emit_cell(inner, outer): mark the cell emitted, walk the dust list, clamp every entry's grid
//             position ((int)(world * 1/20), 0x54b4d0) into the grid, and for the entries of this cell grow the
//             cell object's three vectors (+0x50 0x60-byte frames, +0x70 / +0x90 0x20-byte records) to fit their
//             chain (object + its masters) sizes, then copy the objects into them: the frames get (x, y) from the
//             vertex table times 0.2 (0x54b4c8), the records the spawned particle frames and the object mesh
//             list; the cell object's matrix is rebuilt (set_matrix + terrain colour) and finally the dust
//             entries of the cell are relit (0x471b30) unless shaders are on in quality 2.
// Layouts: mission / dust entry from s0047a0c0, chain node + vector accessors from s0040ea00. Names provisional.
#include <math.h>
#include "rgl.h"

using rgl::matrix;
using rgl::vector4;

extern int g_use_shaders;                       // 0x5e28fc: programmable pipeline in use
extern int g_x5aa084;                           // 0x5aa084: shader quality / mode
extern float g_k_0_2;                           // 0x54b4c8
extern float g_k_1_20;                          // 0x54b4d0
extern float g_k_reach;                         // 0x598e8c (36.0)

struct ptype {                                  // 0x20, table at *0x5e28f8
    char pad0[0x10];
    unsigned flags;                             // +0x10 bit 0x20000
    char pad14[4];
    int nframes;                                // +0x18
    void* frames;                               // +0x1c
};
extern ptype* g_ptypes;                         // 0x5e28f8

// ---- an engine std::vector (alloc, begin, end, cap) ----------------------------------------------------
struct rvec {
    void* alloc;                                // +0
    char* begin;                                // +4
    char* end;                                  // +8
    char* cap;                                  // +0xc
};

struct rnode {                                  // particle mesh record, 0x18..0xa4 (s0040ea00)
    char pad00[0x18];
    rnode* master;                              // +0x18
    char pad1c[0x50 - 0x1c];
    rvec f50;                                   // +0x50 (0x10-byte elements)
    rvec f60;                                   // +0x60
    rvec f70;                                   // +0x70 (0x20-byte elements)
    rvec f90;                                   // +0x90 (0x20-byte elements)
    char flag_a0;                               // +0xa0
    char flag_a1;                               // +0xa1
};

struct frame60 {                                // 0x60: one copied frame
    rnode* node;                                // +0x00 its chain node
    char pad04[0x14];
    void* owner;                                // +0x18
    char pad1c[0x0c];
    int count;                                  // +0x28
    char pad2c[0x0c];
    int mesh_count;                             // +0x38
    char pad3c[0x0c];
    float x;                                    // +0x48
    float y;                                    // +0x4c
    int type_index;                             // +0x50
    char pad54[0x0c];
};

struct placeable {                              // the object behind a cell slot (list[0])
    char pad00[0x50];
    rvec frames;                                // +0x50 0x60-byte frames
    char pad5c[0x14];
    rvec recs;                                  // +0x70 0x20-byte records
    char pad7c[0x14];
    rvec objs;                                  // +0x90 0x20-byte records
    char pad9c[0x1d4 - 0x9c];
    matrix frame;                               // +0x1d4 (the tint rides in its `o` row at +0x204)
};

struct dust_item {                              // 0xa0, list at +0x5c3b94c
    char pad00[4];
    float gx;                                   // +0x04 world x
    float gy;                                   // +0x08 world y
    int f0c;                                    // +0x0c
    int f10;                                    // +0x10
    matrix frame;                               // +0x14
    vector4 dir;                                // +0x54
    int anim;                                   // +0x64
    int f68;                                    // +0x68
    void* obj;                                  // +0x6c
    int f70;                                    // +0x70
    int f74;                                    // +0x74
    char pad78[0x9c - 0x78];
    int nmesh;                                  // +0x9c
};

struct cell_obj {                               // the scene object of one cell
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void refresh();                     // +0x10
    virtual float length();                     // +0x14
    char pad[0xc4 - 4];
    vector4 pos;                                // +0xc4
    char pad_d4[0x11c - 0xd4];
    placeable** list;                           // +0x11c
    placeable** list_end;                       // +0x120
    char pad124[0x140 - 0x124];
    char active;                                // +0x140
};

struct cell_rec {                               // 0x34 per (outer, inner); outer stride 0x9c0
    float x, y;                                 // +0x00 / +0x04
    char pad08[8];
    float reach;                                // +0x10
    char pad14[0x20];
};

// ---- the mission --------------------------------------------------------------------------------------
struct Mission {
    char pad0[0x5c3b8f4];
    int dust_count;                             // +0x5c3b8f4
    char pad3[0x5c3b94c - 0x5c3b8f8];
    dust_item dust[0x40000];                    // +0x5c3b94c (stride 0xa0)
    char pad4[0x843bd78 - 0x5c3b94c - 0x20000 * 0xa0];
    cell_obj* grid[0x30];                       // +0x843bd78, index outer * 0x30 + inner
    char pad5[0x843e178 - 0x843bd78 - 0x30 * 4];
    int emitted[0x30];                          // +0x843e178, index outer * 0x30 + inner
    char pad6[0x845d97c - 0x843e178 - 0x30 * 4];
    int n_inner;                                // +0x845d97c
    int n_outer;                                // +0x845d980

    void update_particles(const matrix* view);  // 0x4f3e10 (free __fastcall form below)
};
extern cell_rec g_cells[];                       // +0x8440598 (a fixed address in the exe)

// ---- callees ------------------------------------------------------------------------------------------
struct node_wrap { rnode* n; };
void* node_get50(rnode* n);                     // 0x40ebd0: { rnode* x = n; while (!x->flag_a1) x = x->master; return &x->f50; }
void* node_get70(rnode* n);                     // 0x40eba0: same with f70
void* node_get90(rnode* n);                     // 0x40ec30: while (!x->flag_a0) x = x->master; return &x->f90;
int   count10(char* v);                         // (f50 count: (end - begin) / 0x10)
int   count20(char* v);                         // (f70 / f90 count: (end - begin) / 0x20)
unsigned cap32(const void* v);                  // 0x40f090 capacity of a 0x60-byte element vector
unsigned cap96(const void* v);                  // 0x40f030 capacity of a 0x60-byte element vector
int  ftol(float f);                             // 0x535a64 (float -> int)
void terrain_color(vector4* out, const float* pos, int a);            // 0x464d50 (thiscall on terrain)
unsigned pack_color(const float* c);                                  // 0x40e440
void identity3(void* out);                                            // 0x40cd60
void rot3(void* out, const matrix* m);                                // 0x40a920
void set_matrix(placeable* o);                                        // 0x419300
void refresh(cell_obj* o);                                            // +0x10 virtual
void emit_shader(placeable* o, const frame60* f, unsigned color);     // 0x47aa40
void emit_plain(placeable* o, const frame60* f, int local, unsigned color, int b);  // 0x44eb00
void resize_frames(placeable* o, int n);                              // 0x4f4860
void resize_recs(placeable* o, int n);                                // 0x4f46d0
void light_item(cell_obj* o, void* list, int i, int unused);          // 0x471b30

static inline int clamp_cell(int v, int n)
{
    if (v < 0)
        return 0;
    if (v >= n)
        return n - 1;
    return v;
}
// 0x4f4060: the dust re-emission of one cell (inner, outer = grid indices)
void __fastcall emit_cell(Mission* self2, int eax_garbage, int inner, int outer);
static inline int cell_of(float world, int n) { return clamp_cell(ftol(world * g_k_1_20), n); }
// @ 0x4f3e10
void __fastcall update_particles(Mission* self, int, const matrix* view)
{
    for (int a = 0; a < self->n_outer; a++) {
        cell_rec* rec = &g_cells[a * 0x34];
        for (int b = 0; b < self->n_inner; b++) {
            if (self->emitted[a * 0x30 + b] == 0) {
                float dx = rec->x - view->o.x;
                float dy = rec->y - view->o.y;
                if ((float)sqrt(dx * dx + dy * dy) < rec->reach + 36.0f)
                    emit_cell(self, 0, b, a);
            }
            cell_obj* o = self->grid[a * 0x30 + b];
            if (o->list && (o->list_end - o->list) > 0) {
                float dx = o->pos.x - view->o.x;
                float dy = o->pos.y - view->o.y;
                if (o->length() + 36.0f < (float)sqrt(dx * dx + dy * dy)) {
                    o->active = 0;
                } else {
                    o->active = 1;
                    placeable* p = o->list[0];
                    matrix* m = &p->frame;
                    m->s = view->s;
                    m->f = view->f;
                    m->u = view->u;
                    m->o.w = 1.0f;
                    m->s.w = 0.0f;
                    m->f.w = 0.0f;
                    m->u.w = 0.0f;
                    m->s = view->s;
                    m->f = view->f;
                    m->u = view->u;
                    m->s.x *= 0.2f;
                    m->s.y *= 0.2f;
                    m->s.z *= 0.2f;
                    m->f.x *= 0.2f;
                    m->f.y *= 0.2f;
                    m->f.z *= 0.2f;
                    m->u.x *= 0.2f;
                    m->u.y *= 0.2f;
                    m->u.z *= 0.2f;
                    m->o.x = 0.5f;
                    m->o.y = 0.5f;
                    m->o.z = 0.5f;
                    m->o.w = 1.0f;
                }
            }
        }
    }
}

// @ 0x4f4060
void __fastcall emit_cell(Mission* self2, int eax_garbage, int inner, int outer)
{
    Mission* s = self2;
    s->emitted[outer * 0x30 + inner] = 1;
    int n = s->dust_count;
    int nf = 0, nr = 0, no = 0;
    if (n > 0) {
        char* d = (char*)s->dust;
        do {
            int ix = cell_of(*(float*)(d - 0xc), s->n_inner);
            int iy = cell_of(*(float*)(d - 8), s->n_outer);
            if (ix == inner && iy == outer) {
                frame60* f = (frame60*)*(void**)(d - 0x44);
                rnode* nd = f->node;
                nf += count10((char*)node_get50(nd));
                nr += count20((char*)node_get70(nd));
                no += count20((char*)node_get90(nd));
            }
            d += 0xa0;
        } while (--n);
    }
    cell_obj* o = s->grid[outer * 0x30 + inner];
    placeable* p = o->list[0];
    if ((unsigned)no > cap32(&p->frames))
        resize_frames(p, no);
    if ((unsigned)nf > cap96(&p->recs))
        resize_recs(p, nf);
    if ((unsigned)nr > cap32(&p->objs))
        resize_frames(p, nr);
}
