//-----------------------------------------------------------------------------
// File: terrain_a.cpp  (0x464200..0x466cd0)
//
// Desc: the first object of the world-map terrain code: the sco terrain description reader, the 15-weight cell
//       record, the bit-field helpers, the world position -> cell lookup (cell_of), the ground generator (field
//       setters, ground specs, scatter lists, noise, the bilinear sample) and the height / weight / diagonal / type /
//       erosion passes. The rest of the original's terrain code from 0x466cd0 is terrain_emit.cpp: Terrain::sample
//       0x465af0 is byte-exact only with cell_of's body visible, Terrain::emit_particle 0x466cd0 only with it opaque
//       (docs/original_tus.md). Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
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
unsigned g_river_mask_a = (unsigned)g_bit[4];               // 0x5e2580
unsigned g_river_mask_b = (unsigned)g_bit[2];               // 0x5e2584
#include "../mission.h"                                      // Mission, g_mission (0x5e292c)
#include "../../include/render_flags.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----










const ivec2 g_nbr[9] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 0}};  // 0x591340 (rdata); entry 8 is the cell itself: order[0] = 8 in pick_triangle_types / accumulate_kinds reads it (the original's rdata has the zero pair there)
// File-static (the extern declarations in terrain.h are unused and should go): with no address taken, cl knows a store
// through a pointer cannot change them, so their loads schedule above the layer type stores, as in the original.
static const char* s_name_elevation = "ground_elevation";  // 0x5a9bf4
static const char* s_name_leveling = "ground_leveling";    // 0x5a9bf8


using rgl::vector4;
using rgl::matrix;

#include "../../rgl/scene/world.h"                                                        // rgl::world: create_entity 0x428440

static inline float fmin_(float a, float b) { return b < a ? b : a; }
static inline float fmax_(float a, float b) { return b > a ? b : a; }
#define IFLOOR(e) ((int)((e) > 0.0f ? (e) : (e) - 0.99999988f))

// ---- the sco terrain description ---------------------------------------------------------------------------

// @ 0x464200
void TerrainDescEntry::read(stream_base* s, Dims* d)
{
    s->r_int(&type);
    s->r_tstr(&name);
    int idx = -1;
    int i = 0;
    int p = (int)g_ground_types;
    do {
        if (((GroundType*)p)->name.equals(name))
            idx = i;
        p += 0x20;
        i++;
    } while (p < (int)(g_ground_types + 11));
    if (idx >= 0)
        type = idx;
    int present;
    s->r_int(&present);
    if (present == 0) {
        if (data != 0)
            delete[] data;
        data = 0;
        return;
    }
    if (data == 0)
        data = new float[0x2b110];
    bool value_run = false;
    int run_a = 0, run_b = 0;
    int y = 0;
    s->r_int(&run_a);
    if (run_a == 0) {
        value_run = true;
        s->r_int(&run_b);
    }
    bool need = false;
    for (y = 0; y < d->h; y++) {
        for (int x = 0; x < d->w; x++) {
            int idx2 = x * 0x1a4 + y;
            if (value_run) {
                if (need) {
                    s->r_int(&run_b);
                    need = false;
                }
                if (type >= 0) {
                    unsigned char c;
                    s->r_byte((char*)&c);
                    data[idx2] = (float)c * 0.003921569f;
                } else {
                    s->r_float(&data[idx2]);
                }
                run_b--;
                if (run_b <= 0) {
                    value_run = false;
                    need = true;
                }
            } else {
                if (need) {
                    s->r_int(&run_a);
                    need = false;
                }
                if (type == -0x3115)
                    ((int*)data)[idx2] = -1;
                else
                    ((int*)data)[idx2] = 0;
                run_a--;
                if (run_a <= 0) {
                    value_run = true;
                    need = true;
                }
            }
        }
    }
}

// @ 0x464400
void TerrainDesc::init()
{
    for (int i = 0; i < 13; i++) {
        if (layers[i].data)
            delete[] layers[i].data;
        layers[i].data = 0;
        layers[i].type = -1;
    }
    n = 0;
}

// @ 0x464440
void TerrainDesc::read(stream_base* s)
{
    s->r_int(&n);
    if (n > 13)
        n = 13;
    n_minus2 = n - 2;
    n_minus1 = n - 1;
    s->r_int(&dims.w);
    s->r_int(&dims.h);
    if (dims.w > 0x1a4 || dims.h > 0x1a4)
        mb_error("Illegal terrain size in sco file");
    for (int i = 0; i < n; i++)
        layers[i].read(s, &dims);
}

// ---- the cell and quad records -------------------------------------------------------------------------------

