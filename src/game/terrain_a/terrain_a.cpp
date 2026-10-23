//-----------------------------------------------------------------------------
// File: terrain_a.cpp  (0x464200..0x46e590)
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
#include "./terrain.h"
#include "./game_state.h"
#include "./misc.h"

using rgl::vector4;
using rgl::matrix;

// 0x466cd0 (emit_particle) calls the cell lookup as a function with a void result; terrain.h declares that view as
// terrain_obj::cell_of, which is the function Terrain::cell_of (0x464b40) defined below.
#pragma comment(linker, "/alternatename:?cell_of@terrain_obj@@QAEXPAHPBM@Z=?cell_of@Terrain@@QAEPAHPAHPBM@Z")

static inline float fmin_(float a, float b) { return b < a ? b : a; }
static inline float fmax_(float a, float b) { return b > a ? b : a; }
#define IFLOOR(e) ((int)((e) > 0.0f ? (e) : (e) - 0.99999988f))

// ---- the sco terrain description ---------------------------------------------------------------------------

// @ 0x464200
void TerrainDescEntry::read(stream_base* s, Dims* d)
{
    s->r_int(&type);
    s->r_tstr((rgl::string*)&name);
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

// @ 0x4644e0
void terrain_cell::init(const float* q)
{
    P() = *(const rgl::vector4*)q;
    *(float*)&flags = 0.0f;
    weight = 1.0f;
    f14 = -1;
    f10 = -1;
    V30() = rgl::vector4(q[0] * 0.7f, q[1] * 0.7f, 0.0f);
    order = -1;
    v30.w = 1.0f;
    for (int i = 1; i < 15; i++)
        w[i] = 0.0f;
    f88 = 0.0f;
    w[0] = 1.0f;
    f90 = 1.0f;
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
    int* a = (int*)((char*)this + 0x2c);
    int* b = (int*)((char*)this + 0x845da54);
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
    BW(0x5c3b8f4) = 0;
    BW(0x843b8f8) = 0;
    BW(0xc3b0e4) = 0;
    BW(0x343b0e8) = 0;
    BW(0x343b4ec) = 0;
    BW(0x5c3b4f0) = 0;
    BW(0x845d97c) = 0;
    BW(0x845d980) = 0;
}

// ---- bit-field accessors ---------------------------------------------------------------------------------------

// @ 0x464960
void BitWords::set(const unsigned& lo, const int& hi, const unsigned& v)
{
    unsigned a = lo;
    int b = hi;
    unsigned mask = (unsigned)g_pow2[b - a] - 1;
    unsigned vv = v;
    if (vv > mask)
        vv = mask;
    unsigned sh = a & 0x1f;
    unsigned idx = a >> 5;
    unsigned old = w[idx];
    w[idx] = (~(mask << sh) & old) | (vv << sh);
}

// @ 0x4649b0
unsigned BitWords::get(const unsigned& lo, const int& hi)
{
    unsigned mask = (unsigned)g_pow2[hi - lo] - 1;
    unsigned sh = lo & 0x1f;
    return ((w[lo >> 5] & (mask << sh)) >> sh) & mask;
}

// @ 0x4649f0
void Pct::set_pct(const float& f)
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
    unsigned mask = g_field_mask - 1;
    if ((unsigned)i > mask)
        i = mask;
    bits = (~mask & bits) | i;
}

// ---- the ground generator -----------------------------------------------------------------------------------------

// @ 0x464a60
void Terrain::set_field_a(const float* p)
{
    int i = IFLOOR(*p);
    unsigned v = i < 0 ? 0 : (i >= 128 ? 127 : i);
    unsigned m = g_field_mask - 1;
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
    unsigned m = g_field_mask - 1;
    if (v > m) v = m;
    flags = (flags & ~(m << 21)) | (v << 21);
}

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
    int m = *mode;
    int t;
    if (m == 0 && (g->flags & 0x400000) == 0)
        return 0.0f;
    if (m == 1) {
        __int64 f = g->flags;
        if ((f & 0x400000) != 0 || (f & 0x80000) != 0)
            return 0.0f;
    }
    if ((m != 2 || (g->flags & 0x80000) != 0) &&
        (t = *type, t != 2 || (g->flags & 0x8) != 0) &&
        (t != 10 || (g->flags & 0x800) != 0) &&
        (t != 4 || (g->flags & 0x10) != 0) &&
        (t != 12 || (g->flags & 0x1000) != 0) &&
        (t != 5 || (g->flags & 0x20) != 0) &&
        (t != 13 || (g->flags & 0x2000) != 0) &&
        (t != 3 || (g->flags & 0x4) != 0) &&
        (t != 11 || (g->flags & 0x400) != 0) &&
        ((t & 7) != 7 || (g->flags & 0x4) != 0))
        return 1.0f;
    return 0.0f;
}

// @ 0x464d50
vector4* Terrain::ground_color(vector4* out, const float* pos)
{
    unsigned t = flags >> 28 & g_type_mask - 1U;
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
        layer[0].fc = g_ground_types[0].f10;
        layer[0].f14 = g_ground_types[0].f18;
        layer[0].f4 = g_ground_types[0].f1c;
        layer[0].f8 = -1;
        break;
    case 1:
        layer[0].type = 1;
        layer[0].fc = g_ground_types[1].f10;
        layer[0].f14 = g_ground_types[1].f18;
        layer[0].f4 = g_ground_types[1].f1c;
        layer[0].f8 = -1;
        break;
    }
    rand();
    int a = -1, b = 2, c = 5;
    switch (flags >> 28 & g_type_mask - 1U) {
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
        layer[1].fc = gt->f10;
        layer[1].f14 = gt->f18;
        layer[1].f4 = gt->f1c;
        layer[1].f8 = -1;
    }
    layer[2].type = b;
    if (b >= 0) {
        GroundType* gt = &g_ground_types[b];
        layer[2].fc = gt->f10;
        layer[2].f14 = gt->f18;
        layer[2].f4 = gt->f1c;
        layer[2].f8 = -1;
    }
    layer[3].type = 8;
    layer[3].fc = g_ground_types[8].f10;
    layer[3].f14 = g_ground_types[8].f18;
    layer[3].f4 = g_ground_types[8].f1c;
    layer[3].f8 = -1;
    layer[1].f10 = 0;
    if (a >= 0)
        layer[1].f10 = g_ground_types[a].f14;
}

