// Slice s004faee0: the flare light's shadow-map visibility sampler (0x4faee0) and the sun-lit agent pass (0x4fb180).
// Both are __thiscall members of the object at 0xa76630 (the same object as s004fa790 / s004fb350).
// Reuses the verified ShadowMap (s004640d0), AgentTable / Mission (s0043bdb0, s00457b40) and set_fade (s004b1160)
// layouts. Names are provisional; every callee is declared with its VA and is not defined in this slice.
#include <windows.h>
#include <d3d9.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- mission / agent table (verified: s0043bdb0, s00457b40) ---------------------------------------------
struct Agent;
struct Node;
struct AgentTable {
    char pad0[0x18];
    Agent** pages;                                  // +0x18 (id >> 4 -> page of 16 agents)
    Agent* get(unsigned id);                        // 0x43b1f0 (thiscall, ret 4)
};
struct Mission {
    int f0;
    AgentTable tbl;                                 // +4
};
extern Mission* g_mission;                          // 0x5e292c

// an agent as used here: +0x04 non-zero when it exists, +0x88 origin, +0x250 the held item node,
// +0x08 / +0x30 the two item slots
struct Agent {
    char pad0[4];
    void* holder;                                   // +0x04
    int slot_a;                                     // +0x08
    char pad0c[0x30 - 0x0c];
    int slot_b;                                     // +0x30
    char pad34[0x88 - 0x34];
    vector4 origin;                                 // +0x88
    char pad98[0x250 - 0x98];
    Node* node;                                     // +0x250 (the agent's scene node)
};

// ---- shadow map (verified: s004640d0) --------------------------------------------------------------------
extern IDirect3DDevice9* g_device;                  // 0xa17f28
struct ShadowMap {
    char pad0[0xc];
    void* surf;                                     // +0x0c
    int w, h;                                       // +0x10 / +0x14
    void read_pixel(int unused, int x, int y, unsigned* a, unsigned* b);   // 0x4641a0
};

// ---- light camera: to_clip of the frustum at +0x10c (0x4621f0) -------------------------------------------
struct Frustum {
    vector4* to_clip(vector4* out, const vector4* p);   // 0x4621f0 (thiscall, ret 8)
};

// ---- the applicator / scene node of an agent's item ------------------------------------------------------
struct Node {                                       // the agent's scene object: origin at +0x88
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4(); virtual void v5(); virtual void v6(); virtual void v7();
    virtual void set_intensity(const float* v);     // +0x20 (ret 4)
    char pad[0x88 - 4];
    vector4 origin;                                 // +0x88
    void set_fade(const float* v);                  // 0x4b1800 (thiscall, ret 4)
};

// ---- helpers ----------------------------------------------------------------------------------------------
int  clamp_int(int* v, int* lo, int* hi);           // 0x432510 (cdecl: max(lo, min(v, hi-1)))
int  round_int(float f);                            // 0x535a64 (lib_crt _ftol2)

// ---- globals ----------------------------------------------------------------------------------------------
extern int g_shadow_enabled;                        // 0x5e28fc
extern int g_flag_a17108;                           // 0xa17108
extern int g_flag_a17148;                           // 0xa17148
extern int g_flag_a17348;                           // 0xa17348
extern int g_screen_w;                              // 0x5d75c8
extern int g_screen_h;                              // 0x5d75cc
extern float g_scale_a;                             // 0x5aa08c
extern float g_scale_b;                             // 0x5aa090
extern float g_fx_scale;                            // 0x599480
extern float g_half;                                // 0x549d70
extern float g_one_a;                               // 0x549cdc
extern float g_render_dist;                         // 0x590d8c
extern float g_fade_step;                           // 0x59947c (0.13513514)
extern float g_dist_add;                            // 0x54b8ac
extern ShadowMap* g_shadow;                         // (the map object read at 0x4fb0f1)

// ---- the 0xa76630 object (only the members touched here; layout provisional) ------------------------------
struct View {
    int f0;                                         // +0x00
    char pad4[0x0c - 4];
    void* light_src;                                // +0x0c
    char pad10[0x20 - 0x10];
    int light_x;                                    // +0x20
    int light_y;                                    // +0x24
    char pad28[0x1f0 - 0x28];
    void* sun_cam;                                  // +0x1f0
    bool shadow_sample(const vector4* p, float* out);       // 0x4faee0
    void apply_sun_light(const vector4* pos, const float* scale);    // 0x4fb180
};