static __forceinline vec2 cell_v2(float x, float y) { vec2 r; r.x = x; r.y = y; return r; }
static __forceinline void cell_set_weight(terrain_cell* c, const float& v) { c->w[0] = v; c->f90 = v; }

// @ 0x4644e0
void terrain_cell::init(const float* q)
{
    P() = *(const rgl::vector4*)q;
    *(float*)&flags = 0.0f;
    weight = 1.0f;
    f14 = -1;
    f10 = -1;
    disp = cell_v2(q[0] * 0.7f, q[1] * 0.7f);
    dir = cell_v2(0.0f, 1.0f);
    order = -1;
    for (int i = 1; i < 15; i++)
        w[i] = 0.0f;
    f88 = 0.0f;
    cell_set_weight(this, 1.0f);
    h2 = pz;
    f8c = -1;
}

// @ 0x4645b0
bool terrain_cell::visible(int i)
{
    bool ok = true;
    if (w[i] < 0.01f)
        return false;
    float prod = 1.0f;
    for (int j = i + 1; j < 15; j++) {
        prod = (1.0f - w[j]) * prod;
        if (prod < 0.01f)
            ok = false;
    }
    return ok;
}

// @ 0x4646b0
// The callers in this object (0x4666b0, 0x466930, 0x466aa0) were compiled knowing that this call writes no memory
// (they keep this->w cached across it): the body must be visible here and must not be inlined.
__declspec(noinline) int terrain_cell::kind()
{
    int r = 0;
    for (int j = 1; j < 15; j++) {
        if (w[j] > 0.4f)
            r = j;
    }
    return r;
}

// @ 0x464760
void terrain_quad::init()
{
    diag = 0;
    tri[0] = 0;
    tri[1] = 0;
    *(int*)&f34 = 0;
    nrm[0] = rgl::vector4(0.0f, 0.0f, 1.0f);
    nrm[1] = rgl::vector4(0.0f, 0.0f, 1.0f);
    slot[0] = -1;
    slot[1] = -1;
}

// ---------------------------------------------------------------------------------------------------------
// Terrain::reset: the original brackets the scalar-reset part with two inline-asm data markers
// (jmp +0x10; "WL  ", 12 resp. 13, 0, "WL  "). Fields are reached by offset.
// ---------------------------------------------------------------------------------------------------------
#define BW(o) (*(int*)((char*)this + (o)))

// @ 0x4647e0
void Terrain::reset()
{
    int* b = (int*)((char*)this + 0x845da54);
    int* a = (int*)((char*)this + 0x2c);
    for (int i = 0; i < 15; i++) {
        a[-1] = 0;
        a[0] = 0;
        a[1] = 0;
        a[2] = 0;
        a[3] = 0;
        a[4] = 0x3f800000;
        for (int k = 0; k < 25; k++)
            b[k] = 0;
        a += 10;
        b += 25;
    }
    for (int o = 0x845d9f0; o <= 0x845da50; o += 4)
        BW(o) = 0;
    WL_MARKER(12)
    BW(0x5c3b8f4) = 0;
    BW(0x843b8f8) = 0;
    BW(0xc3b0e4) = 0;
    BW(0x343b0e8) = 0;
    BW(0x343b4ec) = 0;
    BW(0x5c3b4f0) = 0;
    BW(0x845d97c) = 0;
    BW(0x845d980) = 0;
    WL_MARKER(13)
}

// ---- bit-field accessors ---------------------------------------------------------------------------------------

// @ 0x464960
void Terrain::set_bits(const unsigned& lo, const int& hi, const unsigned& v)
{
    unsigned a = lo;
    unsigned mask = (unsigned)(g_bit + 1)[hi - a] - 1;
    unsigned vv = v;
    if (vv > mask)
        vv = mask;
    unsigned idx = a >> 5;
    unsigned sh = a & 0x1f;
    bits[idx] = (~(mask << sh) & bits[idx]) | (vv << sh);
}

// @ 0x4649b0
unsigned Terrain::get_bits(const unsigned& lo, const int& hi)
{
    unsigned mask = (unsigned)(g_bit + 1)[hi - lo] - 1;
    unsigned sh = lo & 0x1f;
    return ((bits[lo >> 5] & (mask << sh)) >> sh) & mask;
}

// @ 0x4649f0
void Terrain::set_pct(const float& f)
{
    float x = f * 100.0f;
    float fl = (float)floor(x);
    int i = (int)fl;
    if (!(x - fl <= 0.5f))
        i++;
    if (i < 0)
        i = 0;
    else if (i >= 100)
        i = 99;
    unsigned mask = ((unsigned*)g_bit)[14] - 1;
    if ((unsigned)i > mask)
        i = mask;
    bits[4] = (~mask & bits[4]) | i;      // the percentage rides in the low bits of bits[4] (+0x10 = Terrain::flags)
}

