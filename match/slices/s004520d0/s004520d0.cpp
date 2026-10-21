// Slice s004520d0: text layout into a mesh (Font::layout_text: UTF-8 decode, greedy word wrap, alignment flags,
// one quad per glyph via Font::draw_glyph 0x44f5d0), FxDef::skin_lods (skin every particle mesh of a lod
// group), Mission::blood_burst (splash blood on every agent within 3.5 units of a point) and
// Agent::refresh_carried (swap the display mesh of a carried item). Names provisional.
#include <stdlib.h>
#include <math.h>
#include <vector>
#include "mesh.h"

using rgl::vector4;
using rgl::mat4;
using rgl::mesh;
using rgl::lod_mesh;

// ---- font (see s0043db60 / s0044f3c0) ----------------------------------------------------------------------
struct Glyph {                                       // 0x1c bytes: quad uv rectangle + metrics
    float u0, v0, u1, v1;
    float xoff, yoff, xadv;
};
struct GlyphNode { GlyphNode *left, *parent, *right; Glyph val; };
struct GlyphMap {                                    // std::map<int, glyph>
    int pad0[2];
    GlyphNode* head;                                 // +8
    void lookup(GlyphNode** out, const int* key);    // 0x43b210 (ret 8)
};
struct vec2f { float x, y; };
struct tstring {
    int pos; char* buf; int cap; int len;
};
struct Font {
    char pad0[0x18];
    float f18, f1c;                                  // +0x18 (glyph cell width / height in texels, provisional)
    float size;                                      // +0x20
    float spacing;                                   // +0x24 (line spacing factor)
    Glyph dflt;                                      // +0x28 (returned for unknown characters)
    GlyphMap map;                                    // +0x44
    char pad50[0x6c - 0x50];
    Glyph ascii[0x200];                              // +0x6c
    __forceinline const Glyph* get(int c)
    {
        if (c >= 0x200) {
            GlyphNode* it;
            map.lookup(&it, &c);
            if (it != map.head)
                return &it->val;
            return &dflt;
        }
        return &ascii[c];
    }
    float measure(const int* text, int len, int* start, int* end);   // 0x43e200 (ret 0x10)
    void draw_glyph(int ch, const vector4* pos, const vec2f* scale, mesh* m, int arg);   // 0x44f5d0 (ret 0x14)
    void layout_text(float* out, const tstring* text, const vector4* pos_in, const float* scale_in, mesh* m,
                     int arg, const float* xmin, const float* xmax, unsigned flags);   // 0x4520d0 (ret 0x24)
};

enum {
    TXT_NOWRAP2 = 0x2, TXT_ALIGN_L = 0x4, TXT_ALIGN_R = 0x8, TXT_ALIGN_C = 0x10, TXT_NOSHRINK = 0x100,
    TXT_ALIGN_L2 = 0x200, TXT_START_X = 0x400, TXT_TALL_LINES = 0x800
};

// UTF-8 -> int code points (0x487b00; custom register convention in the original: text in edi, count out in edx)
const int g_max_chars = 8192;                        // 0x59095c
static __declspec(noinline) void decode_utf8(int* out_len, const tstring* s, int* buf, const int* maxn)   // 0x487b00
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            if (*out_len >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((((q[0] & 7) << 6) | (q[1] & 0x3f)) << 6 | (q[2] & 0x3f)) << 6) | (q[3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = ((((q[0] & 0xf) << 6) | (q[1] & 0x3f)) << 6) | (q[2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((q[0] & 0x1f) << 6) | (q[1] & 0x3f));
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[*out_len] = ch;
            (*out_len)++;
        } while (i < n);
    }
}