// the pass's per-agent source slot (+0x04/+0x1c) is the agent table itself
struct ViewLight {
    AgentTable tbl;                                 // +0x4 (vector<Agent*> at +4: begin at +4, end +8, cap +0xc)
    char pad1c[0x5c - 0x1c];
    int local_id;                                   // +0x5c
};

// @ 0x4faee0
bool View::shadow_sample(const vector4* p, float* out)
{
    static const int cells[18] = {
        0, 1, 1, 0, 2, -2, -1, 1, -2, 0,
        -1, -1, 0, 0, 1, 1, -1, 2
    };
    static const float w[9] = { 1.0f, 0.9f, 0.9f, 0.9f, 0.9f, 0.7f, 0.7f, 0.7f, 0.7f };

    *out = 1.0f;
    if (!sun_cam || !g_shadow_enabled || !light_src)
        return false;
    Frustum* fr = (Frustum*)((char*)sun_cam + 0x10c);
    matrix clip;
    fr->to_clip((vector4*)&clip, p);
    if (!(clip.o.x >= 0.0f && clip.o.x < 1.0f && clip.o.y >= 0.0f && clip.o.y < 1.0f))
        return true;
    int cx = round_int((1.0f - clip.o.y) * (float)light_y);
    int cy = round_int(clip.o.x * (float)light_x);
    *out = 0.0f;
    float depth = clip.o.z - 0.001f;
    for (int i = 0; i < 9; i++) {
        int lo = 0;
        int x = cells[i * 2 + 1] + cx;
        int y = cells[i * 2] + cy;
        int cl = clamp_int(&y, &lo, &light_y);
        int cl2 = clamp_int(&x, &lo, &light_x);
        (void)cl;
        unsigned a = 0, b = 0;
        float s = 0.0f;
        g_shadow->read_pixel(0, cl2, cl, &a, &b);
        float t = (float)a * (1.0f / 255.0f);
        (void)s;
        if (!(t < depth))
            *out += w[i];
    }
    *out = *out * 0.13513514f;
    return true;
}

// @ 0x4fb180
void View::apply_sun_light(const vector4* pos, const float* scale)
{
    float px = (float)(g_screen_w + g_screen_h);
    float area = (px * 0.5f) * 0.00086956524f;
    float size_v = ((g_scale_a * 0.5f + 0.75f) * *scale) * area;
    float& size = *(float*)&scale;       // the original stores the result back into the (dead) parameter slot
    size = size_v;
    if (g_flag_a17108 && (g_flag_a17148 || g_flag_a17348))
        size = 1000.0f;
    float range = g_scale_b * area;
    ViewLight* vl = (ViewLight*)((char*)this);
    AgentTable* tbl = (AgentTable*)((char*)this + 4);
    int n = *(int*)tbl;
    unsigned i = 0;
    if (0 < n) {
        do {
            if (tbl->get(i)->holder)
                break;
            i++;
        } while ((int)i < n);
    }
    if (i < *(unsigned*)tbl) {
        do {
            char** pp = &(*(char***)((char*)this + 0x1c))[i >> 4];
            Agent* a = (Agent*)(*pp + (i & 0xf) * 0x5928);
            if (a->node) {
                float one = 1.0f;
                float intensity;
                float dx = a->node->origin.x - pos->x;
                float dy = a->node->origin.y - pos->y;
                float dz = a->node->origin.z - pos->z;
                float d = (float)sqrt(dy * dy + (dx * dx + dz * dz));
                float dist = 3.5f / (d > 3.5f ? d : 3.5f);
                if (!(d > 1.0f))
                    d = 1.0f;
                intensity = (1.0f / d) * size;
                a->node->set_intensity(&intensity);
                const float* v;
                if (a->slot_a == vl->local_id || a->slot_b == vl->local_id)
                    v = &one;
                else {
                    dist = dist * range;
                    v = &dist;
                }
                a->node->set_fade(v);
            }
            int n2 = *(int*)tbl;
            for (i++; (int)i < n2; i++) {
                if (tbl->get(i)->holder)
                    break;
            }
        } while (i < *(unsigned*)tbl);
    }
}