// ---- the ground generator -----------------------------------------------------------------------------------------

// @ 0x464a60
void Terrain::set_field_a(const float* p)
{
    int i = IFLOOR(*p);
    unsigned v = i < 0 ? 0 : (i >= 128 ? 127 : i);
    unsigned m = ((unsigned*)g_bit)[14] - 1;
    if (v > m) v = m;
    flags = (flags & ~(m << 7)) | (v << 7);
}

// @ 0x464ac0
void Terrain::set_field_b(const float* p)
{
    float x = *p * 100.0f;
    float fl = (float)floor((double)x);
    int i = (int)fl;
    if (!(x - fl <= 0.5f)) i++;
    unsigned v = i < 0 ? 0 : (i >= 100 ? 99 : i);
    unsigned m = ((unsigned*)g_bit)[14] - 1;
    if (v > m) v = m;
    flags = (flags & ~(m << 21)) | (v << 21);
}

// Called by Terrain::sample 0x465af0 here, which sees the body, and by Terrain::emit_particle 0x466cd0 in terrain_emit.cpp,
// which must not.
// @ 0x464b40
int* Terrain::cell_of(int* out, const float* p)
{
    float s = 1.0f / scale;
    int x = IFLOOR(s * p[0]);
    int ix;
    if (x < 0) ix = 0; else { ix = w - 1; if (x < w) ix = x; }
    int y = IFLOOR(s * p[1]);
    int iy;
    if (y < 0) iy = 0; else if (y >= h) iy = h - 1; else iy = y;
    out[0] = ix; out[1] = iy;
    return out;
}

// @ 0x464bf0
float Terrain::spec_weight(int* spec, int* type, int* mode)
{
    ptype* g = &g_ptypes[*spec];
    if (*mode == 0 && (g->flags & 0x400000) == 0)
        return 0.0f;
    if (*mode == 1) {
        __int64 f = g->flags;
        if ((f & 0x400000) != 0 || (f & 0x80000) != 0)
            return 0.0f;
    }
    if (*mode != 2 || (g->flags & 0x80000) != 0) {
        if (*type != 2 || (g->flags & 0x8) != 0) {
        if (*type != 10 || (g->flags & 0x800) != 0) {
        if (*type != 4 || (g->flags & 0x10) != 0) {
        if (*type != 12 || (g->flags & 0x1000) != 0) {
        if (*type != 5 || (g->flags & 0x20) != 0) {
        if (*type != 13 || (g->flags & 0x2000) != 0) {
        if (*type != 3 || (g->flags & 0x4) != 0) {
        if (*type != 11 || (g->flags & 0x400) != 0) {
        if ((*type & 7) != 7 || (g->flags & 0x4) != 0)
            return 1.0f;
        }}}}}}}}
    }
    return 0.0f;
}

// @ 0x464d50
vector4* Terrain::ground_color(vector4* out, const float* pos)
{
    const unsigned& m = ((unsigned*)g_bit)[8] - 1U;
    unsigned t = flags >> 28 & m;
    if (t == 4 || t == 12) {
        out->x = 1.0f; out->y = 1.0f; out->z = 1.0f;
        return out;
    }
    vector4 c;
    float n[4];
    c.x = off_b[0] + pos[0];
    c.y = off_b[1] + pos[1];
    c.z = off_b[2] + pos[2];
    noise_fractal(15.0f, 0.6f, 3, 0, &c.x, n);
    c.x = fmin_(n[0] * 1.2f + 0.97f, 1.0f);
    c.y = fmin_(n[1] + 0.975f, 1.0f);
    c.y = fmax_(c.x - 0.022f, c.y);
    c.x = fmax_(c.y - 0.2f, c.x);
    c.z = fmin_(n[2] + 0.97f, 1.0f);
    c.z = fmin_(c.z, c.x);
    c.z = fmin_(c.z, c.y);
    if (g_use_shaders) {
        c.x = c.x * c.x;
        c.y = c.y * c.y;
        c.z = c.z * c.z;
    }
    *out = c;
    return out;
}

