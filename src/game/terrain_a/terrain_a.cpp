//-----------------------------------------------------------------------------
// File: terrain_a.cpp  (0x464200..0x4681b0)
//
// Desc: the first part of the world-map terrain object's translation unit: the sco terrain description
//       reader, the 15-weight cell record, the bit-field helpers, the ground generator (field setters, ground
//       specs, scatter lists, noise, height / weight / diagonal passes, particles, rivers, normals), and the
//       classes the same object holds around it: a mission helper, the std::vector / list / stable_sort
//       instantiations, the prop tween, the missile, texture / shader / skin binders, the mesh vertex and index
//       fillers, the troop auto-equip, the scene view setup, the vertex / index buffer uploader and the shadow
//       view draw pass. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
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
const char* g_name_elevation = "ground_elevation";  // 0x5a9bf4
const char* g_name_leveling = "ground_leveling";    // 0x5a9bf8


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

// @ 0x464bf0
float Terrain::spec_weight(int* spec, int* type, int* mode)
{
    ptype* g = &g_ptypes[*spec];
    int t;
    if (*mode == 0 && (g->flags & 0x400000) == 0)
        return 0.0f;
    if (*mode == 1) {
        __int64 f = g->flags;
        if ((f & 0x400000) != 0 || (f & 0x80000) != 0)
            return 0.0f;
    }
    if (*mode != 2 || (g->flags & 0x80000) != 0) {
        t = *type;
        if (t != 2 || (g->flags & 0x8) != 0) {
        if (t != 10 || (g->flags & 0x800) != 0) {
        if (t != 4 || (g->flags & 0x10) != 0) {
        if (t != 12 || (g->flags & 0x1000) != 0) {
        if (t != 5 || (g->flags & 0x20) != 0) {
        if (t != 13 || (g->flags & 0x2000) != 0) {
        if (t != 3 || (g->flags & 0x4) != 0) {
        if (t != 11 || (g->flags & 0x400) != 0) {
        if ((t & 7) != 7 || (g->flags & 0x4) != 0)
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
    switch (flags >> 28 & ((unsigned*)g_bit)[8] - 1U) {
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
    layer[1].type = c;
    if (c >= 0) {
        GroundType* gt = &g_ground_types[c];
        layer[1].fc = (int)gt->mat;
        layer[1].f14 = gt->f18;
        layer[1].f4 = gt->f1c;
        layer[1].f8 = -1;
    }
    layer[2].type = b;
    if (b >= 0) {
        GroundType* gt = &g_ground_types[b];
        layer[2].fc = (int)gt->mat;
        layer[2].f14 = gt->f18;
        layer[2].f4 = gt->f1c;
        layer[2].f8 = -1;
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
    unsigned t = flags >> 28 & ((unsigned*)g_bit)[8] - 1U;
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
    unsigned t = flags >> 28 & ((unsigned*)g_bit)[8] - 1U;
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
    d->layers[d->n_minus2].name.assign(g_name_elevation);
    d->layers[d->n_minus1].name.assign(g_name_leveling);
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
    d->layers[d->n_minus2].name.assign(g_name_elevation);
    d->layers[d->n_minus1].name.assign(g_name_leveling);
    d->n = 0;
    WL_MARKER(13)
    if (d->dims.w > 0x1a4 || d->dims.h > 0x1a4)
        mb_error("Illegal terrain size");
}

// @ 0x4658c0
void Terrain::parse(const tstring& s)
{
    int start = 0;
    bits[0] = 0; bits[1] = 0; bits[2] = 0; bits[3] = 0; bits[4] = 0; bits[5] = 0;
    int end = s.len;
    int count = 0;
    int lo;
    tstring t;
    int base = 0;
    if (s.buf[0] == '0' && (s.buf[1] == 'x' || s.buf[1] == 'X')) {
        base = 2;
        start = 2;
    }
    if (end > start) {
        do {
            if (count >= 6) break;
            lo = end - 8;
            int b = lo;
            if (lo < start) b = start;
            t = s.substr(b, end);
            int val = 0;
            int i = 0;
            if (0 < t.len) {
                do {
                    unsigned c = t.buf[i];
                    int d;
                    val = val * 16;
                    d = 0;
                    if (c >= '0' && c <= '9') d = c - '0';
                    else if (c >= 'a' && c <= 'f') d = c - 87;
                    else if (c >= 'A' && c <= 'F') d = c - 55;
                    val += d;
                    i++;
                } while (i < t.len);
            }
            bits[count] = val;
            count++;
            end = lo;
        } while (lo > start);
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

// @ 0x465af0
vector4 Terrain::sample(const float* p)
{
    int ij[2];
    cell_of(ij, p);
    int i = ij[0], j = ij[1];
    terrain_cell* C = &cell[0][0];
    vector4* q00 = (vector4*)&C[i * 251 + j];
    vector4* q10 = (vector4*)&C[(i + 1) * 251 + j];
    vector4* q01 = (vector4*)&C[i * 251 + (j + 1)];
    vector4* q11 = (vector4*)&C[(i + 1) * 251 + (j + 1)];
    vector4 c00 = *q00;
    vector4 c11 = *q11;
    vector4 c01 = *q01;
    vector4 c10 = *q10;
    vector4 a, b;
    // the vector4(x, y, z) temporaries leave w untouched: when a blended a / b leaves through the early
    // returns, the original returns whatever its stack held in that lane (an uninitialised slot; the harness models it as uninitialised-in-both)
    if (p[1] < c00.y) {
        a = *q00;
    } else if (p[1] > c01.y) {
        a = *q01;
    } else {
        float t = (p[1] - c00.y) / (c01.y - c00.y);
        float u = 1.0f - t;
        float sx = c00.x * u;
        float sy = c00.y * u;
        a = vector4(q01->x * t + sx, t * c01.y + sy, q01->z * t + u * q00->z);
    }
    if (p[1] < c10.y) {
        b = *q10;
    } else if (p[1] > c11.y) {
        b = *q11;
    } else {
        float t = (p[1] - c10.y) / (c11.y - c10.y);
        float u = 1.0f - t;
        float sx = c10.x * u;
        float sy = c10.y * u;
        b = vector4(q11->x * t + sx, t * c11.y + sy, q11->z * t + u * q10->z);
    }
    if (c00.x > p[0]) {
        return a;
    }
    if (c10.x < p[0]) {
        return b;
    }
    float t = (p[0] - c00.x) / (c10.x - c00.x);
    float u = 1.0f - t;
    float sx = a.x * u;
    float sy = a.y * u;
    vector4 r(b.x * t + sx, b.y * t + sy, b.z * t + u * a.z);
    r.w = q10->w;
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

static inline void t_cross3(vector4& r, const vector4& a, const vector4& b) { r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x; }

// @ 0x4661e0
void Terrain::smooth_heights()
{
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; this->w >= x; x++) {
            float sum = this->at(x, y).pz;
            float cnt = 1.0f;
            #define INR(a, b) ((a) >= 0 && (a) <= this->w && (b) >= 0 && (b) <= this->h)
            if (INR(x, y + 1))     { sum += this->at(x, y + 1).pz;     cnt += 1.0f; }
            if (INR(x + 1, y + 1)) { sum += this->at(x + 1, y + 1).pz; cnt += 1.0f; }
            if (INR(x + 1, y))     { sum += this->at(x + 1, y).pz;     cnt += 1.0f; }
            if (INR(x + 1, y - 1)) { sum += this->at(x + 1, y - 1).pz; cnt += 1.0f; }
            if (INR(x, y - 1))     { sum += this->at(x, y - 1).pz;     cnt += 1.0f; }
            if (INR(x - 1, y - 1)) { sum += this->at(x - 1, y - 1).pz; cnt += 1.0f; }
            if (INR(x - 1, y))     { sum += this->at(x - 1, y).pz;     cnt += 1.0f; }
            if (INR(x - 1, y + 1)) { sum += this->at(x - 1, y + 1).pz; cnt += 1.0f; }
            #undef INR
            float d = sum / cnt - this->at(x, y).pz;
            if (d < -10.0f) d = -10.0f;
            else if (d > 2.0f) d = 2.0f;
            float nh = ((float)(rand() % 0x3dc9) * 2.5289246e-05f + 0.2f) * d + this->at(x, y).pz;
            this->at(x, y).pz = nh;
            this->at(x, y).h2 = nh;
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
                terrain_quad& q = this->quad(x, y);
                if (!(v[0] == v[3] || v[2] == v[3])) goto set1;
                switch (r) {
                case -1: {
                    rgl::vector4 e1 = ((this->at(x + 1, y + 1).P()) - (this->at(x, y).P()));
                    rgl::vector4 e2 = ((this->at(x + 1, y).P()) - (this->at(x, y).P()));
                    rgl::vector4 e3 = ((this->at(x, y + 1).P()) - (this->at(x, y).P()));
                    rgl::vector4 cr;
                    t_cross3(cr, e2, e3);
                    if (e1.y * cr.y + e1.z * cr.z + e1.x * cr.x > 0.0f) q.diag = 0; else q.diag = 1;   // x is added last (0x46689a: the spilled cr component goes in after the other two)
                    break; }
                case 0:
                case 2:
                    q.diag = 0;
                    break;
                default:
                set1:
                    q.diag = 1;
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
    int ww = w;
    nx = ww / block;
    if (block / 2 < ww % block) nx++;
    int t = nx;
    if (t < 1) t = 1;
    else if (t >= 5) t = 4;
    nx = t;
    int hh = h;
    ny = hh / block;
    if (block / 2 < hh % block) ny++;
    t = ny;
    if (t < 1) t = 1;
    else if (t >= 5) t = 4;
    ny = t;
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            TEntity** slot = &ents[i * 5 + j];
            *slot = (TEntity*)world->create_entity(9);   // 0x428440
            int ch = block;
            int cw = ch;
            if (i == nx - 1) {
                cw = w;
                if (!(ch / 2 < cw % ch)) { cw = cw % block + block; } else { cw = cw % ch; }
            }
            if (j == ny - 1) {
                if (block / 2 < h % block) ch = h % ch;
                else ch = h % block + block;
            }
            int zero = 0;
            (*slot)->s6(&zero);
            terrain_cell& cell = cells[block * (i * 251 + j) + (cw / 2) * 251 + ch / 2];
            (*slot)->pos = *(rgl::vector4*)&cell;
            (*slot)->pos.z += 2.0f;
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
                        at(x, y).f88 = 0.0f;
                        at(x, y).pz = at(x, y).f88 + at(x, y).h2;
                    }
            } else {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++) {
                        at(x, y).f88 = *(float*)(y * 4 + x * 0x690 + set[idx * 6 + 8]);
                        at(x, y).pz = at(x, y).f88 + at(x, y).h2;
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
            tlayer* l = &layers[idx];
            int t = set[idx * 6 + 7];
            l->type = t;
            if (t >= 0) {
                GroundType* tt = &g_ground_types[t];
                l->f1c = (int)tt->mat;
                l->f24 = tt->f18;
                l->f14 = tt->f1c;
                l->f18 = -1;
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
void Prop::update(int unused)
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