// @ 0x4650f0
void Terrain::scatter(particle_list* set, int mode, int n)
{
    set->nsrc = 0;
    int cnt = g_ptype_count;
    ptype* specs = g_ptypes;
    unsigned t = flags >> 28 & g_type_mask - 1U;
    float total = 0.0f;
    int fits = 0;
    int idx = 0;
    if (0 < cnt) {
        do {
            if ((specs[idx].flags & 0x1000000) != 0) {
                int tt = t;
                if (spec_weight(&idx, &tt, &mode) > 0.1f)
                    fits++;
            }
            idx++;
        } while (idx < cnt);
    }
    idx = 0;
    int tt = t;
    if (0 < cnt) {
        do {
            float w_ = spec_weight(&idx, &tt, &mode);
            if (w_ < 0.0f) w_ = 0.0f;
            else if (w_ > 1.0f) w_ = 1.0f;
            idx++;
            total = w_ + total;
        } while (idx < cnt);
    }
    int pick_fit = 0;
    if (fits != 0) pick_fit = 1;
    if (0 < n) {
        int left = n;
        do {
            if (0.0f < total) {
                int pick;
                if (pick_fit) {
                    int r;
                    if (fits < 2) r = 0;
                    else {
                        r = rand() % fits;
                        cnt = g_ptype_count;
                        specs = g_ptypes;
                    }
                    float target = (float)r;
                    int seen = 0;
                    idx = 0;
                    if (0.0f <= target) {
                        do {
                            if (cnt <= idx) break;
                            if ((specs[idx].flags & 0x1000000) != 0) {
                                int t2 = t;
                                if (spec_weight(&idx, &t2, &mode) > 0.1f)
                                    seen++;
                            }
                            idx++;
                        } while ((float)seen <= target);
                    }
                    pick_fit = 0;
                    pick = idx;
                } else {
                    int r = rand() % 15817;
                    cnt = g_ptype_count;
                    idx = 0;
                    float acc = 0.0f;
                    float target = (float)r * total * 6.3223117e-05f;
                    int j = 0;
                    if (0.0f < target) {
                        do {
                            if (cnt <= j) break;
                            int t2 = t;
                            float w_ = spec_weight(&j, &t2, &mode);
                            j++;
                            acc = w_ + acc;
                        } while (acc < target);
                    }
                    pick = j;
                }
                int sel = pick - 1;
                sel = ((sel < 1) - 1) & sel;
                scatter_src* it = &set->src[set->nsrc];
                it->kind = sel;
                float rx = (float)(rand() % 15817) * 0.63223112f;
                float ry = (float)(rand() % 15817) * 0.63223112f;
                float rz = (float)(rand() % 15817) * 0.63223112f;
                it->v = vector4(rz, ry, rx);
                it->freq = (float)(rand() % 15817) * 0.0031611556f + 50.0f;
                it->thresh = (float)(rand() % 15817) * 3.1611557e-06f;
                int r5 = rand() % 15817;
                ptype* gs = &g_ptypes[sel];
                int dens = (unsigned short)(((unsigned*)&gs->flags)[1]);
                it->rate = ((float)r5 * 6.3223117e-05f + 1.0f) * ((it->thresh + 0.4f) * ((3.0f / ((float)n + 2.0f)) * (float)dens)) * 0.015f;
                if (gs->flags & 0x80000) {
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
    unsigned t = flags >> 28 & g_type_mask - 1U;
    int s2 = 0;
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
    if (g_terrain_gate < 10)
        s2 = 0;
    scatter(&list_a, 1, c);
    scatter(&list_c, 2, s2);
    scatter(&list_b, 0, b);
}

// @ 0x4656c0
void Terrain::fill_desc_named(TerrainDesc* d)
{
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
    if (d->dims.w > 0x1a4 || d->dims.h > 0x1a4)
        mb_error("Illegal terrain size");
}

// @ 0x4657c0
void Terrain::fill_desc_none(TerrainDesc* d)
{
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
    if (d->dims.w > 0x1a4 || d->dims.h > 0x1a4)
        mb_error("Illegal terrain size");
}

// @ 0x4658c0
void HexWords::parse(const tstring& s)
{
    int start = 0;
    v[0] = 0; v[1] = 0; v[2] = 0; v[3] = 0; v[4] = 0; v[5] = 0;
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
            v[count] = val;
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
    vector4 c10 = *q10;
    vector4 c01 = *q01;
    vector4 c11 = *q11;
    vector4 a, b;
    // the vector4(x, y, z) temporaries leave w untouched: when a blended a / b leaves through the early
    // returns, the original returns whatever its stack held in that lane (an uninitialised slot)
    if (p[1] < c00.y) {
        a = *q00;
    } else if (c01.y < p[1]) {
        a = *q01;
    } else {
        float t = (p[1] - c00.y) / (c01.y - c00.y);
        float u = 1.0f - t;
        volatile float sx = c00.x * u;
        volatile float sy = c00.y * u;
        a = vector4(q01->x * t + sx, t * c01.y + sy, q01->z * t + u * q00->z);
    }
    if (p[1] < c10.y) {
        b = *q10;
    } else if (c11.y < p[1]) {
        b = *q11;
    } else {
        float t = (p[1] - c10.y) / (c11.y - c10.y);
        float u = 1.0f - t;
        volatile float sx = c10.x * u;
        volatile float sy = c10.y * u;
        b = vector4(q11->x * t + sx, t * c11.y + sy, q11->z * t + u * q10->z);
    }
    if (p[0] < c00.x) {
        return a;
    }
    if (c10.x < p[0]) {
        return b;
    }
    float t = (p[0] - c00.x) / (c10.x - c00.x);
    float u = 1.0f - t;
    volatile float sx = a.x * u;
    volatile float sy = a.y * u;
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
                    if (y >= 0 && y <= h && (cell[x - 1][y].flags & m) != 0) {
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

static inline vector4 t_sub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline void t_cross3(vector4& r, const vector4& a, const vector4& b) { r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x; }
static inline float t_dot(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// @ 0x4661e0
void terrain_obj::smooth_heights()
{
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
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
void terrain_obj::clear_flags(unsigned mask)
{
    unsigned inv = ~mask;
    for (int y = 0; y <= this->h; y++)
        for (int x = 0; x <= this->w; x++)
            this->at(x, y).flags &= inv;
}

// @ 0x4664e0
void terrain_obj::compute_weights()
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
void terrain_obj::pick_diagonals()
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
                    rgl::vector4 e1 = t_sub(this->at(x + 1, y + 1).P(), this->at(x, y).P());
                    rgl::vector4 e2 = t_sub(this->at(x + 1, y).P(), this->at(x, y).P());
                    rgl::vector4 e3 = t_sub(this->at(x, y + 1).P(), this->at(x, y).P());
                    rgl::vector4 cr;
                    t_cross3(cr, e2, e3);
                    if (t_dot(e1, cr) > 0.0f) q.diag = 0; else q.diag = 1;
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
void terrain_obj::pick_triangle_types()
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
void terrain_obj::accumulate_kinds()
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
void terrain_obj::erode()
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
void terrain_obj::emit_particle(particle_list* l, int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim)
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
void terrain_obj::emit_by_type(int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim)
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
void terrain_obj::triangle_corners(int x, int y, int flip, int* a, int* b, int* c)
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
void terrain_obj::sun_light(rgl::vector4* out, const float* dir, const float* scale)
{
    rgl::vector4 amb = sun_ambient;
    float t = -(sun_dir[2] * dir[2] + sun_dir[1] * dir[1] + sun_dir[0] * dir[0]);
    if (0.0f > t) t = 0.0f;
    t = t * scale[0];
    rgl::vector4 d(t * sun_diffuse[0], t * sun_diffuse[1], t * sun_diffuse[2]);
    rgl::vector4 r;                 // r.w is never written: the original copies the stack slot as is
    r.x = amb.x + d.x;
    r.y = amb.y + d.y;
    r.z = amb.z + d.z;
    *out = r;
}

// @ 0x466fa0
void terrain_obj::create_chunk_entities()
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
            *slot = world->create_entity(9);
            int ch = block;
            int cw = ch;
            if (i == nx - 1) {
                cw = w;
                if (ch / 2 < cw % ch) cw = cw % ch;
                else cw = cw % block + block;
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
void terrain_obj::load_layer(int* set, int idx)
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
                l->f1c = tt->f10;
                l->f24 = tt->f18;
                l->f14 = tt->f1c;
                l->f18 = -1;
            }
        }
    }
}

// @ 0x467530
int terrain_obj::count_low_quads()
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
void Mission3::clear_done()
{
    for (int i = 0; i < 64; i++) done[i] = 0;
}

// @ 0x4676e0
void Mission3::copy_slot(int i)
{
    char* base = (char*)this + i * 0x130;       // slot i of the 0x130-byte records at +0x6b0 (cl keeps the +0x6b0 separate)
    *(m64*)(base + 0x6b0) = *(m64*)(base + 0x6f0);
    *(int*)(base + 0x774) = 0;
    *(int*)(base + 0x770) = 0;
    *(m64*)(base + 0x730) = *(m64*)(base + 0x6f0);
    *(int*)(base + 0x798) = 0;
}

// @ 0x467740
float Mission3::speed_factor(float x)
{
    float base = g_speed_base * 2.5f + 0.833333f;
    float t = (x + 15.0f) * 0.06666667f;
    return base * (t < 0.2f ? 0.2f : (t >= 2.5f ? 2.5f : t));
}

// @ 0x4677a0
void Mission3::add_amount(int idx, unsigned flags)
{
    unsigned mask = flags & 0xfffffff;
    char* rec = g_modes2[mode].recs + idx * 0x4c;
    if (mask == 0) return;
    unsigned rf = *(unsigned*)(rec + 4);
    unsigned amount;
    if ((rf & 4) || (rf & 0x2000000)) {
        amount = mask;
    } else if (rf & 8) {
        amount = 1;
    } else if (rf & 0x10) {
        amount = g_world3->tbl[0xc5 + f5df4 * 0x125 + idx];
    } else {
        int v;
        if ((rf & 1) || ((rf & 0x40) && f6214 == 1) || ((rf & 0x80) && f6214 == 0)
            || (f6214 == 2 && (rf & 0x40) && f6218 == 0) || (f6214 == 2 && (rf & 0x80) && f6218 == 1))
            v = -f6520;
        else
            v = f6520;
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
bool __stdcall rule_accepts(RuleRec* a, CandRec* b, int p3, char p4, char p5, char p6)
{
    bool ok = true;
    if (b->f8 & 1) ok = false;
    unsigned fl = a->flags;
    if ((fl & 0x400) && (g_world3->troops[b->troop].flags30 & 0x10) == 0)
        ok = false;
    if ((fl & 0x200) && p3 != 0 && (g_world3->troops[b->troop].flags30 & 0x10) != 0)
        ok = false;
    if ((fl & 0x100) && p3 == 0 && (g_world3->troops[b->troop].flags30 & 0x10) != 0)
        return false;
    if (ok && (fl & 0x70000)) {
        Troop* t = &g_world3->troops[b->troop];
        __int64 tf = t->flags30;
        bool x3 = false, x5 = false, x4 = false;
        if ((tf & 0x1000000) && (a->f8 & 0x100) == 0) {
            x5 = true;
        } else if ((tf & 0x4000000)) {
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

static inline vector4 p_scale(const vector4& v, float f) { return vector4(v.x * f, v.y * f, v.z * f); }
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
            m.o = p_add(p_scale(to.o, t), p_scale(from.o, 1.0f - t));
            m.s = p_add(p_scale(to.s, t), p_scale(from.s, 1.0f - t));
            m.f = p_add(p_scale(to.f, t), p_scale(from.f, 1.0f - t));
            m.u = p_add(p_scale(to.u, t), p_scale(from.u, 1.0f - t));
            m.orthonormalize();
            cur = m;
            if (ent) ent->set_frame(&cur, extra);
        }
        last_t = t;
    }
}

// ---- missile -----------------------------------------------------------------------------------------------------------

// the item record's field at +0x20c (the shared ItemKind has no name for it)
static inline int item_f20c(int i) { return *(int*)((char*)&g_items[i] + 0x20c); }
// @ 0x467f80
float Missile::ammo_scale()
{
    if (ammo_item >= 0)
        return (float)item_f20c(ammo_item) * 0.01f;
    return 0.8f;
}

// @ 0x467fb0
void Missile::init()
{
    f54 = 0.01f;
    f68 = 0;
    t0 = g_ticks[clock];
    f90 = 0;
    fa0 = 0;
    f94 = 1.0f;
    fa4 = -1;
    f9c = 1;
    vector4 d(g_mission->cam[0] - p20.x, g_mission->cam[1] - p20.y, g_mission->cam[2] - p20.z);
    if (0.1f < d.x * p40.x + (d.z * p40.z + d.y * p40.y))
        fa0 = 1;
}

// @ 0x468050
elem56* __cdecl copy56(elem56* f, elem56* l, elem56* d) { for (; f != l; ++f, ++d) *d = *f; return d; }

// @ 0x4680c0
elem56* __cdecl copy_backward56(elem56* f, elem56* l, elem56* d) { while (f != l) *--d = *--l; return d; }

// ---- allocator::construct; _Uninit_copy ----------------------------------------------------------------------------------

// @ 0x468130
void alloc24_def::construct(elem24* p, const elem24& v) { new ((void*)p) elem24(v); }

// @ 0x468170
void alloc100_def::construct(particle100* p, const particle100& v) { new ((void*)p) particle100(v); }

// @ 0x468190
void alloc52_def::construct(elem52* p, const elem52& v) { new ((void*)p) elem52(v); }

// @ 0x4681b0
elem24* __cdecl uninit_copy24(elem24* f, elem24* l, elem24* d, alloc24& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    elem24* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}

// @ 0x468220
particle100* __cdecl uninit_copy100(particle100* f, particle100* l, particle100* d, alloc100& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    particle100* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}

// @ 0x468290
elem52* __cdecl uninit_copy52(elem52* f, elem52* l, elem52* d, alloc52& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    elem52* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}

// ---- component-wise max / min of two 3-vectors -------------------------------------------------------------------------

static inline float vm_max(float x, float y) { return x > y ? x : y; }
static inline float vm_min(float x, float y) { return x < y ? x : y; }
// @ 0x468300
void __cdecl vmax3(float* a, const float* b)
{
    a[0] = vm_max(a[0], b[0]);
    a[1] = vm_max(a[1], b[1]);
    a[2] = vm_max(a[2], b[2]);
}

// @ 0x468360
void __cdecl vmin3(float* a, const float* b)
{
    a[0] = vm_min(a[0], b[0]);
    a[1] = vm_min(a[1], b[1]);
    a[2] = vm_min(a[2], b[2]);
}

// ---- material texture stages ----------------------------------------------------------------------------------------------

// @ 0x4683c0
void texture_x::bind(int stage, float t)
{
    if (loaded) {
        int idx = 0;
        if (flags & 0x0f000000)
            idx = (int)(owns_texture * t) % owns_texture;
        IDirect3DBaseTexture9* tex = ((IDirect3DBaseTexture9**)d3d_tex)[idx];
        if (g_tex_cache[stage] != tex) {
            g_device->SetTexture(stage, tex);
            g_tex_cache[stage] = tex;
        }
        g_render.set_stage_sampler(stage, ((int)flags >> 20) & 3);
    }
}

// @ 0x468440
void material_x::bind(texture** textures, const float* t)
{
    int n = nstages;
    int last = -1;
    int k = 0;
    if (0 < n) {
        stage_t* s = stages;
        for (; k < n; ++k, ++s) {
            last = k;
            if (s->a != -1 && textures[s->a])
                ((texture_x*)textures[s->a])->bind(k, *t);
            g_render.set_stage_color(k, s->b);
            g_render.set_stage_alpha(k, s->c);
        }
    }
    g_render.set_stage_color(last + 1, 1);
    g_render.set_stage_alpha(last + 1, 1);
}

// @ 0x4684d0
void skin_x::apply_state(const float* t)
{
    unsigned char a = ~(flags >> 4) & 1;
    if (a != g_rs_zfunc_le) {
        g_rs_zfunc_le = a;
        if (a)
            g_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        else
            g_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    }
    unsigned char b = (flags >> 3) & 1;
    if (b != g_rs_zwrite_off) {
        BOOL on = b == 0;
        g_rs_zwrite_off = b;
        g_device->SetRenderState(D3DRS_ZWRITEENABLE, on);
    }
    g_render.set_alpha_mode((flags >> 12) & 3);
    unsigned char c = (flags >> 11) & 1;
    if (c != g_rs_normalize) {
        g_device->SetRenderState(D3DRS_NORMALIZENORMALS, c);
        g_rs_normalize = c;
    }
    unsigned char d = (shader->flags20 >> 5) & 1;
    if (d != g_rs_specular) {
        g_device->SetRenderState(D3DRS_SPECULARENABLE, d);
        g_rs_specular = d;
    }
    if (shader->flags20 & 0x20) {
        vector4 col;
        col.x = color.x * 0.5f;
        col.y = color.y * 0.5f;
        col.z = color.z * 0.5f;
        g_render.set_material(f44 + f44, &col.x);
    }
    ((material_x*)shader)->bind(tex, t);
}

// @ 0x468620
int mesh_x::count(int kind)
{
    if (kind == 1) {
        std::vector<rgl::face20>& f = local_a0 ? v90 : ((mesh_x*)parent)->get90_();
        return f.size() * 3;
    }
    std::vector<rgl::vertex60>& v = local_a1 ? v50 : ((mesh_x*)parent)->get50_();
    return v.size();
}

// ---- effect slot record ------------------------------------------------------------------------------------------------------

// @ 0x4686a0
void EnvRec::reset()
{
    f3c = -1;
    f8 = -1;
    fb4 = 1;
    f0 = 0;
    f30 = 0;
    fa4 = 10;
    f9c = 13;
    fa8 = 20;
    fb8 = 0;
    f34 = 1.3f;
    f38 = 4.3f;
    f4c = 0;
    k0 = 0;
    k1 = 0;
    f54 = 0;
    f58 = 0;
    f40 = 1.0f;
    fbc = 1.0f;
    fa0 = 1.0f;
    fc = -1;
    f50 = 1;
    t0 = g_ticks[clock];
    fac = 0;
    fb0 = 0;
    q.zero();
    f5c = 0;
    q.x = 1.0f;
    k2 = 0;
}

// @ 0x468770
int Rec468770::level_index()
{
    int base = f58;
    int v = b7a & 0xf;
    if (v < 0)
        v = 0;
    else if (v > g_lod_max)
        v = g_lod_max;
    return base + v * 2 + 0x23;
}

// ---- troop auto-equip (built on Troop::add and Troop::equip_best, src/game/ui/troop_inv.cpp) ------------------------------------

// @ 0x4687a0
void TroopEq::unequip_all()
{
    ItemRef* p = equip;
    int n = 10;
    do {
        if (p->item >= 0) {
            if (add(p))
                p->item = -1;
        }
        ++p;
        --n;
    } while (n != 0);
}

// @ 0x4687e0
void TroopEq::auto_equip(int seed, int unused, char civilian, char no_helmet)
{
    if ((flags30 & tf_inactive) == 0) {
        bool b7 = false, b8 = false, b1 = false, b2 = false, b5 = false, b6 = false, b3 = false, b4 = false, b9 = false, b10 = false;
        unsigned mustnot_ = 0;
        unequip_all();
        unsigned must_ = 0;
        if ((flags30 & tf_hero) && civilian)
            must_ = itp_civilian;
        if ((flags30 & tf_hero) && no_helmet)
            mustnot_ = 0x80000000;
        equip_best(seed, ek_head, must_, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_helmet) != 0);
        equip_best(seed, ek_body, must_, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_armor) != 0);
        equip_best(seed, ek_foot, must_, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_boots) != 0);
        equip_best(seed, ek_gloves, must_, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_gloves) != 0);
        equip_best(seed, ek_horse, 0, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_horse) != 0);
        for (int i = 0; i < 4; ++i) {
            char r = 0;
            if (i == 0) {
                r = equip_best(seed, 0, 0, mustnot_, itp_type_pistol, itp_type_musket, -1, (flags30 & tf_guarantee_ranged) != 0);
                if (!r)
                    r = equip_best(seed, 0, 0, mustnot_, itp_type_bow, itp_type_crossbow, itp_type_thrown, (flags30 & tf_guarantee_ranged) != 0);
                if (r) {
                    unsigned t = g_items[equip[0].item].type_flags & 0xff;
                    if (t == itp_type_bow) b1 = true;
                    else if (t == itp_type_crossbow) b3 = true;
                    else if (t == itp_type_musket || t == itp_type_pistol) b5 = true;
                }
                if (r) continue;
            }
            if (b1 && !b2) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_arrows, -1, -1, 1);
                if (r) b2 = true;
            }
            if (r) continue;
            if (b3 && !b4) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_bolts, -1, -1, 1);
                if (r) b4 = true;
            }
            if (r) continue;
            if (b5 && !b6) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_bullets, -1, -1, 1);
                if (r) b6 = true;
            }
            if (r) continue;
            if (b7 && !b8) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_shield, -1, -1, (flags30 & tf_guarantee_shield) != 0);
                if (r) b8 = true;
            }
            if (r) continue;
            if (!b9) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_one_handed_wpn, itp_type_two_handed_wpn, -1, 0);
                if (r) {
                    b9 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
            if (r) continue;
            if (!b10) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_polearm, -1, -1, 0);
                if (r) {
                    b10 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
            if (r) continue;
            if (!b9 && !b10) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_one_handed_wpn, itp_type_two_handed_wpn, -1, 1);
                if (r) {
                    b9 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                } else {
                    r = equip_best(seed, i, 0, mustnot_, itp_type_polearm, -1, -1, 1);
                    if (!r) continue;
                    b10 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
        }
    }
}

