//-----------------------------------------------------------------------------
// File: terrain_emit.cpp  (0x466cd0..0x467f80, 0x467fb0..0x4681b0)
//
// Desc: the second object of the world-map terrain code, split from terrain_a.cpp at 0x466cd0: the particle emitters,
//       the triangle corners, the sun light, the chunk entities, the layer loader, the low-quad count, and the classes
//       the same object holds around them: a mission helper, the std::vector / list instantiations, the prop tween,
//       the missile and the allocator constructs. Terrain::emit_particle 0x466cd0 is byte-exact only with
//       Terrain::cell_of 0x464b40 opaque, while Terrain::sample 0x465af0 needs its body (docs/original_tus.md).
//       Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./common.h"
#include "../../rgl/options.h"   // g_options
#include "./terrain.h"
#include "./game_state.h"
#include "./misc.h"
#include "../render_batch.h"                // render_batch (draw 0x46d8c0), dev_buffer (upload 0x46d060)
#include "./wind.h"                          // tri_normal (0x4612d0)
#include "../../include/frustum.h"            // rgl::frustum (to_clip 0x4621f0)
#include "../../include/mat4.h"               // pos (identity_pos 0x40cd00)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../../include/item_kind.h"   // g_items 0x5e29d0

extern unsigned __int64 g_bit[];                            // 0x5a8e68 (game_globals.h): g_bit[2] = 4 (size mask), g_bit[4] = 0x10 (type mask)
#include "../mission.h"                                      // Mission, g_mission (0x5e292c)
#include "../../include/render_flags.h"

using rgl::vector4;
using rgl::matrix;

#include "../../rgl/scene/world.h"                                                        // rgl::world: create_entity 0x428440

// ---- particles -----------------------------------------------------------------------------------------------------

// @ 0x466cd0
void Terrain::emit_particle(particle_list* l, int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim)
{
    if (l->count < 0x40000) {
        TerrainParticle* e = &l->items[l->count];
        e->nmesh = 0;
        e->type = type;
        int c[2];
        cell_of(c, &frame->o.x);
        int cx = c[0], cy = c[1];
        e->anim = anim;
        e->cy = cy;
        e->cx = cx;
        e->f0c = 0;
        ptype* t = &g_ptypes[type];
        anim += t->nframes * 30;
        e->anim = anim;
        e->anim = anim % t->nframes;
        e->dir = *dir;
        e->frame = *frame;
        l->count++;
    }
}

// @ 0x466d90
void Terrain::emit_by_type(int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim)
{
    __int64 flags = g_ptypes[type].flags;
    if (flags & 0x80000) {
        emit_particle(&list_c, type, frame, dir, anim);
        return;
    }
    if (flags & 0x400000)
        emit_particle(&list_b, type, frame, dir, anim);
    else
        emit_particle(&list_a, type, frame, dir, anim);
}

// @ 0x466e20
void Terrain::triangle_corners(int x, int y, int flip, int* a, int* b, int* c)
{
    if (this->quad(x, y).diag != 0) {
        if (flip == 0) {
            a[0] = x; a[1] = y;
            b[1] = y; b[0] = x + 1;
            c[0] = x + 1; c[1] = y + 1;
        } else {
            a[1] = y; a[0] = x;
            b[0] = x + 1; b[1] = y + 1;
            c[0] = x; c[1] = y + 1;
        }
    } else {
        if (flip == 0) {
            a[1] = y; a[0] = x + 1;
            b[1] = y + 1; b[0] = x;
            c[0] = x; c[1] = y;
        } else {
            a[0] = x + 1; a[1] = y;
            b[0] = x + 1; b[1] = y + 1;
            c[0] = x; c[1] = y + 1;
        }
    }
}

// @ 0x466ed0
rgl::vector4 Terrain::sun_light(const float* dir, const float* scale)
{
    rgl::vector4 amb = sun_ambient;
    float t = -(sun_dir[2] * dir[2] + sun_dir[1] * dir[1] + sun_dir[0] * dir[0]);
    if (0.0f > t) t = 0.0f;
    t = t * scale[0];
    rgl::vector4 d(t * sun_diffuse.x, t * sun_diffuse.y, t * sun_diffuse.z);
    rgl::vector4 r;                 // r.w is never written: the original copies the stack slot as is
    r.x = amb.x + d.x;
    r.y = d.y + amb.y;
    r.z = d.z + amb.z;
    return r;
}

