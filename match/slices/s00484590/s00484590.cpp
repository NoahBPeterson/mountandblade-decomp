// Slice s00484590: Mission::start_setup (0x484590, 12171 bytes) -- the per-mission start-up sequence:
//   reset mission state and seed the four clock/tick pairs from the global tick table,
//   pick the level + mode, seed the RNG/time state, accumulate the local temperature/humidity
//   from the neighboring agents on the level, derive the weather (temperature, humidity, height,
//   fog distance/amount, rain amount/mode) through the noise field, fill the sky/cloud/fog bit
//   fields, choose the ambient sound slot, set up the sun, snap the placed props onto the ground,
//   build the light table (64 slots, 5 extra for a custom-camera level), run the prop/spawn pass,
//   create the scene effect objects and the intro billboard/UI meshes, then reset the light table,
//   the party spawn counters, the agent counters and the UI, and log "end_setup".
//
// This is a RECONSTRUCTION: the function is ~12 KB. The largest function matched byte-exactly
// anywhere in this repo is ~1996 bytes (frustum::build_planes, s0041e470) and no manifest row
// exceeds 2000 bytes, so a byte-exact match is out of reach for this function. The source below
// follows the original's sequence and calls phase by phase but is not byte-exact and a few of the
// decompiler's lost stack temporaries (light-slot/emitter bookkeeping) are approximated; it is
// recorded in partial.txt, not nonmatching.txt.
//
// Field/VE offsets are recovered from the disassembly (work/match/scratch_s00484590_dis.txt) and
// the Ghidra decompile; names are provisional and follow the neighbouring slices. The Mission is
// accessed as the raw dword array `d[]` (param_1 in the decompile) so that every offset stays
// visible; external records are accessed through byte offsets.
#include <math.h>
#include <stdlib.h>

namespace rgl {
struct vector4 { float x, y, z, w; };
}

// ---------------------------------------------------------------------------
// CRT / engine primitives (never decompiled; symbol only)
// ---------------------------------------------------------------------------
extern "C" int    _rand();                              // 0x536a9f
extern "C" int    _sprintf(char*, const char*, ...);    // 0x536f8f
extern "C" double _floor(double);                       // 0x536b10
extern "C" double f536a6a();                            // 0x536a6a (time getter)
void* operator_new(unsigned);                           // 0x53545e (operator new)

struct Mission {
    int d[0x1a90];
    void start_setup();                                 // 0x484590
};

// ---------------------------------------------------------------------------
// Globals (all `// 0x…` verified against the disassembly)
// ---------------------------------------------------------------------------
extern int     g_tab[];        // 0x5e2948  tick table: 64-bit values, lo/hi at [2k]/[2k+1]
extern __int64 g_time1;        // 0x5e2950  (used as (float) * 1e-5)
extern int     g_time2;        // 0x5e2958
extern char*   g_game;         // 0x5e2930  GameData* (loaded: `mov eax,[0x5e2930]`)
extern char*   g_modes;        // 0x5e2a00  mode table (0x58-byte records, loaded)
extern char*   g_snd;          // 0x5e28f4  sound/ground-spec slots (0x4c-byte records, loaded)
extern int     g_snd_count;    // 0x5e29ec
extern char*   g_ground;       // 0x5e28f8  ground-spec names (0x20-byte tstrings, loaded)
extern int     g_ground_count; // 0x5e29e8
extern char*   g_mode_recs;    // 0x5e29dc  mode records (0x78 bytes, loaded)
extern char*   g_level;        // 0xa732b4  (loaded, null-checked then dereferenced)
extern unsigned g_sky0;        // 0xa76630
extern unsigned g_sky1;        // 0xa76634
extern unsigned g_sky2;        // 0xa76638
extern unsigned g_sky3;        // 0xa7663c
extern unsigned g_sky4;        // 0xa76640
extern int     g_mask_col;     // 0x5a8f60
extern int     g_mask_cloud;   // 0x5a8e88
extern int     g_mask_flag;    // 0x5a8e70
extern int     g_mask_amt;     // 0x5a8ea0
extern int     g_mask_bits;    // 0x5a8eb8
extern int     g_rand_seed;    // 0x5d7078
extern int     g_emit_count;   // 0x08ed5094
extern int     g_emit_idx1;    // 0x08ed5098
extern int     g_emit_idx2;    // 0x08ed509c
extern float   g_emit_pos[];   // 0x08eb235c (matrix + origin for the emit table)
extern int     g_a70448;       // 0xa70448
extern int     g_a7044c;       // 0xa7044c
extern int     g_no_cam;       // 0x5a9fb0
extern int     g_env_on;       // 0x5aa028
extern int     g_env2;         // 0x5e28fc

extern const char g_s_begin_setup[];  // 0x591b08 "begin_setup"
extern const char g_s_end_setup[];    // 0x591a54 "end_setup"
extern const char g_s_mesh_name[];    // 0x591ab8
extern const char g_s_name2[];        // 0x591ac4
extern const char g_s_cursor[];       // 0x591aa0 "edit_cursor"
extern const char g_s_vcolor[];       // 0x591a8c "vertex_color_add_mat"
extern const char g_s_fmt[];          // 0x591a60 "temper = %3.2f, humidity = %3.2f, height = %3.2f fog_far = %d"

// ---------------------------------------------------------------------------
// Callees (see the slice notes; conventions loosely reproduced)
// ---------------------------------------------------------------------------
void  f479160(Mission*);                              // 0x479160  mission reset
void  f47dc50(Mission*);                              // 0x47dc50
void  f462790();                                      // 0x462790  snd_slot::randomize
void  f47fbd0(Mission*);                              // 0x47fbd0
unsigned f48ad00(Mission*);                           // 0x48ad00  PartyMgr::index_of
void  f471040(void*, void*, float, float);             // 0x471040  (this=g_game, out, x, y)
void  f414fc0(float, float, int, int, float*, float*); // 0x414fc0 noise_fractal
void  f4658c0(void*, void*);                           // 0x4658c0  (ecx=&g_sky0, arg=level+0x68)
void  f477160(Mission*);                              // 0x477160
void  f4624c0(Mission*, void*);                       // 0x4624c0
void  f462540(float, float, void*, void*);            // 0x462540
void  f4625d0();                                      // 0x4625d0  snd_slot::update
void  f42bbe0(Mission*, const char*);                 // 0x42bbe0  tstring(const char*)
void  f42bc40();                                      // 0x42bc40  ~tstring
void  f42bc50();                                      // 0x42bc50  tstring::operator=
int   f42bdd0(void*, void*);                          // 0x42bdd0  tstring::equals
void  f479e20(int, int);                              // 0x479e20
void  f4792d0(int, int, int);                         // 0x4792d0
void* f428440(int);                                   // 0x428440  create_entity
void* f419540(void*);                                 // 0x419540  resmgr::get_object
void* f41c6d0(void*);                                 // 0x41c6d0  mesh::clone
void  f44f0c0(void*, float);                          // 0x44f0c0  mesh::scale
void  f410610(void*, int, void*);                     // 0x410610  vector::_Insert_n slow path
void  f40e770(void*, float*);                         // 0x40e770  mat4::rotate_sf
void  f40cd60(void*);                                 // 0x40cd60  identity3/identity matrix
void  f407ef0(void*);                                 // 0x407ef0  matrix::orthonormalize
unsigned f40e440(float*);                             // 0x40e440  pack_color
float f40e400();                                      // 0x40e400  frand
void  f439be0(void*, float*, float*, void*);           // 0x439be0  floor_at (this=m->d[0])
void  f47a800(int, void*, void*);                     // 0x47a800
void* f412a90();                                      // 0x412a90  mesh::mesh (0x260 ctor)
void* f418a70(void*);                                 // 0x418a70  resmgr::lookup
void  f442110(void*);                                 // 0x442110  add_mesh
void  f4424c0(void*, int, void*);                     // 0x4424c0  EmitTable::apply
void  f420b40(void*);                                 // 0x420b40  emitter ctor
void  f4781d0(void*);                                 // 0x4781d0
void  f470a00();                                      // 0x470a00
int   f467530();                                      // 0x467530
void  f4785e0(int, int);                              // 0x4785e0
void  f470750();                                      // 0x470750
void  f478c50(void*);                                 // 0x478c50
void  f472050();                                      // 0x472050
void  f482170(void*, int);                            // 0x482170
void  f47ced0();                                      // 0x47ced0
void  f47ce00(void*, int);                            // 0x47ce00
void  f466d90(int, void*, void*, int);                // 0x466d90
void  f44c210(void*, void*);                          // 0x44c210  Mission::spawn_prop
void  f4676e0(float);                                 // 0x4676e0
void  f4677a0();                                      // 0x4677a0
void  f4739f0(int);                                   // 0x4739f0
void  f483eb0();                                      // 0x483eb0
void  f483f60();                                      // 0x483f60
void  f4814a0(Mission*);                              // 0x4814a0  weather spec
void  f464440(void*);                                 // 0x464440
void  f481d10(void*, void*, int);                     // 0x481d10
void  f4711e0(void*);                                 // 0x4711e0  ShadowMap::attach_camera
void  f468c20(void*);                                 // 0x468c20
void  f463cc0();                                      // 0x463cc0
void  f47fef0();                                      // 0x47fef0
struct log_base {                                     // text log stream (global object 0x5a9f60), slot 10 (+0x28) = write(cstr)
    virtual void p00(); virtual void p01(); virtual void p02(); virtual void p03(); virtual void p04();
    virtual void p05(); virtual void p06(); virtual void p07(); virtual void p08(); virtual void p09();
    virtual log_base& w_cstr(const char* s);
};
struct log_stream : log_base {
    virtual void p00(); virtual void p01(); virtual void p02(); virtual void p03(); virtual void p04();
    virtual void p05(); virtual void p06(); virtual void p07(); virtual void p08(); virtual void p09();
    virtual log_base& w_cstr(const char* s);
};
extern log_stream g_log;                                // 0x5a9f60
static inline void log_str(const char* s) { ((log_base&)g_log).w_cstr(s); }