// @ 0x4520d0
void Font::layout_text(float* out, const tstring* text, const vector4* pos_in, const float* scale_in, mesh* m,
                       int arg, const float* xmin, const float* xmax, unsigned flags)
{
    int buf[8192];
    int len;
    vector4 pos = *pos_in;
    if (flags & TXT_START_X)
        pos.x = *xmin;
    vec2f scale;
    scale.x = (size / f18) * scale_in[0];
    scale.y = (scale_in[1] / f1c) * size;
    float y0 = pos.y;
    decode_utf8(&len, text, buf, &g_max_chars);
    unsigned noshrink = flags & TXT_NOSHRINK;
    if (noshrink == 0 && (flags & TXT_NOWRAP2) == 0) {
        int start = 0;
        float w = measure(buf, len, &start, &len) * scale.x * 1.001f;
        float avail = *xmax - *xmin;
        if (w > avail && w > 0.0f) {
            float f = avail / w;
            scale.x = scale.x * f;
            scale.y = f * scale.y;
        }
    }
    int cur = 0;
    float maxx = *xmin;
    if (len > 0) {
        float inv = 1.0f / scale.x;
        unsigned f4 = flags & TXT_ALIGN_L;
        unsigned f800 = flags & TXT_TALL_LINES;
        do {
            int line_end = cur;
            float linew = 0.0f;
            bool first = true;
            bool brk = false;
            int p = cur;
            do {
                int s = p;
                brk = false;
                if (p < len) {
                    for (; s < len; s++) {
                        int c = buf[s];
                        if (c == 0x20 || c == 0xa || c == 9) {
                            if (s < len) {
                                if (buf[s] == 0xa)
                                    brk = true;
                                s++;
                            }
                            break;
                        }
                    }
                }
                int e = s;
                float w = measure(buf, len, &cur, &e) * scale.x;
                if (first || w <= *xmax - *xmin) {
                    linew = w;
                    line_end = s;
                    p = s;
                } else {
                    brk = true;
                }
                if (p >= len)
                    brk = true;
                first = false;
            } while (!brk);
            if (f4 != 0 || ((flags & TXT_ALIGN_L2) && noshrink == 0)) {
                pos.x = *xmin;
            } else if (flags & TXT_ALIGN_C) {
                pos.x = (*xmin + *xmax) * 0.5f - linew * 0.5f;
            } else if (flags & TXT_ALIGN_R) {
                pos.x = *xmin + *xmax - linew;
            }
            float ux = inv * pos.x;
            for (int k = cur; k < line_end; k++) {
                int ch = buf[k];
                const Glyph* g = get(ch);
                if (ch != 0xa) {
                    if (ch == 9) {
                        int n = (int)((inv * pos.x - ux) * 4.0f);
                        pos.x = (((float)n + 1.0f) * 0.25f + ux) * scale.x;
                    } else {
                        pos.x = scale.x * g->xoff + pos.x;
                        draw_glyph(ch, &pos, &scale, m, arg);
                        pos.x = scale.x * g->xadv + pos.x;
                    }
                }
            }
            if (maxx < pos.x)
                maxx = pos.x;
            pos.x = *xmin;
            pos.y = pos.y - (((f800 ? 1.2f : 1.0f) * size * scale_in[1]) * spacing) * 0.9f;
            cur = line_end;
        } while (cur < len);
    }
    m->compute_normals();
    if ((flags & TXT_ALIGN_C) && noshrink != 0)
        out[0] = *xmax - *xmin;
    else
        out[0] = (maxx - *xmin) + 0.0001f;
    out[1] = (y0 - pos.y) + 0.001f;
}

// ---- FxDef (see s00428240 world.h) --------------------------------------------------------------------------
struct FxDef {
    void skin(mesh* p, int flag);                       // 0x4291c0 (ret 8)
    void skin_lods(lod_mesh* g, int flag);              // 0x452530 (ret 8)
};

// @ 0x452530
void FxDef::skin_lods(lod_mesh* g, int flag)
{
    for (int k = 0; k < g->count; k++) {
        std::vector<mesh*>& v = g->lists[k].v;
        for (mesh** it = v._Myfirst; it < v._Mylast; ++it)
            skin(*it, flag);
    }
}

// ---- agents and the blood splash (see s00451710 / s00443270) --------------------------------------------------
struct Agent {
    char pad0[4];
    int used;                                           // +4
    char pad8[0x50 - 8];
    vector4 pos;                                        // +0x50
    char pad60[0x5928 - 0x60];
    void apply_blood(const float* center, float amount);   // 0x4519f0 (ret 8)
};
struct AgentTable {                                     // mission+4
    int count;                                          // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
};
struct Mission {
    char pad0[4];
    AgentTable agents;                                  // +4
    void blood_burst(float x, float y, float z, int unused, float amount);   // 0x4525a0 (ret 0x14)
};
extern int g_blood_enabled;                             // 0x5a9fc0 (provisional)