// ---- shadow-map bounds and depth test ------------------------------------------------------------------------------------------

// @ 0x468c20
void ShadowMap::extend(const vector4* p)
{
    vector4 local = cam->world.transform_point_to_local(*p);
    vmin3(&cam->vmin.x, &local.x);
    vmax3(&cam->vmax.x, &local.x);
}

// @ 0x468c80
bool ShadowMap::test(unsigned p, float* out)
{
    float* o = out;
    *o = 1.0f;
    bool result = true;
    if (cam) {
        vector4 pt;
        ((ProjView*)&cam->fr)->project(&pt.x, p);
        if (!(0.0f <= pt.x)) return true;
        if (!(pt.x < 1.0f)) return true;
        if (!(0.0f <= pt.y)) return true;
        if (!(pt.y < 1.0f)) return true;
        float scr[2];
        scr[0] = (float)w2 * pt.x;
        scr[1] = (1.0f - pt.y) * (float)h2;
        if (g_use_shaders && surf) {
            // the depth reader overwrites the two argument slots (&out receives the stored depth)
            read_pixel((int)g_device, (int)scr[0], (int)scr[1], (unsigned*)&out, (unsigned*)&p);
            if ((pt.z - 0.001f) > *(float*)&out)
                *o = 0.0f;
        } else {
            result = false;
        }
    } else {
        result = false;
    }
    return result;
}

// ---- terrain: brush, path walk, flow spreading, ambient particles, normals -------------------------------------------------

// @ 0x468da0
void Terrain::apply_brush(const float* center, float radius, float strength)
{
    float fv = (float)((g_field_mask - 1) & (flags >> 7));
    int lo[2], hi[2];
    const int* lim = &w;
    for (int k = 0; k < 2; ++k) {
        float a = (float)ceil((center[k] - radius) / scale);
        float fl = (float)floor(a);
        int i0 = (int)fl;
        int r = i0;
        if (!(a - fl <= 0.5f)) r = i0 + 1;
        lo[k] = r > 0 ? r : 0;
        float b = (float)floor((center[k] + radius) / scale);
        float fl2 = (float)floor(b);
        int i1 = (int)fl2;
        int r2 = i1;
        if (!(b - fl2 <= 0.5f)) r2 = i1 + 1;
        int h = lim[k];
        if (r2 < h) h = r2;
        hi[k] = h;
    }
    for (int j = lo[1]; j <= hi[1]; ++j) {
        for (int i = lo[0]; i <= hi[0]; ++i) {
            terrain_cell& c = cells[i * 251 + j];
            float dx = (float)i * scale - center[0];
            float dy = (float)j * scale - center[1];
            float d = (1.0f / radius) * (float)sqrt(dy * dy + dx * dx);
            if (d < 1.0f) {
                float v = ((((float)cos(d * 3.1415927f) + 1.0f) * strength) * 0.5f * c.weight * fv) * 0.01f + c.pz;
                c.pz = v;
                c.h2 = v;
            }
        }
    }
}

static __forceinline void vnorm(vec2& v)
{
    float l = (float)sqrt(v.y * v.y + v.x * v.x);
    if (l > 0.0f) {
        l = 1.0f / l;
        v.x = l * v.x;
        v.y = l * v.y;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
    }
}

