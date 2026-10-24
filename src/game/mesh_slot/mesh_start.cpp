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

// ---------------------------------------------------------------------------
// Local layouts (names provisional)
// ---------------------------------------------------------------------------
struct Raw { template<class T> T& at(int o) { return *(T*)((char*)this + o); } };

struct vector4 { float x, y, z, w; };
struct vec2 { float x, y; vec2(float a, float b) { x = a; y = b; } };
struct mat3 { float m[12]; };                                // 3 rows (x, y, z, w lanes)
struct matrix {                                              // 4 rows; row 3 = position
    vector4 r[4];
    void orthonormalize();                                   // 0x407ef0 (rgl::matrix)
    void rotate_sf(float* angle);                            // 0x40e770
};
struct Rect { float x0, y0, x1, y1; };
mat3* __cdecl identity3(mat3* out);                          // 0x40cd60
unsigned __cdecl pack_color(float* c);                       // 0x40e440
float __cdecl frand();                                       // 0x40e400
void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0

struct tstring {                                             // 16 bytes (src/game/tstring.h)
    int pos; char* buf; int cap; int len;
    tstring(const char* s);                                  // 0x42bbe0
    ~tstring();                                              // 0x42bc40
    void operator=(const tstring& o);                        // 0x42bc50
    char equals(const tstring& o) const;                     // 0x42bdd0
};
extern tstring g_tstr;                                       // 0x8ed4db4

struct Rec1c8 : Raw { char b[0x1c8]; };                      // placed prop record
struct RecVec {                                              // std::vector<Rec1c8> (allocator lane at +0)
    int alloc; Rec1c8* first; Rec1c8* last; Rec1c8* end;
    RecVec& operator=(const RecVec& o);                      // 0x477160
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
};
struct Body { char b[0x24]; };
struct BodyVec {
    int alloc; Body* first; Body* last; Body* end;
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
};
struct IntVec {                                              // std::vector<int> (mesh pointers)
    int alloc; int* first; int* last; int* end;
    void _Insert_n(int* where, unsigned n, const int& v);    // 0x410610
    void push_back(const int& v)
    {
        unsigned sz = first == 0 ? 0 : (unsigned)(last - first);
        unsigned cp = first == 0 ? 0 : (unsigned)(end - first);
        if (sz < cp) { *last = v; ++last; }
        else _Insert_n(last, 1, v);
    }
};

struct Mesh : Raw {                                          // rgl::mesh, 0x260 bytes
    char b[0x260];
    Mesh();                                                  // 0x412a90
    Mesh* clone();                                           // 0x41c6d0
    void scale(float s);                                     // 0x44f0c0
};
struct Emit : Raw {                                          // 0x1bc bytes
    char b[0x1bc];
    Emit() { at<int>(0x10) = 0; at<int>(0x14) = 0; at<int>(0x18) = 0; at<int>(0x1b8) = 0; init(); }
    void init();                                             // 0x420b40
};
struct EmitTable {                                           // 0x8ed5090
    int pad0; int count; int idx1; int idx2;
    void apply(Emit* e, int idx, vector4* pos);              // 0x4424c0
};
extern EmitTable g_emit_defs;                                // 0x8ed5090

