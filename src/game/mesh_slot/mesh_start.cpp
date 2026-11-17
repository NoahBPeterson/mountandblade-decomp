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
// The Mission object is accessed through its members (game/mission.h); the fields mission.h does not
// declare yet go through the named accessors above start_setup. The original opens with `and esp,-8` (an __int64 local) behind a
// WinLicense marker pair (`jmp +0x10` over 'WL  ' data at 0x4845a2 etc.); the markers are not
// reproduced (they are inline-asm _emit data, not source).
#include <math.h>
#include "../../include/layout32.h"
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
#include "../std_vec_insert_n.h"      // std::vector<int>::_Insert_n 0x410610 (the entity's mesh list at +0x118)
// stand-in for std::vector<int>::push_back: the toolkit's _Ufill is a trip-count-1 fill loop, whose unroll makes cl fold the page loads of this
// whole function (docs/codegen.md 6.6); the original's code is exactly this store and step (call 0x410610 on the slow path).
struct MeshList : std::vector<int> {
    void push(const int& v) { if (size() < capacity()) { int* q = _Mylast; *q = v; _Mylast = q + 1; } else insert(end(), v); }
};

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
    void add_extra(rgl::mesh* m) { ((Poser*)this)->add_extra((int)m); }     // 0x442110
    void add_mesh(rgl::mesh* m) { int mi = (int)m; at<MeshList>(0x118).push(mi); v4(); }
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
    void extend(NavVert* b) { ((ShadowMap*)this)->extend(&b->p); }          // 0x468c20
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
struct LevelRaw {                                            // the 0x494-byte level record (game_data.h Level494) with the fields start_setup reads
    char pad0[0x30];
    LevelBits flags;                                         // +0x30 (b0: no weather / sun, b8: outdoor terrain, b9: random weather, b10: custom camera slots)
    char pad34[0x54 - 0x34];
    float rect[4];                                           // +0x54 play-area rectangle (min x, min y, max x, max y) of an interior
    float base_z;                                            // +0x64 its ground height
    tstring terrain_code;                                    // +0x68 (Terrain::parse)
    char loaded;                                             // +0x78 (0: GameData::load_scene first)
    char pad79[0x104 - 0x79];
    std::vector<PlacedObj> props;                            // +0x104 the scene props (copied to Mission::props)
    char pad114[0x414 - 0x114];
    char terrain_stream[0x444 - 0x414];                      // +0x414 the terrain description (a stream_base: rewind / at_end / TerrainDesc::read)
    char nav_stream[0x484 - 0x444];                          // +0x444 the ai mesh (a stream_base: PolyMesh::load)
    tstring f484;                                            // +0x484 (copied to Mission::str and Terrain::tail)
};
MB_LAYOUT32_ASSERT(LevelRaw_size_check, sizeof(LevelRaw) == 0x494);
#include "../../include/game_data.h"                           // GameData (g_game 0x5e2930; was a local GameState view with load_scene / blend)
#include "../../include/phys_set.h"

#include "../../rgl/stream.h"                           // g_log (log_stream)

struct SceneObjRec : Raw { char b[0x88]; };
struct SceneObjs { char pad0[0x14]; SceneObjRec* first; SceneObjRec* last; };    // g_level +0xb0
struct Scene { char pad0[0xb0]; SceneObjs* objs; };


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

static inline void put(unsigned& dst, const unsigned mask, int shift, const int& val)
{
    unsigned v = (unsigned)val <= mask ? (unsigned)val : mask;
    dst &= ~(mask << shift);
    dst |= v << shift;
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
    return pm->pages[i >> 4] + (i & 15);
}

// stand-in for `new rgl::psys(0)`: the inline ctor's vector::_Buy(0) makes cl fold this function's page loads (docs/codegen.md 6.6). Same stores as the original (0x486978..).
static inline rgl::psys* new_psys()
{
    rgl::psys* em = (rgl::psys*)operator new(0x1bc);
    if (em != 0) {
        em->particles._Myfirst = 0;
        em->particles._Mylast = 0;
        em->particles._Myend = 0;
        em->noise_index = 0;
        em->init();
    } else
        em = 0;
    return em;
}

// ---- Mission fields that mission.h leaves in padding or declares with another type (reported for mission.h; named
// accessors meanwhile). Names from this function: the sprintf "temper = .., humidity = .., height = .. fog_far = %d",
// the "sun" / "edit_cursor" / "vertex_color_add_mat" resources, the rain / snow emitters (g_emit_defs.used[0] / [1]).
static inline char& water(Mission* m) { return *((char*)m + 0x63c0); }               // the water object (Terrain::destroy_water)
static inline int& f6524(Mission* m) { return *(int*)((char*)&m->terrain + 0x154); } // +0x6524 (TerrainDesc +0x154)
static inline RectF& scene_box(Mission* m) { return *(RectF*)((char*)&m->terrain + 0x15c); }   // +0x652c the props' bounding box (TerrainDesc +0x15c)
static inline Entity*& precip_ent(Mission* m) { return (Entity*&)m->precip_fx; }       // the rain / snow emitter entity
static inline Entity*& sun_ent(Mission* m) { return (Entity*&)m->sun_fx; }           // the sun billboard entity
static inline Entity*& cursor_ent(Mission* m) { return (Entity*&)m->cursor; }           // the edit cursor entity
static inline Entity*& sel_marker_ent(Mission* m) { return (Entity*&)m->sel_marker; }
static inline Entity*& f65e4_ent(Mission* m) { return (Entity*&)m->f65e4; }