// @ 0x468fc0
void terrain_obj::trace_path(const int* from, const int* to, unsigned char bit, int* steps, float* minz, float w1, float w2, int unused, char record)
{
    *minz = 10000.0f;
    ivec2 pos = *(const ivec2*)from;
    *steps = 0;
    vec2 g;
    float gx = (float)(to[0] - pos.dx);
    float gy = (float)(to[1] - pos.dy);
    float gl = (float)sqrt(gx * gx + gy * gy);
    if (gl > 0.0f) {
        gl = 1.0f / gl;
        g.x = gl * gx;
        g.y = gl * gy;
    } else {
        g.x = 0.0f;
        g.y = 1.0f;
    }
    vec2 nd[8];
    for (int k = 0; k < 8; k++) {
        nd[k].x = (float)g_nbr[k].dx;
        nd[k].y = (float)g_nbr[k].dy;
        vnorm(nd[k]);
    }
    unsigned bitmask = 1 << bit;
    terrain_cell* cur = &at(pos.dx, pos.dy);
    cur->flags |= bitmask;
    if (record) {
        cur->dir = g;
        vec2 t;
        t.x = 0.0f;
        t.y = (float)*steps;
        cur->disp = t;
    }
    bool blocked = false;
    for (;;) {
        unsigned fl = flags;
        float tx = (float)(to[0] - pos.dx);
        int w = this->w;
        int h = this->h;
        float ty = (float)(to[1] - pos.dy);
        float sc[8];
        for (int k = 0; k < 8; k++) {
            int nx = g_nbr[k].dx + pos.dx;
            int ny = g_nbr[k].dy + pos.dy;
            sc[k] = 0.0f;
            float tl = (float)sqrt(tx * tx + ty * ty);
            float ux, uy;
            if (tl > 0.0f) {
                tl = 1.0f / tl;
                ux = tl * tx;
                uy = tl * ty;
            } else {
                ux = 0.0f;
                uy = 1.0f;
            }
            int a = ny - h + 20;
            int b = nx - w + 20;
            int pen = (a < 0 ? 0 : a) + (20 - ny < 0 ? 0 : 20 - ny) + (b < 0 ? 0 : b) + (20 - nx < 0 ? 0 : 20 - nx);
            float edge = 0.0f;
            if (pen > 0 &&
                ((nx < 21 && nd[k].x <= 0.0f) || (ny < 21 && nd[k].y <= 0.0f) ||
                 (w - 20 <= nx && 0.0f <= nd[k].x) || (h - 20 <= ny && 0.0f <= nd[k].y))) {
                edge = (float)pen;
                if (edge < 0.0f) edge = 0.0f;
                else if (24.0f <= edge) edge = 24.0f;
                edge = edge * -0.04f;
            }
            if (nx < 0 || w < nx || ny < 0 || h < ny) {
                sc[k] = ((g.x * nd[k].x + g.y * nd[k].y) + 1.0f) * 0.5f;
            } else {
                terrain_cell* n = &at(nx, ny);
                if ((bitmask & n->flags) == 0) {
                    float dz = ((cur->pz - n->pz) * w2) * 3.0f;
                    if (dz < 0.0f)
                        dz = (0.25f - (float)((fl >> 7) & (g_field_mask - 1)) * 0.0022f) * dz;
                    sc[k] = (((((uy * nd[k].y + ux * nd[k].x) + 0.7f) * 0.5f) * 0.65f + dz) +
                             (((g.y * nd[k].y + g.x * nd[k].x) + 0.7f) * 0.5f) * w1) + edge;
                } else {
                    if (n->order < *steps - 30)
                        blocked = true;
                    sc[k] = -1.0f;
                }
            }
        }
#define CUBE(i) if (0.0f < sc[i]) sc[i] = (sc[i] * sc[i]) * sc[i];
        CUBE(0) CUBE(1) CUBE(2) CUBE(3) CUBE(4) CUBE(5) CUBE(6) CUBE(7)
#undef CUBE
        float total = 0.0f;
#define ADD0(i) if (0.0f < sc[i]) total = sc[i];
#define ADD(i) if (0.0f < sc[i]) total = sc[i] + total;
        ADD0(0) ADD(1) ADD(2) ADD(3) ADD(4) ADD(5) ADD(6) ADD(7)
#undef ADD
#undef ADD0
        int r = rand();
        int pick = -1;
        float roll = ((float)(r % 0x3dc9) * total) * 6.322312e-05f;
        float acc = 0.0f;
        if (0.0f < sc[0]) {
            acc = sc[0];
            if (0.0f <= roll && roll < sc[0]) pick = 0;
        }
#define SEL(i) if (0.0f < sc[i]) { if (acc <= roll && roll < sc[i] + acc) pick = i; acc = acc + sc[i]; }
        SEL(1) SEL(2) SEL(3) SEL(4) SEL(5) SEL(6)
#undef SEL
        if (0.0f < sc[7] && acc <= roll && roll < sc[7] + acc)
            pick = 7;
        else if (pick == -1)
            return;
        pos.dx = g_nbr[pick].dx + pos.dx;
        if (pos.dx < 3) return;
        if (this->w - 3 < pos.dx) return;
        pos.dy = g_nbr[pick].dy + pos.dy;
        if (pos.dy < 3) return;
        if (this->h - 3 < pos.dy) return;
        if (sc[pick] < 0.0f) return;
        float mx = g.x * 0.9f + nd[pick].x * 0.1f;
        float my = g.y * 0.9f + nd[pick].y * 0.1f;
        g.y = sqrtf(mx * mx + my * my);
        if (g.y > 0.0f) {
            g.y = 1.0f / g.y;
            g.x = mx * g.y;
            g.y = my * g.y;
        } else {
            g.x = 0.0f;
            g.y = 1.0f;
        }
        (*steps)++;
        cur = &at(pos.dx, pos.dy);
        cur->flags |= bitmask;
        cur->order = *steps;
        if (record) {
            cur->dir = g;
            vec2 t;
            t.x = 0.0f;
            t.y = (float)*steps * 0.7f;
            cur->disp = t;
        }
        if (cur->pz < *minz)
            *minz = cur->pz;
        if (*steps == 100000)
            return;
        if (blocked)
            return;
    }
}

static const int kStride = 251;

// @ 0x469a70
void terrain_obj::spread_flow(unsigned mask, int mode, unsigned mark)
{
    int order[9];
    unsigned n;
    if (mode == 1) {
        n = 4;
        order[0] = 2; order[1] = 4; order[2] = 5; order[3] = 6;
    } else {
        n = 8;
        for (int i = 0; i < 8; i++) order[i] = i;
    }
    static int seen_dummy;
    int seen[kStride * kStride];
    for (int y = 0; y <= this->h; y++)
        for (int x = 0; x <= this->w; x++)
            seen[x * kStride + y] = 0;
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            terrain_cell& c = at(x, y);
            int cnt = 0;
            vec2 dsum, rsum;
            dsum.x = 0.0f; dsum.y = 0.0f; rsum.x = 0.0f; rsum.y = 0.0f;
            for (unsigned k = 0; k < n; k++) {
                int nx = g_nbr[order[k]].dx + x;
                int ny = g_nbr[order[k]].dy + y;
                if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h) {
                    terrain_cell& d = at(nx, ny);
                    if ((mask & d.flags) != 0) {
                        cnt++;
                        float e[2], r[2], q[2];
                        e[0] = c.px - d.px; e[1] = c.py - d.py;
                        e[0] *= 0.3f; e[1] *= 0.3f;
                        r[0] = e[0] * d.dir.y - e[1] * d.dir.x;
                        r[1] = e[1] * d.dir.y + e[0] * d.dir.x;
                        q[0] = r[0] + d.disp.x;
                        q[1] = r[1] + d.disp.y;
                        dsum.x = q[0] + dsum.x;
                        dsum.y = q[1] + dsum.y;
                        rsum.x = rsum.x + d.dir.x;
                        rsum.y = rsum.y + d.dir.y;
                    }
                }
            }
            if (cnt > 0 || (mark != 0 && cnt != 0)) {
                seen[x * kStride + y] = 1;
                float inv = 1.0f / (float)cnt;
                vec2 nd1, nd2;
                nd1.x = dsum.x * inv; nd1.y = dsum.y * inv;
                c.disp = nd1;
                nd2.x = rsum.x * inv; nd2.y = rsum.y * inv;
                c.dir = nd2;
                float l = (float)sqrt(c.dir.y * c.dir.y + c.dir.x * c.dir.x);
                if (l > 0.0f) {
                    l = 1.0f / l;
                    c.dir.x = l * c.dir.x;
                    c.dir.y = l * c.dir.y;
                } else {
                    c.dir.x = 0.0f;
                    c.dir.y = 1.0f;
                }
            }
        }
    }
    if (mark != 0) {
        for (int y = 0; y <= this->h; y++)
            for (int x = 0; x <= this->w; x++)
                if (seen[x * kStride + y] != 0)
                    at(x, y).flags |= mark;
    }
}

