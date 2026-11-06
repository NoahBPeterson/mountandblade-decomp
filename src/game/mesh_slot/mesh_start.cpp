// mesh_start.cpp: TU 63 "mesh", part 8 (0x484590..0x4875e0): Mission::start_setup (0x484590, 12294 bytes) and the rounding helper
// that ends the TU (0x4875a0). start_setup is the per-mission start-up sequence:
//   reset mission state and seed the four clock/tick pairs from the global tick table,
//   pick the level + mode, accumulate the local temperature from the neighbouring scene objects,
//   derive the weather (temperature, humidity, height, fog distance/amount, rain amount/mode)
//   through the noise field, fill the terrain's sky/cloud/fog bit fields, choose the ambient sound
//   slot, create the sun, build the 64-slot light/frame table (+5 custom-camera slots), run the
//   prop/spawn pass, create the scene effect objects and the intro billboard/UI meshes, reset the
//   counters and the light table, and log "end_setup".
//
// The Mission object is accessed through at<T>(byte offset) so every offset stays the one in the
// disassembly; std::vector-like members are small local structs with the verified 16-byte layout
// (allocator lane at +0). The original opens with `and esp,-8` (an __int64 local) behind a
// WinLicense marker pair (`jmp +0x10` over 'WL  ' data at 0x4845a2 etc.); the markers are not
// reproduced (they are inline-asm _emit data, not source).
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../rgl/rgl_math.h"
#include "../../rgl/rgl_string.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// ---------------------------------------------------------------------------
// Local layouts (names provisional)
// ---------------------------------------------------------------------------
struct Raw { template<class T> T& at(int o) { return *(T*)((char*)this + o); } };
// the same displacement access for the shared classes (PlacedObj, Party), which have no Raw base
#define RAWAT(T, obj, off) (*(T*)((char*)(obj) + (off)))

using rgl::vector4;                                          // the engine's vector (match/include/rgl.h); the slice's own copy clashed with the shared headers' using-declarations
#include "../../include/vec.h"
#include "../../include/mat4.h"
using rgl::mat3;
// ---- the defining classes of the callees below ------------------------------------------------------------------------------
// This slice carries its own views (Mesh, Entity, World, terrain_obj, Psys, ...) of objects whose classes live in other modules and cannot
// be included here; the calls go to the members of the classes that define them, which are declared (members only) in this block.
#include "../../rgl/mesh.h"          // rgl::mesh
#include "../../rgl/particles/mesh_obj.h"   // before scene.h: rgl::emitter in full (scene/emitter.h declares only its update)
#include "../../rgl/scene/scene.h"      // resmgr (res_manager()), resource_manager g_resmgr 0x5d6fa0
struct skin; struct face; namespace rgl { struct world; } struct BodyRecC;
namespace rgl {
struct entity; struct scene;
unsigned __cdecl pack_color(const vector4* c);               // 0x40e440
}
struct Poser {
    void add_extra(int val);   // 0x442110 (agent3/agent_fx_sound.h)
};
struct ShadowMapP {
    void render();   // 0x47fef0 (mesh_char.cpp)
};
#include "../terrain_b/cell_grid.h"        // CellGrid (build 0x478c50), level (floor_at 0x439be0), PolyMesh (build_geometry 0x470a00, compute_edge_creases 0x470750)
struct snd_slot {
    void update(int arg);   // 0x4625d0 (ui/item_ref.cpp)
};
struct snd_bank {
    void randomize();   // 0x462790 (ui/item_ref.cpp)
};

struct RectF { float x0, y0, x1, y1; };
float __cdecl frand();                                       // 0x40e400
void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0

#include "../tstring.h"

#include "../placed_obj.h"
#include "../../include/level_global.h"      // g_level (0xa732b4)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
unsigned g_rand_seed;                               // 0x5d7078

#include <vector>
template<> std::vector<PlacedObj>& std::vector<PlacedObj>::operator=(const std::vector<PlacedObj>& o);   // 0x477160 (terrain_b/terrain_b.cpp: the out-of-line instance)
struct BodyRec { char b[0x24]; };
struct BodyVec {
    int alloc; BodyRec* first; BodyRec* last; BodyRec* end;
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
};
#include "../std_vec_insert_n.h"      // std::vector<int>::_Insert_n 0x410610 (the entity's mesh list at +0x118)

struct Mesh : Raw {                                          // rgl::mesh, 0x260 bytes
    char b[0x260];
    Mesh() { ((rgl::mesh*)this)->rgl::mesh::mesh(); }        // 0x412a90
    Mesh* clone() { return (Mesh*)((rgl::mesh*)this)->clone(); }   // 0x41c6d0
    void scale(float s) { ((rgl::mesh*)this)->scale(s); }    // 0x44f0c0
};
#include "../emit.h"
#include "../ptype.h"
#include "../../include/render_flags.h"
#include "../config/config_block.h"
#include "../shadow_map.h"