#define RECT(i) ((&terrain.min_x)[i])      // the play-area rectangle as four lanes
#define PROP(i) (this->props[i])
#define LV (*(LevelRaw*)&g_game->levels[scene_idx])

// ---------------------------------------------------------------------------
// @ 0x484590
// ---------------------------------------------------------------------------
void Mission::start_setup()
{
    Mission* m = this;
    vector4 in, out, v;

    WL_MARKER(4)
    reset();
    ((Timer*)&time_idx)->restamp();   // +0x62e8: time_idx / time_base as a Timer
    timers[0].restamp();
    timers[1].restamp();
    timers[2].restamp();
    ((stream_base&)g_log).w_cstr("begin_setup");
    g_phys.reset();

    std::vector<PlacedObj>& props = this->props;
    int mi0 = mode;   // stand-in: named indices, neutral here, kept only because the symbol ids they add flip x87 operand order in the prop-frame loop to the original's (docs/codegen.md 6.2)
    int li0 = scene_idx;
    int lo = li0 * 0x494;      // a byte offset: head unchanged, the added symbol id flips one x87 operand pair in the prop-frame loop to the original's (docs/codegen.md 6.2)
    ModeRec* mrec = &g_mission_templates[mi0];
    terrain.f180 = 0;
    LevelRaw* lrec = (LevelRaw*)((char*)g_game->levels + lo);
    bank_of(mrec)->randomize();
    if (lrec->loaded == 0)
        g_game->load_scene(scene_idx);

    // ---- camera party + neighbour temperature accumulation ---------------------------------
    PartyMgr* pm = &g_game->parties;
    int pid = g_game->main_party;
    Party* pa = party_rec(pm, pid);
    float temp[16];
    {
    int ptype;
    ptype = party_rec(pm, pid)->current_terrain;

    for (int t = 0; t < 16; t++) temp[t] = 0.0f;
    {
    vector4 ppos = *(vector4*)&party_rec(pm, pid)->pos;

    if (g_script.level != 0) {
        // guarded do-while, not `for`: a for over this walk makes cl fold the page loads of the whole function (docs/codegen.md 6.6)
        SceneObjs* objs = ((Scene*)g_script.level)->objs;
        unsigned p = (unsigned)objs->first;
        unsigned end = (unsigned)objs->last;
        if (p < end) do {
            float dx = *(float*)(p + 0x74) - ppos.x;
            float dy = *(float*)(p + 0x78) - ppos.y;
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
        } while ((p += 0x88) < end);
    } else {
        for (int t = 0; t < 16; t++) temp[t] = 0.2f;
    }
    }
    {
    vector4 u;
    {
    float temper, humidity, fo;

    WL_MARKER(5)
    // ---- day time, anchor blend, weather ---------------------------------------------------
    this->day_time = (float)fmod((double)(g_ticks[1] * 1e-5f), 24.0);
    g_game->blend(&u.x, pa->pos.x, pa->pos.y);
    WL_MARKER(0xc)

    temper = temp[0] * 0.1f + (temp[8] * 0.01f + (temp[3] * 0.001f + (temp[11] * 0.002f +
                   (temp[4] * 0.005f + (temp[12] * 0.01f + (temp[1] * 0.003f + (temp[9] * 0.005f +
                   (temp[2] * 0.0005f + temp[10] * 0.0015f))))))));
    if (temper < 0.0f) temper = 0.0f;
    else if (temper >= 1.0f) temper = 1.0f;

    in.x = (float)g_game->f4 * 0.1f;
    in.z = 0.0f;
    in.y = g_ticks[1] * 1e-5f;
    noise_fractal(96.0f, 0.7f, 4, 0, &in.x, &out.x);
    float f14 = (out.x + out.y + out.z) * 5.0f;
    {
        v.z = 700.0f;
        v.x = (float)g_game->f4 * 0.1f;
        v.y = g_ticks[1] * 1e-5f;
        in = v;
    }
    noise_fractal(72.0f, 0.7f, 4, 0, &in.x, &out.x);
    fo = (out.x + out.y + out.z) * 5.0f;

    humidity = clamp01(((f14 + f14) + 0.5f) * temper + (RND() * 6.32231167e-05f) * 0.0500000007f);
    (int&)this->height = (int&)pa->pos.z;   // before the humidity store: the original loads pa+0x38 ahead of it (0x484a..)
    this->humidity = humidity;
    (int&)this->temper = *(int*)&u.x;
    (int&)this->wind_x = *(int*)&u.z;
    (int&)this->wind_y = *(int*)&u.w;
    (int&)this->rain_amount = 0;
    precip_mode = 0;

    this->cloudiness = iround(((temper + 1.5f) * g_game->cloud_amount) * 1.5f);

    this->fog_density = ((g_game->cloud_amount + 0.3f) * (temper + 0.5f)) * g_game->haze_amount *
                g_game->haze_amount * 0.00800000038f + 9.99999975e-05f;
    if (g_game->haze_amount > 0.899999976f) {
        this->fog_density = this->fog_density * 1.29999995f + 0.0259999987f;
        if (g_game->haze_amount > 0.939999998f) {
            this->fog_density = this->fog_density * 1.20000005f + 0.0240000002f;
            if (g_game->haze_amount > 0.980000019f)
                this->fog_density = this->fog_density + 0.00999999978f;
        }
    }

    fog_far = (RND() * 3.16115584e-05f + 0.5f) * (1.20000005f - humidity) * 2000.0f;
    if (this->day_time > 3.0f && this->day_time < 10.0f)
        fog_far = (RND() * 4.42561795e-05f + 0.300000012f) * fog_far;

    if (g_game->cloud_amount > 0.600000024f) {
        if (humidity * 0.100000001f + (g_game->cloud_amount * 0.200000003f + (RND() * 6.32231167e-05f) * 0.0500000007f) +
            fo > 0.460000008f) {
            if (ptype == 4 || ptype == 0xc)
                precip_mode = 2;
            else
                precip_mode = 1;
            float amt = RND() * 3.16115584e-05f + 0.5f;
            this->rain_amount = amt;
            if (this->cloudiness < 2) this->cloudiness = 2;
            if (amt > 0.300000012f) this->cloudiness = 3;
            float ff = fog_far / (3.0f * amt + 1.0f);
            fog_far = ff;
            this->fog_density = amt * 0.00150000001f + this->fog_density;
            if (precip_mode == 2) {
                fog_far = ff * 0.5f;
                this->fog_density = this->fog_density * 1.5f + amt * 0.00374999992f;
            }
        }
    }
    if (fog_far < 60.0f) fog_far = 60.0f;
    if (this->fog_density > 0.150000006f) this->fog_density = 0.150000006f;

    }
    WL_MARKER(0xd)
    {
        void* o = pa->mesh;
        if (o != 0)
            this->f65ec = *(int*)((char*)o + 4);
        else
            this->f65ec = 0;
    }

    f6608 = ptype;
    props = lrec->props;
    for (unsigned i = 0; i < props.size(); i++) {
        this->props[i].m14 = i;
    }

    g_script.param1 = ptype;

    {
        SndBank* b = bank_of(&g_mission_templates[mode]);
        for (int i = 0; i < b->count; i++)
            b->recs[i].f0c = 0;
    }
    {
        SndBank* b = bank_of(&g_mission_templates[mode]);
        for (int i = 0; i < b->count; i++)
            b->recs[i].update(-0x13);
    }
    str = lrec->f484;

    // ---- ambient sound slot selection -------------------------------------------------------
    for (int i = 0; i < g_num_ground_specs; i++) {
        EnvRow* s = &g_env_rows[i];
        float r = RND() * 6.32231149e-07f;
        s->rank = r;
        if (this->day_time > 7.0f && this->day_time < 17.0f) {
            if ((s->f10 & 3) != 0) goto no_bonus;
        } else if (this->day_time > 4.0f && this->day_time < 20.0f) {
            if ((s->f10 & 3) != 1) goto no_bonus;
        } else {
            if ((s->f10 & 3) != 2) goto no_bonus;
        }
        s->rank = r + 1.0f;
    no_bonus:
        s->rank = s->rank - abs(this->cloudiness - ((s->f10 >> 4) & 0xf)) * 0.100000001f;
    }
    env_row = 0;
    for (int i = 1; i < g_num_ground_specs; i++)
        if (g_env_rows[i].rank > g_env_rows[env_row].rank)
            env_row = i;

    // ---- sky / cloud / fog bit fields -------------------------------------------------------
    if (LV.flags.b8) {
        g_terrain.parse(LV.terrain_code);
        GameData* gg = g_game;
        if (((LevelRaw*)&gg->levels[scene_idx])->flags.b9) {
            PartyMgr* pm2 = &gg->parties;
            int x = (int)party_rec(pm2, gg->main_party)->pos.x;
            int y = (int)party_rec(pm2, gg->main_party)->pos.y;
            int parity = this->scene_counter % 3;
            int fr = gg->f4;
            int a = fr * 0x1b0b + parity + y * 0x230b + x * 17;
            int c = fr * 0xf4d + parity + y * 0x16a1 + x * 19;
            int b = y * 0x19c9 + parity + fr * 0x1e6f + x * 31;
            a &= 0x7fffff; c &= 0x7fffff; b &= 0x7fffff;

            put(g_terrain.bits[0], (unsigned)g_bit[31] - 1, 0, a);
            put(g_terrain.bits[2], (unsigned)g_bit[31] - 1, 0, c);
            put(g_terrain.bits[1], (unsigned)g_bit[31] - 1, 0, b);


            int storm = 0;
            int pt = ptype;   // a block copy (ptype is dead after this block): puts the weather-bit locals in the original's slot groups (slot hunks 347 -> 160)
            unsigned seed = (this->scene_counter % 3 + g_game->f4 * 0x4abb + y * 0xb1 + x * 0x17 + 0x8a767) *
                            0x1ddcf5f + 0x2f784719;
            float chance = RNDU(seed) * 6.32231167e-05f;
            if (!(pt == 7)) { if (temp[8] > 27.0f) {
                storm = 1;
            } else if ((pt == 3 || pt == 0xb) && chance < 0.150000006f) {
                storm = 1;
            } } else {
                storm = 1;
                pt = 3;
            }
            put(g_terrain.bits[4], (unsigned)g_bit[4] - 1, 28, pt);
            {
                seed = lcg(seed);
                unsigned m1 = (unsigned)g_bit[1] - 1;
                unsigned v1 = (unsigned)(storm != 0) <= m1 ? (unsigned)(storm != 0) : m1;
                g_terrain.bits[3] &= ~(m1 << 31);
                g_terrain.bits[3] |= v1 << 31;
                g_terrain.bits[3] &= ~(m1 << 30);
            }
            g_rand_seed = seed;
            float ff = RNDU(seed) * 6.32231149e-06f + 0.449999988f;
            if (storm) {
                seed = lcg(seed);
                g_rand_seed = seed;
                ff = ff + RNDU(seed) * 6.32231149e-06f + 0.349999994f;
            } else if (pt == 0xb || pt == 0xc) {
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
            if (pt == 0xb) {
                cc = ifloor(RNDU(seed) * 0.00189669349f) + 30;
            } else if (pt == 0xc) {
                cc = ifloor(RNDU(seed) * 0.00158057781f) + 25;
            } else {
                g_rand_seed = seed;
                cc += iround((RNDU(seed) * 6.32231167e-05f + 0.400000006f) * temp[1] * 0.449999988f);
            }
            if (cc > 0x41) cc = 0x41;
            put(g_terrain.bits[4], (unsigned)g_bit[7] - 1, 14, cc);

            seed = lcg(g_rand_seed);   // the original reloads the global here (0x4856b5): in the 0xb/0xc arms it was not restored, so this is one lcg step behind the register
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
            *(vec2*)&scene_box(this).x0 = z;
            scene_box(this).x1 = (float)(g_terrain.bits[3] & ((unsigned)g_bit[10] - 1));
            scene_box(this).y1 = (float)((g_terrain.bits[3] >> 10) & ((unsigned)g_bit[10] - 1));
            // statement order found by enumeration (24 orders): this one makes the else arm's scratch rotation the original's (its 18 copies start at ecx);
            // the original's own order of this arm (0x6538 loaded first, then stores 0x6540, 0x654c, 0x653c, 0x6544, 0x6548, z in ecx/edx) is still not reached
            terrain.base_z = -0.300000012f;
            (int&)terrain.max_x = (int&)scene_box(this).x1;
            *(vec2*)&terrain.min_x = z;      // from z, not a copy of 0x652c: the original stores both pairs from the registers holding z (0x485a8a..)
            (int&)terrain.max_y = (int&)scene_box(this).y1;
        }
    } else {
        (int&)terrain.base_z = (int&)lrec->base_z;
        (int&)terrain.min_x = (int&)lrec->rect[0];
        (int&)terrain.min_y = (int&)lrec->rect[1];
        (int&)terrain.max_x = (int&)lrec->rect[2];
        (int&)terrain.max_y = (int&)lrec->rect[3];
        (int&)scene_box(this).x0 = (int&)terrain.min_x;
        (int&)scene_box(this).y0 = (int&)terrain.min_y;
        (int&)scene_box(this).x1 = (int&)terrain.max_x;
        (int&)scene_box(this).y1 = (int&)terrain.max_y;
    }
    g_terrain.tail = str;


    // ---- prop bounding box, two passes ------------------------------------------------------
    {
    vector4 hi;
    for (unsigned i = 0; i < this->props.size(); i++) {
        PlacedObj* p = &this->props[i];
        if (p->kind == 0 && p->id >= 0) {
            PropKind* k = &g_propkinds[p->id];
            v.x = (p->m18.s.x * k->f58 + (p->m18.f.x * k->f5c + p->m18.u.x * k->fx60)) + p->m18.o.x;
            v.y = (p->m18.s.y * k->f58 + (p->m18.f.y * k->f5c + p->m18.u.y * k->fx60)) + p->m18.o.y;
            v.z = (p->m18.s.z * k->f58 + (p->m18.f.z * k->f5c + p->m18.u.z * k->fx60)) + p->m18.o.z;
            hi = v;
            float lx = v.x - k->f68;
            v.y -= k->f68; hi.x += k->f68; hi.y += k->f68;
            scene_box(this).x0 = fmin2(scene_box(this).x0, lx);
            scene_box(this).y0 = fmin2(scene_box(this).y0, v.y);
            scene_box(this).x1 = fmax2(scene_box(this).x1, hi.x);
            scene_box(this).y1 = fmax2(scene_box(this).y1, hi.y);
        }
    }
    for (unsigned i = 0; i < this->props.size(); i++) {
        PlacedObj* p = &this->props[i];
        if (p->kind == 0 && p->id >= 0) {
            PropKind* k = &g_propkinds[p->id];
            v.x = (p->m18.s.x * k->f58 + (p->m18.u.x * k->fx60 + p->m18.f.x * k->f5c)) + p->m18.o.x;
            v.y = (p->m18.f.y * k->f5c + (p->m18.u.y * k->fx60 + p->m18.s.y * k->f58)) + p->m18.o.y;
            v.z = (p->m18.f.z * k->f5c + (p->m18.u.z * k->fx60 + p->m18.s.z * k->f58)) + p->m18.o.z;
            hi = v;
            float lx = v.x - k->f68;
            v.y -= k->f68; hi.x += k->f68; hi.y += k->f68;
            scene_box(this).x0 = fmin2(scene_box(this).x0, lx);
            scene_box(this).y0 = fmin2(scene_box(this).y0, v.y);
            scene_box(this).x1 = fmax2(scene_box(this).x1, hi.x);
            scene_box(this).y1 = fmax2(scene_box(this).y1, hi.y);
        }
    }
    }

    }
    {
    // ---- fade, params, ambient, sun ---------------------------------------------------------
    rebuild_nav(scene_idx, mode);
    {
        EnvRow* e = &g_env_rows[env_row];
        this->f5274 = 0;
        scene->set_fade(&this->fog_density, (const int*)&e->f44);   // 0x4624c0
    }
    {
        unsigned color = 0xff334455;
        scene->set_params(-5.0f, 10.0f, (const int*)&color, &terrain.base_z);     // 0x462540
    }
    setup_ambient(scene_idx, env_row, this->fog_density);

    Entity* sun = (Entity*)((rgl::world*)scene)->create_entity(9);
    Mesh* sm = (Mesh*)g_resmgr.get_object(&g_env_rows[env_row].name)->clone();   // stand-in: the local view and its Raw::at for the mask below; `rgl::mesh* sm` with sm->mask (or a raw store) moves the rest of the function (chk aligned 1675 -> 1699: an inline-call id effect)
    sm->scale(200.0f);
    sm->at<unsigned>(0x10c) &= 0xfffeffff;
    {
        int mi = (int)sm;
        sun->at<MeshList>(0x118).push(mi);
    }
    sun->v4();
    {
        GameData* g1 = g_game;
        int secs = (int)((g_ticks[g1->f13b18] - g1->clock) * 1e-5f);
        int h = secs % 24;
        {
        float angle = g_env_rows[env_row].f14 * 0.0174532924f -
                      (float)(h - 12) * 0.0174532924f * 15.0;
        sun->at<rgl::matrix>(0x58).rotate_about_u(&angle);
        }
    }
    {   // a sibling block, not angle's parent: an address-taken local conflicts with locals nested in its block, and the original has far1 in angle's slot (0x18)
        float far1 = 1.0e8f;
        sun->v6(&far1);
    }
    scene->sky = (rgl::entity*)sun;

    // ---- terrain generate / interior ----------------------------------------------------------
    if (LV.flags.b8) {
        g_terrain.sun_ambient = scene->ambient;
        g_terrain.sun_diffuse = *(vector4*)((char*)wind_a + 4);
        stream_base* s414 = (stream_base*)lrec->terrain_stream;
        char flagc = 1;
        s414->rewind();
        if (s414->at_end() == 0) {
            if (LV.flags.b8) {
                stream_base* s2 = (stream_base*)LV.terrain_stream;
                s2->rewind();
                terrain.read(s2);
            }
            flagc = 0;
        }
        g_terrain.generate((int)scene, (int*)&terrain, flagc);
    } else {
        build_interior(scene_idx, this->interior, &this->num_interior);
    }
    stream_base* s444 = (stream_base*)lrec->nav_stream;
    s444->rewind();
    PolyMesh& nav = this->nav;
    nav.load(s444);
    nav.build_geometry();
    {
        LevelBits f2 = LV.flags;
        if (f2.b9 && f2.b8 && g_terrain.count_low_quads() > 10) {
            nav.build(scene->hf, 1);
            nav.build_geometry();
            nav.compute_edge_creases();
        }
    }
    ((rgl::world*)scene)->build(&nav);   // 0x478c50

    // ---- light/frame table: 64 slots + 5 extra for a custom camera --------------------------
    this->num_marks = 0x40;
    this->f5274 = 0;
    {
    vector4 c0;
    c0.x = 0.0f; c0.y = 0.0f;
    {
    RectF& rc = (RectF&)terrain.min_x;   // through a reference: the original's loads stay below the 0x5278/0x5274 stores (direct at<> loads were hoisted above them)
    vec2 ext = { rc.x1 - rc.x0, rc.y1 - rc.y0 };
    vec2 mid;
    mid.x = ext.x * 0.5f;
    mid.y = ext.y * 0.5f;
    mid.x = mid.x + terrain.min_x;
    mid.y = mid.y + terrain.min_y;
    if (lrec->flags.b8) *(vec2*)&c0 = mid;

    }
    {
    v.z = 500.0f;
    int k = 0;
    do {
        mat3 tmp;
        *(mat3*)&recs[k].origin = *identity3(&tmp);
        vec2 g = { (float)(k % 8 + 1), (float)(k / 8 + 1) };
        v.x = g.x + c0.x;
        v.y = g.y + c0.y;
        recs[k].origin.o = v;
    } while (++k < 0x40);
    }
    }

    if (LV.flags.b10) {
        this->num_marks = 5;
        int par = rand() % 2;
        for (int s = 0; s < 5; s++) {
            MarkRec* sl = &recs[s];
            lane(v, 1 - par) = ((RND() * 6.32231167e-05f) * 0.400000006f + 0.300000012f) * (RECT(3 - par) - RECT(1 - par)) + RECT(1 - par);
            float b = (((RND() * 6.32231167e-05f - 0.5f) * 0.100000001f + s) * 0.25f) * 0.600000024f + 0.200000003f;
            if (s < 2)
                b = b * 0.75f;
            else if (s > 2)
                b = 1.0f - (1.0f - b) * 0.75f;
            lane(v, par) = b * (RECT(2 + par) - RECT(par)) + RECT(par);
            v.z = 1000.0f;
            mat3 tmp;
            rgl::matrix* a = &sl->origin;



            *(mat3*)a = *identity3(&tmp);
            a->o = v;
            vector4 fwd;
            fwd.z = 0.0f;
            lane(fwd, 1 - par) = 0.0f;
            if (s < 2) lane(fwd, par) = 1.0f;
            else lane(fwd, par) = -1.0f;
            a->f = fwd;
            a->orthonormalize();
            recs[s].m = recs[s].origin;   // not `sl->m = *a`: the original rebuilds the copy addresses from the slot (lea edi,[esi+0x40]) instead of keeping a+0x10 in edi across orthonormalize

        }
    }
    {
    if (LV.flags.b8) {
        terrain.min_x = terrain.min_x + 24.0f;
        terrain.min_y = terrain.min_y + 24.0f;
        terrain.max_x = terrain.max_x - 24.0f;
        terrain.max_y = terrain.max_y - 24.0f;
    }

    // ---- per-prop frame copy into the table ----------------------------------------------------
    {
    vector4 t20;
    for (int ip = 0; (unsigned)ip < props.size(); ip++) {
        PlacedObj* p = &PROP(ip);
        if (p->kind == 1) {
            int sl = p->value;
            if (sl >= 0 && sl < 0x40) {
                recs[sl].origin.o = p->m18.o;
                *(mat3*)&recs[sl].origin = *(mat3*)&PROP(ip).m18;
                recs[sl].m = recs[sl].origin;
                vector4 yup;
                yup.x = 0.0f; yup.y = 0.0f; yup.z = 1.0f;
                vector4* up = &recs[sl].m.u;
                *up = yup;
                vector4* sd = &recs[sl].m.s;
                v.x = up->z * recs[sl].m.f.y - up->y * recs[sl].m.f.z;
                v.y = recs[sl].m.f.z * up->x - recs[sl].m.f.x * up->z;
                v.z = recs[sl].m.f.x * up->y - up->x * recs[sl].m.f.y;
                *sd = v;
                norm3(&sd->x);
                cross2(t20, *up, *sd);
                recs[sl].m.f = t20;
                recs[sl].m1 = recs[sl].m;
                recs[sl].m2 = recs[sl].m;
            }
        }
    }
    }

    // ---- prop ground / spawn pass --------------------------------------------------------------
    {
    int pc = 0;
    int pi;
    int& pr = pi;
    pr = pc;
    while ((unsigned)pc < this->props.size()) {
        PROP(pc).ent = 0;
        if (g_cfg.enable_edit_mode == 0 && PROP(pc).kind == 4 && LV.flags.b8) {
            PlacedObj* p = &PROP(pc);
            int j;
            for (j = 0; j < g_ptype_count; j++)
                if (g_ptypes[j].name.equals(p->name))   // 0x4866c0: add esi, 0x20
                    goto found;
            j = 0;
        found:
            g_terrain.emit_by_type(j, &p->m18, &p->v158, p->value);
        } else {
            int foundv = -1;
            spawn_prop(&pr, &foundv);
        }
        pc = ++pr;
    }
    }
    update_prop_flags();
    {
        LevelBits f = LV.flags;
        if (f.b8)
            g_terrain.regenerate_water((void**)&f65a8, f.b9 == 0);
    }
    light_props();
    if (!LV.flags.b8)
        light_entities(this->interior, this->num_interior);
    f65bc = 0;
    precip_fx = 0;

    // ---- scene effect object (rain / snow emitter) ---------------------------------------------
    if (g_options.i20 != 0 && !lrec->flags.b0 && precip_mode > 0) {
        vector4 sky = *(vector4*)g_env_rows[env_row].base;
        float scale = (sky.x * 3.0f + 1.0f) * 0.400000006f;
        Entity* fx = (Entity*)((rgl::world*)scene)->create_entity(9);
        precip_ent(this) = fx;
        fx->at<vector4>(0x88) = vector4(0.0f, 0.0f, 30.0f);
        {
        float far_clip = 1.0e7f;
        precip_ent(this)->v6(&far_clip);
        }
        vector4 t20;
        vector4 zero(0.0f, 0.0f, 0.0f);
        rgl::matrix mx;
        mat3 tmp;
        *(mat3*)&mx.s = *identity3(&tmp);
        t20.x = this->wind_x * 0.349999994f;
        t20.y = this->wind_y * 0.349999994f;
        mx.o = zero;
        t20.z = -10.0f;
        int ix;
        if (precip_mode == 1) {
            mx.o.z = 6.0f;
            mx.o.y = 6.5f;
            ix = g_emit_defs.used[0];
            if (g_emit_defs.used[0] >= 0 && g_emit_defs.used[0] < g_emit_defs.count) {
                rgl::psys* em = new_psys();
                g_emit_defs.setup(em, ix, (const rgl::vector4*)&mx.o);
                if (em != 0) {
                    float inv = 1.0f / em->drag;
                    em->vel_base = vector4(t20.x * inv, t20.y * inv, inv * -10.0f);
                    em->accel = t20;
                    em->axis_z = t20;
                    norm3(&em->axis_z.x);
                    em->spawn_rate *= this->rain_amount;
                    em->lifetime = 11.0f / (float)(10.0 / em->drag);
                    {
                    rgl::float_pair* cv = em->curve;   // through a pointer: the original scales the curve values after the vc4/vb4 copies, not interleaved with them
                    cv[2].v = scale * cv[2].v;
                    cv[3].v = scale * cv[3].v;
                    cv[4].v = scale * cv[4].v;
                    cv[5].v = scale * cv[5].v;
                    cv[6].v = scale * cv[6].v;
                    cv[7].v = scale * cv[7].v;
                    }
                    precip_ent(this)->v25(em);
                }
            }
        } else if (precip_mode == 2) {
            mx.o.z = 6.0f;
            mx.o.y = 6.5f;
            ix = g_emit_defs.used[1];
            if (g_emit_defs.used[1] >= 0 && g_emit_defs.used[1] < g_emit_defs.count) {
                rgl::psys* em = new_psys();
                g_emit_defs.setup(em, ix, (const rgl::vector4*)&mx.o);
                if (em != 0) {
                    em->accel = t20;
                    {
                    rgl::float_pair* cv = em->curve;   // through a pointer: the original scales the curve values after the vc4/vb4 copies, not interleaved with them
                    cv[2].v = scale * cv[2].v;
                    cv[3].v = scale * cv[3].v;
                    cv[4].v = scale * cv[4].v;
                    cv[5].v = scale * cv[5].v;
                    cv[6].v = scale * cv[6].v;
                    cv[7].v = scale * cv[7].v;
                    }
                    vector4* vb = &em->axis_z;
                    *vb = t20;
                    norm3(&vb->x);
                    em->spawn_rate *= this->rain_amount;
                    precip_ent(this)->v25(em);
                }
            }
        }
    }

    }
    }
    }
    // ---- intro billboard (sun mesh) -------------------------------------------------------------
    sun_fx = 0;
    sun_fx_mesh = 0;
    sun_mesh = 0;
    if (!lrec->flags.b0 && g_options.i20 != 0) {
        Entity* eb = (Entity*)((rgl::world*)scene)->create_entity(9);
        sun_ent(this) = eb;
        {
        float two = 2.0f;
        eb->v6(&two);
        }
        this->sun_fx_mesh = new rgl::mesh;
        this->sun_fx_mesh->mask &= 0xfffeffff;
        this->sun_fx_mesh->slots[0].f4 = 1;
        this->sun_fx_mesh->slots[1].f4 = 1;
        {
            sun_ent(this)->add_mesh(this->sun_fx_mesh);
        {
            tstring nm("sun");
            this->sun_mesh = g_resmgr.get_object(&nm)->clone();
        }
        }
        this->sun_mesh->slots[0].f4 = 1;
        this->sun_mesh->slots[1].f4 = 1;
        float c = (1.0f - 150.0f / (fog_far + 150.0f)) * g_env_rows[env_row].size * 0.899999976f;
        {
        vector4 u;
        u.x = c; u.y = c; u.z = c;
        this->sun_mesh->color = rgl::pack_color(&u);
        }

        this->sun_mesh->color |= 0xff000000;
        this->sun_mesh->mask &= 0xfffeffff;
        this->sun_mesh->scale(50.0f);
        {
            sun_ent(this)->add_mesh(this->sun_mesh);
        }
    }

    // ---- cursor / vertex-colour helper entities -------------------------------------------------
    cursor = 0;
    cursor_ent(this) = (Entity*)((rgl::world*)scene)->create_entity(9);
    {
        float f290 = 290.5f;
        cursor_ent(this)->v6(&f290);
    }
    {
        tstring nm("edit_cursor");
        this->cursor_mesh = g_resmgr.get_object(&nm)->clone();
    }
    {
        cursor_ent(this)->add_mesh(this->cursor_mesh);
    }
    this->cursor_fill = new rgl::mesh;
    {
        tstring nm("vertex_color_add_mat");
        this->cursor_fill->res = (skin*)g_resmgr.get_material(nm);
    }
    cursor_ent(this)->add_extra(this->cursor_fill);
    sel_marker = 0;
    sel_marker_ent(this) = (Entity*)((rgl::world*)scene)->create_entity(9);
    {
        float f33 = 33.5f;
        sel_marker_ent(this)->v6(&f33);
    }
    this->sel_mesh = new rgl::mesh;
    {
        tstring nm("vertex_color_add_mat");
        this->sel_mesh->res = (skin*)g_resmgr.get_material(nm);
    }
    sel_marker_ent(this)->add_extra(this->sel_mesh);
    f65e4_ent(this) = (Entity*)((rgl::world*)scene)->create_entity(9);
    {
        float f33 = 33.5f;
        f65e4_ent(this)->v6(&f33);
    }
    cursor->visible = 0;
    sel_marker->visible = 0;
    f65e4->visible = 0;

    // ---- counters, light table ground snap -------------------------------------------------------
    for (int i = 0; i < 8; i++) {
        int j = 0;
        do {
            counts[i][j] = 0;
            changed_at = g_ticks[2] * 1e-5f;
        } while (++j < 6);
    }
    f6524(this) = 0;
    clock_set = 0;
    for (int k = 0; k < 0x40; k++) {
        float outY;
        MarkRec* sl = &recs[k];
        Timer* tm = &sl->timer;
        face** fp = &sl->floor;
        int n = 0;
        *fp = 0;
        do {
            ((rgl::world*)scene)->floor_at(&recs[k].m.o.x, &outY, fp);     // 0x439be0
            if (*fp == 0) {
                recs[k].m.o.x = (frand() - 0.5f) * 0.01f + recs[k].m.o.x;
                recs[k].m.o.y = (frand() - 0.5f) * 0.01f + recs[k].m.o.y;
                n++;
                if (n > 100) {
                    recs[k].m.o.x = (frand() - 0.5f) * 100.0f;
                    recs[k].m.o.y = (frand() - 0.5f) * 100.0f;
                    if (n > 10000)
                        *fp = (face*)scene->objs._Myfirst[0];
                }
            }
        } while (*fp == 0);
        sl->m.o.z = outY;   // the tail copies through sl (+4)
        sl->m1 = sl->m;
        sl->m2 = sl->m;
        tm->restamp();
        recs[k].delay = 0;
        copy_slot(k);
    }

    // ---- party spawn counters, troops, mode records ----------------------------------------------
    {
        int* lists = &list[0][0];
        int* cnts = cnt;
        for (int sd = 0; sd < 2; sd++)
            for (int i = 0; i < cnts[sd]; i++)
                reset_spawned(lists[sd * 128 + i]);
    }
    for (int i = 0; i < g_game->ntroops; i++)
        g_game->troops[i].used = 0;
    memset(done, 0, 0x40);
    for (int i = 0; i < mrec->nrecs; i++) {
        mrec->recs[i].f18 = 0;
        mrec->recs[i].f1c = 0;
    }
    for (int i = 0; i < mrec->nrecs; i++)
        add_amount(i, mrec->recs[i].f14);
    run_spawns();
    char buf[0x50];
    sprintf(buf, "temper = %3.2f, humidity = %3.2f, height = %3.2f fog_far = %d",
            (double)this->temper, (double)this->humidity, (double)this->height, (int)fog_far);
    for (int i = 0; i < 2; i++)
        frame(0.100000001f, 1);

    // ---- placed-prop jitter + ground snap ----------------------------------------------------------
    for (unsigned i = 0; i < props.size(); i++) {
        PlacedObj* p = &this->props[i];
        if (p->kind == 0 && (g_propkinds[p->id].flags & 0x1000)) {
            float outY;
            void* sink;
            // STAND-IN (q): the original keeps the w lane of this copy in edi across rand() and floor_at and stores the prop position from
            // [v.x], [v.y], [v.z], edi; that moves the counter to ebx and the byte offset to a frame slot (0x487371..0x4874c3). The real
            // cause is that the original unit saw rgl::world::floor_at's body (callee summary: reads through p, keeps nothing), so v does
            // not escape: with a visible floor_at and plain `v = origin; ...; pos = v` this loop is instruction-exact, but v not escaping
            // also changes the frame packing and the candidate count of the prop-frame loop, which the rest of this file is not yet fitted
            // to. Until then q (a non-escaping copy, split into lanes by cl) carries w; it costs seven extra copy instructions here.
            vector4 q = f6598;
            v = q;
            v.x = (RND() * 6.32231167e-05f - 0.5f) * 2.0f + v.x;
            v.y = (RND() * 6.32231167e-05f - 0.5f) * 2.0f + v.y;
            ((rgl::world*)scene)->floor_at(&v.x, &outY, (face**)&sink);
            v.z = outY;
            q.x = v.x; q.y = v.y; q.z = v.z;
            p->m18.o = q;
            if (((PropObj*)p->ent) != 0)
                ((PropObj*)p->ent)->v22(&p->m18, &p->v158);
        }
    }

    // ---- water / particle system teardown or rebuild --------------------------------------------------
    {
        LevelBits f3 = LV.flags;
        if (f3.b8) {
            g_terrain.destroy_water((int)&water(this));
        } else if (!f3.b0 && g_use_shaders) {
            ((Psys*)&g_shadow_map)->attach_camera(scene);
            for (unsigned i = 0; i < scene->ground->verts.size(); i++)
                ((Psys*)&g_shadow_map)->extend(&scene->ground->verts[i]);
            ((Psys*)&g_shadow_map)->fit_bounds();
            ((Psys*)&g_shadow_map)->render();
        }
    }
    ((stream_base&)g_log).w_cstr("end_setup");
    terrain.muted = 0;
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