#include "../../include/mat4.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../arr_set.h"
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../shadow_map.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../terrain_desc.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../view_params.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../view_state.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/renderer.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh_x.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
using rgl::mat3;
__declspec(noinline) mat3* identity3(mat3* out);                             // 0x40cd60
struct pos2 : rgl::matrix {
    void rotate_sf(const float* angle);     // 0x40e770
};
static __forceinline rgl::vector4 t_mul(const rgl::vector4& a, const float& s) { return rgl::vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline rgl::vector4 t_add(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline rgl::vector4 t_subf(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline rgl::vector4 t_cross(const rgl::vector4& a, const rgl::vector4& b)
{
    return rgl::vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static __forceinline void t_normalize(rgl::vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv; v.y = v.y * inv; v.z = v.z * inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
}
#define RND() ((float)(rand() % 0x3dc9) * 6.322312e-05f)

// @ 0x46a270
void terrain_obj::spawn_particles(particle_list* list, int unused)
{
    list->count = 0;
    unsigned lv = (flags >> 0x15) & (g_field_mask - 1);
    float lv_scale = (float)lv * 0.01f;
    unsigned sel = (flags14 >> 2) & (g_sel_mask - 1);
    int maxn = (int)(scale * scale * 0.8f);
    float k1 = (float)pow(scale, 1.7f) * 0.3f;
    float k2 = (float)g_terrain_gate * 0.02f;
    for (int y = 0; y < this->h; y++) {
        for (int x = 0; x < this->w; x++) {
            if (!(at(x, y).pz > -0.3f && at(x + 1, y).pz > -0.3f && at(x, y + 1).pz > -0.3f && at(x + 1, y + 1).pz > -0.3f))
                continue;
            terrain_quad& q = quad(x, y);
            for (int t = 0; t < 2; t++) {
                q.slot[t] = -1;
                if (list->nsrc != 0 && (q.tri[t] == 2 || q.tri[t] == 1) && list->count < 0x40000) {
                    float fx = (float)x;
                    float fy = (float)y;
                    for (int j = 0; j < list->nsrc; j++) {
                        scatter_src& s = list->src[j];
                        int kind = s.kind;
                        float p[3];
                        p[0] = fx + s.v.x;
                        p[1] = fy + s.v.y;
                        p[2] = s.v.z;
                        float nz[3];
                        noise_fractal(s.freq, 0.6f, 3, 0, p, nz);
                        float v = (nz[2] + nz[1]) + nz[0];
                        if (!(v < 0.5f)) v = 0.5f;
                        ptype* pt = &g_ptypes[kind];
                        float rate = ((v - s.thresh) * s.rate) * k1;
                        if ((pt->flags & 0x80000) == 0) {
                            rate = rate * lv_scale;
                        } else {
                            rate = rate * k2;
                            if (sel != 0) rate = 0.0f;
                        }
                        int n = 0;
                        if (RND() < rate) {
                            do {
                                if (maxn <= n) break;
                                if (!(RND() < q.f34) && (pt->flags & 0x80000) != 0) break;
                                rate = rate - 1.0f;
                                TerrainParticle* e = &list->items[list->count];
                                e->type = kind;
                                e->cx = x;
                                e->cy = y;
                                e->nmesh = 0;
                                e->f0c = t;
                                n++;
                                int a = 0;
                                if (1 < pt->nframes)
                                    a = rand() % pt->nframes;
                                e->anim = a;
                                mat3 id;
                                rgl::matrix m;
                                {
                                    mat3* ip = identity3(&id);
                                    memcpy(&m, ip, 0x30);
                                }
                                int cr[6];
                                int* ca = &cr[0]; int* cb = &cr[2]; int* cc = &cr[4];
                                triangle_corners(x, y, t, ca, cb, cc);
                                rand(); rand(); rand();
                                const rgl::vector4& PA = (const rgl::vector4&)at(ca[0], ca[1]);
                                const rgl::vector4& PB = (const rgl::vector4&)at(cb[0], cb[1]);
                                const rgl::vector4& PC = (const rgl::vector4&)at(cc[0], cc[1]);
                                m.o = t_add(t_add(t_mul(PA, 0.3333f), t_mul(PB, 0.3333f)), t_mul(PC, 0.3333f));
                                float r1 = RND();
                                rgl::vector4 d1 = t_mul(t_subf(PA, m.o), r1);
                                float r2 = RND();
                                rgl::vector4 d2 = t_mul(t_subf(PB, m.o), r2);
                                float r3 = RND();
                                rgl::vector4 d3 = t_mul(t_subf(PC, m.o), r3);
                                m.o = t_add(t_add(t_add(d2, d1), d3), m.o);
                                if ((pt->flags & 0x40000) != 0) {
                                    m.u = q.nrm[t];
                                    m.f = t_cross(m.u, m.s);
                                    t_normalize(m.f);
                                    m.s = t_cross(m.f, m.u);
                                }
                                float ang = (float)(rand() % 0x3dc9) * 0.00039724255f;
                                ((pos2*)&m)->rotate_sf(&ang);
                                e->frame = m;
                                float sc = (float)(rand() % 0x3dc9) * 4.425618e-05f + 0.65f;
                                e->dir = rgl::vector4(((float)(rand() % 0x3dc9) * 1.2644623e-05f + 0.9f) * sc,
                                                      ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 0.9f) * sc,
                                                      ((float)(rand() % 0x3dc9) * 1.8966935e-05f + 0.85f) * sc);
                                list->count++;
                            } while (RND() < rate);
                        }
                    }
                }
            }
        }
    }
}

vector4* tri_normal(vector4* out, const vector4* p);     // 0x4612d0 (cdecl, returns out)

// @ 0x46ad50
void terrain_obj::compute_normals()
{
    int x, y, flip;
    vector4 tri[3];
    int ab[6];
    vector4 n;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            for (flip = 0; flip < 2; flip++) {
                triangle_corners(x, y, flip, &ab[0], &ab[2], &ab[4]);
                tri[0] = at(ab[0], ab[1]).P();
                tri[1] = at(ab[2], ab[3]).P();
                tri[2] = at(ab[4], ab[5]).P();
                quad(x, y).nrm[flip] = *tri_normal(&n, tri);
            }
        }
    }
    for (y = 0; y <= h; y++) {
        n.x = 0.0f; n.y = 0.0f; n.z = 0.0f;
        for (x = 0; x <= w; x++)
            at(x, y).N() = n;
    }
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            for (flip = 0; flip < 2; flip++) {
                triangle_corners(x, y, flip, &ab[0], &ab[2], &ab[4]);
                const vector4& q = quad(x, y).nrm[flip];
                vector4* v;
                v = &at(ab[0], ab[1]).N(); v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
                v = &at(ab[2], ab[3]).N(); v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
                v = &at(ab[4], ab[5]).N(); v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
            }
        }
    }
    for (y = 0; y <= h; y++) {
        for (x = 0; x <= w; x++) {
            vector4& v = at(x, y).N();
            float len = v.length();
            if (len > 1e-10f) {
                float inv = 1.0f / len;
                v.x *= inv; v.y *= inv; v.z *= inv;
            } else {
                v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
            }
        }
    }
}

// ---- an agent's equipment slots, line of fire ---------------------------------------------------------------------------------

// @ 0x46b190
__int64 __cdecl pick_slot_item(int troop, int slot, int mask, int n, int* cand, char remove, char* found)
{
    *found = 0;
    if (((__int64)mask & g_bit[slot]) == 0 &&
        ((mask & 0x200) == 0 || (g_bit[slot] & 0x10) == 0 ||
         (int)g_items[g_world3->troops[troop].equip[slot].item].type_flags >= 0)) {
        *found = 1;
        return *(__int64*)&g_world3->troops[troop].equip[slot];
    }
    int i = 0;
    ItemRefX res;
    res.item = -1;
    res.m4 = 0;
    do {
        if (n <= i) break;
        int v = cand[i];
        ItemRef r;
        r.m4 = 0;
        if (v >= 0) {
            r.item = v;
            if (g_world3->troops[troop].fits_slot(&r, slot)) {
                res.item = v;
                res.m4 = 0;
                res.reduce(0);
                if (remove) cand[i] = -1;
            }
        }
        i++;
    } while (res.item == -1);
    return *(__int64*)&res;
}

// @ 0x46b2d0
void __stdcall AgentEq_pick_slots(AgentEq* a)
{
    if (a->f10 == 0) {
        int cand[8];
        for (int j = 0; j < a->ncand; j++)
            cand[j] = a->cand[j];
        for (int i = 0; i < 10; i++)
            *(__int64*)&a->sel[i] = pick_slot_item(a->troop, i, a->mask, a->ncand, cand, 1, &a->found[i]);
        return;
    }
    for (int i = 0; i < 10; i++)
        a->sel[i].item = -1;
}

// @ 0x46b3a0
void __cdecl fill56(elem56* f, elem56* l, const elem56& v) { for (; f != l; ++f) *f = v; }

// @ 0x46b410
void FireAgent::ray(FireRay* r)
{
    r->width = 0.002f;
    r->from = pos;
    float s;
    if (troop >= 0) s = (float)item_f20c(troop) * 0.01f;
    else s = 0.8f;
    vector4 d = p_scale(dir, s);
    r->to = p_add(d, org);
}

// @ 0x46b4c0
E1c8* __cdecl copy1c8(E1c8* f, E1c8* l, E1c8* d)
{
    for (; f != l; ++f, ++d) *d = *f;
    return d;
}

// ---- std::_Uninit_fill_n with an opaque allocator::construct (catch(...) { throw; } funclets follow) -------------------------------

// @ 0x46b500 (+ funclet 0x46b564)
void __cdecl uninit_fill_n36(rec36* first, unsigned count, const rec36& val, alloc36& al)
{
    rec36* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// @ 0x46b570 (+ funclet 0x46b5d4)
void __cdecl uninit_fill_n24(elem24* first, unsigned count, const elem24& val, alloc24& al)
{
    elem24* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// @ 0x46b5e0 (+ funclet 0x46b644)
void __cdecl uninit_fill_n100(particle100* first, unsigned count, const particle100& val, alloc100& al)
{
    particle100* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// @ 0x46b650 (+ funclet 0x46b6b4)
void __cdecl uninit_fill_n52(elem52* first, unsigned count, const elem52& val, alloc52& al)
{
    elem52* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// ---- construct a 0x38-byte ArrSet (name + five count/array pairs) from another -----------------------------------------------------

// @ 0x46b6c0
void __stdcall construct_arrset(ArrSet* p, const ArrSet& v) { if (p) new (p) ArrSet(v); }

// ---- _Uninit_copy with an opaque allocator::construct ---------------------------------------------------------------------------------

// @ 0x46b6e0 (+ catch(...) funclet 0x46b746)
rec36* __cdecl uninit_copy36(rec36* first, rec36* last, rec36* dest, alloc36& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    rec36* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) ;
        throw;
    }
    return dest;
}

// @ 0x46b750 (+ catch(...) funclet 0x46b7bc)
rec136* __cdecl uninit_copy136(rec136* first, rec136* last, rec136* dest, alloc136& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    rec136* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) ;
        throw;
    }
    return dest;
}

// ---- std::stable_sort over a vector of 8-byte records: the original has these instantiations in two groups (0x46b7d0 _Copy_opt,
// ---- 0x46b860 _Rotate; 0x46c260 copy, 0x46c2c0 merge, 0x46c3f0 copy, 0x46c420 copy_backward, 0x46c450 _Lower_bound, 0x46c4a0
// ---- _Upper_bound) with other functions between them; one instantiation point emits them together. -----------------------------------

void sort_e8(std::vector<E8>& v, Pr8 pred) { std::stable_sort(v.begin(), v.end(), pred); }

// @ 0x46b940
ViewParams::ViewParams()
{
    f14 = 0.1f; f28 = 0.1f;
    fc = 0x384878; f20 = 0x384878;
    f0 = 1000.0f; f8 = -100.0f; f4 = -1.0f;
    f18 = -1.0f; f1c = -100.0f; f24 = -1.0e7f;
    b10 = 0; b2c = 0;
    matrix tmp;
    matrix* id = rgl::identity(&tmp);
    world = *id;
    light = *id;
    f30 = 0;
}

// @ 0x46b9d0
void FxDef2::set_env_count(int n)
{
    if (env != 0) {
        delete[] env;
        delete[] envidx;
    }
    nenv = n;
    env = new EnvRec[n];
    envidx = new int[nenv];
    for (int i = 0; i < nenv; i++) {
        EnvRec* b = env;
        EnvRec* e = b + i;
        int c = clock;
        env[i].fd0 = c;
        e->clock = c;
        e->t0 = g_ticks[c];
        env[i].alloc(count);
    }
}

// @ 0x46baf0
void Terrain::seed_brushes(float size, float density, float scale)
{
    float r = (float)(rand() % 15817) * 1.2644623e-5f + 0.4f;
    density = density * wx * wy / (size * size * 0.25f);
    float fl = (float)floor(density);
    int k = (int)fl;
    if (!(density - fl <= 0.5f)) k++;
    int n = (int)((float)k * r * 30.0f);
    for (; n > 0; n--) {
        density = size + size;      // (the original reuses the argument slot)
        float f = (rand_avg4() * 0.5f + 0.05f) * size;
        float pos[2];
        pos[1] = (density + wy) * (float)(rand() % 15817) * 6.322312e-05f - size;
        pos[0] = (density + wx) * (float)(rand() % 15817) * 6.322312e-05f - size;
        apply_brush(pos, f, ((float)(rand() % 15817) * 6.3223115e-06f + 0.2f) * f * scale);
    }
}

#define RAND01() ((float)(rand() % 15817) * 6.3223117e-05f)