// small helpers for raw-byte record access
static inline float&  rf0(void* p, int off) { return *(float*)((char*)p + off); }
static inline int&    ri0(void* p, int off) { return *(int*)((char*)p + off); }
static inline void*&  rp0(void* p, int off) { return *(void**)((char*)p + off); }
// float-typed dword slots of the mission object: the original writes them with fst/fstp dword
// [ebp+0x65xx] and reads them back with fld, so the stored bits must stay float (not a truncated int).
static inline float&  dfd(Mission* m, int i) { return *(float*)&m->d[i]; }

// virtual call helpers (this in ecx; the offsets are the vtable byte offsets from the decompile)
static void vcall(void* o, int slot) { void** vt = *(void***)o; ((void(*)(void*))vt[slot / 4])(o); }
static void vcall1(void* o, int slot, void* a) { void** vt = *(void***)o; ((void(*)(void*, void*))vt[slot / 4])(o, a); }
static int  vcall_i(void* o, int slot) { void** vt = *(void***)o; return ((int(*)(void*))vt[slot / 4])(o); }

// ---------------------------------------------------------------------------
// @ 0x484590
// ---------------------------------------------------------------------------
void Mission::start_setup()
{
    Mission* m = this;
    int* lv;                                   // level (scene object), lv = sceneobjs[level_idx]
    int i, j;

    // ---- reset + tick/clock seeding (0x4845b4..) ---------------------------------------
    f479160(m);
    m->d[0x18bc] = g_tab[m->d[0x18ba] * 2];
    m->d[0x18bd] = g_tab[m->d[0x18ba] * 2 + 1];
    m->d[0x18c0] = g_tab[m->d[0x18be] * 2];
    m->d[0x18c1] = g_tab[m->d[0x18be] * 2 + 1];
    m->d[0x18c4] = g_tab[m->d[0x18c2] * 2];
    m->d[0x18c5] = g_tab[m->d[0x18c2] * 2 + 1];
    m->d[0x18c8] = g_tab[m->d[0x18c6] * 2];
    m->d[0x18c9] = g_tab[m->d[0x18c6] * 2 + 1];
    log_str(g_s_begin_setup);
    f47dc50(m);
    m->d[0x1954] = 0;

    int mode = m->d[0x177c];
    lv = (int*)(m->d[0x177d] * 0x494 + *(int*)(g_game + 0x13a50));   // level
    int* level = lv;
    f462790();
    if (*((char*)lv + 0x78) == 0)
        f47fbd0(m);

    // ---- camera party + neighbour temperature accumulation (0x48473f..) ---------------
    unsigned pa = f48ad00(m);
    char* partyA = (char*)((pa & 0xf) * 0x5ac8 + *(int*)(*(int*)(g_game + 0x139c8) + (pa >> 4) * 4));
    unsigned pb = f48ad00(m);
    void* partyB_ptr = (void*)((pb & 0xf) * 0x5ac8 + 0x5978 + *(int*)(*(int*)(g_game + 0x139c8) + (pb >> 4) * 4));
    (void)partyB_ptr;

    float temp[15];
    for (i = 0; i < 15; i++) temp[i] = 0.0f;
    float weight = 0.0f;                                     // pfStack_a8

    unsigned pc = f48ad00(m);
    float* ppos = (float*)((pc & 0xf) * 0x5ac8 + 0x30 + *(int*)(*(int*)(g_game + 0x139c8) + (pc >> 4) * 4));
    float cx = ppos[0], cy = ppos[1], cz = ppos[2], cw = ppos[3];

    if (g_level == 0) {
        for (i = 0; i < 15; i++) temp[i] = 0.2f;
        weight = 0.2f;
    } else {
        int* arr = (int*)(*(int*)(g_level + 0xb0));
        unsigned end = *(unsigned*)((char*)arr + 0x18);
        for (unsigned u = *(unsigned*)((char*)arr + 0x14); u < end; u += 0x88) {
            float dx = *(float*)(u + 0x74) - cx;
            float dy = *(float*)(u + 0x78) - cy;
            float d2 = dx * dx + dy * dy;
            int slot;
            if (d2 < 1.0f) {
                d2 = 1.0f;
            } else if (!(d2 < 324.0f)) {
                continue;
            }
            slot = *(int*)(u + 0x24);
            if (slot >= 0 && slot < 0x10)
                temp[slot] = 10.0f / d2 + temp[slot];
        }
    }

    // ---- day time + anchor blend ------------------------------------------------------
    // 0x4848f9 fstp dword [ebp+0x65e8]: the day-time seed is a float
    dfd(m, 0x197a) = (float)f536a6a();
    {
        float anchorOut = 0.0f;
        f471040(g_game, &anchorOut, rf0(partyA, 0x30), rf0(partyA, 0x34));  // Anchors::blend
    }

    float t = temp[0] * 0.1f + (temp[8] * 0.01f + (temp[3] * 0.001f +
             (temp[0xb] * 0.002f + (temp[4] * 0.005f + (temp[0xc] * 0.01f +
             (temp[1] * 0.003f + (temp[9] * 0.005f + (temp[2] * 0.0005f + temp[10] * 0.0015f))))))));
    if (t < 0.0f) t = 0.0f; else if (t >= 1.0f) t = 1.0f;
    float temper = t;                                        // pfStack_150

    // ---- noise weather ladder (0x4848b9..) ---------------------------------------------
    float night = (float)*(int*)(g_game + 4) * 0.1f;
    float frac = (float)g_time1 * 1e-5f;
    float nA = 0.0f, nB = 0.0f, nC = 0.0f;
    f414fc0(50.0f, 0.7f, 4, 0, 0, &nC);                      // noise_fractal(50,0.7,4,0,...)
    float f14 = (nC + nB + nA) * 5.0f;
    f414fc0(72.0f, 0.7f, 4, 0, 0, &nC);                      // second octave set
    float fo = (nC + nB + nA) * 5.0f;

    int r = _rand();
    float humidity = (((f14 + f14) + 0.5f) * temper + ((float)(r % 0x3dc9) * 6.322312e-05f) * 0.05f);
    if (humidity < 0.0f) humidity = 0.0f; else if (humidity >= 1.0f) humidity = 1.0f;

    int height_ = *(int*)((char*)partyA + 0x38);
    // 0x484b4d fstp dword [ebp+0x65f4] -> the humidity slot is a float; 0x484b61/0x484b6b/0x484b71/
    // 0x484b77 store raw register dwords (party height plus three floats already spilled to the stack)
    dfd(m, 0x197d) = humidity;                               // humidity (float)
    m->d[0x197e] = height_;                                  // height (int)
    m->d[0x197c] = *(int*)&cx;                               // camera-party x, float bits
    m->d[0x1986] = *(int*)&cz;
    m->d[0x1987] = *(int*)&cw;
    m->d[0x1980] = 0;
    m->d[0x1981] = 0;

    float rainAmount = (((temper + 1.5f) * *(float*)(g_game + 0x13b3c)) * 1.5f);
    int rf = (int)_floor((double)rainAmount);
    if (!((float)rainAmount - (float)rf <= 0.5f)) rf += 1;
    m->d[0x197f] = rf;

    float fog = (((((temper + 0.5f) * (*(float*)(g_game + 0x13b3c) + 0.3f)) *
                  *(float*)(g_game + 0x13b40)) * *(float*)(g_game + 0x13b40)) * 0.008f) + 0.0001f;
    // 0x484c0a/0x484c35/0x484c60 fst (keep st0) then 0x484c84 fstp -> four float stores
    dfd(m, 0x1985) = fog;
    if (0.9f < *(float*)(g_game + 0x13b40)) {
        fog = fog * 1.3f + 0.025999999f; dfd(m, 0x1985) = fog;
        if (0.94f < *(float*)(g_game + 0x13b40)) {
            fog = fog * 1.2f + 0.024f; dfd(m, 0x1985) = fog;
            if (0.98f < *(float*)(g_game + 0x13b40))
                dfd(m, 0x1985) = fog + 0.01f;
        }
    }

    r = _rand();
    dfd(m, 0x1984) = (((1.2f - humidity) * ((float)(r % 0x3dc9) * 3.161156e-05f + 0.5f)) * 2000.0f);
    if (3.0f < (float)m->d[0x197a] && (float)m->d[0x197a] < 10.0f) {
        r = _rand();
        dfd(m, 0x1984) = ((float)(r % 0x3dc9) * 4.425618e-05f + 0.3f) * dfd(m, 0x1984);
    }
    if (0.6f < *(float*)(g_game + 0x13b3c)) {
        r = _rand();
        if (0.46f < ((humidity * 0.1f + (((float)(r % 0x3dc9) * 6.322312e-05f) * 0.05f +
                    *(float*)(g_game + 0x13b3c) * 0.2f)) + fo)) {
            if (mode == 0x4 || mode == 0xc)
                m->d[0x1981] = 2;
            else
                m->d[0x1981] = 1;
            r = _rand();
            float amt = (float)(r % 0x3dc9) * 3.161156e-05f + 0.5f;
            dfd(m, 0x1980) = amt;                        // 0x484ddb fst dword [ebp+0x6600]
            if (m->d[0x197f] < 2) m->d[0x197f] = 2;
            if (0.3f < amt) m->d[0x197f] = 3;
            float f5 = dfd(m, 0x1984) / (amt * 3.0f + 1.0f);
            dfd(m, 0x1984) = f5;                         // 0x484e1c fst / 0x484e3e fstp
            dfd(m, 0x1985) = amt * 0.0015f + dfd(m, 0x1985);
            if (m->d[0x1981] == 2) {
                dfd(m, 0x1984) = f5 * 0.5f;
                dfd(m, 0x1985) = amt * 0.00375f + dfd(m, 0x1985) * 1.5f;
            }
        }
    }
    if ((float)m->d[0x1984] < 60.0f) m->d[0x1984] = 0x42700000;
    if (0.15f < (float)m->d[0x1985]) m->d[0x1985] = 0x3e19999a;

    if (*(int*)((char*)partyA + 0x5974) == 0)
        m->d[0x197b] = 0;
    else
        m->d[0x197b] = *(int*)(*(int*)((char*)partyA + 0x5974) + 4);
    m->d[0x1982] = mode;
    f477160(m);

    // ---- prop indices + scene reset (0x484d..) -----------------------------------------
    int off = 0;
    for (unsigned u = 0; m->d[0x19a3] != 0 && (u < (unsigned)((m->d[0x19a4] - m->d[0x19a3]) / 0x1c8)); u++) {
        *(unsigned*)(m->d[0x19a3] + off + 0x14) = u;
        off += 0x1c8;
    }
    g_a7044c = mode >> 0x1f;
    g_a70448 = mode;

    int* slots = (int*)(m->d[0x177c] * 0x58 + 0x4c + g_modes);
    off = 0;
    i = 0;
    if (0 < *slots) {
        int o2 = 0;
        do { *(int*)(o2 + 0xc + slots[1]) = 0; i++; o2 += 0x78; } while (i < *slots);
    }
    slots = (int*)(m->d[0x177c] * 0x58 + 0x4c + g_modes);
    j = 0;
    if (0 < *(int*)(m->d[0x177c] * 0x58 + 0x4c + g_modes)) {
        do { f4625d0(); j++; } while (j < *slots);
    }
    f42bc50();

    // ---- ambient sound slot selection (0x484fb3..) -------------------------------------
    if (0 < g_snd_count) {
        int so = 0;
        i = 0;
        do {
            char* s = g_snd + so;
            int rr = _rand();
            *(float*)(s + 0x48) = (float)(rr % 0x3dc9) * 6.3223115e-07f;
            bool b;
            if (7.0f < (float)m->d[0x197a] && (float)m->d[0x197a] < 17.0f)        b = (ri0(s, 0x10) & 3) == 0;
            else if (4.0f < (float)m->d[0x197a] && (float)m->d[0x197a] < 20.0f)   b = ((ri0(s, 0x10) & 3) == 1);
            else                                                                  b = ((ri0(s, 0x10) & 3) == 2);
            if (b) *(float*)(s + 0x48) = *(float*)(s + 0x48) + 1.0f;
            unsigned uv = ri0(s, 0x10);
            int diff = m->d[0x197f] - ((int)uv >> 4 & 0xf);
            unsigned ad = (diff < 0) ? (unsigned)(-diff) : (unsigned)diff;
            *(float*)(s + 0x48) = *(float*)(s + 0x48) - (float)(int)ad * 0.1f;
            i++; so += 0x4c;
        } while (i < g_snd_count);
    }
    m->d[0x1983] = 0;
    if (1 < g_snd_count) {
        int idx = 1, so = 0x4c;
        do {
            if (*(float*)(g_snd + m->d[0x1983] * 0x4c + 0x48) < *(float*)(g_snd + 0x48 + so))
                m->d[0x1983] = idx;
            idx++; so += 0x4c;
        } while (idx < g_snd_count);
    }

    // ---- sun / sky / fog setup (0x4850..) ----------------------------------------------
    unsigned flags = *(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50));
    if ((flags & 0x100) == 0) {
        m->d[0x1953] = *(int*)((char*)level + 100);
        m->d[0x194f] = *(int*)((char*)level + 0x54);
        m->d[0x1950] = *(int*)((char*)level + 0x58);
        m->d[0x1951] = *(int*)((char*)level + 0x5c);
        m->d[0x1952] = *(int*)((char*)level + 0x60);
        m->d[0x194b] = m->d[0x194f];
        m->d[0x194c] = m->d[0x1950];
        m->d[0x194d] = m->d[0x1951];
        m->d[0x194e] = m->d[0x1952];
    } else {
        f4658c0((void*)&g_sky0, (void*)((char*)level + 0x68));
        if ((*(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50)) & 0x200) != 0) {
            // two party lookups -> x = (int)party0[0x30], y = (int)party1[0x34]
            unsigned pa2 = f48ad00(m);
            char* r0 = (char*)((pa2 & 0xf) * 0x5ac8 + *(int*)(*(int*)(g_game + 0x139c8) + (pa2 >> 4) * 4));
            unsigned pb2 = f48ad00(m);
            char* r1 = (char*)((pb2 & 0xf) * 0x5ac8 + *(int*)(*(int*)(g_game + 0x139c8) + (pb2 >> 4) * 4));
            int x5 = (int)rf0(r0, 0x30);
            int y5 = (int)rf0(r1, 0x34);
            int parity = m->d[0x149c] % 3;
            int frame = *(int*)(g_game + 4);
            unsigned mask = (unsigned)(g_mask_col - 1);
            // g_sky0 <- min(x*17 + parity + frame*0x1b0b + y*0x230b, mask)
            // g_sky2 <- min(x*19 + frame*0xf4d + parity + y*0x16a1, mask)
            // g_sky1 <- min(x*31 + y*0x19c9 + parity + frame*0x1e6f, mask)
            unsigned s0 = (unsigned)(x5 * 17 + parity + frame * 0x1b0b + y5 * 0x230b) & 0x7fffffu;
            unsigned s1 = (unsigned)(x5 * 31 + y5 * 0x19c9 + parity + frame * 0x1e6f) & 0x7fffffu;
            unsigned s2 = (unsigned)(x5 * 19 + frame * 0xf4d + parity + y5 * 0x16a1) & 0x7fffffu;
            if (mask < s0) s0 = mask;
            if (mask < s1) s1 = mask;
            if (mask < s2) s2 = mask;
            g_sky0 = (g_sky0 & ~mask) | s0;
            g_sky2 = (g_sky2 & ~mask) | s2;
            g_sky1 = (g_sky1 & ~mask) | s1;

            unsigned seed = (unsigned)((x5 + y5 * 0xb1 + parity + *(int*)(g_game + 4) * 0x4abb + 0x8a767) * 0x1ddcf5f + 0x2f784719);
            bool storm = false;
            int flag1 = 0;
            float chance = (float)(seed % 0x3dc9) * 6.322312e-05f;
            if (mode == 0x7) { storm = true; flag1 = 1; mode = 3; }
            else if (27.0f < temp[8]) { storm = true; flag1 = 1; }
            else if ((mode == 0x3 || mode == 0xb) && chance < 0.15f) { storm = true; flag1 = 1; }
            (void)flag1;

            unsigned cmask = (unsigned)(g_mask_cloud - 1);
            unsigned cloud = (mode <= (int)cmask) ? (unsigned)mode : cmask;
            g_sky4 = (g_sky4 & ~(cmask * 0x10000000)) | (cloud << 0x1c);

            unsigned fmask = (unsigned)(g_mask_flag - 1);
            unsigned flagv = storm ? 1u : 0u;
            if (fmask < flagv) flagv = fmask;
            int iseed = (int)(seed * 0x1c13 + 0xfffffffff2f67aaa);
            g_rand_seed = iseed * 0x1105;
            g_sky3 = (g_sky3 & ~(fmask * 0x80000000u) | (flagv << 0x1f)) & ~(fmask * 0x40000000u);

            // 0x4853ae fild/fmul[0x590d68]/fadd[0x590dbc] -> base amount; then the storm
            // (0x4853ce), mode-b/c (0x485493) or cold-temp (0x485477) adjustment, each of
            // which steps the LCG once with (x*0x1c13 - 0xd098556)*0x1105
            float ff = (float)((unsigned)g_rand_seed % 0x3dc9) * 6.3223115e-06f + 0.45f;
            if (storm) {
                g_rand_seed = (g_rand_seed * 0x1c13 + 0xfffffffff2f67aaa) * 0x1105;
                ff = ((float)((unsigned)g_rand_seed % 0x3dc9) * 6.3223115e-06f + ff) + 0.35f;
            } else if (mode == 0xb || mode == 0xc) {
                g_rand_seed = (g_rand_seed * 0x1c13 + 0xfffffffff2f67aaa) * 0x1105;
                ff = ff - ((float)((unsigned)g_rand_seed % 0x3dc9) * 1.2644623e-05f + 0.25f);
            } else if (35.0f < temp[1]) {
                ff = ff - 0.3f;
            }
            if (ff < 0.0f) ff = 0.0f; else if (1.0f < ff) ff = 1.0f;

            // 0x485502..0x485557: the original rounds with floor(x) + (frac > 0.5f)
            // (fld st(0) / _ftol / fsub / fcomp [0x549d70]=0.5 then a conditional inc)
            float amt_f = ff * 100.0f;
            int f100 = (int)_floor((double)amt_f);
            if (amt_f - (float)f100 > 0.5f) f100++;
            if (f100 < 0) f100 = 0; else if (99 < f100) f100 = 99;
            unsigned amt_mask = (unsigned)(g_mask_amt - 1);
            if ((int)amt_mask < f100) f100 = (int)amt_mask;
            g_sky4 = (g_sky4 & ~amt_mask) | (unsigned)f100;

            // ---- g_sky4 bits 14..20: cloud count, a switch on d[0x1982] (0x485563..0x4856fc) --
            // LCG step 1 (0x485563) then (int)(x*0.00126446225f + 12.0f) as the base (0x48559e),
            // LCG step 2 (0x4855af), then: mode 0xb -> (int)floor(x*0.00189669349f) + 30,
            // mode 0xc -> (int)floor(x*0.00158057781f) + 25, anything else accumulates
            // (int)floor(((x*6.32231167e-05f + 0.4f) * temp[3]) * 0.45f) (0x4856bb add ebx,eax)
            int sd = (int)((g_rand_seed * 0x1c13 + 0xfffffffff2f67aaa) * 0x1105);
            int cloud_cnt = (int)((float)((unsigned)sd % 0x3dc9) * 0.00126446225f + 12.0f);
            sd = (int)((sd * 0x1c13 + 0xfffffffff2f67aaa) * 0x1105);
            float cbase = (float)((unsigned)sd % 0x3dc9);
            if (mode == 0xb) {
                float cb = cbase * 0.00189669349f;
                if (!(0.0f < cb)) cb = cb - 0.99999988f;          // floor idiom (fcom 0 / fsub [0x54b664])
                cloud_cnt = (int)cb + 30;
            } else if (mode == 0xc) {
                float cb = cbase * 0.00158057781f;
                if (!(0.0f < cb)) cb = cb - 0.99999988f;
                cloud_cnt = (int)cb + 25;
            } else {
                g_rand_seed = sd;                                 // at 48564f
                float ct = (cbase * 6.32231167e-05f + 0.4f) * temp[3] * 0.45f;
                int cti = (int)_floor((double)ct);
                if (ct - (float)cti > 0.5f) cti++;
                cloud_cnt += cti;
            }
            if (cloud_cnt > 0x41) cloud_cnt = 0x41;
            if ((int)amt_mask < cloud_cnt) cloud_cnt = (int)amt_mask;
            g_sky4 = (g_sky4 & ~(amt_mask * 0x4000)) | ((unsigned)cloud_cnt << 0xe);

            // ---- g_sky4 bits 7..13: temperature/storm amount (0x485704..0x4857d4) --------------
            sd = (int)((sd * 0x1c13 + 0xfffffffff2f67aaa) * 0x1105);   // 0x4856dc: esi = [0x5d7078], stepped again
            g_rand_seed = sd;                                     // at 48570d
            float seed2 = (float)((unsigned)sd % 0x3dc9) * 0.000948346744f + 10.0f;
            float tt = (temp[12] + temp[11]) * 0.35f + temp[1] * 0.6f;
            if (tt < 0.0f) tt = 0.0f; else if (50.0f < tt) tt = 50.0f;
            float total = seed2 + tt;
            if (storm) total = total * 0.5f;                      // [esp+0x10] == the storm flag
            int amt2 = (int)_floor((double)total);
            if (amt2 < 0) amt2 = 0; else if (0x80 <= amt2) amt2 = 0x7f;
            if (amt2 > (int)amt_mask) amt2 = (int)amt_mask;
            g_sky4 = (g_sky4 & ~(amt_mask << 7)) | ((unsigned)amt2 << 7);

            // ---- g_sky4 bits 21..27: rain amount from _rand() (0x4857da..0x485862) ------------
            int rr2 = _rand();
            float rain2 = ((float)(rr2 % 0x3dc9) * 1.2644623e-05f + 0.8f) * 100.0f;
            int ri2 = (int)_floor((double)rain2);
            if (rain2 - (float)ri2 > 0.5f) ri2++;
            if (ri2 < 0) ri2 = 0; else if (99 < ri2) ri2 = 99;
            if ((int)amt_mask < ri2) ri2 = (int)amt_mask;
            g_sky4 = (g_sky4 & ~(amt_mask * 0x200000)) | ((unsigned)ri2 << 0x15);
        }

        m->d[0x194b] = 0;
        m->d[0x194c] = 0;
        unsigned bb = (unsigned)g_mask_bits - 1U & g_sky3;
        float bf = (float)(int)bb;
        if ((int)bb < 0) bf += 4294967296.0f;
        dfd(m, 0x194d) = bf;                              // 0x4858a0 fild / 0x4858ac fstp
        unsigned b1 = (g_sky3 >> 10) & ((unsigned)g_mask_bits - 1U);
        dfd(m, 0x194e) = (float)b1;                       // 0x4858c9 fild / 0x4858d5 fstp
        m->d[0x1950] = 0;
        m->d[0x1953] = -0x41666666;
        m->d[0x194f] = 0;
        m->d[0x1951] = m->d[0x194d];
        m->d[0x1952] = m->d[0x194e];
    }
    f42bc50();

    // ---- prop bounding box, two passes (0x4853.. / 0x4856..) ---------------------------
    for (int pass = 0; pass < 2; pass++) {
        int o = 0;
        for (unsigned u = 0; m->d[0x19a3] != 0 && (u < (unsigned)((m->d[0x19a4] - m->d[0x19a3]) / 0x1c8)); u++) {
            char* p = (char*)(m->d[0x19a3] + o);
            if (ri0(p, 0x188) == 0 && ri0(p, 0x18c) >= 0) {
                int mi = ri0(p, 0x18c) * 0x78;
                char* mr = g_mode_recs + mi;
                float bx = (rf0(mr, 0x58) * rf0(p, 0x18) + (rf0(p, 0x38) * rf0(mr, 0x60) + rf0(p, 0x28) * rf0(mr, 0x5c))) + rf0(p, 0x48);
                float by = (rf0(p, 0x1c) * rf0(mr, 0x58) + (rf0(p, 0x3c) * rf0(mr, 0x60) + rf0(p, 0x2c) * rf0(mr, 0x5c))) + rf0(p, 0x4c);
                float bz = (rf0(p, 0x20) * rf0(mr, 0x58) + (rf0(p, 0x40) * rf0(mr, 0x60) + rf0(p, 0x30) * rf0(mr, 0x5c))) + rf0(p, 0x50);
                float hx = rf0(mr, 0x68);
                // 0x485aa8..0x485aff fstp dword [ebp+0x652c/0x6530/0x6534/0x6538]: all four box slots are floats
                if (dfd(m, 0x194b) < bx - hx) dfd(m, 0x194b) = bx - hx;
                if (!(dfd(m, 0x194c) < by - hx)) dfd(m, 0x194c) = by - hx;
                if (!((bx + hx) < dfd(m, 0x194d))) dfd(m, 0x194d) = bx + hx;
                if (!(bz < dfd(m, 0x194e))) dfd(m, 0x194e) = bz;
            }
            o += 0x1c8;
        }
    }
    f479e20(m->d[0x177d], m->d[0x177c]);

    // ---- ambient sound params (0x485c..) -----------------------------------------------
    int si = m->d[0x1983] * 0x4c + (int)g_snd;
    m->d[0x149d] = 0;
    f4624c0(m, (void*)(si + 0x44));
    int clr = 0xff334455;
    f462540(6.0f, 10.0f, &clr, &m->d[0x1953]);
    f4792d0(m->d[0x177d], m->d[0x1983], m->d[0x1985]);

    // ---- intro billboard mesh (0x485d..) ------------------------------------------------
    void* obj = f428440(9);
    void* base = f419540((void*)0);
    void* mesh = f41c6d0(base);
    f44f0c0(mesh, *(float*)(g_game + 0x13b3c));
    *(unsigned*)((char*)mesh + 0x10c) &= 0xfffeffff;
    int* mv = (int*)obj;
    void* arr = *(void**)((char*)obj + 0x11c);
    if (arr == 0 || ((*(int*)((char*)obj + 0x124) - (int)arr) >> 2) <= ((*(int*)((char*)obj + 0x120) - (int)arr) >> 2)) {
        f410610(*(void**)((char*)obj + 0x120), 1, &mesh);
    } else {
        *(void**)arr = mesh;
        *(int*)((char*)obj + 0x120) = (int)arr + 4;
    }
    vcall(mv, 0x10);

    // ---- mission clock -> sun rotation (0x485e8..) -------------------------------------
    // real: angle = sndrec[0x14]*C - ((v%24 - 0xc) * C * C2),  v = (int)(int64)(g_tab[level*2..] - *(int64*)(g_game+0x13b20)) * C1
    float dt = (float)(g_tab[*(int*)(g_game + 0x13b18) * 2] - *(unsigned*)(g_game + 0x13b20));
    float sunrot = 0.0f;
    {
        int si2 = m->d[0x1983];
        char* sr = (char*)(si2 * 0x4c + g_snd);
        __int64 lvl64 = *(__int64*)(g_tab + *(int*)(g_game + 0x13b18) * 2);
        __int64 now64 = *(__int64*)(g_game + 0x13b20);
        int v24 = (int)((double)(lvl64 - now64) * 0.0009765625) % 0x18;  // * 1/1024, then %24
        sunrot = rf0(sr, 0x14) * 0.01f - (float)(v24 - 0xc) * 0.01f * 0.017453292f * 15.0f;
    }
    float rot = sunrot;
    float rotAng[4] = { dt, rot, 0.0f, 0.0f };
    f40e770(mesh, rotAng);
    vcall1(mv, 0x18, &rotAng[0]);
    *(void**)((int)m + 0x78) = obj;

    // ---- custom-camera branch / shadow map (0x486..) -----------------------------------
    if ((*(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50)) & 0x100) == 0) {
        f47a800(m->d[0x177d], &m->d[0x18d0], &m->d[0x18cf]);
    } else {
        int pv = (int)m;
        g_emit_pos[0] = *(float*)(pv + 0x68);
        g_emit_pos[1] = *(float*)(pv + 0x6c);
        g_emit_pos[2] = *(float*)(pv + 0x70);
        g_emit_pos[3] = *(float*)(pv + 0x74);
        int pv2 = m->d[0x196c];
        g_emit_pos[4] = *(float*)(pv2 + 4);
        g_emit_pos[5] = *(float*)(pv2 + 8);
        g_emit_pos[6] = *(float*)(pv2 + 0xc);
        g_emit_pos[7] = *(float*)(pv2 + 0x10);
        void* cam = (void*)((char*)level + 0x414);
        vcall(cam, 4);
        int c2 = vcall_i(cam, 0x5c);
        if (c2 == 0) {
            if ((*(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50)) & 0x100) != 0) {
                void* sm = (void*)(m->d[0x177d] * 0x494 + *(int*)(g_game + 0x13a50) + 0x414);
                vcall(sm, 4);
                f464440(sm);
            }
        }
        f481d10(m, &m->d[0x18f4], 0);
    }
    vcall((void*)((char*)m + 0x444), 4);
    f4781d0((void*)((char*)m + 0x444));
    f470a00();
    {
        unsigned f2 = *(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50));
        if (((f2 & 0x200) != 0) && ((f2 & 0x100) != 0) && f467530() > 10) {
            f4785e0(*(int*)((int)m + 100), 1);
            f470a00();
            f470750();
        }
    }
    f478c50(&m->d[0x19a6]);

    // ---- light table: 64 slots + 5 extra for custom camera (0x4860b0..) ----------------
    m->d[0x149e] = 0x40;
    m->d[0x149d] = 0;
    float lofs_x = 0.0f, lofs_y = 0.0f;
    float midx = ((float)m->d[0x1951] - (float)m->d[0x194f]) * 0.5f + (float)m->d[0x194f];
    float midy = ((float)m->d[0x1952] - (float)m->d[0x1950]) * 0.5f + (float)m->d[0x1950];
    if ((*(unsigned*)((char*)level + 0xc) & 0x100) != 0) { lofs_x = midx; lofs_y = midy; }

    {
        unsigned cnt = 0;
        float* slot = (float*)&m->d[0x1a8];
        do {
            float idbuf[12];
            f40cd60(idbuf);
            float* dst = slot - 0xc;
            for (i = 0xc; i != 0; i--) *dst++ = *((float*)idbuf + (dst - (slot - 0xc)));
            unsigned gx = cnt % 8;
            unsigned gy = cnt / 8 + 1;
            slot[0] = (float)(int)(gx + 1) + lofs_x;
            slot[1] = (float)(int)gy + lofs_y;
            slot[2] = 500.0f;
            slot[3] = 0.0f;
            cnt++;
            slot = (float*)((char*)slot + 0x130);
        } while ((int)cnt < 0x40);
    }

    if ((*(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50)) & 0x400) != 0) {
        m->d[0x149e] = 5;
        unsigned rr = _rand() & 1;
        for (int s = 0; s < 5; s++) {
            float* p = (float*)&m->d[0x1a8 + s * 0x4c];
            float idbuf[12];
            f40cd60(idbuf);
            for (i = 0; i < 12; i++) p[i] = idbuf[i];
            p[0xc] = 0.0f; p[0xd] = 0.0f; p[0xe] = 1000.0f; p[0xf] = 0.0f;
            int rr2 = _rand();
            float amp = ((float)(rr2 % 0x3dc9) * 6.322312e-05f) * 0.4f + 0.3f;
            p[0] = ((float)m->d[rr + 0x1951] - (float)m->d[rr + 0x194f]) * (((float)(rr2 % 0x3dc9) * 6.322312e-05f - 0.5f) * 0.1f + (float)(s)) * 0.6f + 0.2f;
            (void)amp;
            f407ef0(p);                                      // matrix::orthonormalize
        }
    }
    if ((*(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50)) & 0x100) != 0) {
        // 0x48637b..0x4863bd: four fld/fadd-sub/fstp pairs on the float box slots 0x653c..0x6548
        dfd(m, 0x194f) += 24.0f;
        dfd(m, 0x1950) += 24.0f;
        dfd(m, 0x1951) -= 24.0f;
        dfd(m, 0x1952) -= 24.0f;
    }

    // ---- per-slot prop frame copy (0x4867b0..) ------------------------------------------
    {
        int o = 0;
        for (unsigned u = 0; m->d[0x19a3] != 0 && (u < (unsigned)((m->d[0x19a4] - m->d[0x19a3]) / 0x1c8)); u++) {
            char* p = (char*)(m->d[0x19a3] + o);
            int slot = ri0(p, 0x1a0);
            if (ri0(p, 0x188) == 1 && slot >= 0 && slot < 0x40) {
                float* sp = (float*)&m->d[slot * 0x4c + 0x1a8];
                sp[0] = rf0(p, 0x48); sp[1] = rf0(p, 0x4c); sp[2] = rf0(p, 0x50); sp[3] = rf0(p, 0x54);
                for (i = 0; i < 12; i++) *((float*)&m->d[slot * 0x4c + 0x19c] + i) = *(float*)(p + 0x18 + i * 4);
                for (i = 0; i < 16; i++) *((float*)&m->d[slot * 0x4c + 0x1ac] + i) = *((float*)&m->d[slot * 0x4c + 0x19c] + i);
                // third row = cross(fwd, up) normalized
                float* mtx = (float*)&m->d[slot * 0x4c + 0x19c];
                float y0 = mtx[1], y1 = mtx[5], y2 = mtx[9];
                mtx[2] = mtx[8] * y0 - mtx[4] * y1;
                mtx[6] = mtx[0] * y1 - mtx[8] * y2;
                mtx[10] = mtx[4] * y2 - mtx[0] * y0;
                float len = (float)sqrt(mtx[2] * mtx[2] + (mtx[6] * mtx[6] + mtx[10] * mtx[10]));
                if (1e-10f < len) {
                    len = 1.0f / len;
                    mtx[2] *= len; mtx[6] *= len; mtx[10] *= len;
                } else { mtx[2] = 0.0f; mtx[6] = 1.0f; mtx[10] = 0.0f; }
                *(float*)&m->d[slot * 0x4c + 0x1b0] = mtx[1] * mtx[10] - mtx[6] * mtx[9];
                *(float*)&m->d[slot * 0x4c + 0x1b1] = mtx[8] * mtx[9] - mtx[0] * mtx[10];
                *(float*)&m->d[slot * 0x4c + 0x1b2] = mtx[2] * mtx[0] - mtx[8] * mtx[1];
            }
            o += 0x1c8;
        }
    }

    // ---- prop ground / spawn pass (0x486a..) --------------------------------------------
    {
        float* pi = 0;
        while (true) {
            if ((m->d[0x19a3] == 0) || ((float)((m->d[0x19a4] - m->d[0x19a3]) / 0x1c8) <= (float)(int)(long)pi))
                break;
            char* p = (char*)(m->d[0x19a3] + (int)(long)pi * 0x1c8);
            ri0(p, 0x1b4) = 0;
            if ((g_no_cam == 0) && (ri0(p, 0x188) == 4) &&
                ((*(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50)) & 0x100) != 0)) {
                int idx = 0;
                for (j = 0; j < g_ground_count; j++) {
                    if (f42bdd0(g_ground + j * 0x20, p + 400)) break;
                    idx = j;
                }
                f466d90(idx, p + 0x18, p + 0x158, ri0(p, 0x1a0));
            } else {
                int found = -1;
                int val = (int)(long)pi;
                f44c210(&val, &found);
            }
            pi = (float*)((int)(long)pi + 1);
        }
    }
    f472050();
    {
        unsigned f2 = *(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50));
        if ((f2 & 0x100) != 0)
            f482170(&m->d[0x196a], (int)((f2 >> 0x11) | (~(f2 >> 9) & 1)) & 0x1);
    }
    f47ced0();
    if ((*(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50)) & 0x100) == 0)
        f47ce00(&m->d[0x18d0], m->d[0x18cf]);

    // ---- scene effect objects + intro billboards (0x486bc6..) ---------------------------
    m->d[0x196f] = 0;
    m->d[0x196e] = 0;
    if (g_env_on != 0 && (*(unsigned*)((char*)level + 0xc) & 1) == 0 && m->d[0x1981] >= 1) {
        float* sp = (float*)(m->d[0x1983] * 0x4c + 0x30 + g_snd);
        float ex = sp[0], ey = sp[1], ez = sp[2], ew = sp[3];
        float scale = (ex * 3.0f + 1.0f) * 0.4f;
        void* efx = f428440(9);
        m->d[0x196e] = (int)efx;
        rf0(efx, 0x88) = 0.0f; rf0(efx, 0x8c) = 0.0f; rf0(efx, 0x90) = 30.0f; rf0(efx, 0x94) = ew;
        float one = 1.0e7f;                                  // 0x4b189680
        vcall1(efx, 0x18, &one);
        (void)ey; (void)ez;

        float idbuf[12];
        f40cd60(idbuf);
        int i1 = g_emit_idx1, i2 = g_emit_idx2;
        float dirx = rf0(&m->d[0x1986], 0) * 0.35f, diry = rf0(&m->d[0x1987], 0) * 0.35f;
        // emitter anchor pos = {ex, 6.5f, 6.0f, ew} (0x486932/0x486944 write pos[1],pos[2])
        float emx = ex, emy = 6.5f, emz = 6.0f, emw = ew;
        float depth = -10.0f;
        if (m->d[0x1981] == 1) {
            if (g_emit_idx1 < 0 || g_emit_count <= g_emit_idx1) { /* skip */ }
            else {
                void* e = (void*)operator_new(0x1bc);
                if (e) { rf0(e, 0x10) = 0.0f; rf0(e, 0x14) = 0.0f; rf0(e, 0x18) = 0.0f; rf0(e, 0x1b8) = 0.0f; f420b40(e); }
                float pos[4] = { emx, emy, emz, emw };
                f4424c0(e, i1, pos);
                if (e) {
                    float inv = 1.0f / rf0(e, 0x24);
                    rf0(e, 0x184) = dirx * inv; rf0(e, 0x188) = diry * inv; rf0(e, 0x18c) = inv * -10.0f;
                    rf0(e, 0x190) = ew;
                    rf0(e, 0xc4) = dirx; rf0(e, 0xc8) = diry; rf0(e, 0xcc) = depth; rf0(e, 0xd0) = 0.0f;
                    float* d = (float*)((char*)e + 0xb4);
                    d[0] = dirx; d[1] = diry; d[2] = depth; d[3] = 0.0f;
                    float len = (float)sqrt(d[2] * d[2] + (d[1] * d[1] + d[0] * d[0]));
                    if (1e-10f < len) { len = 1.0f / len; d[0] *= len; d[1] *= len; d[2] *= len; }
                    else { d[0] = 0.0f; d[1] = 1.0f; d[2] = 0.0f; }
                    rf0(e, 0x198) = (float)m->d[0x1980] * rf0(e, 0x198);
                    rf0(e, 0xa0) = 11.0f / (10.0f / rf0(e, 0x24));
                    rf0(e, 0x3c) *= scale; rf0(e, 0x44) *= scale; rf0(e, 0x4c) *= scale;
                    rf0(e, 0x54) *= scale; rf0(e, 0x5c) *= scale; rf0(e, 0x64) *= scale;
                }
            }
        } else if (m->d[0x1981] == 2) {
            if (!(g_emit_idx2 < 0 || g_emit_count <= g_emit_idx2)) {
                void* e = (void*)operator_new(0x1bc);
                if (e) { rf0(e, 0x10) = 0.0f; rf0(e, 0x14) = 0.0f; rf0(e, 0x18) = 0.0f; rf0(e, 0x1b8) = 0.0f; f420b40(e); }
                float pos[4] = { emx, emy, emz, emw };
                f4424c0(e, i2, pos);
                if (e) {
                    rf0(e, 0xc4) = dirx; rf0(e, 0xc8) = diry; rf0(e, 0xcc) = depth; rf0(e, 0xd0) = 0.0f;
                    rf0(e, 0x3c) *= scale; rf0(e, 0x44) *= scale; rf0(e, 0x4c) *= scale;
                    rf0(e, 0x54) *= scale; rf0(e, 0x5c) *= scale; rf0(e, 0x64) *= scale;
                    float* d = (float*)((char*)e + 0xb4);
                    d[0] = dirx; d[1] = diry; d[2] = depth; d[3] = 0.0f;
                    float len = (float)sqrt(d[2] * d[2] + (d[1] * d[1] + d[0] * d[0]));
                    if (1e-10f < len) { len = 1.0f / len; d[0] *= len; d[1] *= len; d[2] *= len; }
                    else { d[0] = 0.0f; d[1] = 1.0f; d[2] = 0.0f; }
                    rf0(e, 0x198) = (float)m->d[0x1980] * rf0(e, 0x198);
                }
                vcall1((void*)m->d[0x196e], 0x64, e);
            }
        }
    }

    // ---- UI / cursor / vertex-color meshes (0x486c6f..) --------------------------------
    m->d[0x1970] = 0; m->d[0x1971] = 0; m->d[0x1972] = 0;
    if ((*(unsigned*)((char*)level + 0xc) & 1) == 0 && g_env_on != 0) {
        void* o = f428440(9);
        m->d[0x1970] = (int)o;
        float s2 = 2.0f;
        vcall1(o, 0x18, &s2);
        void* mm = (void*)operator_new(0x260);
        mm = mm ? f412a90() : 0;
        m->d[0x1971] = (int)mm;
        *(unsigned*)((char*)mm + 0x10c) &= 0xfffeffff;
        *(int*)((char*)mm + 0x114) = 1;
        *(int*)((char*)mm + 0x140) = 1;
        void** arr = (void**)*(void**)((char*)o + 0x11c);
        if (arr == 0 || ((*(int*)((char*)o + 0x124) - (int)arr) >> 2) <= ((*(int*)((char*)o + 0x120) - (int)arr) >> 2))
            f410610(*(void**)((char*)o + 0x120), 1, &mm);
        else { *arr = mm; *(int*)((char*)o + 0x120) = (int)arr + 4; }
        vcall(o, 0x10);
        f42bbe0(m, g_s_name2);
        void* res = f419540(&m->d[0x18c9]);
        void* m2 = f41c6d0(res);
        m->d[0x1972] = (int)m2;
        f42bc40();
        *(int*)((char*)m2 + 0x114) = 1;
        *(int*)((char*)m2 + 0x140) = 1;
        float cv = ((1.0f - 150.0f / ((float)m->d[0x1984] + 150.0f)) * *(float*)(m->d[0x1983] * 0x4c + 0x1c + g_snd)) * 0.9f;
        float col[4] = { cv, cv, cv, 0.0f };
        unsigned packed = f40e440(col);
        *(unsigned*)((char*)m2 + 0x3c) = packed | 0xff000000;
        *(unsigned*)((char*)m2 + 0x10c) &= 0xfffeffff;
        f44f0c0(m2, 50.0f);
        void** a2 = (void**)*(void**)((char*)o + 0x11c);
        if (a2 == 0 || ((*(int*)((char*)o + 0x124) - (int)a2) >> 2) <= ((*(int*)((char*)o + 0x120) - (int)a2) >> 2))
            f410610(*(void**)((char*)o + 0x120), 1, &m2);
        else { *a2 = m2; *(int*)((char*)o + 0x120) = (int)a2 + 4; }
        vcall(o, 0x10);
    }

    m->d[0x1974] = 0;
    {
        void* o = f428440(9);
        m->d[0x1974] = (int)o;
        float s3 = 290.0f;
        vcall1(o, 0x18, &s3);
        f42bbe0(m, g_s_cursor);
        void* res = f419540(&m->d[0x18c9]);
        void* mm = f41c6d0(res);
        m->d[0x1975] = (int)mm;
        f42bc40();
        void** arr = (void**)*(void**)((char*)o + 0x11c);
        if (arr == 0 || ((*(int*)((char*)o + 0x124) - (int)arr) >> 2) <= ((*(int*)((char*)o + 0x120) - (int)arr) >> 2))
            f410610(*(void**)((char*)o + 0x120), 1, &mm);
        else { *arr = mm; *(int*)((char*)o + 0x120) = (int)arr + 4; }
        vcall(o, 0x10);

        void* m3 = (void*)operator_new(0x260);
        m3 = m3 ? f412a90() : 0;
        m->d[0x1976] = (int)m3;
        f42bbe0(m, g_s_vcolor);
        *(void**)((char*)m3 + 0x2c) = f418a70(&m->d[0x18c9]);
        f42bc40();
        f442110(m3);

        m->d[0x1977] = 0;
        o = f428440(9);
        m->d[0x1977] = (int)o;
        float s4 = 33.5f;
        vcall1(o, 0x18, &s4);
        void* m4 = (void*)operator_new(0x260);
        m4 = m4 ? f412a90() : 0;
        m->d[0x1978] = (int)m4;
        f42bbe0(m, g_s_vcolor);
        *(void**)((char*)m4 + 0x2c) = f418a70(&m->d[0x18c9]);
        f42bc40();
        f442110(m4);

        o = f428440(9);
        m->d[0x1979] = (int)o;
        float s5 = 33.5f;
        vcall1(o, 0x18, &s5);
        *(char*)(m->d[0x1974] + 0x140) = 0;
        *(char*)(m->d[0x1977] + 0x140) = 0;
        *(char*)(m->d[0x1979] + 0x140) = 0;
    }

    // ---- counts[8][6] + clock, light-table pass + resets (0x486d60..) ------------------
    {
        int* p = &m->d[0x1887];
        for (j = 8; j != 0; j--) {
            for (i = 6; i != 0; i--) { *p++ = 0; dfd(m, 0x18b7) = (float)g_time2 * 1e-05f; }
        }
    }
    m->d[0x1949] = 0;
    *((char*)&m->d[0x18ce]) = 0;
    {
        float* p = (float*)&m->d[0x1e0];
        unsigned slotN = 0;
        do {
            int* outface = (int*)(p + 6);
            *outface = 0;
            float outY = 0.0f;
            i = 0;
            for (;;) {
                float* px = p - 0x28;
                f439be0((void*)m->d[0], px, &outY, outface);
                if (*outface != 0) break;
                *px = (f40e400() - 0.5f) * 0.01f + *px;
                px[1] = ((f40e400() - 0.5f) * 0.01f + px[1]);
                i++;
                if (100 < i) {
                    *px = (f40e400() - 0.5f) * 100.0f;
                    px[1] = (f40e400() - 0.5f) * 100.0f;
                    if (10000 < i) *outface = **(int**)((int)m + 0x58);
                }
            }
            p[-0x26] = outY;   // light-slot ground height (out of 0x439be0)
            for (i = 0x10; i != 0; i--) *((int*)p - 0x24 + (0x10 - i)) = *((int*)p - 0x34 + (0x10 - i));
            for (i = 0x10; i != 0; i--) *((int*)p - 0x14 + (0x10 - i)) = *((int*)p - 0x34 + (0x10 - i));
            ((int*)p)[2] = g_tab[((int*)p)[0] * 2];
            ((int*)p)[3] = g_tab[((int*)p)[0] * 2 + 1];
            ((int*)p)[4] = 0;
            f4676e0((float)slotN);
            slotN++;
            p = (float*)((char*)p + 0x4c * 4);
        } while ((int)slotN < 0x40);
    }

    // ---- party spawn counters, agent reset, mode slots, UI, log (0x486f..) -------------
    {
        int* p = &m->d[0x1783];
        int* cnt = &m->d[0x1883];
        for (int t = 0; t < 2; t++) {
            int* save = p;
            for (i = 0; i < *cnt; i++) f4739f0(*p++);
            p = save + 0x80;
            cnt++;
        }
    }
    for (i = 0; i < *(int*)(g_game + 0x13a04); i++)
        *(int*)(*(int*)(g_game + 0x13a00) + 0x3bc + i * 0x508) = 0;
    for (i = 0; i < 0x10; i++) m->d[0x1a09 + i] = 0;
    {
        int cnt = *(int*)(g_snd + 0x38);
        for (i = 0; i < cnt; i++) {
            *(int*)(*(int*)(g_snd + 0x34) + 0x18 + i * 0x4c) = 0;
            *(int*)(*(int*)(g_snd + 0x34) + 0x1c + i * 0x4c) = 0;
        }
        for (i = 0; i < *(int*)(g_snd + 0x38); i++) f4677a0();
    }
    f483eb0();
    int tim = (int)(float)m->d[0x1984];
    _sprintf((char*)&m->d[0x196f], g_s_fmt,
             (double)rf0(&m->d[0x197c], 0), (double)rf0(&m->d[0x197d], 0), (double)rf0(&m->d[0x197e], 0), tim);
    for (i = 2; i != 0; i--) f483f60();

    // ---- placed-prop jitter + ground snap (0x4872..) -----------------------------------
    {
        int o = 0;
        for (unsigned u = 0; m->d[0x19a3] != 0 && (u < (unsigned)((m->d[0x19a4] - m->d[0x19a3]) / 0x1c8)); u++) {
            char* p = (char*)(m->d[0x19a3] + o);
            if (ri0(p, 0x188) == 0 &&
                (*(unsigned*)(ri0(p, 0x18c) * 0x78 + 0x10 + g_mode_recs) & 0x1000) != 0) {
                float jx = (float)m->d[0x1966], jy = (float)m->d[0x1967], jz = (float)m->d[0x1968];
                int rr = _rand();
                float a = (float)(rr % 0x3dc9) * 6.322312e-05f - 0.5f;
                jx = (a + a) + jx;
                rr = _rand();
                float b = (float)(rr % 0x3dc9) * 6.322312e-05f - 0.5f;
                jy = (b + b) + jy;
                float outY2 = 0.0f;
                f439be0((void*)m->d[0], &jx, &outY2, &temp[0xc]);
                rf0(p, 0x48) = jx; rf0(p, 0x4c) = jy; rf0(p, 0x50) = jz; rf0(p, 0x54) = (float)m->d[0x1969];
                if (ri0(p, 0x1b4) != 0) vcall((void*)ri0(p, 0x1b4), 0x58);
            }
            o += 0x1c8;
        }
    }
    {
        unsigned f2 = *(unsigned*)(m->d[0x177d] * 0x494 + 0x30 + *(int*)(g_game + 0x13a50));
        if ((f2 & 0x100) == 0) {
            if (((f2 & 1) == 0) && (g_env2 != 0 || 1)) {
                f4711e0((void*)m);
                unsigned u = 0;
                int o = 0;
                while (true) {
                    int t = *(int*)((int)m + 0xb0);
                    if (*(int*)(t + 4) == 0 || (unsigned)((*(int*)(t + 8) - *(int*)(t + 4)) / 0x24) <= u) break;
                    f468c20((void*)(*(int*)(t + 4) + o));
                    u++; o += 0x24;
                }
                f463cc0();
                f47fef0();
            }
        } else {
            f4814a0(m);
        }
    }
    log_str(g_s_end_setup);
    *((char*)m + 0x6555) = 0;
}