// @ 0x464f10
void Terrain::init_layers()
{
    int r = rand() % 2;
    int* q = (int*)((char*)this + 0x50);
    for (int g = 0; g < 3; g++) {
        for (int k = 0; k < 5; k++)
            q[k * 10 - 10] = -1;
        q += 50;
    }
    switch (r) {
    case 0:
        layer[0].type = 0;
        layer[0].fc = (int)g_ground_types[0].mat;
        layer[0].f14 = g_ground_types[0].f18;
        layer[0].f4 = g_ground_types[0].f1c;
        layer[0].f8 = -1;
        break;
    case 1:
        layer[0].type = 1;
        layer[0].fc = (int)g_ground_types[1].mat;
        layer[0].f14 = g_ground_types[1].f18;
        layer[0].f4 = g_ground_types[1].f1c;
        layer[0].f8 = -1;
        break;
    }
    rand();
    int a = -1, b = 2, c = 5;
    const unsigned& m = ((unsigned*)g_bit)[8] - 1U;    // the mask as a const reference temporary (as in ground_color): loaded first
    switch (flags >> 28 & m) {
    case 3:
        a = b = 2;
        break;
    case 7:
        b = 2;
        break;
    case 2:
        b = 3;
        rand();
        break;
    case 4:
    case 12:
        c = 4;
        b = -1;
        break;
    case 5:
    case 13:
        b = 6;
        break;
    case 11:
        b = 2;
        c = 7;
        break;
    case 10:
        b = 3;
        c = 7;
        break;
    }
    // the type store in both arms, as in load_layer (merged back above the jl; c stays live past gt: mov eax,ebx)
    if (c >= 0) {
        GroundType* gt = &g_ground_types[c];
        layer[1].type = c;
        layer[1].fc = (int)gt->mat;
        layer[1].f14 = gt->f18;
        layer[1].f4 = gt->f1c;
        layer[1].f8 = -1;
    } else {
        layer[1].type = c;
    }
    if (b >= 0) {
        GroundType* gt = &g_ground_types[b];
        layer[2].type = b;
        layer[2].fc = (int)gt->mat;
        layer[2].f14 = gt->f18;
        layer[2].f4 = gt->f1c;
        layer[2].f8 = -1;
    } else {
        layer[2].type = b;
    }
    layer[3].type = 8;
    layer[3].fc = (int)g_ground_types[8].mat;
    layer[3].f14 = g_ground_types[8].f18;
    layer[3].f4 = g_ground_types[8].f1c;
    layer[3].f8 = -1;
    layer[1].f10 = 0;
    if (a >= 0)
        layer[1].f10 = (int)g_ground_types[a].mat2;
}

// @ 0x4650f0
void Terrain::scatter(particle_list* set, int mode, int n)
{
    set->nsrc = 0;
    int cnt = g_ptype_count;
    ptype* specs = g_ptypes;
    float total = 0.0f;
    int fits = 0;
    const unsigned& m = ((unsigned*)g_bit)[8] - 1U;    // as in ground_color: the mask is loaded first
    unsigned t = flags >> 28 & m;
    for (int i = 0; i < cnt; i++) {
        if ((specs[i].flags & 0x1000000) != 0) {
            int tt = t;
            if (spec_weight(&i, &tt, &mode) > 0.1f)
                fits++;
        }
    }
    {
        int tt = t;
        for (int i = 0; i < cnt; i++) {
            float w_ = spec_weight(&i, &tt, &mode);
            if (w_ < 0.0f) w_ = 0.0f;
            else if (w_ > 1.0f) w_ = 1.0f;
            total = w_ + total;
        }
    }
    int pick_fit = 0;
    if (fits != 0) pick_fit = 1;
    if (0 < n) {
        int left = n;
        do {
            if (total > 0.0f) {
                int pick;
                if (pick_fit) {
                    int r;
                    if (fits <= 1)
                        r = 0;
                    else {
                        r = rand() % fits;
                        cnt = g_ptype_count;
                        specs = g_ptypes;
                    }
                    float target = (float)r;
                    int seen = 0;
                    int i = 0;
                    if (0.0f <= target) {
                        do {
                            if (i >= cnt) break;
                            if ((specs[i].flags & 0x1000000) != 0) {
                                int t2 = t;
                                if (spec_weight(&i, &t2, &mode) > 0.1f)
                                    seen++;
                            }
                            i++;
                        } while ((float)seen <= target);
                    }
                    pick_fit = 0;
                    pick = i;
                } else {
                    int r = rand() % 15817;
                    float acc = 0.0f;
                    float target = (float)r * total * 6.3223117e-05f;
                    int j = 0;
                    if (0.0f < target) {
                        int cnt2 = g_ptype_count;
                        do {
                            if (j >= cnt2) break;
                            int t2 = t;
                            float w_ = spec_weight(&j, &t2, &mode);
                            acc += w_;
                            j++;
                        } while (acc < target);
                    }
                    pick = j;
                }
                int sel = pick - 1;
                scatter_src* it = &set->src[set->nsrc];
                sel = ((sel <= 0) - 1) & sel;
                it->kind = sel;
                float rx = (float)(rand() % 15817) * 0.63223112f;
                float ry = (float)(rand() % 15817) * 0.63223112f;
                float rz = (float)(rand() % 15817) * 0.63223112f;
                it->v = vector4(rz, ry, rx);
                it->freq = (float)(rand() % 15817) * 0.0031611556f + 50.0f;
                it->thresh = (float)(rand() % 15817) * 3.1611557e-06f;
                float rr = (float)(rand() % 15817) * 6.3223117e-05f;
                int dens = (unsigned short)(((unsigned*)&g_ptypes[sel].flags)[1]);
                it->rate = (rr + 1.0f) * ((it->thresh + 0.4f) * ((3.0f / ((float)n + 2.0f)) * (float)dens)) * 0.015f;
                if (g_ptypes[sel].flags & 0x80000) {
                    it->freq = it->freq * 0.45f;
                    it->thresh = it->thresh - 0.11f;
                }
                if (mode == 0 && (t == 11 || t == 12 || t == 10))
                    it->thresh = it->thresh - 0.07f;
                set->nsrc++;
                cnt = g_ptype_count;
                specs = g_ptypes;
            }
            left--;
        } while (left != 0);
    }
}