// create_chunk_entities' chunk count along one edge: an inline helper writing through a reference (the store through
// it is not disambiguated from `block`, which the original reloads before the remainder's second idiv).
static inline void chunk_count(Terrain* t, int& n, int size)
{
    n = size / t->block;
    if (size % t->block > t->block / 2) n++;
    int v = n;
    if (v < 1) v = 1;
    else if (v >= 5) v = 4;
    n = v;
}

// @ 0x466fa0
void Terrain::create_chunk_entities()
{
    TEntity** p = ents;
    for (int a = 5; a; a--) {
        for (int b = 5; b; b--) {
            p[0] = 0;
            ((int*)p)[25] = 0;                  // arr2[i]
            int* q = (int*)p + 50;              // arr3[0][i]
            for (int r = 15; r; r--) { *q = 0; q += 25; }
            p++;
        }
    }
    chunk_count(this, nx, w);
    chunk_count(this, ny, h);
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            ents[i * 5 + j] = (TEntity*)world->create_entity(9);   // 0x428440
            int ch = block;
            int cw = ch;
            if (i == nx - 1) {
                cw = w;
                if (!(ch / 2 < cw % ch)) { cw = cw % block + block; } else { cw = cw % ch; }
            }
            if (j == ny - 1) {
                if (h % block > block / 2) ch = h % ch;
                else ch = h % block + block;
            }
            int zero = 0;
            ents[i * 5 + j]->s6(&zero);
            terrain_cell& cell = cells[block * (i * 251 + j) + (cw / 2) * 251 + ch / 2];
            ents[i * 5 + j]->pos = *(rgl::vector4*)&cell;
            ents[i * 5 + j]->pos.z += 2.0f;
        }
    }
}

// @ 0x467250
void Terrain::load_layer(int* set, int idx)
{
    // set[0] = count; entry idx has its type at set[idx * 6 + 7] and its data pointer at set[idx * 6 + 8]
    if (set != 0 && idx < *set) {
        if (set[idx * 6 + 7] == -0x1e71) {
            // height offset layer: f88 is the offset, pz = h2 + offset
            if (set[idx * 6 + 8] == 0) {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++) {
                        // the zero is stored through at() and read back through c: cl then keeps the reload
                        // (mov [f88],0; fld [f88]; fadd [h2]) instead of folding 0.0f + h2
                        at(x, y).f88 = 0.0f;
                        terrain_cell* c = &at(x, y);
                        c->pz = c->f88 + c->h2;
                    }
            } else {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++) {
                        terrain_cell* c = &at(x, y);
                        c->f88 = *(float*)(y * 4 + x * 0x690 + set[idx * 6 + 8]);
                        c->pz = c->f88 + c->h2;
                    }
            }
        } else if (set[idx * 6 + 7] == -0x3115) {
            if (set[idx * 6 + 8] == 0) {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++)
                        at(x, y).f8c = -1;
            } else {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++)
                        at(x, y).f8c = *(int*)(y * 4 + x * 0x690 + set[idx * 6 + 8]);
            }
        } else {
            if (set[idx * 6 + 8] == 0) {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++)
                        at(x, y).layer[idx] = 0;
            } else {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++)
                        at(x, y).layer[idx] = *(int*)(y * 4 + x * 0x690 + set[idx * 6 + 8]);
            }
            int t = set[idx * 6 + 7];
            tlayer* l = &layers[idx];
            // the type store is written in both arms, after tt in the first (cl merges it back above the jl after
            // allocation): t is live past tt's definition, so tt gets its own register (mov ecx,ebx; shl ecx,5)
            if (t >= 0) {
                GroundType* tt = &g_ground_types[t];
                l->type = t;
                l->f1c = (int)tt->mat;
                l->f24 = tt->f18;
                l->f14 = tt->f1c;
                l->f18 = -1;
            } else {
                l->type = t;
            }
        }
    }
}