struct Entity : Raw {                                        // scene entity (vtable object)
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6(float* p);
    virtual void v7(); virtual void v8(); virtual void v9(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15(); virtual void v16();
    virtual void v17(); virtual void v18(); virtual void v19(); virtual void v20(); virtual void v21();
    virtual void v22(); virtual void v23(); virtual void v24();
    virtual void v25(Emit* e);
    void add_mesh(Mesh* m);                                  // 0x442110
};
struct Streamer {                                            // level sub-object with a vtable (+0x414, +0x444)
    virtual void v0();
    virtual void v1();
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15(); virtual void v16();
    virtual void v17(); virtual void v18(); virtual void v19(); virtual void v20(); virtual void v21();
    virtual void v22();
    virtual char v23();
};
struct PropObj {                                             // object at prop +0x1b4
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void v22(const matrix* m, const vector4* p);
};

struct World : Raw {                                         // scene container (Mission +0)
    Entity* create_entity(int kind);                         // 0x428440
    void set_fade(float* fog, void* spec);                   // 0x4624c0
    void set_params(float a, float b, unsigned* color, float* z);   // 0x462540
    void floor_at(float* pos, float* y, void** face);        // 0x439be0
    void f478c50(void* nav);                                 // 0x478c50
};
struct NavMesh : Raw {                                       // Mission +0x6698
    void f4781d0(Streamer* s);                               // 0x4781d0
    void f470a00();                                          // 0x470a00
    void f4785e0(int a, int b);                              // 0x4785e0
    void f470750();                                          // 0x470750
};
struct TerrainDesc : Raw {                                   // Mission +0x63d0
    void read(Streamer* s);                                  // 0x464440
};
struct terrain_obj {                                         // global at 0xa76630 (sky/terrain generator)
    unsigned s0, s1, s2, s3, s4;
    void f4658c0(void* spec);                                // 0x4658c0
    void generate(World* w, void* desc, char flag);          // 0x481d10
    int count_low_quads();                                   // 0x467530
    void emit_by_type(int idx, const matrix* m, const vector4* p, int what);   // 0x466d90
    void regenerate_water(void** out, char flag);            // 0x482170
    void destroy_water(void* p);                             // 0x4814a0
};
extern terrain_obj g_terrain;                                // 0xa76630
struct Psys {
    void f4711e0(World* w);                                  // 0x4711e0
    void f468c20(Body* b);                                   // 0x468c20
    void f463cc0();                                          // 0x463cc0
    void reset();                                            // 0x47fef0
};
extern Psys g_psys;                                          // 0xa70248
struct ResMgr {
    Mesh* get_object(const tstring& name);                   // 0x419540
    Mesh* get_object(void* name);
    void* get_material(tstring* name);                       // 0x418a70
};
extern ResMgr g_resmgr;                                      // 0x5d6fa0

struct SndSlot {                                             // 0x78 bytes, array in the mode record bank
    char pad0[0xc]; int f0c; char pad10[0x78 - 0x10];
    void update(int what);                                   // 0x4625d0
};
struct SndBank {                                             // at mode record +0x4c
    int count; SndSlot* recs; int pad8;
    void randomize();                                        // 0x462790
};
struct SpawnRec { char pad0[0x14]; int f14; int f18; int f1c; char pad20[0x4c - 0x20]; };
struct ModeRec {
    char pad0[0x34]; SpawnRec* recs; int nrecs; char pad3c[0x4c - 0x3c]; SndBank bank;
};
struct SndRec : Raw {                                        // 0x4c bytes, array g_snd
    char b[0x4c];
};
struct PropKind : Raw { char b[0x78]; };                     // g_map_points records

struct Party : Raw {                                         // 0x5ac8 bytes
    char pad0[0x30]; vector4 pos;                            // +0x30
};
struct PartyMgr {
    char pad0[0x138d0]; Party** pages;                       // page table (index >> 4)
    int index_of(int id);                                    // 0x48ad00
};
struct LevelBits {                                           // Level +0x30 flags (bit fields: tested with test ah,n)
    unsigned b0 : 1; unsigned pad1 : 7; unsigned b8 : 1; unsigned b9 : 1; unsigned b10 : 1; unsigned pad11 : 21;
};
struct Level : Raw {                                         // 0x494-byte level record
    char pad0[0x30]; LevelBits flags; char rest[0x494 - 0x34];
};
struct GameState : Raw {
    void load_scene(int idx);                                // 0x47fbd0
    void blend(float* out, float x, float y);                // 0x471040
};
struct PhysSet {
    void f47dc50();                                          // 0x47dc50
};
extern PhysSet g_phys;                                       // 0xa181f8

struct log_base {
    virtual void p00(); virtual void p01(); virtual void p02(); virtual void p03(); virtual void p04();
    virtual void p05(); virtual void p06(); virtual void p07(); virtual void p08(); virtual void p09();
    virtual log_base& w_cstr(const char* s);
};
struct log_stream : log_base {
    virtual void p00(); virtual void p01(); virtual void p02(); virtual void p03(); virtual void p04();
    virtual void p05(); virtual void p06(); virtual void p07(); virtual void p08(); virtual void p09();
    virtual log_base& w_cstr(const char* s);
};
extern log_stream g_log;                                     // 0x5a9f60
extern vector4 g_emit_pos[2];                                // 0x8eb235c

struct SceneObj : Raw { char b[0x88]; };
struct SceneObjs { char pad0[0x14]; SceneObj* first; SceneObj* last; };    // g_scene +0xb0
struct Scene { char pad0[0xb0]; SceneObjs* objs; };

struct Slot {                                                // 0x130 bytes, 64 at Mission +0x670
    matrix A, B, C, D;
    char pad100[0x10];
    int clock;                                               // +0x110
    int pad114;
    __int64 start;                                           // +0x118
    int f120;
    int pad124;
    void* face;                                              // +0x128
    int pad12c;
};

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
extern __int64    g_time[];       // 0x5e2948  tick table (8-byte entries)
extern GameState* g_game;         // 0x5e2930
extern ModeRec*   g_modes;        // 0x5e2a00  mode table (0x58-byte records)
extern SndRec*    g_snd;          // 0x5e28f4  sound slot records (0x4c bytes)
extern int        g_snd_count;    // 0x5e29ec
extern tstring*   g_map;          // 0x5e28f8  ground-spec names
extern int        g_map_count;    // 0x5e29e8
extern PropKind*  g_map_points;   // 0x5e29dc  prop kind records (0x78 bytes)
extern int        g_dinput_enabled;   // 0x5e28fc
extern __int64    g_spawn;        // 0xa70448
extern Scene*     g_scene;        // 0xa732b4
extern int        g_env_on;       // 0x5aa028
extern int        g_no_cam;       // 0x5a9fb0
extern unsigned   g_mask_col;     // 0x5a8f60
extern unsigned   g_mask_cloud;   // 0x5a8e88
extern unsigned   g_mask_flag;    // 0x5a8e70
extern unsigned   g_mask_amt;     // 0x5a8ea0
extern unsigned   g_mask_bits;    // 0x5a8eb8
extern unsigned   g_rand_seed;    // 0x5d7078

struct Mission : Raw {
    void reset();                                            // 0x479160
    void rebuild_nav(int which, int mode);                   // 0x479e20
    void setup_ambient(int level_idx, int spec_idx, float t);   // 0x4792d0
    void build_interior(int which, Mesh** out, int* n);      // 0x47a800
    void spawn_prop(int* idx, int* out);                     // 0x44c210
    void update_prop_flags();                                // 0x472050
    void f47ced0();                                          // 0x47ced0
    void f47ce00(Mesh** meshes, int n);                      // 0x47ce00
    void copy_slot(int k);                                   // 0x4676e0
    void f4739f0(int id);                                    // 0x4739f0
    void add_amount(int i, unsigned n);                      // 0x4677a0
    void f483eb0();                                          // 0x483eb0
    void f483f60(float a, int b);                            // 0x483f60
    void start_setup();                                      // 0x484590
};

// WinLicense SecureEngine marker (SDK macro): jmp over 16 bytes 'WL  ' id 0 0 0 0 0 0 0 'WL  '
#define WL_MARK(id) __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x57 __asm _emit 0x4c __asm _emit 0x20 __asm _emit 0x20 __asm _emit id __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0x57 __asm _emit 0x4c __asm _emit 0x20 __asm _emit 0x20

#define RND() ((float)(rand() % 0x3dc9))
#define RNDU(s) ((float)((s) % 0x3dc9))

static inline unsigned lcg(unsigned x) { return (x * 0x1c13 - 0xd098556) * 0x1105; }

static inline void put(unsigned& dst, unsigned mask, int shift, unsigned v)
{
    if (v > mask) v = mask;
    dst = (dst & ~(mask << shift)) | (v << shift);
}

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
    if (!(x - f <= 0.5f)) r++;
    return r;
}

static inline int ifloor(float x)
{
    if (!(x > 0.0f)) x -= 0.999999881f;
    return (int)x;
}