// @ 0x465540
void Terrain::scatter_all()
{
    int a, b, c;
    rand();
    const unsigned& m = ((unsigned*)g_bit)[8] - 1U;
    unsigned t = flags >> 28 & m;
    int s2 = 0;
    WL_MARKER(12)
    if (t == 11) {
        b = rand() % 4 + 2;
        c = rand() % 2 + 2;
        s2 = 4;
    } else if (t == 12) {
        b = 2;
        c = rand() % 2 + 2;
    } else if (t == 10) {
        b = rand() % 4 + 2;
        c = rand() % 2 + 2;
        s2 = 2;
    } else if (t == 5) {
        b = 0;
        c = 2;
    } else if (t == 13) {
        int k = 2;
        b = rand_below(&k) + 3;
        c = 2;
    } else {
        s2 = 4;
        int k = 4;
        b = rand_below(&k) + 3;
        k = 2;
        c = rand_below(&k) + 4;
    }
    if (g_options.i34 < 10)
        s2 = 0;
    WL_MARKER(13)
    scatter(&list_a, 1, c);
    scatter(&list_c, 2, s2);
    scatter(&list_b, 0, b);
}

// @ 0x4656c0
void Terrain::fill_desc_named(TerrainDesc* d)
{
    WL_MARKER(12)
    d->init();
    d->dims.w = w + 1;
    d->dims.h = h + 1;
    d->n = 13;
    for (int i = 0; i < 11; i++) {
        d->layers[i].type = i;
        d->layers[i].name = g_ground_types[i].name;
    }
    d->n_minus2 = 11;
    d->n_minus1 = 12;
    d->layers[d->n_minus2].type = -0x1e71;
    d->layers[d->n_minus1].type = -0x3115;
    d->layers[d->n_minus2].name.assign(s_name_elevation);
    d->layers[d->n_minus1].name.assign(s_name_leveling);
    WL_MARKER(13)
    if (d->dims.w > 0x1a4 || d->dims.h > 0x1a4)
        mb_error("Illegal terrain size");
}

// @ 0x4657c0
void Terrain::fill_desc_none(TerrainDesc* d)
{
    WL_MARKER(12)
    d->init();
    d->dims.w = w + 1;
    d->dims.h = h + 1;
    d->n = 13;
    for (int i = 0; i < 11; i++) {
        d->layers[i].type = -1;
        d->layers[i].name.assign("none");
    }
    d->n_minus2 = 11;
    d->n_minus1 = 12;
    d->layers[d->n_minus2].type = -0x1e71;
    d->layers[d->n_minus1].type = -0x3115;
    d->layers[d->n_minus2].name.assign(s_name_elevation);
    d->layers[d->n_minus1].name.assign(s_name_leveling);
    d->n = 0;
    WL_MARKER(13)
    if (d->dims.w > 0x1a4 || d->dims.h > 0x1a4)
        mb_error("Illegal terrain size");
}