// @ 0x46bc70
void terrain_obj::make_river()
{
    int a[2];
    int b[2];
    int steps;
    float minz;
    a[1] = 0;
    bool ok = false;
    if (flag843bd75) {
        int tries = 0;
        int bx = 0;
        do {
            if (3 < tries) break;
            int bestx = 0, besty = 0;
            float bestv = -10000.0f;
            int k = 20;
            do {
                int px, py;
                if (RAND01() < 0.5f) {
                    px = rand() % 4 + 3;
                    if (RAND01() < 0.5f) px = w - px;
                    int t = h - 3;
                    if (t < 2) py = 3;
                    else py = rand() % t + 3;
                } else {
                    int t = w - 3;
                    if (t < 2) px = 3;
                    else px = rand() % t + 3;
                    py = rand() % 4 + 3;
                    if (RAND01() < 0.5f) py = h - py;
                }
                float v = noise_at(bx, a[1]);
                if (bestv < v) {
                    bestv = v;
                    bestx = px;
                    besty = py;
                }
            } while (--k != 0);
            a[0] = bestx;
            a[1] = besty;
            int n = 0;
            bool is_far = false;
            do {
                if (is_far) goto trace;
                int ex, ey;
                if (RAND01() < 0.5f) {
                    ex = 0;
                    b[0] = 0;
                    if (RAND01() < 0.5f) {
                        ex = w + 1;
                        b[0] = ex;
                    }
                    int t = h + 1;
                    if (t < 2) {
                        ey = 0;
                        b[1] = 0;
                    } else {
                        ey = rand() % t;
                        b[1] = ey;
                    }
                } else {
                    ey = 0;
                    b[1] = 0;
                    if (RAND01() < 0.5f) {
                        ey = h + 1;
                        b[1] = ey;
                    }
                    int t = w + 1;
                    if (t < 2) {
                        ex = 0;
                        b[0] = 0;
                    } else {
                        ex = rand() % t;
                        b[0] = ex;
                    }
                }
                ex = ex - bestx;
                if (ex < 0) ex = -ex;
                ey = ey - besty;
                if (ey < 0) ey = -ey;
                if ((float)(w + h) * 0.7f < (float)(ey + ex)) is_far = true;
                n++;
            } while (n < 50);
            if (is_far) {
            trace:
                trace_path(a, b, 2, &steps, &minz, 1.5f, 0.9f, 0, 1);
                if (minz < 3.5f) ok = true;
                else clear_flags(g_river_mask_a | g_river_mask_b);
                tries++;
            }
            bx = a[0];
        } while (!ok);
    }
    int r = rand();
    n284 = r % 3;
    if (2.1f < scale && 1 < r % 3) n284 = 1;
    if (3.1f < scale) n284 = 0;
    if ((unsigned char)(g_pow2_low - 1) & (unsigned char)((unsigned)f4 >> 31)) n284 = 0;
    if (ok) {
        unsigned mb = g_river_mask_b;
        unsigned both = g_river_mask_a | mb;
        spread_flow(mb, 1, both);
        for (int i = 0; i < n284; i++)
            spread_flow(mb, 2, both);
        unsigned ma = g_river_mask_a;
        spread_flow(mb, 2, ma);
        spread_flow(ma, 2, ma);
        float v = -1.3f;
        if ((unsigned char)(g_pow2_low - 1) & (unsigned char)((unsigned)f4 >> 31)) v = -2.3f;
        set_cells(2, v);
        smooth_cells(2, 0.5f);
    }
}

// ---- std::list<int>::_Buynode() (12-byte node, head links to itself); the 0x46c12f funclet frees the node and rethrows -----------------

struct IntList { std::list<int> l; };
IntList* make_int_list() { return new IntList; }

// ---- _Ucopy thunks: vector<T>::_Ucopy(first, last, dest) -> _Uninit_copy(first, last, dest, _Alval, tag) --------------------------------

