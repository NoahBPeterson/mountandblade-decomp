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

// ---- engine std::vector views (alloc, begin, end, cap): size() is inline, capacity()/resize are out of line ---
struct mat3 { vector4 a, b, c; };
struct mxo { mat3 r; vector4 o; };            // a matrix as three rows + origin
struct vec96 {                                  // 0x60-byte elements (s0040ea00 vec96)
    void* alloc;
    char* first;
    char* last;
    char* end;
    unsigned size() const { return first == 0 ? 0 : (last - first) / 0x60; }
    unsigned capacity() const;                  // 0x40f030
    void resize(unsigned n);                    // 0x4f46d0
};
struct vec32 {                                  // 0x20-byte elements (s0040ea00 vec32)
    void* alloc;
    char* first;
    char* last;
    char* end;
    unsigned size() const { return first == 0 ? 0 : (last - first) >> 5; }
    unsigned capacity() const;                  // 0x40f090
    void resize(unsigned n);                    // 0x4f4860
};

struct dmesh;
struct dmesh {                                  // an rgl::mesh as far as this slice sees it
    char pad00[0x18];
    dmesh* master;                              // +0x18
    char pad1c[0x50 - 0x1c];
    vec96 f50;                                  // +0x50 frames (0x60 bytes)
    char pad60[0x10];                           // +0x60
    vec32 f70;                                  // +0x70 records (0x20 bytes)
    char pad80[0x10];                           // +0x80
    vec32 f90;                                  // +0x90 records (0x20 bytes)
    char flag_a0;                               // +0xa0
    char flag_a1;                               // +0xa1
    char pada2[0x1d4 - 0xa2];
    matrix frame;                               // +0x1d4

    vec96* get50_();                            // 0x40ebd0 (walks master until flag_a1; called on the master)
    vec32* get70_();                            // 0x40eba0 (same)
    vec32* get90_();                            // 0x40ec30 (walks master until flag_a0)
    vec96* get50() { return flag_a1 ? &f50 : master->get50_(); }
    vec32* get70() { return flag_a1 ? &f70 : master->get70_(); }
    vec32* get90() { return flag_a0 ? &f90 : master->get90_(); }
    void finish_load();                         // 0x419300
    void append_skinned(dmesh* src, const matrix* m, int bone, unsigned color, char flip);   // 0x44eb00 (ret 0x14)
    void append_shaded(dmesh* src, const matrix* m, unsigned color);                          // 0x47aa40 (ret 0xc)
};

struct vec2 {
    float x, y;
    vec2(float a, float b) : x(a), y(b) {}
};
struct frame60 {                                // 0x60: one frame of a dust mesh
    char pad00[0x48];
    vec2 xy;                                    // +0x48 / +0x4c
    int type_index;                             // +0x50
    char pad54[0x0c];
};

struct AnimBox {                                // 0x48
    char pad0[0x14];
    dmesh** meshes;                             // +0x14
    char pad18[0x48 - 0x18];
};
struct ptype {                                  // 0x20, table at *0x5e28f8
    char pad0[0x10];
    __int64 flags;                              // +0x10 bit 0x20000
    int nframes;                                // +0x18
    AnimBox* boxes;                             // +0x1c
};
extern ptype* g_ptypes;                         // 0x5e28f8

struct particle {                               // 0xa0, list at mission + 0x5c3b8f8
    int type;                                   // +0x00
    char pad04[0x10];
    matrix frame;                               // +0x14
    vector4 dir;                                // +0x54
    int anim;                                   // +0x64
    char pad68[4];
    dmesh* meshes[4];                           // +0x6c
    int first[4];                               // +0x7c
    int last[4];                                // +0x8c
    int nmesh;                                  // +0x9c
};

struct cell_obj {                               // the scene object of one cell
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void refresh();                     // +0x10
    virtual float length();                     // +0x14
    char pad[0xc4 - 4];
    vector4 pos;                                // +0xc4
    char pad_d4[0x11c - 0xd4];
    dmesh** list;                               // +0x11c
    dmesh** list_end;                           // +0x120
    char pad124[0x140 - 0x124];
    char active;                                // +0x140
};