// @ 0x4658c0
void Terrain::parse(const tstring& s)
{
    bits[0] = 0; bits[1] = 0; bits[2] = 0; bits[3] = 0; bits[4] = 0; bits[5] = 0;
    int end = s.len;
    int count = 0;
    tstring t;
    int start = 0;
    if (s.buf[0] == '0' && (s.buf[1] == 'x' || s.buf[1] == 'X'))
        start = 2;
    // eight hex digits per word, from the end of the string backwards
    while (end > start) {
        if (count >= 6) break;
        int b = end - 8;
        if (b < start) b = start;
        t = s.substr(b, end);
        int val = 0;
        for (int i = 0; i < t.len; i++) {
            unsigned c = t.buf[i];
            int d;
            val = val * 16;
            d = 0;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 87;
            else if (c >= 'A' && c <= 'F') d = c - 55;
            val += d;
        }
        bits[count] = val;
        count++;
        end -= 8;
    }
}

// @ 0x4659e0
float Terrain::rand_avg4()
{
    float a = (float)(rand() % 15817) * 6.3223117e-05f;
    float b = (float)(rand() % 15817) * 6.3223117e-05f;
    float c = (float)(rand() % 15817) * 6.3223117e-05f;
    return ((float)(rand() % 15817) * 6.3223117e-05f + c + b + a) * 0.25f;
}

// @ 0x465a70
float Terrain::noise_at(int x, int y)
{
    float p[4];
    float n[4];
    terrain_cell* c = &cell[x][y];
    p[0] = off_a[0] + c->px;
    p[1] = off_a[1] + c->py;
    p[2] = off_a[2] + c->pz;
    noise_fractal(noise_seed, 0.5f, 1, 0, p, n);
    return n[0] + n[1] + n[2];
}

// a cell coordinate pair as cell_of writes it
struct ti2 { int x, y; };

// @ 0x465af0
vector4 Terrain::sample(const float* p)
{
    ti2 c;
    cell_of(&c.x, p);
    // the three neighbours are copies of the cell pair bumped in place (each copy's y field keeps its dead store of j)
    ti2 c10 = c, c01 = c, c11 = c;
    c10.x++;
    c01.y++;
    c11.x++;
    c11.y++;
    vector4* q00 = (vector4*)&cell[c.x][c.y];
    vector4 c00 = *q00;
    vector4* q10 = (vector4*)&cell[c10.x][c10.y];
    vector4 c10v = *q10;
    vector4* q01 = (vector4*)&cell[c01.x][c01.y];
    vector4 c01v = *q01;
    vector4* q11 = (vector4*)&cell[c11.x][c11.y];
    vector4 c11v = *q11;
    // the blends go through the inline vector operators: hi * t + lo * u (the lo * u temporary's x, y spilled, every
    // result's w the sum temporary's unset lane)
    vector4 a, b;
    if (p[1] < c00.y) {
        a = *q00;
    } else if (p[1] > c01v.y) {
        a = *q01;
    } else {
        float t = (p[1] - c00.y) / (c01v.y - c00.y);
        float u = 1.0f - t;
        a = c01v * t + c00 * u;
    }
    if (p[1] < c10v.y) {
        b = *q10;
    } else if (p[1] > c11v.y) {
        b = *q11;
    } else {
        float t = (p[1] - c10v.y) / (c11v.y - c10v.y);
        float u = 1.0f - t;
        b = c11v * t + c10v * u;
    }
    vector4 r;
    if (p[0] < c00.x) {
        r = a;
    } else if (p[0] > c10v.x) {
        r = b;
    } else {
        float t = (p[0] - c00.x) / (c10v.x - c00.x);
        float u = 1.0f - t;
        r = b * t + a * u;
    }
    return r;
}

// @ 0x465e70
void Terrain::set_cells(int bit, float v)
{
    unsigned m = 1u << bit;
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            terrain_cell* c = &cell[x][y];
            if (c->flags & m) {
                c->h2 = v;
                c->pz = v;
            }
        }
    }
}