// @ 0x46c150
rec36* vec36::_Ucopy(rec36* f, rec36* l, rec36* d) { return uninit_copy36(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c170
elem24* vec24::_Ucopy(elem24* f, elem24* l, elem24* d) { return uninit_copy24(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c190
particle100* vec100::_Ucopy(particle100* f, particle100* l, particle100* d) { return uninit_copy100(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c1b0
elem52* vec52::_Ucopy(elem52* f, elem52* l, elem52* d) { return uninit_copy52(f, l, d, _Alval, std::_Ptr_cat(f, d)); }

// ---- _Uninit_copy of the 0x38-byte ArrSet records (the 0x46c23a funclet destroys the constructed prefix, then rethrows) ----------------

// @ 0x46c1d0 (+ catch(...) funclet 0x46c23a: ~ArrSet over the constructed prefix, then rethrow)
ArrSet* __cdecl uninit_copy_arrset(ArrSet* first, ArrSet* last, ArrSet* dest, allocArr& al)
{
    ArrSet* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) next->~ArrSet();
        throw;
    }
    return dest;
}

// ---- Direct3D vertex / index data of a mesh; transform of the child meshes of a lod_mesh ---------------------------------------------------

// @ 0x46c4f0
#define VS(f) (*(int*)&vs[i].f)
#define PT(f) (*(int*)&ps[vs[i].index].f)
void mesh_x::fill_vertices(int* p, const unsigned* fmt)
{
    bool b18 = (res->shader->stages[0].b & 0x1f) == 0x18;
    std::vector<rgl::vertex60>& vs = local_a1 ? v50 : parent->get50();
    std::vector<rgl::point20>& ps = local_a1 ? v70 : parent->get70();
    int *p7 = p + 7, *p6 = p + 6, *p5 = p + 5, *p4 = p + 4, *p3 = p + 3, *p2 = p + 2;
    for (unsigned i = 0; i < vs.size(); i++) {
        if (*fmt & 0x2000000) {
            p[0] = PT(b.x);
            p[1] = PT(b.y);
            *p2 = PT(b.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        } else if (*fmt & 0x1000000) {
            p[0] = PT(a.x);
            p[1] = PT(a.y);
            *p2 = PT(a.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        }
        if (*fmt & 1) {
            p[0] = VS(normal.x);
            p[1] = VS(normal.y);
            *p2 = VS(normal.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        }
        if (*fmt & 2) {
            p[0] = b18 ? VS(f58) : VS(color);
            p += 1; p2 += 1; p3 += 1; p4 += 1; p5 += 1; p6 += 1; p7 += 1;
        }
        if (*fmt & 0x40) {
            p[0] = VS(f5c);
            p += 1; p2 += 1; p3 += 1; p4 += 1; p5 += 1; p6 += 1; p7 += 1;
        }
        if (*fmt & 0x10) {
            p[0] = VS(uv.x);
            *(float*)&p[1] = 1.0f - vs[i].uv.y;
            p += 2; p2 += 2; p3 += 2; p4 += 2; p5 += 2; p6 += 2; p7 += 2;
        }
        if (*fmt & 0x20) {
            p[0] = VS(f48);
            *(float*)&p[1] = 1.0f - *(float*)&vs[i].f4c;
            p += 2; p2 += 2; p3 += 2; p4 += 2; p5 += 2; p6 += 2; p7 += 2;
        }
        if (*fmt & 0x100) {
            p[0] = VS(f10.x);
            p[1] = VS(f10.y);
            *p2 = VS(f10.z);
            *p3 = VS(f20.x);
            *p4 = VS(f20.y);
            *p5 = VS(f20.z);
            p += 6; p2 += 6; p3 += 6; p4 += 6; p5 += 6; p6 += 6; p7 += 6;
        }
        if (*fmt & 0x1000) {
            std::vector<rgl::skin_vert>& es = local_a1 ? v60 : parent->get60();
            p[0] = *(int*)&es[i].w[0];
            p[1] = *(int*)&es[i].w[1];
            *p2 = *(int*)&es[i].w[2];
            *p3 = *(int*)&es[i].w[3];
            *p4 = *(int*)&es[i].bonef[0];
            *p5 = *(int*)&es[i].bonef[1];
            *p6 = *(int*)&es[i].bonef[2];
            *p7 = *(int*)&es[i].bonef[3];
            p += 8; p2 += 8; p3 += 8; p4 += 8; p5 += 8; p6 += 8; p7 += 8;
        }
    }
}

// @ 0x46c920
void mesh_x::fill_indices(unsigned short* out)
{
    std::vector<rgl::face20>& fs = local_a0 ? v90 : ((mesh_x*)parent)->get90_();
    for (unsigned i = 0; i < fs.size(); i++) {
        *out++ = (unsigned short)(&fs[i].b)[-1];
        *out++ = (unsigned short)(&fs[i].b)[0];
        *out++ = (unsigned short)(&fs[i].b)[1];
    }
}

// @ 0x46c990
void mesh_list_x::apply(matrix m)
{
    rgl::mesh** it = v._Myfirst;
    if (it < v._Mylast) {
        matrix t = m;
        do {
            rgl::mesh* x = *it;
            x->m_cc = t * x->m_cc;
            ++it;
        } while (it < v._Mylast);
    }
}

// @ 0x46c9f0
void lod_mesh_x::apply(const matrix* m)
{
    for (int i = 0; i < count; i++)
        ((mesh_list_x*)&lists[i])->apply(*m);
}

// ---- per-frame Direct3D 9 setup of a scene / view object -------------------------------------------------------------------------------------

// @ 0x46ca50
void RenderScene::apply_state()
{
    D3DVIEWPORT9 vp;
    DWORD clear = 0;
    if (use_alt_vp) {
        vp.X = vp_alt.X;
        vp.Y = vp_alt.Y;
        vp.Width = vp_alt.Width;
        vp.Height = vp_alt.Height;
        vp.MinZ = vp_alt.MinZ;
        vp.MaxZ = vp_alt.MaxZ;
    } else {
        vp.X = vp_local.X;
        vp.Y = vp_local.Y;
        vp.Width = vp_local.Width;
        vp.Height = vp_local.Height;
        vp.MinZ = vp_local.MinZ;
        vp.MaxZ = vp_local.MaxZ;
    }
    g_device->SetViewport(&vp);
    if (flags & 0x100)
        clear = D3DCLEAR_TARGET;
    if (flags & 0x200)
        clear |= D3DCLEAR_ZBUFFER;
    if (clear)
        g_device->Clear(0, 0, clear, clear_color, 1.0f, 0);
    if (use_alt_vp) {
        vp.X = vp_local.X;
        vp.Y = vp_local.Y;
        vp.Width = vp_local.Width;
        vp.Height = vp_local.Height;
        vp.MinZ = vp_local.MinZ;
        vp.MaxZ = vp_local.MaxZ;
        g_device->SetViewport(&vp);
    }
    if (msaa)
        g_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 1);
    else
        g_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 0);
    if (clip_on) {
        if (g_use_shaders == 0) {
            g_device->SetClipPlane(0, clip_plane);
            g_device->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);
        }
    } else {
        g_device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    }
    rgl::frustum* c = &cam;
    {
        D3DXMATRIX proj = c->projection();
        g_device->SetTransform(D3DTS_PROJECTION, &proj);
    }
    g_render.set_view((const rgl::mat4*)&c->m);
    if (cull_flip) {
        if (g_use_hw_shadow)
            g_cull_mode = 1;
        else
            g_cull_mode = cam.left_handed ? 2 : 3;
    } else {
        g_cull_mode = (cam.left_handed != 0) + 2;
    }
    unsigned char fog_enable = (fog && fog->enabled) ? 1 : 0;
    if (g_fog_on != fog_enable) {
        g_fog_on = fog_enable;
        g_device->SetRenderState(D3DRS_FOGENABLE, fog_enable);
    }
    if (fog && fog->enabled) {
        bool below = cam.m.o.z < fog->level;
        if (fog_invert)
            below = !below;
        if (below) {
            g_device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            g_device->SetRenderState(D3DRS_FOGCOLOR, fog->b_color);
            g_device->SetRenderState(D3DRS_FOGSTART, fog->b_start);
            g_device->SetRenderState(D3DRS_FOGEND, fog->b_end);
        } else {
            g_device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            g_device->SetRenderState(D3DRS_FOGCOLOR, fog->a_color);
            g_device->SetRenderState(D3DRS_FOGSTART, fog->a_start);
            g_device->SetRenderState(D3DRS_FOGEND, fog->a_end);
        }
    }
    if (g_use_shaders == 0) {
        if (lights.size() > 0 && lights_active && g_lights_enabled) {
            lights_active = 1;
            g_render.set_lighting(true);
            bool found_spot = false;
            D3DLIGHT9 L;
            unsigned i;
            for (i = 0; i < lights.size() && (int)i < 0x40; i++) {
                memset(&L, 0, sizeof(L));
                if (lights[i]->type == 0)
                    L.Type = D3DLIGHT_POINT;
                else if (lights[i]->type == 1)
                    L.Type = D3DLIGHT_SPOT;
                else if (lights[i]->type == 2)
                    L.Type = D3DLIGHT_DIRECTIONAL;
                L.Diffuse.r = lights[i]->diffuse[0];
                L.Diffuse.g = lights[i]->diffuse[1];
                L.Diffuse.b = lights[i]->diffuse[2];
                L.Ambient.r = 0;
                L.Ambient.g = 0;
                L.Ambient.b = 0;
                if (!found_spot && lights[i]->type == 2) {
                    found_spot = true;
                    L.Ambient.r = ambient[0];
                    L.Ambient.g = ambient[1];
                    L.Ambient.b = ambient[2];
                }
                L.Specular.r = lights[i]->specular[0];
                L.Specular.g = lights[i]->specular[1];
                L.Specular.b = lights[i]->specular[2];
                L.Position.x = lights[i]->pos[0];
                L.Position.y = lights[i]->pos[1];
                L.Position.z = lights[i]->pos[2];
                L.Direction.x = lights[i]->dir[0];
                L.Direction.y = lights[i]->dir[1];
                L.Direction.z = lights[i]->dir[2];
                L.Attenuation0 = lights[i]->atten0;
                L.Attenuation1 = lights[i]->atten1;
                L.Attenuation2 = lights[i]->atten2;
                L.Range = lights[i]->range;
                L.Theta = lights[i]->theta;
                L.Phi = lights[i]->phi;
                L.Falloff = 1.0f;
                g_device->SetLight(i, &L);
            }
            if (!found_spot) {
                memset(&L, 0, sizeof(L));
                L.Diffuse.r = 0;
                L.Diffuse.g = 0;
                L.Diffuse.b = 0;
                L.Type = D3DLIGHT_DIRECTIONAL;
                L.Ambient.r = ambient[0];
                L.Ambient.g = ambient[1];
                L.Ambient.b = ambient[2];
                L.Specular.r = 0;
                L.Specular.g = 0;
                L.Specular.b = 0;
                L.Position.x = 1000000.0f;
                L.Position.y = 0;
                L.Position.z = 0;
                L.Direction.x = -1.0f;
                L.Direction.y = 0;
                L.Direction.z = 0;
                L.Range = 1000000.0f;
                g_device->SetLight(i, &L);
            }
        } else {
            lights_active = 0;
            if (g_lighting_on) {
                g_lighting_on = 0;
                g_device->SetRenderState(D3DRS_LIGHTING, 0);
            }
        }
    }
}

// ---- vertex / index buffer uploader ---------------------------------------------------------------------------------------------------------

// Body of mesh_x::count (0x468620, above in this object): visible here so that cl sees it has no side effects, which lets it hoist loads
// across the calls below.
#define LOG_GROW(KIND_WORD, KIND_V_WORD)                                                           \
    do {                                                                                           \
        stream_base& lg = g_log;                                                                   \
        stream_base* ls;                                                                           \
        if (kind == 1)                                                                             \
            ls = &lg.w_cstr("Out of " KIND_WORD " index buffer memory. Needed = ");                \
        else                                                                                       \
            ls = &lg.w_tstr((rgl::string*)&name).w_cstr(KIND_V_WORD);                              \
        int needed = total;\
        ls = &ls->w_int(&needed);                                                                   \
        ls = &ls->w_cstr("Available:");                                                            \
        ls = &ls->w_int(pcap);                                                                \
        ls->w_cstr("\n");                                                                          \
        int grow = total / 4;                                                                      \
        if (grow <= 0x20)                                                                          \
            grow = 0x20;                                                                           \
        *pcap = grow + total;                                                                   \
        ls = &lg.w_cstr("Expanding Buffer Capacity to ");                                          \
        ls = &ls->w_int(pcap);                                                                \
        ls->w_cstr("\n");                                                                          \
        *pcap = *pcap / 32 * 32;                                                             \
        g_buffers_dirty = 1;                                                                       \
        return 0xfff3ebc3;                                                                         \
    } while (0)

// @ 0x46d060
int GeomBuffer::upload(void* scratch, void* tag)
{
    int n = 0;                          // elements staged in scratch
    int base = 0;                       // elements assigned so far in this upload
    int written = 0;                    // bytes already copied into the locked buffer
    char* locked;
    if (kind == 1)
        stride = 2;
    std::vector<rgl::mesh*>::iterator it;
    for (it = meshes.begin(); it != meshes.end(); ++it) {
        rgl::slot& s = (*it)->slots[kind];
        if (s.f24 != (int)tag) {
            s.f24 = (int)tag;
            (&s.pa)[s.f20] = (unsigned char)s.f28;
            s.f20 = (s.f20 + 1) % 2;
        }
    }
    it = meshes.begin();
    if (!is_dynamic) {
        int need = 0;
        int off = kind * sizeof(rgl::slot);
        for (; it != meshes.end(); ++it) {
            rgl::slot& s = *(rgl::slot*)((char*)&(*it)->slots[0] + off);
            if (gen != s.fc || s.f28 != 0 || s.f14 != s.f4)
                need += ((mesh_x*)*it)->count(kind);
        }
        if (need == 0)
            return 0;
        int* pcap = &capacity;
        if (cursor + need < *pcap) {
            int tries = 0;
            if (kind == 1) {
                do {
                    if (ib->Lock(cursor * 2, need * 2, (void**)&locked, D3DLOCK_NOOVERWRITE) >= 0)
                        break;
                    tries++;
                    Sleep(10);
                    if (tries > 0x5a)
                        mb_error("unable to lock index buffer");
                } while (tries < 100);
            } else {
                do {
                    if (vb->Lock(cursor * stride, stride * need, (void**)&locked, D3DLOCK_NOOVERWRITE) >= 0)
                        break;
                    tries++;
                    Sleep(10);
                    if (tries > 0x5a)
                        mb_error("unable to lock vertex buffer");
                } while (tries < 100);
            }
            for (it = meshes.begin(); it != meshes.end(); ++it) {
                rgl::slot& s = (*it)->slots[kind];
                if (s.fc != gen || s.f28 != 0 || s.f14 != s.f4) {
                    if ((((mesh_x*)*it)->count(kind) + n) * stride >= 0x418000) {
                        memcpy(locked + written, scratch, stride * n);
                        written += stride * n;
                        n = 0;
                    }
                    char* dst = (char*)scratch + stride * n;
                    if (kind == 1)
                        ((mesh_x*)*it)->fill_indices((unsigned short*)dst);
                    else
                        ((mesh_x*)*it)->fill_vertices((int*)dst, &fmt);
                    s.f8 = cursor + base;
                    s.fc = gen;
                    s.f14 = s.f4;
                    s.f28 = 0;
                    int k = kind;
                    mesh_x* tm = (mesh_x*)*it;
                    n += tm->count(k);
                    base += tm->count(k);
                }
            }
            memcpy(locked + written, scratch, stride * n);
            if (kind == 1) {
                ib->Unlock();
                cursor += base;
                return 0;
            }
            vb->Unlock();
            cursor += base;
            return 0;
        }
        gen++;
        base = 0;
        int total = 0;
        for (it = meshes.begin(); it != meshes.end(); ++it)
            total += ((mesh_x*)*it)->count(kind);
        if (total > *pcap)
            LOG_GROW("Static", ":Out of Static vertex buffer memory. Needed = ");
        if (kind == 1) {
            if (ib->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
                mb_error("unable to lock index buffer");
        } else {
            if (vb->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
                mb_error("unable to lock vertex buffer");
        }
        for (it = meshes.begin(); it != meshes.end(); ++it) {
            rgl::slot& s = (*it)->slots[kind];
            if ((((mesh_x*)*it)->count(kind) + n) * stride >= 0x418000) {
                memcpy(locked + written, scratch, stride * n);
                written += stride * n;
                n = 0;
            }
            char* dst = (char*)scratch + stride * n;
            if (kind == 1)
                ((mesh_x*)*it)->fill_indices((unsigned short*)dst);
            else
                ((mesh_x*)*it)->fill_vertices((int*)dst, &fmt);
            s.f8 = base;
            s.fc = gen;
            s.f14 = s.f4;
            s.f28 = 0;
            int k = kind;
            mesh_x* tm = (mesh_x*)*it;
            base += tm->count(k);
            n += tm->count(k);
        }
        memcpy(locked + written, scratch, stride * n);
        if (kind == 1) {
            ib->Unlock();
            cursor = base;
            return 0;
        }
        vb->Unlock();
        cursor = base;
        return 0;
    }
    int total = 0;
    for (; it != meshes.end(); ++it)
        total += ((mesh_x*)*it)->count(kind);
    int* pcap = &capacity;
    if (total > capacity)
        LOG_GROW("dynamic", ":Out of dynamic vertex buffer memory. Needed = ");
    if (kind == 1) {
        if (ib->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
            mb_error("unable to lock index buffer");
    } else if (capacity != 0) {
        int tries = 0;
        do {
            if (vb->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) >= 0)
                break;
            tries++;
            Sleep(10);
            if (tries > 0x5a)
                mb_error("unable to lock vertex buffer");
        } while (tries < 100);
    }
    for (it = meshes.begin(); it != meshes.end(); ++it) {
        rgl::slot& s = (*it)->slots[kind];
        if ((((mesh_x*)*it)->count(kind) + n) * stride >= 0x418000) {
            memcpy(locked + written, scratch, stride * n);
            written += stride * n;
            n = 0;
        }
        char* dst = (char*)scratch + stride * n;
        if (kind == 1)
            ((mesh_x*)*it)->fill_indices((unsigned short*)dst);
        else
            ((mesh_x*)*it)->fill_vertices((int*)dst, &fmt);
        s.f8 = base;
        s.f14 = s.f4;
        int k = kind;
        mesh_x* tm = (mesh_x*)*it;
        base += tm->count(k);
        n += tm->count(k);
    }
    memcpy(locked + written, scratch, stride * n);
    if (kind == 1) {
        ib->Unlock();
        return 0;
    }
    if (capacity != 0)
        vb->Unlock();
    return 0;
}

// ---- shadow-view draw pass ---------------------------------------------------------------------------------------------------------------------

static __forceinline vector4 sub_i(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 cross_i(const vector4& a, const vector4& b)
{
    return vector4(a.z * b.y - b.z * a.y, b.z * a.x - a.z * b.x, a.y * b.x - b.y * a.x);
}

// @ 0x46d8c0
void drawer::draw(view_t* v)
{
    if (v->b162 || v->f164) {
        if (g_lighting) {
            g_lighting = 0;
            g_device->SetRenderState(D3DRS_LIGHTING, 0);
        }
        if (g_alpha_test)
            g_device->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
        g_alpha_test = 0;
        g_render.set_blend_mode(2);
        if (v->f164) {
            if (g_zfunc_always) {
                g_zfunc_always = 0;
                g_device->SetRenderState(D3DRS_ZFUNC, 8);
            }
            if (g_zwrite != 1) {
                g_zwrite = 1;
                g_device->SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }
            g_render.set_stage_alpha(0, 0x4a4);
        } else {
            if (g_zfunc_always != 1) {
                g_zfunc_always = 1;
                g_device->SetRenderState(D3DRS_ZFUNC, 4);
            }
            if (g_zwrite) {
                g_zwrite = 0;
                g_device->SetRenderState(D3DRS_ZWRITEENABLE, 1);
            }
            g_render.set_stage_alpha(0, 0xa2);
        }
        if (g_b58) {
            g_device->SetRenderState(D3DRS_NORMALIZENORMALS, 0);
            g_b58 = 0;
        }
        if (g_b57) {
            g_device->SetRenderState(D3DRS_SPECULARENABLE, 0);
            g_b57 = 0;
        }
        g_render.set_stage_color(0, 0xa2);
        g_render.set_stage_color(1, 2);
        g_render.set_stage_alpha(1, 2);
        g_render.set_stage_color(2, 1);
        g_render.set_stage_alpha(2, 1);
    }

    matrix M;
    (rot3&)M = v->rot;
    M.s.x *= 0.5f; M.s.y *= 0.5f; M.s.z *= 0.5f;
    vector4 half(0.5f, 0.5f, 0.5f);
    M.o = half;
    M.f.x *= 0.5f; M.f.y *= 0.5f; M.f.z *= 0.5f;
    M.u.x *= 0.5f; M.u.y *= 0.5f; M.u.z *= 0.5f;
    matrix A;
    rgl::identity(&A);
    float ang = 1.5707964f;
    A.rotate_about_s(&ang);
    M = A * M;
    cur_part = 0;
    cur_mat = 0;
    M.s.w = 0.0f; M.f.w = 0.0f; M.u.w = 0.0f; M.o.w = 1.0f;

    entry* p = v->ents._Myfirst;
    if (p == v->ents._Mylast) return;
    do {
        skin_x* part = (skin_x*)p->m->res;
        int b = p->m->slots[0].f4;
        int a = p->m->slots[0].f0;
        if (cur_a != b || cur_b != a) {
            cur_a = b;
            cur_b = a;
            g_device->SetVertexDeclaration(vbs[b + a * 2].decl);
            g_device->SetStreamSource(0, vbs[b + a * 2].vb, 0, vbs[b + a * 2].stride);
        }
        rgl::mesh* node = p->m->local_a0 ? p->m : p->m->parent;
        int ibi = node->slots[1].f4;
        if (cur_ib != ibi) {
            cur_ib = ibi;
            g_device->SetIndices(ib(ibi));
        }
        if (!v->b162) {
            if (v->f164) goto L_state;
            if (part != cur_part) {
                cur_part = part;
                part->apply_state((const float*)&p->m->f48);
                if (cur_mat != part->shader) {
                    ((material_x*)part->shader)->bind_pixel_shader();
                    cur_mat = part->shader;
                }
            }
            if (v->p1b0 && v->p1b0->on) {
                if (part->flags & 1) {
                    if (g_b15) {
                        g_b15 = 0;
                        g_device->SetRenderState(D3DRS_FOGENABLE, 0);
                    }
                } else {
                    if (g_b15 != 1) {
                        g_b15 = 1;
                        g_device->SetRenderState(D3DRS_FOGENABLE, 1);
                    }
                }
            }
            if (v->b160 && !(part->shader->flags20 & 0x80)) {
                if (g_lighting != 1) {
                    g_lighting = 1;
                    g_device->SetRenderState(D3DRS_LIGHTING, 1);
                }
                unsigned i = 0;
                light_t** b;
                while ((b = v->lights._Myfirst, b != 0) && i < (unsigned)(v->lights._Mylast - b) && (int)i < 0x40) {
                    light_t** pl = &b[i];
                    if ((*pl)->type == 2) {
                        if (g_light_on[i] != 1) {
                            g_light_on[i] = 1;
                            g_device->LightEnable(i, 1);
                        }
                        i = i + 1;
                    } else {
                        light_t* l = *pl;
                        vector4 d = sub_i(l->pos, p->m->m194.o);
                        if (d.length() - p->m->f44 < (*pl)->range) {
                            if (g_light_on[i] != 1) {
                                g_light_on[i] = 1;
                                g_device->LightEnable(i, 1);
                            }
                            i = i + 1;
                        } else {
                            if (g_light_on[i]) {
                                g_light_on[i] = 0;
                                g_device->LightEnable(i, 0);
                            }
                            i = i + 1;
                        }
                    }
                }
            } else {
                if (g_lighting) {
                    g_lighting = 0;
                    g_device->SetRenderState(D3DRS_LIGHTING, 0);
                }
            }
            p->m->setup_material();
            unsigned k = part->shader->stages[0].d & 0xff;
            if (k == 0x30)
                g_device->SetTransform((D3DTRANSFORMSTATETYPE)0x10, (const D3DMATRIX*)&M);
            else if (k == 0x20)
                g_device->SetTransform((D3DTRANSFORMSTATETYPE)0x10, (const D3DMATRIX*)&M);
        } else {
            if (v->f164) {
L_state:
                if (g_tss_c != 0xa2) {
                    g_tss_c = 0xa2;
                    g_device->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
                    g_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, g_stage_arg[5]);
                    g_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, g_stage_arg[0]);
                }
                if (g_tss_d != 1) {
                    g_tss_d = 1;
                    g_device->SetTextureStageState(1, D3DTSS_ALPHAOP, 1);
                    g_device->SetTextureStageState(1, D3DTSS_ALPHAARG1, g_stage_arg[0]);
                    g_device->SetTextureStageState(1, D3DTSS_ALPHAARG2, g_stage_arg[0]);
                }
                if (g_tss_a != 0xa2) {
                    g_tss_a = 0xa2;
                    g_device->SetTextureStageState(0, D3DTSS_COLOROP, 2);
                    g_device->SetTextureStageState(0, D3DTSS_COLORARG1, g_stage_arg[5]);
                    g_device->SetTextureStageState(0, D3DTSS_COLORARG2, g_stage_arg[0]);
                }
                if (g_tss_b != 1) {
                    g_tss_b = 1;
                    g_device->SetTextureStageState(1, D3DTSS_COLOROP, 1);
                    g_device->SetTextureStageState(1, D3DTSS_COLORARG1, g_stage_arg[0]);
                    g_device->SetTextureStageState(1, D3DTSS_COLORARG2, g_stage_arg[0]);
                }
            } else {
                if (!(p->m->mask & 0x8000)) {
                    if (g_zwrite) {
                        g_zwrite = 0;
                        g_device->SetRenderState(D3DRS_ZWRITEENABLE, 1);
                    }
                    ((texture_x*)part->tex[0])->bind(0, 0.0f);
                    g_render.set_stage_alpha(0, 0xa2);
                    g_render.set_stage_alpha(1, 0xaa);
                } else {
                    if (part->tex[0]) {
                        if (g_zwrite != 1) {
                            g_zwrite = 1;
                            g_device->SetRenderState(D3DRS_ZWRITEENABLE, 0);
                        }
                        ((texture_x*)part->tex[0])->bind(0, 0.0f);
                        g_render.set_stage_alpha(0, 0x4a4);
                    } else {
                        if (g_zwrite != 1) {
                            g_zwrite = 1;
                            g_device->SetRenderState(D3DRS_ZWRITEENABLE, 0);
                        }
                        g_render.set_stage_alpha(0, 0xa2);
                    }
                    g_render.set_stage_alpha(1, 2);
                }
            }
            if (g_e68) {
                g_device->SetPixelShader(0);
                g_e68 = 0;
            }
            int col = p->m->color;
            if (g_texfactor != col) {
                g_texfactor = col;
                g_device->SetRenderState(D3DRS_TEXTUREFACTOR, col);
            }
        }

        int cull = g_cull_base;
        if (p->m->f4c) {
            if (cull == 3) cull = 2;
            else if (cull == 2) cull = 3;
        }
        if (cull != g_cull_cur) {
            g_cull_cur = cull;
            g_device->SetRenderState(D3DRS_CULLMODE, cull);
        }

        if (p->m->f34 == 0) {
            if (!(part->flags & 0x20000)) {
                A = p->m->m194;
            } else {
                (rot3&)A = *(rot3*)&p->m->m194;
                A.o = v->org;
                A.o.z = A.o.z * 0.75f;
            }
        } else if (p->m->f34 == 1) {
            matrix B;
            B.o = p->m->m194.o;
            B.u = p->m->m194.u;
            B.s = v->rot.s;
            B.f = cross_i(B.s, B.u);
            B.u.normalize();
            B.s = cross_i(B.u, B.f);
            A = B;
        } else {
            matrix B;
            B.o = p->m->m194.o;
            B.s = v->rot.s; B.f = v->rot.f; B.u = v->rot.u;
            A = B;
        }
        A.s.w = 0.0f; A.f.w = 0.0f; A.u.w = 0.0f; A.o.w = 1.0f;
        g_device->SetTransform((D3DTRANSFORMSTATETYPE)0x100, (const D3DMATRIX*)&A);

        std::vector<rgl::face20>& fv = node->local_a0 ? node->v90 : ((mesh_x*)node->parent)->get90_();
        int nf = fv.size();
        std::vector<rgl::vertex60>& vv = p->m->local_a1 ? p->m->v50 : ((mesh_x*)p->m->parent)->get50_();
        int nv = vv.size();
        g_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, p->m->slots[0].f8, 0, nv, node->slots[1].f8, (nf * 3) / 3);
        p++;
    } while (p != v->ents._Mylast);
}