struct Entity : Raw {                                        // scene entity (vtable object)
    // Raw::at measures from the Raw base, which MSVC places after the vptr (+4) in a polymorphic class: every entity
    // field access through it landed 4 bytes high (the +0x118 mesh list was written at +0x11c.., leaving first = 0 and
    // last set, so the next push copied from address 0). Entity offsets are from the object start.
    template<class T> T& at(int o) { return *(T*)((char*)this + o); }
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6(float* p);
    virtual void v7(); virtual void v8(); virtual void v9(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15(); virtual void v16();
    virtual void v17(); virtual void v18(); virtual void v19(); virtual void v20(); virtual void v21();
    virtual void v22(); virtual void v23(); virtual void v24();
    virtual void v25(rgl::psys* e);
    void add_extra(Mesh* m) { ((Poser*)this)->add_extra((int)m); }     // 0x442110
};
struct PropObj {                                             // object at prop +0x1b4
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void v22(const rgl::matrix* m, const vector4* p);
};

#include "../../rgl/scene/world.h"                      // rgl::world: the 0x344-byte scene (step 0x427f40, reset 0x421390, create_entity 0x428440)
#include "../terrain_b/poly_mesh.h"
#include "../terrain_desc.h"
#include "../terrain_a/terrain.h"                         // Terrain, g_terrain (0xa76630)

// ---- data this module owns ----
Terrain g_terrain;                                  // 0xa76630 (the world-map terrain, used by Mission::start_setup: 0x845e7c8 bytes; constructed by Terrain::Terrain 0x4ce720)
struct Psys {
    void attach_camera(rgl::world* w) { ((ShadowMap*)this)->attach_camera((rgl::world*)w); }       // 0x4711e0
    void extend(BodyRec* b) { ((ShadowMap*)this)->extend((const vector4*)b); }          // 0x468c20
    void fit_bounds() { ((ShadowMap*)this)->fit_bounds(); }                              // 0x463cc0
    void render() { ((ShadowMapP*)this)->render(); }                                    // 0x47fef0
};

struct SndSlot {                                             // 0x78 bytes, array in the mode record bank
    char pad0[0xc]; int f0c; char pad10[0x78 - 0x10];
    void update(int what) { ((snd_slot*)this)->update(what); }         // 0x4625d0
};
struct SndBank {                                             // at mode record +0x4c
    int count; SndSlot* recs; int pad8;
    void randomize() { ((snd_bank*)this)->randomize(); }               // 0x462790
};
#include "../mode_rec.h"

#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../prop_kind.h"                                    // PropKind (g_propkinds records, 0x78 bytes; shared with terrain_b)

#include "../../include/party.h"

struct LevelBits {                                           // Level +0x30 flags (bit fields: tested with test ah,n)
    unsigned b0 : 1; unsigned pad1 : 7; unsigned b8 : 1; unsigned b9 : 1; unsigned b10 : 1; unsigned pad11 : 21;
};
struct LevelRaw : Raw {                                      // 0x494-byte level record (renamed from Level: game_data.h declares Level494 and other Level names)
    char pad0[0x30]; LevelBits flags; char rest[0x494 - 0x34];
};
#include "../../include/game_data.h"                           // GameData (g_game 0x5e2930; was a local GameState view with load_scene / blend)
#include "../../include/phys_set.h"

#include "../../rgl/stream.h"                           // g_log (log_stream)

struct SceneObjRec : Raw { char b[0x88]; };
struct SceneObjs { char pad0[0x14]; SceneObjRec* first; SceneObjRec* last; };    // g_level +0xb0
struct Scene { char pad0[0xb0]; SceneObjs* objs; };

struct SceneSlot {                                                // 0x130 bytes, 64 at Mission +0x670
    rgl::matrix A, B, C, D;
    char pad100[0x10];
    Timer timer;                                             // +0x110
    int f120;
    int pad124;
    void* face;                                              // +0x128
    int pad12c;
};

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
#include "../../include/fx.h"
extern ModeRec*   g_mission_templates;        // 0x5e2a00  mode table (0x58-byte records)
static inline SndBank* bank_of(ModeRec* m) { return (SndBank*)((char*)m + 0x4c); }   // the sound bank at mode record +0x4c
#include "../env_row.h"                               // EnvRow, g_env_rows (0x5e28f4)
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../prop_kind.h"
#include "../config/config_block.h"   // Config g_cfg (0x5a9f98)
extern unsigned __int64 g_bit[]; // 0x5a8e68

#include "../mission.h"

#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers

#define RND() ((float)(rand() % 0x3dc9))
#define RNDU(s) ((float)((s) % 0x3dc9))

static inline unsigned lcg(unsigned x) { return (x * 0x1c13 - 0xd098556) * 0x1105; }

static inline void put(unsigned& dst, const unsigned& mask, int shift, const int& val)
{
    unsigned v = (unsigned)val <= mask ? (unsigned)val : mask;
    dst = (dst & ~(mask << shift)) | (v << shift);
}

static inline void cross2(vector4& o, const vector4& a, const vector4& b)
{
    o.x = a.y * b.z - b.y * a.z;
    o.y = b.x * a.z - a.x * b.z;
    o.z = b.y * a.x - b.x * a.y;
}
static inline float& lane(vector4& v, int i) { return (&v.x)[i]; }
static inline float fmin2(float a, float b) { return a < b ? a : b; }
static inline float fmax2(float a, float b) { return a > b ? a : b; }

static inline void norm3(float* v)
{
    float len = (float)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1.00000001e-10f) {
        float inv = 1.0f / len;
        v[0] *= inv; v[1] *= inv; v[2] *= inv;
    } else {
        v[0] = 0.0f; v[1] = 1.0f; v[2] = 0.0f;
    }
}

static inline float clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;
    return x;
}

static inline int iround(float x)
{
    float f = floor(x);
    int r = (int)f;
    return (x - f <= 0.5f) ? r : r + 1;
}

static inline int ifloor(float x)
{
    return (int)(x > 0.0f ? x : x - 0.999999881f);
}

static inline Party* party_rec(PartyMgr* pm, int id)
{
    int i = pm->lookup(id);
    return &pm->pages[i >> 4][i & 15];
}

#define RECT(i) (((float*)this)[0x194f + (i)])
#define SLOT(i) (((SceneSlot*)((char*)this + 0x670))[i])
#define PROP(i) (at<std::vector<PlacedObj> >(0x6688)[i])
#define LV (*(LevelRaw*)(RAWAT(char*, g_game, 0x13a50) + at<int>(0x5df4) * 0x494))