// @ 0x465ee0
void Terrain::smooth_cells(int bit, float k)
{
    unsigned m = 1u << bit;
    float nv = k;
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            terrain_cell* c = &cell[x][y];
            if ((c->flags & m) == 0) {
                int kind = 0;
                int nx = x + 1, ny = y + 1;
                if (x >= 0 && x <= w && ny >= 0 && ny <= h && (cell[x][ny].flags & m) != 0) {
                    kind = 2;
                    nv = cell[x][ny].pz;
                }
                if (nx >= 0 && nx <= w && ny >= 0 && ny <= h && (cell[nx][ny].flags & m) != 0) {
                    if (kind == 0) kind = 1;
                    nv = cell[nx][ny].pz;
                }
                if (nx >= 0 && nx <= w && y >= 0 && y <= h && (cell[nx][y].flags & m) != 0) {
                    kind = 2;
                    nv = cell[nx][y].pz;
                }
                int py = y - 1;
                if (nx >= 0 && nx <= w && py >= 0 && py <= h && (cell[nx][py].flags & m) != 0) {
                    if (kind == 0) kind = 1;
                    nv = cell[nx][py].pz;
                }
                if (x >= 0 && x <= w && py >= 0 && py <= h && (cell[x][py].flags & m) != 0) {
                    kind = 2;
                    nv = cell[x][py].pz;
                }
                if (x - 1 >= 0 && x - 1 <= w) {
                    if (py >= 0 && py <= h && (cell[x - 1][py].flags & m) != 0) {
                        if (kind == 0) kind = 1;
                        nv = cell[x - 1][py].pz;
                    }
                    if (y >= 0 && h >= y && (cell[x - 1][y].flags & m) != 0) {
                        kind = 2;
                        nv = cell[x - 1][y].pz;
                    }
                }
                if (x - 1 >= 0 && x - 1 <= w && ny >= 0 && ny <= h && (cell[x - 1][ny].flags & m) != 0) {
                    if (kind == 0) kind = 1;
                    nv = cell[x - 1][ny].pz;
                }
                if (kind != 0) {
                    float a = k;
                    if (kind == 1) a = k * 0.7f;
                    a = (nv - c->pz) * a + c->pz;
                    c->pz = a;
                    c->h2 = a;
                }
            }
        }
    }
}

// ---- the terrain object's passes ------------------------------------------------------------------------------

// The cross product is a MEMBER in the original (this = a x b): as a member its inline expansion's parameter symbols
// take other symbol keys than a free function's, and the keys order the x87 operands of r.z (0x4666b0). It belongs in
// rgl::vector4 (rgl_math.h); kept here as a file-local subclass until the shared header gets it.
struct t_vector4 : vector4 {
    void cross(const vector4& a, const vector4& b) { x = a.y * b.z - a.z * b.y; y = a.z * b.x - a.x * b.z; z = a.x * b.y - a.y * b.x; }
};

// smooth_heights' neighbour step: the original expands an inline helper per neighbour, with the coordinates as by-value
// parameters (the y+-1 tests reuse the parameter copy, so those cells are addressed flat, not through the row pointer).
static inline bool cell_in_grid(Terrain* t, int x, int y) { return x >= 0 && x <= t->w && y >= 0 && y <= t->h; }
static inline void add_neighbour(Terrain* t, int x, int y, float& sum, float& cnt)
{
    if (cell_in_grid(t, x, y)) {
        sum += t->at(x, y).pz;
        cnt += 1.0f;
    }
}

// @ 0x4661e0
void Terrain::smooth_heights()
{
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            float sum = this->at(x, y).pz;
            float cnt = 1.0f;
            add_neighbour(this, x, y + 1, sum, cnt);
            add_neighbour(this, x + 1, y + 1, sum, cnt);
            add_neighbour(this, x + 1, y, sum, cnt);
            add_neighbour(this, x + 1, y - 1, sum, cnt);
            add_neighbour(this, x, y - 1, sum, cnt);
            add_neighbour(this, x - 1, y - 1, sum, cnt);
            add_neighbour(this, x - 1, y, sum, cnt);
            add_neighbour(this, x - 1, y + 1, sum, cnt);
            float d = sum / cnt - this->at(x, y).pz;
            if (d < -10.0f) d = -10.0f;
            else if (d > 2.0f) d = 2.0f;
            float nh = ((float)(rand() % 0x3dc9) * 2.5289246e-05f + 0.2f) * d + this->at(x, y).pz;
            {
                terrain_cell& c = this->at(x, y);   // a block-local reference: the row pointer anchors at at(x-1, y)
                c.pz = nh;
                c.h2 = nh;
            }
        }
    }
}

// @ 0x466480
void Terrain::clear_flags(unsigned mask)
{
    unsigned inv = ~mask;
    for (int y = 0; y <= this->h; y++)
        for (int x = 0; x <= this->w; x++)
            this->at(x, y).flags &= inv;
}

// @ 0x4664e0
void Terrain::compute_weights()
{
    struct { float lead; float big; float z[13]; } lim = { 0.0f, 10000.0f };
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            terrain_cell& c = this->at(x, y);
            if ((this->layer[1].f8 == -1 || (c.flags & (1 << this->layer[1].f8))) && c.pz <= lim.big) {
                float v = ((1.0f - c.nrm.z) - 0.19f) * 12.0f;
                if (v < 0.0f) v = 0.0f;
                else if (v >= 1.0f) v = 1.0f;
                c.v4c = 1.0f - v;
            }
            if (this->layer[2].type >= 0) {
                rgl::vector4 in, out;
                in.x = (float)x;
                in.y = (float)y;
                in.z = 0.0f;
                noise_fractal(15.0f, 0.6f, 3, 0, &in.x, &out.x);
                float v = (out.x + out.y + out.z) * 4.5f + 0.7f;
                if (v < 0.0f) v = 0.0f;
                else if (v >= 1.0f) v = 1.0f;
                c.v50 = v * c.v4c;
                if (v > 0.99f) c.v4c = 0.0f;
            }
            if (this->flag843bd75 && c.pz < 0.0f)
                c.v54 = 1.0f;
        }
    }
}