// @ 0x467530
int Terrain::count_low_quads()
{
    int count = 0;
    for (int y = 20; y < h - 20; y++)
        for (int x = 20; x < w - 20; x++) {
            terrain_quad& q = quad(x, y);
            if (q.nrm[0].z < 0.6f || q.nrm[1].z < 0.45f)
                count++;
        }
    return count;
}

// ---- mission helpers --------------------------------------------------------------------------------------------

// @ 0x4676c0
void Mission::clear_done()
{
    for (int i = 0; i < 64; i++) done[i] = 0;
}

// @ 0x4676e0
void Mission::copy_slot(int i)
{
    char* base = (char*)this + i * 0x130;       // slot i of the 0x130-byte records at +0x6b0 (cl keeps the +0x6b0 separate)
    *(m64*)(base + 0x6b0) = *(m64*)(base + 0x6f0);
    *(int*)(base + 0x774) = 0;
    *(int*)(base + 0x770) = 0;
    *(m64*)(base + 0x730) = *(m64*)(base + 0x6f0);
    *(int*)(base + 0x798) = 0;
}

// @ 0x467740
float Mission::speed_factor(float x)
{
    float base = g_options.f74 * 2.5f + 0.833333f;
    float t = (x + 15.0f) * 0.06666667f;
    return base * (t < 0.2f ? 0.2f : (t >= 2.5f ? 2.5f : t));
}

// @ 0x4677a0
void Mission::add_amount(int idx, unsigned flags)
{
    unsigned mask = flags & 0xfffffff;
    char* rec = (char*)g_mission_templates[mode].recs + idx * 0x4c;
    if (mask == 0) return;
    unsigned rf = *(unsigned*)(rec + 4);
    unsigned amount;
    if ((rf & 4) || (rf & 0x2000000)) {
        amount = mask;
    } else if (rf & 8) {
        amount = 1;
    } else if (rf & 0x10) {
        amount = ((World3*)g_game)->tbl[0xc5 + f5df4 * 0x125 + idx];
    } else {
        int v;
        if ((rf & 1) || ((rf & 0x40) && f6214 == 1) || ((rf & 0x80) && f6214 == 0)
            || (f6214 == 2 && (rf & 0x40) && f6218 == 0) || (f6214 == 2 && (rf & 0x80) && f6218 == 1))
            v = -terrain.f150;
        else
            v = terrain.f150;
        float f = speed_factor((float)v) * (float)(int)mask;
        amount = round_float(&f);
        if ((int)amount <= 0) amount = 1;
    }
    if (flags & 0x20000000) {
        float f = ((float)(rand() % 0x3dc9) * 5.0578492e-05f + 0.6f) * (float)(int)amount;
        float fl = (float)floor((double)f);     // 0x432540 inlined
        int n = (int)fl;
        amount = (f - fl <= 0.5f) ? n : n + 1;
    }
    *(int*)(rec + 0x18) += amount;
    done[idx] = 0;
}

// ---- test whether a candidate (b) passes the filter flags of a rule (a) -------------------------------------

// @ 0x467940
bool Mission::rule_accepts(SpawnRec* ra, Stack* sb, int p3, char p4, char p5, char p6)   // a member that ignores this
{
    RuleRec* a = (RuleRec*)ra;      // the fields this test reads, as misc.h names them
    CandRec* b = (CandRec*)sb;
    bool ok = true;
    if (b->f8 & 1) ok = false;
    unsigned fl = a->flags;
    if ((fl & 0x400) && (((World3*)g_game)->troops[b->troop].flags30 & 0x10) == 0)
        ok = false;
    if ((fl & 0x200) && p3 != 0 && (((World3*)g_game)->troops[b->troop].flags30 & 0x10) != 0)
        ok = false;
    if ((fl & 0x100) && p3 == 0 && (((World3*)g_game)->troops[b->troop].flags30 & 0x10) != 0)
        return false;
    if (ok && (fl & 0x70000)) {
        Troop* t = &(*(World3**)&g_game)->troops[b->troop];
        char x5 = 0;
        char x4 = 0;
        bool x3 = false;
        if ((t->flags30 & 0x1000000) && (a->f8 & 0x100) == 0) {
            x5 = true;
        } else if ((t->flags30 & 0x4000000)) {
            x4 = true;
        } else {
            x3 = true;
        }
        ok = false;
        if ((fl & 0x10000) && (p4 == 0 || x3)) ok = true;
        if ((fl & 0x20000) && (p5 == 0 || x4)) ok = true;
        if ((fl & 0x40000) && (p6 == 0 || x5)) ok = true;
    }
    return ok;
}

