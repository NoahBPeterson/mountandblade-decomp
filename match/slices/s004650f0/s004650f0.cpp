// Slice s004650f0: Terrain scatter-object generation (ground-spec weighted random picks), the per-type scatter
// parameters, two terrain-descriptor initialisers ("Illegal terrain size"), a hex-word parser and a random
// value helper. Names are provisional. Flags: default.
#include <stdlib.h>
#include "../s00464a60/terrain.h"


// Same-TU copy of Terrain::spec_weight (0x464bf0, defined in slice s00464a60): lets the compiler see that it does
// not modify its pointer arguments (the original scatter code keeps the index in a register across the calls).
float Terrain::spec_weight(int* spec, int* type, int* mode)
{
    GroundSpec* g = &g_ground_specs[*spec];
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

// @ 0x4650f0
void Terrain::scatter(ScatterSet* set, int mode, int n)
{
    set->count = 0;
    int cnt = g_ground_spec_count;
    GroundSpec* specs = g_ground_specs;
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
                        cnt = g_ground_spec_count;
                        specs = g_ground_specs;
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
                    cnt = g_ground_spec_count;
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
                ScatterItem* it = &set->items[set->count];
                it->kind = sel;
                float rx = (float)(rand() % 15817) * 0.63223112f;
                float ry = (float)(rand() % 15817) * 0.63223112f;
                float rz = (float)(rand() % 15817) * 0.63223112f;
                it->v = vector4(rz, ry, rx);
                it->f4 = (float)(rand() % 15817) * 0.0031611556f + 50.0f;
                it->f8 = (float)(rand() % 15817) * 3.1611557e-06f;
                int r5 = rand() % 15817;
                GroundSpec* gs = &g_ground_specs[sel];
                int dens = (unsigned short)(((unsigned*)&gs->flags)[1]);
                it->f1c = ((float)r5 * 6.3223117e-05f + 1.0f) * ((it->f8 + 0.4f) * ((3.0f / ((float)n + 2.0f)) * (float)dens)) * 0.015f;
                if (gs->flags & 0x80000) {
                    it->f4 = it->f4 * 0.45f;
                    it->f8 = it->f8 - 0.11f;
                }
                if (mode == 0 && (t == 11 || t == 12 || t == 10))
                    it->f8 = it->f8 - 0.07f;
                set->count++;
                cnt = g_ground_spec_count;
                specs = g_ground_specs;
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
    scatter(&set1, 1, c);
    scatter(&set2, 2, s2);
    scatter(&set0, 0, b);
}

// @ 0x4656c0
void Terrain::fill_desc_named(TerrainDesc* d)
{
    d->init();
    d->w = w + 1;
    d->h = h + 1;
    d->mode = 13;
    for (int i = 0; i < 11; i++) {
        d->e[i].idx = i;
        d->e[i].name = g_ground_types[i].name;
    }
    d->elev_idx = 11;
    d->level_idx = 12;
    d->e[d->elev_idx].idx = -0x1e71;
    d->e[d->level_idx].idx = -0x3115;
    d->e[d->elev_idx].name.assign(g_name_elevation);
    d->e[d->level_idx].name.assign(g_name_leveling);
    if (d->w > 0x1a4 || d->h > 0x1a4)
        mb_error("Illegal terrain size");
}

// @ 0x4657c0
void Terrain::fill_desc_none(TerrainDesc* d)
{
    d->init();
    d->w = w + 1;
    d->h = h + 1;
    d->mode = 13;
    for (int i = 0; i < 11; i++) {
        d->e[i].idx = -1;
        d->e[i].name.assign("none");
    }
    d->elev_idx = 11;
    d->level_idx = 12;
    d->e[d->elev_idx].idx = -0x1e71;
    d->e[d->level_idx].idx = -0x3115;
    d->e[d->elev_idx].name.assign(g_name_elevation);
    d->e[d->level_idx].name.assign(g_name_leveling);
    d->mode = 0;
    if (d->w > 0x1a4 || d->h > 0x1a4)
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
float rand_avg4()
{
    float a = (float)(rand() % 15817) * 6.3223117e-05f;
    float b = (float)(rand() % 15817) * 6.3223117e-05f;
    float c = (float)(rand() % 15817) * 6.3223117e-05f;
    return ((float)(rand() % 15817) * 6.3223117e-05f + c + b + a) * 0.25f;
}