// @ 0x4666b0
void Terrain::pick_diagonals()
{
    for (int y = 0; y < this->h; y++) {
        if (this->w > 0) {
            int order[4] = { 8, 2, 1, 0 };
            for (int x = 0; x < this->w; x++) {
                int v[4];
                for (int k = 0; k < 4; k++) {
                    int nx = g_nbr[order[k]].dx + x;
                    int ny = g_nbr[order[k]].dy + y;
                    if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h)
                        v[k] = this->at(nx, ny).kind();
                    else
                        v[k] = -1;
                }
                int r = -1;
                if (v[1] != v[0] && v[3] != v[0]) r = 0;
                if (v[2] != v[1] && v[0] != v[1]) r = 1;
                if (v[3] != v[2] && v[1] != v[2]) r = 2;
                if (!(v[0] == v[3] || v[2] == v[3])) goto set1;
                switch (r) {
                case -1: {
                    rgl::vector4 e1 = ((this->at(x + 1, y + 1).P()) - (this->at(x, y).P()));
                    rgl::vector4 e2 = ((this->at(x + 1, y).P()) - (this->at(x, y).P()));
                    rgl::vector4 e3 = ((this->at(x, y + 1).P()) - (this->at(x, y).P()));
                    t_vector4 cr;
                    cr.cross(e2, e3);
                    if (e1.y * cr.y + cr.x * e1.x + e1.z * cr.z > 0.0f) this->quad(x, y).diag = 0; else this->quad(x, y).diag = 1;
                    break; }
                case 0:
                case 2:
                    this->quad(x, y).diag = 0;
                    break;
                default:
                set1:
                    this->quad(x, y).diag = 1;
                    break;
                }
            }
        }
    }
}

// @ 0x466930
void Terrain::pick_triangle_types()
{
    for (int y = 0; y < this->h; y++) {
        if (this->w > 0) {
            int order[4] = { 8, 2, 1, 0 };
            for (int x = 0; x < this->w; x++) {
                int v[4];
                for (int k = 0; k < 4; k++) {
                    int nx = g_nbr[order[k]].dx + x;
                    int ny = g_nbr[order[k]].dy + y;
                    if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h)
                        v[k] = this->at(nx, ny).kind();
                    else
                        v[k] = -1;
                }
                terrain_quad& q = this->quad(x, y);
                int* pa = &q.tri[0];
                if (q.diag != 0) {
                    q.tri[1] = (v[0] > 0 ? v[0] : (v[2] > 0 ? v[2] : v[3]));
                    *pa = (v[0] > 0 ? v[0] : (v[1] > 0 ? v[1] : v[2]));
                } else {
                    q.tri[1] = (v[1] > 0 ? v[1] : (v[2] > 0 ? v[2] : v[3]));
                    *pa = (v[1] > 0 ? v[1] : (v[3] > 0 ? v[3] : v[0]));
                }
            }
        }
    }
}

// @ 0x466aa0
void Terrain::accumulate_kinds()
{
    for (int y = 0; y < this->h; y++) {
        if (this->w > 0) {
            int order[4] = { 8, 2, 1, 0 };
            for (int x = 0; x < this->w; x++) {
                int v[4];
                int cnt = 0;
                float sum = 0.0f;
                for (int k = 0; k < 4; k++) {
                    int nx = g_nbr[order[k]].dx + x;
                    int ny = g_nbr[order[k]].dy + y;
                    terrain_cell& c = this->at(nx, ny);
                    if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h)
                        v[k] = c.kind();
                    else
                        v[k] = -1;
                    if (v[k] == 2) cnt++;
                    sum += c.v50;
                }
                if (cnt > 2 && sum > 2.75f)
                    this->quad(x, y).f34 = sum * 0.22727272f;
            }
        }
    }
}

// @ 0x466c10
void Terrain::erode()
{
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            terrain_cell& c = this->at(x, y);
            if (c.v50 + c.v4c < 0.5f) {
                float r = (float)(rand() % 0x3dc9) * 6.322312e-05f;
                float f = ((r - 0.5f) * (1.0f - c.v4c)) * this->scale * 0.7f;
                c.h2 += f;
                c.pz += f;
            }
        }
    }
}