static inline Party* party_rec(PartyMgr* pm, int id)
{
    int i = pm->index_of(id);
    Party*& pg = pm->pages[i >> 4];
    return (Party*)((char*)pg + (i & 0xf) * 0x5ac8);
}

#define LV (*(Level*)(g_game->at<char*>(0x13a50) + at<int>(0x5df4) * 0x494))

// ---------------------------------------------------------------------------
// @ 0x484590
// ---------------------------------------------------------------------------
void Mission::start_setup()
{
    Mission* m = this;
    vector4 t20, u, v, in, out;
    mat3 tmp;
    int ptype;

    WL_MARK(4);
    reset();
    at<__int64>(0x62f0) = g_time[at<int>(0x62e8)];
    at<__int64>(0x6300) = g_time[at<int>(0x62f8)];
    at<__int64>(0x6310) = g_time[at<int>(0x6308)];
    at<__int64>(0x6320) = g_time[at<int>(0x6318)];
    ((log_base&)g_log).w_cstr("begin_setup");
    g_phys.f47dc50();

    ModeRec* mrec = (ModeRec*)((char*)g_modes + at<int>(0x5df0) * 0x58);
    Level* lrec = (Level*)(g_game->at<char*>(0x13a50) + at<int>(0x5df4) * 0x494);
    at<int>(0x6550) = 0;
    mrec->bank.randomize();
    if (lrec->at<char>(0x78) == 0)
        g_game->load_scene(at<int>(0x5df4));

    // ---- camera party + neighbour temperature accumulation ---------------------------------
    PartyMgr* pm = (PartyMgr*)(g_game->at<char*>(0xf8));
    pm = (PartyMgr*)((char*)g_game + 0xf8);
    int pid = g_game->at<int>(0x78);
    Party* pa = party_rec(pm, pid);
    ptype = party_rec(pm, pid)->at<int>(0x5978);
    volatile float temp[16];
    temp[0] = 0.0f; temp[1] = 0.0f; temp[2] = 0.0f; temp[3] = 0.0f;
    temp[4] = 0.0f; temp[5] = 0.0f; temp[6] = 0.0f; temp[7] = 0.0f;
    temp[8] = 0.0f; temp[9] = 0.0f; temp[10] = 0.0f; temp[11] = 0.0f;
    temp[12] = 0.0f; temp[13] = 0.0f; temp[14] = 0.0f; temp[15] = 0.0f;
    u = party_rec(pm, pid)->pos;

    if (g_scene != 0) {
        for (unsigned p = (unsigned)g_scene->objs->first; p < (unsigned)g_scene->objs->last; p += 0x88) {
            float dx = *(float*)(p + 0x74) - u.x;
            float dy = *(float*)(p + 0x78) - u.y;
            float d2 = dy * dy + dx * dx;
            if (d2 < 1.0f)
                d2 = 1.0f;
            else if (!(d2 < 324.0f))
                continue;
            int slot = *(int*)(p + 0x24);
            if (slot >= 0 && slot < 16)
                temp[slot] = 10.0f / d2 + temp[slot];
        }
    } else {
        temp[0] = 0.2f; temp[1] = 0.2f; temp[2] = 0.2f; temp[3] = 0.2f;
        temp[4] = 0.2f; temp[5] = 0.2f; temp[6] = 0.2f; temp[7] = 0.2f;
        temp[8] = 0.2f; temp[9] = 0.2f; temp[10] = 0.2f; temp[11] = 0.2f;
        temp[12] = 0.2f; temp[13] = 0.2f; temp[14] = 0.2f; temp[15] = 0.2f;
    }

    WL_MARK(5);
    // ---- day time, anchor blend, weather ---------------------------------------------------
    at<float>(0x65e8) = (float)fmod((double)(g_time[1] * 1e-5f), 24.0);
    g_game->blend(&u.x, pa->pos.x, pa->pos.y);
    WL_MARK(0xc);

    float temper = temp[0] * 0.1f + (temp[8] * 0.01f + (temp[3] * 0.001f + (temp[11] * 0.002f +
                   (temp[4] * 0.005f + (temp[12] * 0.01f + (temp[1] * 0.003f + (temp[9] * 0.005f +
                   (temp[2] * 0.0005f + temp[10] * 0.0015f))))))));
    if (temper < 0.0f) temper = 0.0f;
    else if (temper >= 1.0f) temper = 1.0f;

    in.x = (float)g_game->at<int>(4) * 0.1f;
    in.z = 0.0f;
    in.y = g_time[1] * 1e-5f;
    noise_fractal(96.0f, 0.7f, 4, 0, &in.x, &out.x);
    float f14 = (out.z + out.y + out.x) * 5.0f;
    v.z = 700.0f;
    v.x = (float)g_game->at<int>(4) * 0.1f;
    v.y = g_time[1] * 1e-5f;
    in = v;
    noise_fractal(72.0f, 0.7f, 4, 0, &in.x, &out.x);
    float fo = (out.z + out.y + out.x) * 5.0f;

    float humidity = clamp01(((f14 + f14) + 0.5f) * temper + (RND() * 6.32231167e-05f) * 0.0500000007f);
    at<float>(0x65f4) = humidity;
    at<int>(0x65f8) = pa->at<int>(0x38);
    at<int>(0x65f0) = *(int*)&u.x;
    at<int>(0x6618) = *(int*)&u.z;
    at<int>(0x661c) = *(int*)&u.w;
    at<int>(0x6600) = 0;
    at<int>(0x6604) = 0;

    at<int>(0x65fc) = iround(((temper + 1.5f) * g_game->at<float>(0x13b3c)) * 1.5f);

    float fog = ((g_game->at<float>(0x13b3c) + 0.3f) * (temper + 0.5f)) * g_game->at<float>(0x13b40) *
                g_game->at<float>(0x13b40) * 0.00800000038f + 9.99999975e-05f;
    at<float>(0x6614) = fog;
    if (g_game->at<float>(0x13b40) > 0.899999976f) {
        fog = fog * 1.29999995f + 0.0259999987f;
        at<float>(0x6614) = fog;
        if (g_game->at<float>(0x13b40) > 0.939999998f) {
            fog = fog * 1.20000005f + 0.0240000002f;
            at<float>(0x6614) = fog;
            if (g_game->at<float>(0x13b40) > 0.980000019f)
                at<float>(0x6614) = fog + 0.00999999978f;
        }
    }

    at<float>(0x6610) = (RND() * 3.16115584e-05f + 0.5f) * (1.20000005f - humidity) * 2000.0f;
    if (at<float>(0x65e8) > 3.0f && at<float>(0x65e8) < 10.0f)
        at<float>(0x6610) = (RND() * 4.42561795e-05f + 0.300000012f) * at<float>(0x6610);

    if (g_game->at<float>(0x13b3c) > 0.600000024f) {
        if (humidity * 0.100000001f + ((RND() * 6.32231167e-05f) * 0.0500000007f + g_game->at<float>(0x13b3c) * 0.200000003f) +
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
                at<float>(0x6614) = *(volatile float*)&at<float>(0x6614) * 1.5f + amt * 0.00374999992f;
            }
        }
    }
    if (at<float>(0x6610) < 60.0f) at<float>(0x6610) = 60.0f;
    if (at<float>(0x6614) > 0.150000006f) at<float>(0x6614) = 0.150000006f;

    WL_MARK(0xd);
    {
        void* o = pa->at<void*>(0x5974);
        if (o != 0)
            at<int>(0x65ec) = *(int*)((char*)o + 4);
        else
            at<int>(0x65ec) = 0;
    }

    RecVec& props = at<RecVec>(0x6688);
    at<int>(0x6608) = ptype;
    props = *(RecVec*)((char*)lrec + 0x104);
    for (unsigned i = 0; i < props.size(); i++) {
        Rec1c8* pe = at<Rec1c8*>(0x668c) + i;
        pe->at<unsigned>(0x14) = i;
    }

    g_spawn = ptype;

    {
        SndBank* b = &g_modes[at<int>(0x5df0)].bank;
        for (int i = 0; i < b->count; i++)
            b->recs[i].f0c = 0;
    }
    {
        SndBank* b = &g_modes[at<int>(0x5df0)].bank;
        for (int i = 0; i < b->count; i++)
            b->recs[i].update(-0x13);
    }
    at<tstring>(0x6814) = *(tstring*)((char*)lrec + 0x484);

    // ---- ambient sound slot selection -------------------------------------------------------
    for (int i = 0; i < g_snd_count; i++) {
        SndRec* s = &g_snd[i];
        float r = RND() * 6.32231149e-07f;
        s->at<float>(0x48) = r;
        if ((at<float>(0x65e8) > 7.0f && at<float>(0x65e8) < 17.0f) ? (s->at<int>(0x10) & 3) == 0 :
            (at<float>(0x65e8) > 4.0f && at<float>(0x65e8) < 20.0f) ? (s->at<int>(0x10) & 3) == 1 :
            (s->at<int>(0x10) & 3) == 2)
            s->at<float>(0x48) = r + 1.0f;
        s->at<float>(0x48) = s->at<float>(0x48) - abs(at<int>(0x65fc) - ((s->at<int>(0x10) >> 4) & 0xf)) * 0.100000001f;
    }
    at<int>(0x660c) = 0;
    for (int i = 1; i < g_snd_count; i++)
        if (g_snd[i].at<float>(0x48) > g_snd[at<int>(0x660c)].at<float>(0x48))
            at<int>(0x660c) = i;

    // ---- sky / cloud / fog bit fields -------------------------------------------------------
    if (LV.flags.b8) {
        g_terrain.f4658c0((char*)(g_game->at<char*>(0x13a50) + at<int>(0x5df4) * 0x494) + 0x68);
        GameState* gg = g_game;
        if (((Level*)(gg->at<char*>(0x13a50) + at<int>(0x5df4) * 0x494))->flags.b9) {
            PartyMgr* pm2 = (PartyMgr*)((char*)gg + 0xf8);
            int pid2 = gg->at<int>(0x78);
            int x = (int)party_rec(pm2, pid2)->pos.x;
            int y = (int)party_rec(pm2, pid2)->pos.y;
            int parity = at<int>(0x5270) % 3;
            int fr = gg->at<int>(4);
            unsigned a = (fr * 0x1b0b + parity + y * 0x230b + x * 17) & 0x7fffff;
            unsigned c = (fr * 0xf4d + parity + y * 0x16a1 + x * 19) & 0x7fffff;
            unsigned b = (y * 0x19c9 + parity + fr * 0x1e6f + x * 31) & 0x7fffff;
            unsigned mc = g_mask_col - 1;
            put(g_terrain.s0, mc, 0, a);
            put(g_terrain.s2, mc, 0, c);
            put(g_terrain.s1, mc, 0, b);

            unsigned seed = (at<int>(0x5270) % 3 + g_game->at<int>(4) * 0x4abb + y * 0xb1 + x * 0x17 + 0x8a767) *
                            0x1ddcf5f + 0x2f784719;
            int storm = 0;
            float chance = RNDU(seed) * 6.32231167e-05f;
            if (ptype == 7) {
                storm = 1;
                ptype = 3;
            } else if (temp[8] > 27.0f) {
                storm = 1;
            } else if ((ptype == 3 || ptype == 0xb) && chance < 0.150000006f) {
                storm = 1;
            }
            put(g_terrain.s4, g_mask_cloud - 1, 28, ptype);
            put(g_terrain.s3, g_mask_flag - 1, 31, storm != 0);
            put(g_terrain.s3, g_mask_flag - 1, 30, 0);
            seed = lcg(seed);
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
            else if (ff >= 1.0f) ff = 1.0f;
            int ai = iround(ff * 100.0f);
            if (ai < 0) ai = 0;
            else if (ai >= 100) ai = 99;
            put(g_terrain.s4, g_mask_amt - 1, 0, ai);

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
            put(g_terrain.s4, g_mask_amt - 1, 14, cc);

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
            put(g_terrain.s4, g_mask_amt - 1, 7, ti);

            int ri = iround((RND() * 1.2644623e-05f + 0.800000012f) * 100.0f);
            if (ri < 0) ri = 0;
            else if (ri >= 100) ri = 99;
            put(g_terrain.s4, g_mask_amt - 1, 21, ri);
        }
        at<Rect>(0x652c).x0 = vec2(0.0f, 0.0f).x;
        at<Rect>(0x652c).y0 = vec2(0.0f, 0.0f).y;
        at<float>(0x6534) = (float)(g_terrain.s3 & (g_mask_bits - 1));
        at<float>(0x6538) = (float)((g_terrain.s3 >> 10) & (g_mask_bits - 1));
        at<Rect>(0x653c) = at<Rect>(0x652c);
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
    g_tstr = at<tstring>(0x6814);

    // ---- prop bounding box, two passes ------------------------------------------------------
    for (unsigned i = 0; i < props.size(); i++) {
        Rec1c8* p = &at<Rec1c8*>(0x668c)[i];
        if (p->at<int>(0x188) == 0 && p->at<int>(0x18c) >= 0) {
            PropKind* k = &g_map_points[p->at<int>(0x18c)];
            v.x = (p->at<float>(0x28) * k->at<float>(0x5c) + p->at<float>(0x38) * k->at<float>(0x60) +
                   p->at<float>(0x18) * k->at<float>(0x58)) + p->at<float>(0x48);
            v.y = (p->at<float>(0x2c) * k->at<float>(0x5c) + p->at<float>(0x3c) * k->at<float>(0x60) +
                   p->at<float>(0x1c) * k->at<float>(0x58)) + p->at<float>(0x4c);
            v.z = (p->at<float>(0x30) * k->at<float>(0x5c) + p->at<float>(0x40) * k->at<float>(0x60) +
                   p->at<float>(0x20) * k->at<float>(0x58)) + p->at<float>(0x50);
            vector4 hi = v;
            v.x -= k->at<float>(0x68); v.y -= k->at<float>(0x68); hi.x += k->at<float>(0x68); hi.y += k->at<float>(0x68);
            Rect& box = at<Rect>(0x652c);
            box.x0 = box.x0 < v.x ? box.x0 : v.x;
            box.y0 = box.y0 < v.y ? box.y0 : v.y;
            box.x1 = box.x1 > hi.x ? box.x1 : hi.x;
            box.y1 = box.y1 > hi.y ? box.y1 : hi.y;
        }
    }
    for (unsigned i = 0; i < props.size(); i++) {
        Rec1c8* p = &at<Rec1c8*>(0x668c)[i];
        if (p->at<int>(0x188) == 0 && p->at<int>(0x18c) >= 0) {
            PropKind* k = &g_map_points[p->at<int>(0x18c)];
            v.x = (p->at<float>(0x38) * k->at<float>(0x60) + p->at<float>(0x28) * k->at<float>(0x5c) +
                   p->at<float>(0x18) * k->at<float>(0x58)) + p->at<float>(0x48);
            v.y = (p->at<float>(0x1c) * k->at<float>(0x58) + p->at<float>(0x3c) * k->at<float>(0x60) +
                   p->at<float>(0x2c) * k->at<float>(0x5c)) + p->at<float>(0x4c);
            v.z = (p->at<float>(0x20) * k->at<float>(0x58) + p->at<float>(0x40) * k->at<float>(0x60) +
                   p->at<float>(0x30) * k->at<float>(0x5c)) + p->at<float>(0x50);
            vector4 hi = v;
            v.x -= k->at<float>(0x68); v.y -= k->at<float>(0x68); hi.x += k->at<float>(0x68); hi.y += k->at<float>(0x68);
            Rect& box = at<Rect>(0x652c);
            box.x0 = box.x0 < v.x ? box.x0 : v.x;
            box.y0 = box.y0 < v.y ? box.y0 : v.y;
            box.x1 = box.x1 > hi.x ? box.x1 : hi.x;
            box.y1 = box.y1 > hi.y ? box.y1 : hi.y;
        }
    }

    // ---- fade, params, ambient, sun ---------------------------------------------------------
    rebuild_nav(at<int>(0x5df4), at<int>(0x5df0));
    at<int>(0x5274) = 0;
    at<World*>(0)->set_fade(&at<float>(0x6614), (char*)&g_snd[at<int>(0x660c)] + 0x44);
    unsigned color = 0xff334455;
    at<World*>(0)->set_params(-5.0f, 10.0f, &color, &at<float>(0x654c));
    setup_ambient(at<int>(0x5df4), at<int>(0x660c), at<float>(0x6614));

    Entity* sun = at<World*>(0)->create_entity(9);
    Mesh* sm = g_resmgr.get_object(&g_snd[at<int>(0x660c)]) ->clone();
    sm->scale(200.0f);
    sm->at<unsigned>(0x10c) &= 0xfffeffff;
    {
        int mi = (int)sm;
        sun->at<IntVec>(0x118).push_back(mi);
    }
    sun->v4();
    {
        GameState* g1 = g_game;
        int secs = (int)((g_time[g1->at<int>(0x13b18)] - g1->at<__int64>(0x13b20)) * 1e-5f);
        int h = secs % 24;
        float angle = g_snd[at<int>(0x660c)].at<float>(0x14) * 0.0174532924f -
                      (float)(h - 12) * 0.0174532924f * 15.0;
        sun->at<matrix>(0x58).rotate_sf(&angle);
        angle = 1.0e8f;
        sun->v6(&angle);
    }
    at<World*>(0)->at<Entity*>(0x78) = sun;

    // ---- terrain generate / interior ----------------------------------------------------------
    if (LV.flags.b8) {
        g_emit_pos[0] = at<World*>(0)->at<vector4>(0x68);
        g_emit_pos[1] = *(vector4*)(at<char*>(0x65b0) + 4);
        Streamer* s414 = (Streamer*)((char*)lrec + 0x414);
        char flagc = 1;
        s414->v1();
        if (s414->v23() == 0) {
            if (LV.flags.b8) {
                Streamer* s2 = (Streamer*)(g_game->at<char*>(0x13a50) + at<int>(0x5df4) * 0x494 + 0x414);
                s2->v1();
                at<TerrainDesc>(0x63d0).read(s2);
            }
            flagc = 0;
        }
        g_terrain.generate(at<World*>(0), &at<TerrainDesc>(0x63d0), flagc);
    } else {
        build_interior(at<int>(0x5df4), &at<Mesh*>(0x6340), &at<int>(0x633c));
    }
    Streamer* s444 = (Streamer*)((char*)lrec + 0x444);
    s444->v1();
    NavMesh& nav = at<NavMesh>(0x6698);
    nav.f4781d0(s444);
    nav.f470a00();
    {
        LevelBits f2 = LV.flags;
        if (f2.b9 && f2.b8 && g_terrain.count_low_quads() > 10) {
            nav.f4785e0(at<World*>(0)->at<int>(0x64), 1);
            nav.f470a00();
            nav.f470750();
        }
    }
    at<World*>(0)->f478c50(&nav);

    // ---- light/frame table: 64 slots + 5 extra for a custom camera --------------------------
    Slot* slots = &at<Slot>(0x670);
    at<int>(0x5278) = 0x40;
    at<int>(0x5274) = 0;
    vec2 center((at<float>(0x6544) - at<float>(0x653c)) * 0.5f + at<float>(0x653c),
                (at<float>(0x6548) - at<float>(0x6540)) * 0.5f + at<float>(0x6540));
    vec2 lofs(0.0f, 0.0f);
    if (lrec->flags.b8) lofs = center;
    v.z = 500.0f;
    for (int k = 0; k < 0x40; k++) {
        *(mat3*)&slots[k].A = *identity3(&tmp);
        v.x = (float)(k % 8 + 1) + lofs.x;
        v.y = (float)(k / 8 + 1) + lofs.y;
        slots[k].A.r[3] = v;
    }

    if (LV.flags.b10) {
        at<int>(0x5278) = 5;
        int par = rand() % 2;
        float* box = &at<float>(0x653c);
        float* vf = &v.x;
        float* uf = &u.x;
        for (int s = 0; s < 5; s++) {
            float a = (RND() * 6.32231167e-05f) * 0.400000006f + 0.300000012f;
            vf[1 - par] = a * (box[3 - par] - box[1 - par]) + box[1 - par];
            float b = (((RND() * 6.32231167e-05f - 0.5f) * 0.100000001f + s) * 0.25f) * 0.600000024f + 0.200000003f;
            if (s < 2)
                b = b * 0.75f;
            else if (s > 2)
                b = 1.0f - (1.0f - b) * 0.75f;
            vf[par] = b * (box[2 + par] - box[par]) + box[par];
            v.z = 1000.0f;
            Slot* sl = &slots[s];
            *(mat3*)&sl->A = *identity3(&tmp);
            sl->A.r[3] = v;
            u.z = 0.0f;
            uf[1 - par] = 0.0f;
            if (s < 2) uf[par] = 1.0f;
            else uf[par] = -1.0f;
            sl->A.r[1] = u;
            sl->A.orthonormalize();
            sl->B = sl->A;
        }
    }
    if (LV.flags.b8) {
        at<float>(0x653c) = at<float>(0x653c) + 24.0f;
        at<float>(0x6540) = at<float>(0x6540) + 24.0f;
        at<float>(0x6544) = at<float>(0x6544) - 24.0f;
        at<float>(0x6548) = at<float>(0x6548) - 24.0f;
    }

    // ---- per-prop frame copy into the table ----------------------------------------------------
    for (unsigned i = 0; i < props.size(); i++) {
        Rec1c8* p = &at<Rec1c8*>(0x668c)[i];
        int sl = p->at<int>(0x1a0);
        if (p->at<int>(0x188) == 1 && sl >= 0 && sl < 0x40) {
            Slot* s = &slots[sl];
            s->A.r[3] = p->at<vector4>(0x48);
            *(mat3*)&s->A = p->at<mat3>(0x18);
            s->B = s->A;
            u.x = 0.0f; u.y = 0.0f; u.z = 1.0f;
            s->B.r[2] = u;
            v.x = s->B.r[2].z * s->B.r[1].y - s->B.r[2].y * s->B.r[1].z;
            v.y = s->B.r[2].x * s->B.r[1].z - s->B.r[1].x * s->B.r[2].z;
            v.z = s->B.r[1].x * s->B.r[2].y - s->B.r[2].x * s->B.r[1].y;
            s->B.r[0] = v;
            norm3(&s->B.r[0].x);
            t20.x = s->B.r[2].y * s->B.r[0].z - s->B.r[0].y * s->B.r[2].z;
            t20.y = s->B.r[0].x * s->B.r[2].z - s->B.r[2].x * s->B.r[0].z;
            t20.z = s->B.r[0].y * s->B.r[2].x - s->B.r[0].x * s->B.r[2].y;
            s->B.r[1] = t20;
            s->C = s->B;
            s->D = s->B;
        }
    }

    // ---- prop ground / spawn pass --------------------------------------------------------------
    int pi;
    for (pi = 0; pi < (int)props.size(); ) {
        Rec1c8* p = &at<Rec1c8*>(0x668c)[pi];
        p->at<int>(0x1b4) = 0;
        if (g_no_cam == 0 && p->at<int>(0x188) == 4 && LV.flags.b8) {
            int j;
            for (j = 0; j < g_map_count; j++)
                if (g_map[j].equals(p->at<tstring>(0x190)))
                    goto found;
            j = 0;
        found:
            g_terrain.emit_by_type(j, &p->at<matrix>(0x18), &p->at<vector4>(0x158), p->at<int>(0x1a0));
            pi++;
        } else {
            int foundv = -1;
            spawn_prop(&pi, &foundv);
            pi++;
        }
    }
    update_prop_flags();
    {
        LevelBits f = LV.flags;
        if (f.b8)
            g_terrain.regenerate_water(&at<void*>(0x65a8), f.b9 == 0);
    }
    f47ced0();
    if (!LV.flags.b8)
        f47ce00(&at<Mesh*>(0x6340), at<int>(0x633c));
    at<int>(0x65bc) = 0;
    at<int>(0x65b8) = 0;

    // ---- scene effect object (rain / snow emitter) ---------------------------------------------
    if (g_env_on != 0 && !lrec->flags.b0 && at<int>(0x6604) > 0) {
        SndRec* sr = &g_snd[at<int>(0x660c)];
        u = sr->at<vector4>(0x30);
        float scale = (u.x * 3.0f + 1.0f) * 0.400000006f;
        Entity* fx = at<World*>(0)->create_entity(9);
        at<Entity*>(0x65b8) = fx;
        u.x = 0.0f; u.y = 0.0f; u.z = 30.0f;
        fx->at<vector4>(0x88) = u;
        float far = 1.0e7f;
        at<Entity*>(0x65b8)->v6(&far);
        matrix mx;
        u.x = 0.0f; u.y = 0.0f; u.z = 0.0f;
        *(mat3*)&mx.r[0] = *identity3(&tmp);
        t20.x = at<float>(0x6618) * 0.349999994f;
        t20.y = at<float>(0x661c) * 0.349999994f;
        mx.r[3] = u;
        t20.z = -10.0f;
        if (at<int>(0x6604) == 1) {
            mx.r[3].z = 6.0f;
            mx.r[3].y = 6.5f;
            int ix = g_emit_defs.idx1;
            if (ix >= 0 && ix < g_emit_defs.count) {
                Emit* em = new Emit;
                g_emit_defs.apply(em, ix, &mx.r[3]);
                if (em != 0) {
                    float inv = 1.0f / em->at<float>(0x24);
                    u.x = t20.x * inv;
                    u.y = t20.y * inv;
                    u.z = inv * -10.0f;
                    em->at<vector4>(0x184) = u;
                    em->at<vector4>(0xc4) = t20;
                    em->at<vector4>(0xb4) = t20;
                    norm3(&em->at<float>(0xb4));
                    em->at<float>(0x198) = at<float>(0x6600) * em->at<float>(0x198);
                    em->at<float>(0xa0) = 11.0f / (float)(10.0 / em->at<float>(0x24));
                    em->at<float>(0x3c) = scale * em->at<float>(0x3c);
                    em->at<float>(0x44) = scale * em->at<float>(0x44);
                    em->at<float>(0x4c) = scale * em->at<float>(0x4c);
                    em->at<float>(0x54) = scale * em->at<float>(0x54);
                    em->at<float>(0x5c) = scale * em->at<float>(0x5c);
                    em->at<float>(0x64) = scale * em->at<float>(0x64);
                    at<Entity*>(0x65b8)->v25(em);
                }
            }
        } else if (at<int>(0x6604) == 2) {
            mx.r[3].z = 6.0f;
            mx.r[3].y = 6.5f;
            int ix = g_emit_defs.idx2;
            if (ix >= 0 && ix < g_emit_defs.count) {
                Emit* em = new Emit;
                g_emit_defs.apply(em, ix, &mx.r[3]);
                if (em != 0) {
                    em->at<vector4>(0xc4) = t20;
                    em->at<float>(0x3c) = scale * em->at<float>(0x3c);
                    em->at<float>(0x44) = scale * em->at<float>(0x44);
                    em->at<float>(0x4c) = scale * em->at<float>(0x4c);
                    em->at<float>(0x54) = scale * em->at<float>(0x54);
                    em->at<float>(0x5c) = scale * em->at<float>(0x5c);
                    em->at<float>(0x64) = scale * em->at<float>(0x64);
                    em->at<vector4>(0xb4) = t20;
                    norm3(&em->at<float>(0xb4));
                    em->at<float>(0x198) = at<float>(0x6600) * em->at<float>(0x198);
                    at<Entity*>(0x65b8)->v25(em);
                }
            }
        }
    }

    // ---- intro billboard (sun mesh) -------------------------------------------------------------
    at<int>(0x65c0) = 0;
    at<int>(0x65c4) = 0;
    at<int>(0x65c8) = 0;
    if (!lrec->flags.b0 && g_env_on != 0) {
        Entity* eb = at<World*>(0)->create_entity(9);
        at<Entity*>(0x65c0) = eb;
        float two = 2.0f;
        eb->v6(&two);
        at<Mesh*>(0x65c4) = new Mesh;
        at<Mesh*>(0x65c4)->at<unsigned>(0x10c) &= 0xfffeffff;
        at<Mesh*>(0x65c4)->at<int>(0x114) = 1;
        at<Mesh*>(0x65c4)->at<int>(0x140) = 1;
        {
            Entity* e2 = at<Entity*>(0x65c0);
            int mi = (int)at<Mesh*>(0x65c4);
            e2->at<IntVec>(0x118).push_back(mi);
            e2->v4();
        }
        {
            tstring nm("sun");
            at<Mesh*>(0x65c8) = g_resmgr.get_object(nm)->clone();
        }
        at<Mesh*>(0x65c8)->at<int>(0x114) = 1;
        at<Mesh*>(0x65c8)->at<int>(0x140) = 1;
        float c = (1.0f - 150.0f / (at<float>(0x6610) + 150.0f)) * g_snd[at<int>(0x660c)].at<float>(0x1c) * 0.899999976f;
        u.x = u.y = u.z = c;
        at<Mesh*>(0x65c8)->at<unsigned>(0x3c) = pack_color(&u.x);
        at<Mesh*>(0x65c8)->at<unsigned>(0x3c) |= 0xff000000;
        at<Mesh*>(0x65c8)->at<unsigned>(0x10c) &= 0xfffeffff;
        at<Mesh*>(0x65c8)->scale(50.0f);
        {
            Entity* e2 = at<Entity*>(0x65c0);
            int mi = (int)at<Mesh*>(0x65c8);
            e2->at<IntVec>(0x118).push_back(mi);
            e2->v4();
        }
    }

    // ---- cursor / vertex-colour helper entities -------------------------------------------------
    at<int>(0x65d0) = 0;
    at<Entity*>(0x65d0) = at<World*>(0)->create_entity(9);
    {
        float f290 = 290.5f;
        at<Entity*>(0x65d0)->v6(&f290);
    }
    {
        tstring nm("edit_cursor");
        at<Mesh*>(0x65d4) = g_resmgr.get_object(nm)->clone();
    }
    {
        Entity* e2 = at<Entity*>(0x65d0);
        int mi = (int)at<Mesh*>(0x65d4);
        e2->at<IntVec>(0x118).push_back(mi);
        e2->v4();
    }
    at<Mesh*>(0x65d8) = new Mesh;
    {
        tstring nm("vertex_color_add_mat");
        at<Mesh*>(0x65d8)->at<void*>(0x2c) = g_resmgr.get_material(&nm);
    }
    at<Entity*>(0x65d0)->add_mesh(at<Mesh*>(0x65d8));
    at<int>(0x65dc) = 0;
    at<Entity*>(0x65dc) = at<World*>(0)->create_entity(9);
    {
        float f33 = 33.5f;
        at<Entity*>(0x65dc)->v6(&f33);
    }
    at<Mesh*>(0x65e0) = new Mesh;
    {
        tstring nm("vertex_color_add_mat");
        at<Mesh*>(0x65e0)->at<void*>(0x2c) = g_resmgr.get_material(&nm);
    }
    at<Entity*>(0x65dc)->add_mesh(at<Mesh*>(0x65e0));
    at<Entity*>(0x65e4) = at<World*>(0)->create_entity(9);
    {
        float f33 = 33.5f;
        at<Entity*>(0x65e4)->v6(&f33);
    }
    at<Entity*>(0x65d0)->at<char>(0x140) = 0;
    at<Entity*>(0x65dc)->at<char>(0x140) = 0;
    at<Entity*>(0x65e4)->at<char>(0x140) = 0;

    // ---- counters, light table ground snap -------------------------------------------------------
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 6; j++) {
            at<int>(0x621c + (i * 6 + j) * 4) = 0;
            at<float>(0x62dc) = g_time[2] * 1e-5f;
        }
    at<int>(0x6524) = 0;
    at<char>(0x6338) = 0;
    for (int k = 0; k < 0x40; k++) {
        Slot* s = &slots[k];
        float outY;
        s->face = 0;
        int n = 0;
        do {
            at<World*>(0)->floor_at(&s->B.r[3].x, &outY, &s->face);
            if (s->face == 0) {
                s->B.r[3].x = (frand() - 0.5f) * 0.01f + s->B.r[3].x;
                s->B.r[3].y = (frand() - 0.5f) * 0.01f + s->B.r[3].y;
                n++;
                if (n > 100) {
                    s->B.r[3].x = (frand() - 0.5f) * 100.0f;
                    s->B.r[3].y = (frand() - 0.5f) * 100.0f;
                    if (n > 10000)
                        s->face = *at<World*>(0)->at<void**>(0x58);
                }
            }
        } while (s->face == 0);
        s->B.r[3].z = outY;
        s->C = s->B;
        s->D = s->B;
        s->start = g_time[s->clock];
        s->f120 = 0;
        copy_slot(k);
    }

    // ---- party spawn counters, troops, mode records ----------------------------------------------
    {
        int* lists = &at<int>(0x5e0c);
        int* cnts = &at<int>(0x620c);
        for (int sd = 0; sd < 2; sd++)
            for (int i = 0; i < cnts[sd]; i++)
                f4739f0(lists[sd * 128 + i]);
    }
    for (int i = 0; i < g_game->at<int>(0x13a04); i++)
        *(int*)(g_game->at<char*>(0x13a00) + i * 0x508 + 0x3bc) = 0;
    memset(&at<char>(0x6824), 0, 0x40);
    for (int i = 0; i < mrec->nrecs; i++) {
        mrec->recs[i].f18 = 0;
        mrec->recs[i].f1c = 0;
    }
    for (int i = 0; i < mrec->nrecs; i++)
        add_amount(i, mrec->recs[i].f14);
    f483eb0();
    char buf[0x50];
    sprintf(buf, "temper = %3.2f, humidity = %3.2f, height = %3.2f fog_far = %d",
            (double)at<float>(0x65f0), (double)at<float>(0x65f4), (double)at<float>(0x65f8), (int)at<float>(0x6610));
    for (int i = 0; i < 2; i++)
        f483f60(0.100000001f, 1);

    // ---- placed-prop jitter + ground snap ----------------------------------------------------------
    for (unsigned i = 0; i < props.size(); i++) {
        Rec1c8* p = &at<Rec1c8*>(0x668c)[i];
        if (p->at<int>(0x188) == 0 && (g_map_points[p->at<int>(0x18c)].at<unsigned>(0x10) & 0x1000)) {
            float outY;
            void* sink;
            v = at<vector4>(0x6598);
            v.x = (RND() * 6.32231167e-05f - 0.5f) * 2.0f + v.x;
            v.y = (RND() * 6.32231167e-05f - 0.5f) * 2.0f + v.y;
            at<World*>(0)->floor_at(&v.x, &outY, &sink);
            v.z = outY;
            p->at<vector4>(0x48) = v;
            if (p->at<PropObj*>(0x1b4) != 0)
                p->at<PropObj*>(0x1b4)->v22(&p->at<matrix>(0x18), &p->at<vector4>(0x158));
        }
    }

    // ---- water / particle system teardown or rebuild --------------------------------------------------
    {
        LevelBits f3 = LV.flags;
        if (f3.b8) {
            g_terrain.destroy_water(&at<char>(0x63c0));
        } else if (!f3.b0 && g_dinput_enabled) {
            g_psys.f4711e0(at<World*>(0));
            for (unsigned i = 0; i < at<World*>(0)->at<BodyVec*>(0xb0)->size(); i++)
                g_psys.f468c20(&at<World*>(0)->at<BodyVec*>(0xb0)->first[i]);
            g_psys.f463cc0();
            g_psys.reset();
        }
    }
    ((log_base&)g_log).w_cstr("end_setup");
    at<char>(0x6555) = 0;
}

// ---------------------------------------------------------------------------
// @ 0x4875a0 (same code as round_float 0x432540; a long long is what keeps the add/adc pair)
// ---------------------------------------------------------------------------
int __cdecl near_round(const float* p)
{
    float fl = (float)floor((double)*p);
    long long i = (long long)fl;
    if (!(*p - fl <= 0.5f))
        i += 1;
    return (int)i;
}