struct cell_rec {                               // 0x34 per cell; x stride 0x9c0
    float x, y;                                 // +0x00 / +0x04
    char pad08[8];
    float reach;                                // +0x10
    char pad14[0x20];
};

// ---- the mission --------------------------------------------------------------------------------------
struct Mission {
    char pad0[0x5c3b8f4];
    int dust_count;                             // +0x5c3b8f4 (the list header: count, then the entries)
    particle dust[1];                           // +0x5c3b8f8 (stride 0xa0; really 0x40000)
    char pad1[0x843bd78 - 0x5c3b8f8 - sizeof(particle)];
    cell_obj* grid[0x30][0x30];                 // +0x843bd78, [x][y]
    int emitted[0x30][0x30];                    // +0x843e178, [x][y]
    char pad2[0x8440598 - 0x843e178 - 0x900 * 4];
    cell_rec cells[0x2f][0x30];                 // +0x8440598, [x][y] (0x34 each; the last row is cut short by n_inner)
    char pad3[0x845d97c - 0x8440598 - 0x2f * 0x30 * 0x34];
    int n_inner;                                // +0x845d97c (cells along x)
    int n_outer;                                // +0x845d980 (cells along y)

    void update_particles(const matrix* view);                  // 0x4f3e10 (ret 4)
    void emit_cell(int x, int y);                               // 0x4f4060 (ret 8)
    vector4* ground_color(vector4* out, const float* pos);      // 0x464d50 (s00464a60, ret 8)
    void light_particle(void* list, int idx, bool trace);       // 0x471b30 (s00471b30, ret 0xc)
};

struct pos : matrix {
    mat3 rot3(const matrix& m) const;                           // 0x40a920 (hidden return, ret 4)
};
mat3* __cdecl identity3(mat3* out);                             // 0x40cd60
unsigned __cdecl pack_color(const float* c);                    // 0x40e440

// @ 0x4f3e10
void Mission::update_particles(const matrix* view)
{
    for (int a = 0; a < n_outer; a++) {
        for (int b = 0; b < n_inner; b++) {
            cell_rec* rec = &cells[b][a];
            if (emitted[b][a] == 0) {
                float dx = rec->x - view->o.x;
                float dy = rec->y - view->o.y;
                if ((float)sqrt(dy * dy + dx * dx) < rec->reach + g_k_reach)
                    emit_cell(b, a);
            }
            cell_obj* o = grid[b][a];
            if (o->list && (o->list_end - o->list) != 0) {
                float dx = o->pos.x - view->o.x;
                float dy = o->pos.y - view->o.y;
                if ((float)sqrt(dy * dy + dx * dx) > o->length() + g_k_reach) {
                    o->active = 0;
                } else {
                    o->active = 1;
                    matrix* m = &o->list[0]->frame;
                    *(mat3*)m = *(const mat3*)view;
                    m->o.w = 1.0f;
                    m->s.w = 0.0f;
                    m->f.w = 0.0f;
                    m->u.w = 0.0f;
                    *(mat3*)m = *(const mat3*)view;
                    m->s.x *= g_k_0_2;
                    m->s.y *= g_k_0_2;
                    m->s.z *= g_k_0_2;
                    m->f.x *= g_k_0_2;
                    m->f.y *= g_k_0_2;
                    m->f.z *= g_k_0_2;
                    m->u.x *= g_k_0_2;
                    m->u.y *= g_k_0_2;
                    m->u.z *= g_k_0_2;
                    vector4 tint(0.5f, 0.5f, 0.5f);
                    m->o = tint;
                }
            }
        }
    }
}

static inline int cell_clamp(int v, int n)
{
    if (v < 0)
        return 0;
    return v < n ? v : n - 1;
}