// ---- vector instantiations (names provisional; each is a distinct element type) ------------------------------------

// @ 0x467ac0
int vec36::size() const { return first == 0 ? 0 : (int)(last - first); }

// @ 0x467ae0
int vec24::size() const { return first == 0 ? 0 : (int)(last - first); }

// @ 0x467b00
int vec52::size() const { return first == 0 ? 0 : (int)(last - first); }

// @ 0x467b20
int vec516::size() const { return first == 0 ? 0 : (int)(last - first); }

// @ 0x467b40
void fill36(rec36* first, rec36* last, const rec36* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x467b70
rec36* copy_backward36(rec36* first, rec36* last, rec36* dest)
{
    while (first != last) {
        --last;
        --dest;
        *dest = *last;
    }
    return dest;
}

// @ 0x467ba0
void fill24(elem24* first, elem24* last, const elem24* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x467bf0
void __cdecl fill52(elem52* f, elem52* l, const elem52& v) { for (; f != l; ++f) *f = v; }

// @ 0x467c20
elem52* __cdecl copy_backward52(elem52* f, elem52* l, elem52* d) { while (f != l) *--d = *--l; return d; }

// ---- timed transform tween of a scene prop --------------------------------------------------------------------------

static inline vector4 p_add(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
// @ 0x467c50
void Prop::update(float)
{
    if (state >= 0) {
        __int64 dt = g_ticks[clock] - start;
        float t = (dt * 1e-5f) / duration;
        if (t > 1.0f) {
            cur = to;
            state = -1;
            cur.orthonormalize();
            ent->set_frame(&cur, extra);
        }
        if (state >= 0 && t != last_t) {
            matrix m;
            m.o = p_add((to.o * t), (from.o * (1.0f - t)));
            m.s = p_add((to.s * t), (from.s * (1.0f - t)));
            m.f = p_add((to.f * t), (from.f * (1.0f - t)));
            m.u = p_add((to.u * t), (from.u * (1.0f - t)));
            m.orthonormalize();
            cur = m;
            if (ent) ent->set_frame(&cur, extra);
        }
        last_t = t;
    }
}

// ---- missile -----------------------------------------------------------------------------------------------------------

// 0x467f80 is Missile::ammo_scale (terrain_b/poly_mesh.cpp: the ground correction 0x472650 calls it, so it is compiled where cl sees its registers)

// @ 0x467fb0
void Missile::init()
{
    f54 = 0.01f;
    int& n = f68; n = 0;
    t0 = g_ticks[clock];
    spent = 0;
    fa0 = 0;
    f94 = 1.0f;
    fa4 = -1;
    f9c = 1;
    vector4 d(((MissionCam*)g_mission)->cam[0] - p20.x, ((MissionCam*)g_mission)->cam[1] - p20.y, ((MissionCam*)g_mission)->cam[2] - p20.z);
    if (0.1f < d.x * p40.x + (d.z * p40.z + d.y * p40.y))
        fa0 = 1;
}

// @ 0x468050
elem56* __cdecl copy_arrset(elem56* f, elem56* l, elem56* d) { for (; f != l; ++f, ++d) *d = *f; return d; }

// @ 0x4680c0
elem56* __cdecl copy_backward56(elem56* f, elem56* l, elem56* d) { while (f != l) *--d = *--l; return d; }

// ---- allocator::construct; _Uninit_copy ----------------------------------------------------------------------------------

// @ 0x468130
void alloc24::construct(elem24* p, const elem24& v) { new ((void*)p) elem24(v); }

// @ 0x468170
void alloc100::construct(particle100* p, const particle100& v) { new ((void*)p) particle100(v); }

// @ 0x468190
void alloc52::construct(elem52* p, const elem52& v) { new ((void*)p) elem52(v); }