// ---------------------------------------------------------------------------
// @ 0x484590
// ---------------------------------------------------------------------------
void Mission::start_setup()
{
    Mission* m = this;
    vector4 in, out, v;

    WL_MARKER(4)
    reset();
    at<Timer>(0x62e8).restamp();
    at<Timer>(0x62f8).restamp();
    at<Timer>(0x6308).restamp();
    at<Timer>(0x6318).restamp();
    ((stream_base&)g_log).w_cstr("begin_setup");
    g_phys.reset();

    ModeRec* mrec = &g_mission_templates[at<int>(0x5df0)];
    at<int>(0x6550) = 0;
    LevelRaw* lrec = &RAWAT(LevelRaw*, g_game, 0x13a50)[at<int>(0x5df4)];
    bank_of(mrec)->randomize();
    if (lrec->at<char>(0x78) == 0)
        g_game->load_scene(at<int>(0x5df4));

    // ---- camera party + neighbour temperature accumulation ---------------------------------
    PartyMgr* pm = &g_game->parties;
    int pid = RAWAT(int, g_game, 0x78);
    Party* pa = party_rec(pm, pid);
    float temp[16];
    std::vector<PlacedObj>& props = at<std::vector<PlacedObj> >(0x6688);
    {
    vector4 u;
    int ptype;
    ptype = RAWAT(int, party_rec(pm, pid), 0x5978);

    for (int t = 0; t < 16; t++) temp[t] = 0.0f;
    u = *(vector4*)&party_rec(pm, pid)->pos;

    if (g_script.level != 0) {
        for (unsigned p = (unsigned)((Scene*)g_script.level)->objs->first; p < (unsigned)((Scene*)g_script.level)->objs->last; p += 0x88) {
            float dx = *(float*)(p + 0x74) - u.x;
            float dy = *(float*)(p + 0x78) - u.y;
            float d2 = dy * dy + dx * dx;
            if (d2 < 1.0f)
                d2 = 1.0f;
            else if (!(d2 < 324.0f))
                continue;
            int slot = *(int*)(p + 0x24);
            if (slot >= 0 && slot < 16) {
                float& tr = temp[slot];
                tr = 10.0f / d2 + tr;
            }
        }
    } else {
        for (int t = 0; t < 16; t++) temp[t] = 0.2f;
    }

    WL_MARKER(5)
    // ---- day time, anchor blend, weather ---------------------------------------------------
    at<float>(0x65e8) = (float)fmod((double)(g_ticks[1] * 1e-5f), 24.0);
    g_game->blend(&u.x, pa->pos.x, pa->pos.y);
    WL_MARKER(0xc)

    float temper = temp[0] * 0.1f + (temp[8] * 0.01f + (temp[3] * 0.001f + (temp[11] * 0.002f +
                   (temp[4] * 0.005f + (temp[12] * 0.01f + (temp[1] * 0.003f + (temp[9] * 0.005f +
                   (temp[2] * 0.0005f + temp[10] * 0.0015f))))))));
    if (temper < 0.0f) temper = 0.0f;
    else if (temper >= 1.0f) temper = 1.0f;

    in.x = (float)RAWAT(int, g_game, 4) * 0.1f;
    in.z = 0.0f;
    in.y = g_ticks[1] * 1e-5f;
    noise_fractal(96.0f, 0.7f, 4, 0, &in.x, &out.x);
    float f14 = (out.z + out.y + out.x) * 5.0f;
    {
        v.z = 700.0f;
        v.x = (float)RAWAT(int, g_game, 4) * 0.1f;
        v.y = g_ticks[1] * 1e-5f;
        in = v;
    }
    noise_fractal(72.0f, 0.7f, 4, 0, &in.x, &out.x);
    float fo = (out.z + out.y + out.x) * 5.0f;

    float humidity = clamp01(((f14
 + f14) + 0.5f) * temper + (RND() * 6.32231167e-05f) * 0.0500000007f);
    at<float>(0x65f4) = humidity;
    at<int>(0x65f8) = RAWAT(int, pa, 0x38);
    at<int>(0x65f0) = *(int*)&u.x;
    at<int>(0x6618) = *(int*)&u.z;
    at<int>(0x661c) = *(int*)&u.w;
    at<int>(0x6600) = 0;
    at<int>(0x6604) = 0;

    at<int>(0x65fc) = iround(((temper + 1.5f) * RAWAT(float, g_game, 0x13b3c)) * 1.5f);

    float fog = ((RAWAT(float, g_game, 0x13b3c) + 0.3f) * (temper + 0.5f)) * RAWAT(float, g_game, 0x13b40) *
                RAWAT(float, g_game, 0x13b40) * 0.00800000038f + 9.99999975e-05f;
    at<float>(0x6614) = fog;
    if (RAWAT(float, g_game, 0x13b40) > 0.899999976f) {
        fog = fog * 1.29999995f + 0.0259999987f;
        at<float>(0x6614) = fog;
        if (RAWAT(float, g_game, 0x13b40) > 0.939999998f) {
            fog = fog * 1.20000005f + 0.0240000002f;
            at<float>(0x6614) = fog;
            if (RAWAT(float, g_game, 0x13b40) > 0.980000019f)
                at<float>(0x6614) = fog + 0.00999999978f;
        }
    }

    at<float>(0x6610) = (RND() * 3.16115584e-05f + 0.5f) * (1.20000005f - humidity) * 2000.0f;
    if (at<float>(0x65e8) > 3.0f && at<float>(0x65e8) < 10.0f)
        at<float>(0x6610) = (RND() * 4.42561795e-05f + 0.300000012f) * at<float>(0x6610);

    if (RAWAT(float, g_game, 0x13b3c) > 0.600000024f) {
        if (humidity * 0.100000001f + ((RND() * 6.32231167e-05f) * 0.0500000007f + RAWAT(float, g_game, 0x13b3c) * 0.200000003f) +
            fo > 0.460000008f) {
            if (ptype == 4 || ptype == 0xc)
                at<int>(0x6604) = 2;
            else
                at<int>(0x6604) = 1;
            float amt = RND() * 3.16115584e-05f + 0.5f;
            at<float>(0x6600) = amt;
            if (at<int>(0x65fc) < 2) at<int>(0x65fc) = 2;
            if (amt > 0.300000012f) at<int>(0x65fc) = 3;
            float ff = at<float>(0x6610) / (3.0f * amt + 1.0f);
            at<float>(0x6610) = ff;
            at<float>(0x6614) = amt * 0.00150000001f + at<float>(0x6614);
            if (at<int>(0x6604) == 2) {
                at<float>(0x6610) = ff * 0.5f;
                at<float>(0x6614) = at<float>(0x6614) * 1.5f + amt * 0.00374999992f;
            }
        }
    }
    if (at<float>(0x6610) < 60.0f) at<float>(0x6610) = 60.0f;
    if (at<float>(0x6614) > 0.150000006f) at<float>(0x6614) = 0.150000006f;

    WL_MARKER(0xd)
    {
        void* o = RAWAT(void*, pa, 0x5974);
        if (o != 0)
            at<int>(0x65ec) = *(int*)((char*)o + 4);
        else
            at<int>(0x65ec) = 0;
    }

    at<int>(0x6608) = ptype;
    props = *(std::vector<PlacedObj>*)((char*)lrec + 0x104);
    for (unsigned i = 0; i < props.size(); i++) {
        RAWAT(unsigned, &at<std::vector<PlacedObj> >(0x6688)[i], 0x14) = i;
    }

    g_script.param1 = ptype;

    {
        SndBank* b = bank_of(&g_mission_templates[at<int>(0x5df0)]);
        for (int i = 0; i < b->count; i++)
            b->recs[i].f0c = 0;
    }
    {
        SndBank* b = bank_of(&g_mission_templates[at<int>(0x5df0)]);
        for (int i = 0; i < b->count; i++)
            b->recs[i].update(-0x13);
    }
    at<tstring>(0x6814) = *(tstring*)((char*)lrec + 0x484);

    // ---- ambient sound slot selection -------------------------------------------------------
    for (int i = 0; i < g_num_ground_specs; i++) {
        EnvRow* s = &g_env_rows[i];
        float r = RND() * 6.32231149e-07f;
        s->rank = r;
        if (at<float>(0x65e8) > 7.0f && at<float>(0x65e8) < 17.0f) {
            if ((s->f10 & 3) != 0) goto no_bonus;
        } else if (at<float>(0x65e8) > 4.0f && at<float>(0x65e8) < 20.0f) {
            if ((s->f10 & 3) != 1) goto no_bonus;
        } else {
            if ((s->f10 & 3) != 2) goto no_bonus;
        }
        s->rank = r + 1.0f;
    no_bonus:
        s->rank = s->rank - abs(at<int>(0x65fc) - ((s->f10 >> 4) & 0xf)) * 0.100000001f;
    }
    at<int>(0x660c) = 0;
    for (int i = 1; i < g_num_ground_specs; i++)
        if (g_env_rows[i].rank > g_env_rows[at<int>(0x660c)].rank)
            at<int>(0x660c) = i;

    // ---- sky / cloud / fog bit fields -------------------------------------------------------
    if (LV.flags.b8) {
        g_terrain.parse(*(const tstring*)((char*)(RAWAT(char*, g_game, 0x13a50) + at<int>(0x5df4) * 0x494) + 0x68));
        GameData* gg = g_game;
        if (((LevelRaw*)(RAWAT(char*, gg, 0x13a50) + at<int>(0x5df4) * 0x494))->flags.b9) {
            PartyMgr* pm2 = &gg->parties;
            int pid2 = RAWAT(int, gg, 0x78);
            int x = (int)party_rec(pm2, pid2)->pos.x;
            int y = (int)party_rec(pm2, pid2)->pos.y;
            int parity = at<int>(0x5270) % 3;
            int fr = RAWAT(int, gg, 4);
            int a = fr * 0x1b0b + parity + y * 0x230b + x * 17;
            int c = fr * 0xf4d + parity + y * 0x16a1 + x * 19;
            int b = y * 0x19c9 + parity + fr * 0x1e6f + x * 31;
            a &= 0x7fffff; c &= 0x7fffff; b &= 0x7fffff;

            unsigned mc = (unsigned)g_bit[31] - 1;
            put(g_terrain.bits[0], mc, 0, a);
            put(g_terrain.bits[2], mc, 0, c);
            put(g_terrain.bits[1], mc, 0, b);


            int storm = 0;
            unsigned seed = (at<int>(0x5270) % 3 + RAWAT(int, g_game, 4) * 0x4abb + y * 0xb1 + x * 0x17 + 0x8a767) *
                            0x1ddcf5f + 0x2f784719;
            float chance = RNDU(seed) * 6.32231167e-05f;
            if (ptype == 7) {
                storm = 1;
                ptype = 3;
            } else if (temp[8] > 27.0f) {
                storm = 1;
            } else if ((ptype == 3 || ptype == 0xb) && chance < 0.150000006f) {
                storm = 1;
            }
            put(g_terrain.bits[4], (unsigned)g_bit[4] - 1, 28, ptype);
            {
                unsigned m1 = (unsigned)g_bit[1] - 1;
                unsigned v1 = (unsigned)(storm != 0) <= m1 ? (unsigned)(storm != 0) : m1;
                unsigned w = g_terrain.bits[3];
                seed = lcg(seed);
                w = (w & ~(m1 << 31)) | (v1 << 31);
                w &= ~(m1 << 30);
                g_terrain.bits[3] = w;
            }
            g_rand_seed = seed;
            float ff = RNDU(seed) * 6.32231149e-06f + 0.449999988f;
            if (storm) {
                seed = lcg(seed);
                g_rand_seed = seed;
                ff = ff + RNDU(seed) * 6.32231149e-06f + 0.349999994f;
            } else if (ptype == 0xb || ptype == 0xc) {
                seed = lcg(seed);
                g_rand_seed = seed;
                ff = ff - (RNDU(seed) * 1.2644623e-05f + 0.25f);
            } else if (temp[1] > 35.0f) {
                ff -= 0.300000012f;
            }
            if (ff < 0.0f) ff = 0.0f;
            else if (ff > 1.0f) ff = 1.0f;
            int ai = iround(ff * 100.0f);
            if (ai < 0) ai = 0;
            else if (ai >= 100) ai = 99;
            put(g_terrain.bits[4], (unsigned)g_bit[7] - 1, 0, ai);

            seed = lcg(g_rand_seed);
            int cc = (int)(RNDU(seed) * 0.00126446225f + 12.0f);
            seed = lcg(seed);
            if (ptype == 0xb) {
                cc = ifloor(RNDU(seed) * 0.00189669349f) + 30;
            } else if (ptype == 0xc) {
                cc = ifloor(RNDU(seed) * 0.00158057781f) + 25;
            } else {
                g_rand_seed = seed;
                cc += iround((RNDU(seed) * 6.32231167e-05f + 0.400000006f) * temp[1] * 0.449999988f);
            }
            if (cc > 0x41) cc = 0x41;
            put(g_terrain.bits[4], (unsigned)g_bit[7] - 1, 14, cc);

            seed = lcg(seed);
            g_rand_seed = seed;
            float tot = RNDU(seed) * 0.000948346744f + 10.0f;
            float tb = (temp[12] + temp[11]) * 0.349999994f + temp[1] * 0.600000024f;
            if (tb < 0.0f) tb = 0.0f;
            else if (tb >= 50.0f) tb = 50.0f;
            tot = tot + tb;
            if (storm) tot = tot * 0.5f;
            int ti = ifloor(tot);
            if (ti < 0) ti = 0;
            else if (ti >= 0x80) ti = 0x7f;
            put(g_terrain.bits[4], (unsigned)g_bit[7] - 1, 7, ti);

            int ri = iround((RND() * 1.2644623e-05f + 0.800000012f) * 100.0f);
            if (ri < 0) ri = 0;
            else if (ri >= 100) ri = 99;
            put(g_terrain.bits[4], (unsigned)g_bit[7] - 1, 21, ri);
        }
        {
            vec2 z;
            z.x = 0.0f;
            z.y = 0.0f;
            at<vec2>(0x652c) = z;

        }
        at<float>(0x6534) = (float)(g_terrain.bits[3] & ((unsigned)g_bit[10] - 1));
        at<float>(0x6538) = (float)((g_terrain.bits[3] >> 10) & ((unsigned)g_bit[10] - 1));
        at<int>(0x653c) = at<int>(0x652c);
        at<int>(0x6540) = at<int>(0x6530);
        at<int>(0x6544) = at<int>(0x6534);
        at<int>(0x6548) = at<int>(0x6538);
        at<float>(0x654c) = -0.300000012f;
    } else {
        at<int>(0x654c) = lrec->at<int>(0x64);
        at<int>(0x653c) = lrec->at<int>(0x54);
        at<int>(0x6540) = lrec->at<int>(0x58);
        at<int>(0x6544) = lrec->at<int>(0x5c);
        at<int>(0x6548) = lrec->at<int>(0x60);
        at<int>(0x652c) = at<int>(0x653c);
        at<int>(0x6530) = at<int>(0x6540);
        at<int>(0x6534) = at<int>(0x6544);
        at<int>(0x6538) = at<int>(0x6548);
    }
    g_terrain.tail = at<tstring>(0x6814);


    // ---- prop bounding box, two passes ------------------------------------------------------
    {
    vector4 hi;
    for (unsigned i = 0; i < at<std::vector<PlacedObj> >(0x6688).size(); i++) {
        PlacedObj* p = &at<std::vector<PlacedObj> >(0x6688)[i];
        if (RAWAT(int, p, 0x188) == 0 && RAWAT(int, p, 0x18c) >= 0) {
            PropKind* k = &g_propkinds[RAWAT(int, p, 0x18c)];
            v.x = (RAWAT(float, p, 0x18) * k->f58 + (RAWAT(float, p, 0x28) * k->f5c + RAWAT(float, p, 0x38) * k->fx60)) + RAWAT(float, p, 0x48);
            v.y = (RAWAT(float, p, 0x1c) * k->f58 + (RAWAT(float, p, 0x2c) * k->f5c + RAWAT(float, p, 0x3c) * k->fx60)) + RAWAT(float, p, 0x4c);
            v.z = (RAWAT(float, p, 0x20) * k->f58 + (RAWAT(float, p, 0x30) * k->f5c + RAWAT(float, p, 0x40) * k->fx60)) + RAWAT(float, p, 0x50);
            hi = v;
            float lx = v.x - k->f68;
            v.y -= k->f68; hi.x += k->f68; hi.y += k->f68;
            at<float>(0x652c) = fmin2(at<float>(0x652c), lx);
            at<float>(0x6530) = fmin2(at<float>(0x6530), v.y);
            at<float>(0x6534) = fmax2(at<float>(0x6534), hi.x);
            at<float>(0x6538) = fmax2(at<float>(0x6538), hi.y);
        }
    }
    for (unsigned i = 0; i < at<std::vector<PlacedObj> >(0x6688).size(); i++) {
        PlacedObj* p = &at<std::vector<PlacedObj> >(0x6688)[i];
        if (RAWAT(int, p, 0x188) == 0 && RAWAT(int, p, 0x18c) >= 0) {
            PropKind* k = &g_propkinds[RAWAT(int, p, 0x18c)];
            v.x = (RAWAT(float, p, 0x18) * k->f58 + (RAWAT(float, p, 0x38) * k->fx60 + RAWAT(float, p, 0x28) * k->f5c)) + RAWAT(float, p, 0x48);
            v.y = (RAWAT(float, p, 0x2c) * k->f5c + (RAWAT(float, p, 0x3c) * k->fx60 + RAWAT(float, p, 0x1c) * k->f58)) + RAWAT(float, p, 0x4c);
            v.z = (RAWAT(float, p, 0x30) * k->f5c + (RAWAT(float, p, 0x40) * k->fx60 + RAWAT(float, p, 0x20) * k->f58)) + RAWAT(float, p, 0x50);
            hi = v;
            float lx = v.x - k->f68;
            v.y -= k->f68; hi.x += k->f68; hi.y += k->f68;
            at<float>(0x652c) = fmin2(at<float>(0x652c), lx);
            at<float>(0x6530) = fmin2(at<float>(0x6530), v.y);
            at<float>(0x6534) = fmax2(at<float>(0x6534), hi.x);
            at<float>(0x6538) = fmax2(at<float>(0x6538), hi.y);
        }
    }
    }

    // ---- fade, params, ambient, sun ---------------------------------------------------------
    rebuild_nav(at<int>(0x5df4), at<int>(0x5df0));
    {
        unsigned* fc = &g_env_rows[at<int>(0x660c)].f44;
        at<int>(0x5274) = 0;
        at<rgl::world*>(0)->set_fade(&at<float>(0x6614), (const int*)fc);   // 0x4624c0
    }
    {
        unsigned color = 0xff334455;
        at<rgl::world*>(0)->set_params(-5.0f, 10.0f, (const int*)&color, &at<float>(0x654c));     // 0x462540
    }
    setup_ambient(at<int>(0x5df4), at<int>(0x660c), at<float>(0x6614));

    Entity* sun = (Entity*)((rgl::world*)at<rgl::world*>(0))->create_entity(9);
    Mesh* sm = (Mesh*)g_resmgr.get_object(&g_env_rows[at<int>(0x660c)].name)->clone();
    sm->scale(200.0f);
    sm->at<unsigned>(0x10c) &= 0xfffeffff;
    {
        int mi = (int)sm;
        sun->at<std::vector<int> >(0x118).push_back(mi);
    }
    sun->v4();
    {
        GameData* g1 = g_game;
        int secs = (int)((g_ticks[RAWAT(int, g1, 0x13b18)] - RAWAT(__int64, g1, 0x13b20)) * 1e-5f);
        int h = secs % 24;
        float angle = g_env_rows[at<int>(0x660c)].f14 * 0.0174532924f -
                      (float)(h - 12) * 0.0174532924f * 15.0;
        sun->at<rgl::matrix>(0x58).rotate_about_u(&angle);
        angle = 1.0e8f;
        sun->v6(&angle);
    }
    at<rgl::world*>(0)->sky = (rgl::entity*)sun;

    // ---- terrain generate / interior ----------------------------------------------------------
    if (LV.flags.b8) {
        g_terrain.sun_ambient = at<rgl::world*>(0)->ambient;
        g_terrain.sun_diffuse = *(vector4*)(at<char*>(0x65b0) + 4);
        stream_base* s414 = (stream_base*)((char*)lrec + 0x414);
        char flagc = 1;
        s414->rewind();
        if (s414->at_end() == 0) {
            if (LV.flags.b8) {
                stream_base* s2 = (stream_base*)(RAWAT(char*, g_game, 0x13a50) + at<int>(0x5df4) * 0x494 + 0x414);
                s2->rewind();
                at<TerrainDesc>(0x63d0).read(s2);
            }
            flagc = 0;
        }
        g_terrain.generate((int)at<rgl::world*>(0), (int*)&at<TerrainDesc>(0x63d0), flagc);
    } else {
        build_interior(at<int>(0x5df4), (rgl::mesh**)&at<Mesh*>(0x6340), &at<int>(0x633c));
    }
    stream_base* s444 = (stream_base*)((char*)lrec + 0x444);
    s444->rewind();
    PolyMesh& nav = at<PolyMesh>(0x6698);
    nav.load(s444);
    nav.build_geometry();
    {
        LevelBits f2 = LV.flags;
        if (f2.b9 && f2.b8 && g_terrain.count_low_quads() > 10) {
            nav.build(at<rgl::world*>(0)->hf, 1);
            nav.build_geometry();
            nav.compute_edge_creases();
        }
    }
    ((rgl::world*)at<rgl::world*>(0))->build(&nav);   // 0x478c50

    // ---- light/frame table: 64 slots + 5 extra for a custom camera --------------------------
    at<int>(0x5278) = 0x40;
    at<int>(0x5274) = 0;
    u.x = 0.0f; u.y = 0.0f;
    {
    vec2 ext = { at<float>(0x6544) - at<float>(0x653c), at<float>(0x6548) - at<float>(0x6540) };
    vec2 mid;
    mid.x = ext.x * 0.5f;
    mid.y = ext.y * 0.5f;
    mid.x = mid.x + at<float>(0x653c);
    mid.y = mid.y + at<float>(0x6540);
    if (lrec->flags.b8) *(vec2*)&u = mid;

    }
    {
    v.z = 500.0f;
    int k = 0;
    do {
        mat3 tmp;
        *(mat3*)&SLOT(k).A = *identity3(&tmp);
        vec2 g = { (float)(k % 8 + 1), (float)(k / 8 + 1) };
        v.x = g.x + u.x;
        v.y = g.y + u.y;
        SLOT(k).A.o = v;
    } while (++k < 0x40);
    }

    {
    int pi;
    if (LV.flags.b10) {
        at<int>(0x5278) = 5;
        int par = rand() % 2;
        for (int s = 0; s < 5; s++) {
            SceneSlot* sl = &SLOT(s);
            lane(v, 1 - par) = ((RND() * 6.32231167e-05f) * 0.400000006f + 0.300000012f) * (RECT(3 - par) - RECT(1 - par)) + RECT(1 - par);
            float b = (((RND() * 6.32231167e-05f - 0.5f) * 0.100000001f + s) * 0.25f) * 0.600000024f + 0.200000003f;
            if (s < 2)
                b = b * 0.75f;
            else if (s > 2)
                b = 1.0f - (1.0f - b) * 0.75f;
            lane(v, par) = b * (RECT(2 + par) - RECT(par)) + RECT(par);
            v.z = 1000.0f;
            mat3 tmp;
            rgl::matrix* a = &sl->A;



            *(mat3*)a = *identity3(&tmp);
            a->o = v;
            u.z = 0.0f;
            lane(u, 1 - par) = 0.0f;
            if (s < 2) lane(u, par) = 1.0f;
            else lane(u, par) = -1.0f;
            a->f = u;
            a->orthonormalize();
            sl->B = *a;

        }
    }
    if (LV.flags.b8) {
        at<float>(0x653c) = at<float>(0x653c) + 24.0f;
        at<float>(0x6540) = at<float>(0x6540) + 24.0f;
        at<float>(0x6544) = at<float>(0x6544) - 24.0f;
        at<float>(0x6548) = at<float>(0x6548) - 24.0f;
    }

    // ---- per-prop frame copy into the table ----------------------------------------------------
    {
    vector4 t20;
    for (int ip = 0; (unsigned)ip < at<std::vector<PlacedObj> >(0x6688).size(); ip++) {
        PlacedObj* p = &PROP(ip);
        if (RAWAT(int, p, 0x188) == 1) {
            int sl = RAWAT(int, p, 0x1a0);
            if (sl >= 0 && sl < 0x40) {
                SLOT(sl).A.o = RAWAT(vector4, p, 0x48);
                *(mat3*)&SLOT(sl).A = RAWAT(mat3, &PROP(ip), 0x18);
                SLOT(sl).B = SLOT(sl).A;
                u.x = 0.0f; u.y = 0.0f; u.z = 1.0f;
                vector4* up = &SLOT(sl).B.u;
                *up = u;
                vector4* sd = &SLOT(sl).B.s;
                v.x = up->z * SLOT(sl).B.f.y - up->y * SLOT(sl).B.f.z;
                v.y = SLOT(sl).B.f.z * up->x - SLOT(sl).B.f.x * up->z;
                v.z = SLOT(sl).B.f.x * up->y - up->x * SLOT(sl).B.f.y;
                *sd = v;
                norm3(&sd->x);
                cross2(t20, *up, *sd);
                SLOT(sl).B.f = t20;
                SLOT(sl).C = SLOT(sl).B;
                SLOT(sl).D = SLOT(sl).B;
            }
        }
    }
    }

    // ---- prop ground / spawn pass --------------------------------------------------------------
    {
    for (pi = 0; (unsigned)pi < at<std::vector<PlacedObj> >(0x6688).size(); ) {
        RAWAT(int, &PROP(pi), 0x1b4) = 0;
        if (g_cfg.enable_edit_mode == 0 && RAWAT(int, &PROP(pi), 0x188) == 4 && LV.flags.b8) {
            PlacedObj* p = &PROP(pi);
            int j;
            for (j = 0; j < g_ptype_count; j++)
                if (g_ptypes[j].name.equals(RAWAT(tstring, p, 0x190)))   // 0x4866c0: add esi, 0x20
                    goto found;
            j = 0;
        found:
            g_terrain.emit_by_type(j, (const rgl::matrix*)&RAWAT(rgl::matrix, p, 0x18), &RAWAT(vector4, p, 0x158), RAWAT(int, p, 0x1a0));
            pi = pi + 1;
        } else {
            int foundv = -1;
            spawn_prop(&pi, &foundv);
            pi = pi + 1;
        }
    }
    }
    update_prop_flags();
    {
        LevelBits f = LV.flags;
        if (f.b8)
            g_terrain.regenerate_water(&at<void*>(0x65a8), f.b9 == 0);
    }
    light_props();
    if (!LV.flags.b8)
        light_entities((rgl::mesh**)&at<Mesh*>(0x6340), at<int>(0x633c));
    at<int>(0x65bc) = 0;
    at<int>(0x65b8) = 0;

    // ---- scene effect object (rain / snow emitter) ---------------------------------------------
    if ((*(int*)((char*)&g_options + 0x20)) != 0 && !lrec->flags.b0 && at<int>(0x6604) > 0) {
        vector4 t20;
        u = *(vector4*)g_env_rows
[at<int>(0x660c)].base;
        float scale = (u.x * 3.0f + 1.0f) * 0.400000006f;
        Entity* fx = (Entity*)((rgl::world*)at<rgl::world*>(0))->create_entity(9);
        at<Entity*>(0x65b8) = fx;
        u.x = 0.0f; u.y = 0.0f; u.z = 30.0f;
        fx->at<vector4>(0x88) = u;
        {
        float far_clip = 1.0e7f;
        at<Entity*>(0x65b8)->v6(&far_clip);
        }
        rgl::matrix mx;
        u.x = 0.0f; u.y = 0.0f; u.z = 0.0f;
        mat3 tmp;
        *(mat3*)&mx.s = *identity3(&tmp);
        t20.x = at<float>(0x6618) * 0.349999994f;
        t20.y = at<float>(0x661c) * 0.349999994f;
        mx.o = u;
        t20.z = -10.0f;
        int ix;
        if (at<int>(0x6604) == 1) {
            mx.o.z = 6.0f;
            mx.o.y = 6.5f;
            ix = g_emit_defs.used[0];
            if (ix >= 0 && ix < g_emit_defs.count) {
                int owner0 = 0;
                rgl::psys* em = new rgl::psys(owner0);
                g_emit_defs.setup(em, ix, (const rgl::vector4*)&mx.o);
                if (em != 0) {
                    float inv = 1.0f / em->f24;
                    u.x = t20.x * inv;
                    u.y = t20.y * inv;
                    u.z = inv * -10.0f;
                    em->v184 = u;
                    em->vc4 = t20;
                    em->vb4 = t20;
                    norm3(&em->vb4.x);
                    em->f198 *= at<float>(0x6600);
                    RAWAT(float, em, 0xa0) = 11.0f / (float)(10.0 / em->f24);   // a float in the original (0x486a..: fstp [esi+0xa0]); psys::fa0 is declared int
                    for (int c = 2; c < 8; c++)
                        em->curve[c].v = scale * em->curve[c].v;
                    at<Entity*>(0x65b8)->v25(em);
                }
            }
        } else if (at<int>(0x6604) == 2) {
            mx.o.z = 6.0f;
            mx.o.y = 6.5f;
            ix = g_emit_defs.used[1];

            if (ix >= 0 && ix < g_emit_defs.count) {
                int owner0 = 0;
                rgl::psys* em = new rgl::psys(owner0);
                g_emit_defs.setup(em, ix, (const rgl::vector4*)&mx.o);
                if (em != 0) {
                    em->vc4 = t20;
                    for (int c = 2; c < 8; c++)
                        em->curve[c].v = scale * em->curve[c].v;
                    em->vb4 = t20;
                    norm3(&em->vb4.x);
                    em->f198 *= at<float>(0x6600);
                    at<Entity*>(0x65b8)->v25(em);
                }
            }
        }
    }

    }
    }
    // ---- intro billboard (sun mesh) ---
----------------------------------------------------------
    at<int>(0x65c0) = 0;
    at<int>(0x65c4) = 0;
    at<int>(0x65c8) = 0;
    if (!lrec->flags.b0 && (*(int*)((char*)&g_options + 0x20)) != 0) {
        Entity* eb = (Entity*)((rgl::world*)at<rgl::world*>(0))->create_entity(9);
        at<Entity*>(0x65c0) = eb;
        {
        float two = 2.0f;
        eb->v6(&two);
        }
        at<Mesh*>(0x65c4) = (Mesh*)new rgl::mesh;
        RAWAT(unsigned, at<Mesh*>(0x65c4), 0x10c) &= 0xfffeffff;
        RAWAT(int, at<Mesh*>(0x65c4), 0x114) = 1;
        RAWAT(int, at<Mesh*>(0x65c4), 0x140) = 1;
        {
            Entity* e2 = at<Entity*>(0x65c0);
            int mi = (int)at<Mesh*>(0x65c4);
            RAWAT(std::vector<int>, e2, 0x118).push_back(mi);
            e2->v4();
        {
            tstring nm("sun");
            at<Mesh*>(0x65c8) = (Mesh*)g_resmgr.get_object(&nm)->clone();
        }
        }
        RAWAT(int, at<Mesh*>(0x65c8), 0x114) = 1;
        RAWAT(int, at<Mesh*>(0x65c8), 0x140) = 1;
        float c = (1.0f - 150.0f / (at<float>(0x6610) + 150.0f)) * g_env_rows[at<int>(0x660c)].size * 0.899999976f;
        {
        vector4 u;
        u.x = u.y = u.z = c;
        RAWAT(unsigned, at<Mesh*>(0x65c8), 0x3c) = rgl::pack_color(&u);
        }

        RAWAT(unsigned, at<Mesh*>(0x65c8), 0x3c) |= 0xff000000;
        RAWAT(unsigned, at<Mesh*>(0x65c8), 0x10c) &= 0xfffeffff;
        at<Mesh*>(0x65c8)->scale(50.0f);
        {
            Entity* e2 = at<Entity*>(0x65c0);
            int mi = (int)at<Mesh*>(0x65c8);



            RAWAT(std::vector<int>, e2, 0x118).push_back(mi);
            e2->v4();
        }
    }

    // ---- cursor / vertex-colour helper entities -------------------------------------------------
    at<int>(0x65d0) = 0;
    at<Entity*>(0x65d0) = (Entity*)((rgl::world*)at<rgl::world*>(0))->create_entity(9);
    {
        float f290 = 290.5f;
        at<Entity*>(0x65d0)->v6(&f290);
    }
    {
        tstring nm("edit_cursor");
        at<Mesh*>(0x65d4) = (Mesh*)g_resmgr.get_object(&nm)->clone();
    }
    {
        Entity* e2 = at<Entity*>(0x65d0);
        int mi = (int)at<Mesh*>(0x65d4);
        RAWAT(std::vector<int>, e2, 0x118).push_back(mi);
        e2->v4();
    }
    at<Mesh*>(0x65d8) = (Mesh*)new rgl::mesh;
    {
        tstring nm("vertex_color_add_mat");
        RAWAT(void*, at<Mesh*>(0x65d8), 0x2c) = g_resmgr.get_material(nm);
    }
    at<Entity*>(0x65d0)->add_extra(at<Mesh*>(0x65d8));
    at<int>(0x65dc) = 0;
    at<Entity*>(0x65dc) = (Entity*)((rgl::world*)at<rgl::world*>(0))->create_entity(9);
    {
        float f33 = 33.5f;
        at<Entity*>(0x65dc)->v6(&f33);
    }
    at<Mesh*>(0x65e0) = (Mesh*)new rgl::mesh;
    {
        tstring nm("vertex_color_add_mat");
        RAWAT(void*, at<Mesh*>(0x65e0), 0x2c) = g_resmgr.get_material(nm);
    }
    at<Entity*>(0x65dc)->add_extra(at<Mesh*>(0x65e0));
    at<Entity*>(0x65e4) = (Entity*)((rgl::world*)at<rgl::world*>(0))->create_entity(9);
    {
        float f33 = 33.5f;
        at<Entity*>(0x65e4)->v6(&f33);
    }
    RAWAT(char, at<Entity*>(0x65d0), 0x140) = 0;
    RAWAT(char, at<Entity*>(0x65dc), 0x140) = 0;
    RAWAT(char, at<Entity*>(0x65e4), 0x140) = 0;

    // ---- counters, light table ground snap -------------------------------------------------------
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 6; j++) {
            at<int>(0x621c + (i * 6 + j) * 4) = 0;
            at<float>(0x62dc) = g_ticks[2] * 1e-5f;
        }
    at<int>(0x6524) = 0;
    at<char>(0x6338) = 0;
    for (int k = 0; k < 0x40; k++) {
        float outY;
        Timer* tm = &SLOT(k).timer;
        void** fp = &SLOT(k).face;
        *fp = 0;
        int n = 0;
        do {
            ((rgl::world*)at<rgl::world*>(0))->floor_at(&SLOT(k).B.o.x, &outY, (face**)fp);     // 0x439be0
            if (*fp == 0) {
                SLOT(k).B.o.x = (frand() - 0.5f) * 0.01f + SLOT(k).B.o.x;
                SLOT(k).B.o.y = (frand() - 0.5f) * 0.01f + SLOT(k).B.o.y;
                n++;
                if (n > 100) {
                    SLOT(k).B.o.x = (frand() - 0.5f) * 100.0f;
                    SLOT(k).B.o.y = (frand() - 0.5f) * 100.0f;
                    if (n > 10000)
                        *fp = *RAWAT(void**, at<rgl::world*>(0), 0x58);
                }
            }
        } while (*fp == 0);
        SLOT(k).B.o.z = outY;
        SLOT(k).C = SLOT(k).B;
        SLOT(k).D = SLOT(k).B;
        tm->restamp();
        SLOT(k).f120 = 0;
        copy_slot(k);
    }

    // ---- party spawn counters, troops, mode records ----------------------------------------------
    {
        int* lists = &at<int>(0x5e0c);
        int* cnts = &at<int>(0x620c);
        for (int sd = 0; sd < 2; sd++)
            for (int i = 0; i < cnts[sd]; i++)
                reset_spawned(lists[sd * 128 + i]);
    }
    for (int i = 0; i < RAWAT(int, g_game, 0x13a04); i++)
        *(int*)(RAWAT(char*, g_game, 0x13a00) + i * 0x508 + 0x3bc) = 0;
    memset(&at<char>(0x6824), 0, 0x40);
    for (int i = 0; i < mrec->nrecs; i++) {
        mrec->recs[i].f18 = 0;
        mrec->recs[i].f1c = 0;
    }
    for (int i = 0; i < mrec->nrecs; i++)
        add_amount(i, mrec->recs[i].f14);
    run_spawns();
    char buf[0x50];
    sprintf(buf, "temper = %3.2f, humidity = %3.2f, height = %3.2f fog_far = %d",
            (double)at<float>(0x65f0), (double)at<float>(0x65f4), (double)at<float>(0x65f8), (int)at<float>(0x6610));
    for (int i = 0; i < 2; i++)
        frame(0.100000001f, 1);

    // ---- placed-prop jitter + ground snap ----------------------------------------------------------
    for (unsigned i = 0; i < props.size(); i++) {
        PlacedObj* p = &at<std::vector<PlacedObj> >(0x6688)[i];
        if (RAWAT(int, p, 0x188) == 0 && (g_propkinds[RAWAT(int, p, 0x18c)].flags & 0x1000)) {
            float outY;
            void* sink;
            v = at<vector4>(0x6598);
            v.x = (RND() * 6.32231167e-05f - 0.5f) * 2.0f + v.x;
            v.y = (RND() * 6.32231167e-05f - 0.5f) * 2.0f + v.y;
            ((rgl::world*)at<rgl::world*>(0))->floor_at(&v.x, &outY, (face**)&sink);
            v.z = outY;
            RAWAT(vector4, p, 0x48) = v;
            if (RAWAT(PropObj*, p, 0x1b4) != 0)
                RAWAT(PropObj*, p, 0x1b4)->v22(&RAWAT(rgl::matrix, p, 0x18), &RAWAT(vector4, p, 0x158));
        }
    }

    // ---- water / particle system teardown or rebuild --------------------------------------------------
    {
        LevelBits f3 = LV.flags;
        if (f3.b8) {
            g_terrain.destroy_water((int)&at<char>(0x63c0));
        } else if (!f3.b0 && g_use_shaders) {
            ((Psys*)&g_shadow_map)->attach_camera(at<rgl::world*>(0));
            for (unsigned i = 0; i < RAWAT(BodyVec*, at<rgl::world*>(0), 0xb0)->size(); i++)
                ((Psys*)&g_shadow_map)->extend(&RAWAT(BodyVec*, at<rgl::world*>(0), 0xb0)->first[i]);
            ((Psys*)&g_shadow_map)->fit_bounds();
            ((Psys*)&g_shadow_map)->render();
        }
    }
    ((stream_base&)g_log).w_cstr("end_setup");
    at<char>(0x6555) = 0;
}

// ---------------------------------------------------------------------------
// @ 0x4875a0 (same code as round_float 0x432540; a long long is what keeps the add/adc pair)
// ---------------------------------------------------------------------------
long long __cdecl near_round(const float* p)
{
    float fl = (float)floor((double)*p);
    long long i = (long long)fl;
    if (*p - fl <= 0.5f)
        return i;
    return i + 1;
}