// @ 0x4f4060
void Mission::emit_cell(int x, int y)
{
    emitted[x][y] = 1;
    int n = dust_count;
    unsigned nf = 0, nr = 0, no = 0;
    if (n > 0) {
        int i = 0;
        do {
            particle* p = &dust[i];
            int ix = (int)(p->frame.o.x * g_k_1_20);
            int cx;
            if (ix < 0) {
                cx = 0;
            } else {
                cx = n_inner - 1;
                if (ix < n_inner)
                    cx = ix;
            }
            int cy = (int)(p->frame.o.y * g_k_1_20);
            if (cy < 0)
                cy = 0;
            else if (cy >= n_outer)
                cy = n_outer - 1;
            if (cx == x && cy == y) {
                dmesh* src = g_ptypes[p->type].boxes[p->anim].meshes[0];
                nf += src->get50()->size();
                nr += src->get70()->size();
                no += src->get90()->size();
            }
            i++;
        } while (--n);
    }
    vec32* v70 = &grid[x][y]->list[0]->f70;
    if (nr > v70->capacity())
        v70->resize(nr);
    vec96* v50 = &grid[x][y]->list[0]->f50;
    if (nf > v50->capacity())
        v50->resize(nf);
    vec32* v90 = &grid[x][y]->list[0]->f90;
    if (no > v90->capacity())
        v90->resize(no);
    for (int i = 0; i < dust_count; i++) {
        particle* p = &dust[i];
        int cx = (int)(p->frame.o.x * g_k_1_20);
        if (cx < 0)
            cx = 0;
        else if (cx >= n_inner)
            cx = n_inner - 1;
        int cy = (int)(p->frame.o.y * g_k_1_20);
        if (cy < 0)
            cy = 0;
        else if (cy >= n_outer)
            cy = n_outer - 1;
        if (cx == x && cy == y) {
            vector4 col;
            ground_color(&col, &p->frame.o.x);
            unsigned color = pack_color(&col.x) | 0xff000000;
            mat3 m3;
            identity3(&m3);
            m3.a.x = p->dir.x;
            m3.b.y = p->dir.y;
            m3.c.z = p->dir.z;
            mxo mm;
            mm.r = ((pos*)&p->frame)->rot3(*(const matrix*)&m3);
            mm.o = p->frame.o;
            p->nmesh = 1;
            p->meshes[0] = grid[cx][cy]->list[0];
            p->first[0] = grid[cx][cy]->list[0]->f50.size();
            ptype* pt = &g_ptypes[p->type];
            p->anim = p->anim % pt->nframes;
            dmesh* src = pt->boxes[p->anim].meshes[0];
            if ((pt->flags & 0x20000i64) != 0)
                grid[cx][cy]->list[0]->append_shaded(src, (const matrix*)&mm, color);
            else
                grid[cx][cy]->list[0]->append_skinned(src, (const matrix*)&mm, -1, color, 0);
            p->last[0] = grid[cx][cy]->list[0]->f50.size();
        }
    }
    cell_obj* o = grid[x][y];
    if (o->list && (o->list_end - o->list) != 0) {
        dmesh* m = o->list[0];
        m->finish_load();
        grid[x][y]->refresh();
        vector4 zv(0.0f, 0.0f, 0.0f);
        mat3 tmp;
        mxo mm;
        mm.r = *identity3(&tmp);
        mm.o = zv;
        m->frame = *(matrix*)&mm;
        m->frame.s.w = 0.0f;
        m->frame.f.w = 0.0f;
        m->frame.u.w = 0.0f;
        m->frame.o.w = 1.0f;
        for (unsigned i = 0; m->f50.first != 0 && i < m->f50.size(); i++) {
            frame60* fr = (frame60*)m->f50.first + i;
            char* v = m->f70.first + (fr->type_index << 5);
            fr->xy = vec2(*(float*)v * g_k_0_2, *(float*)(v + 4) * g_k_0_2);
        }
    }
    if (g_use_shaders == 0 || g_x5aa084 != 2) {
        for (int i = 0; i < dust_count; i++) {
            particle* p = &dust[i];
            int ix = (int)(p->frame.o.x * g_k_1_20);
            int cx;
            if (ix < 0) {
                cx = 0;
            } else {
                cx = n_inner - 1;
                if (ix < n_inner)
                    cx = ix;
            }
            int cy = (int)(p->frame.o.y * g_k_1_20);
            if (cy < 0)
                cy = 0;
            else if (cy >= n_outer)
                cy = n_outer - 1;
            if (cx == x && cy == y)
                light_particle(&dust_count, i, false);
        }
    }
}