// @ 0x4525a0
void Mission::blood_burst(float x, float y, float z, int unused, float amount)
{
    if (g_blood_enabled) {
        unsigned i = agents.first_used();
        if (i < (unsigned)agents.count) {
            do {
                Agent* a = &agents.pages[i >> 4][i & 0xf];
                float dx = a->pos.x - x;
                float dy = a->pos.y - y;
                float dz = a->pos.z - z;
                if ((float)sqrt(dx * dx + dy * dy + dz * dz) < 3.5f)
                    a->apply_blood(&x, amount);
                agents.next_used(&i);
            } while (i < (unsigned)agents.count);
        }
    }
}

// ---- carried item display (the agent that holds it: "other"; the one that shows it: "this") -----------------
struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[4];
    unsigned __int64 flags;                             // +0x1d0
    char pad1d8[0x238 - 0x1d8];
};
extern ItemKind* g_items;                               // 0x5e29d0

struct Skeleton {                                       // object at body+0x304 (see s0043edf0)
    char* rec(int i);                                   // 0x43f260
    void attach(int id, lod_mesh* l);                   // 0x4424a0 (ret 8)
};
struct AgentBody {
    char pad0[0x58];
    mat4 m;                                             // +0x58
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                     // +0x304
};
struct CarrierSrc {                                     // "other"
    char pad0[0x20];
    vector4 v20;                                        // +0x20 (position)
    char pad30[0x40 - 0x30];
    vector4 v40;                                        // +0x40 (direction)
    char pad50[0x5c - 0x50];
    float f5c;                                          // +0x5c
    char pad60[0x80 - 0x60];
    int item_a;                                         // +0x80
    char pad84[4];
    int item_b;                                         // +0x88
    mesh* make_ranged_mesh();                           // 0x44f6a0
};
struct Carrier {
    char pad0[0x15c];
    lod_mesh* lods[1];                                  // +0x15c
    char pad160[0x230 - 0x160];
    int slot_idx;                                       // +0x230
    char pad234[0x250 - 0x234];
    AgentBody* body;                                    // +0x250
    void update_equipment_display();                    // 0x443aa0
    void refresh_carried(CarrierSrc* other);            // 0x452650 (ret 4)
};
float frand();                                          // 0x40e400

// @ 0x452650
void Carrier::refresh_carried(CarrierSrc* other)
{
    int slot = slot_idx;
    if (slot < 0)
        return;
    int idx = other->item_a;
    if (idx < 0)
        return;
    if ((char)g_items[idx].type_flags != 10)
        idx = other->item_b;
    if (idx < 0)
        return;
    if ((g_items[idx].flags & 0xff000) == 0x10000)
        return;
    mesh* m = other->make_ranged_mesh();
    if (m == 0)
        return;
    if (lods[slot] == 0) {
        lods[slot] = new lod_mesh;
        mesh* nm = new mesh;
        nm->res = m->res;
        lods[slot]->add(0, nm);
        body->skel->attach(0x13, lods[slot]);
        update_equipment_display();
    }
    lod_mesh* L = lods[slot];
    if (L) {
        if ((char)g_items[other->item_a].type_flags == 8)
            m->scale(0.87f);
        mat4 M;
        M = body->m * *(mat4*)body->skel->rec(0xc);
        rgl::matrix fr;
        fr.f = other->v40;
        fr.u.x = 0.0f; fr.u.y = 0.0f; fr.u.z = 1.0f;
        fr.o = other->v20;
        fr.orthonormalize();
        rgl::matrix rel = M.relative(fr);
        vector4 d;
        d.x = rel.o.x;
        d.z = 0.0f;
        d.y = rel.o.y - 0.23f;
        float n = d.normalize();
        if (n < 0.0f)
            n = 0.0f;
        else if (n > 0.19f)
            n = 0.19f;
        d.x = d.x * n;
        rel.o.x = d.x;
        rel.o.w = d.w;
        d.y = d.y * n + 0.23f;
        rel.o.y = d.y;
        d.z = d.z * n;
        rel.o.z = d.z;
        rel.o.z = 0.05f - (frand() * 0.01f + other->f5c * 0.0005f);
        if (rel.o.x < -0.45f)
            rel.o.x = -0.45f;
        else if (rel.o.x > 0.2f)
            rel.o.x = 0.2f;
        rel.o.x = (frand() - 0.5f) * 0.02f + rel.o.x;
        rel.o.y = (frand() - 0.5f) * 0.02f + rel.o.y;
        (*L->lists[0].v.begin())->append_plain(m, (const mat4*)&rel);
    }
    delete m;
}
