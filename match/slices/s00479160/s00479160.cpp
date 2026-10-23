// Slice s00479160: the mission/scene object's reset, the ambient wind/effect setup, and the tail erase
// of std::vector<E56> (called by SetVec::resize 0x47a980 in s0047a0c0). ALL NAMES PROVISIONAL.
//
//   0x479160  Mission::reset()
//   0x4792d0  Mission::setup_ambient(int level_idx, int spec_idx, float t)
//   0x479930  std::vector<E56>::erase(std::vector<E56>::iterator, std::vector<E56>::iterator)
#include <math.h>
#include <vector>

// ---------------------------------------------------------------- rgl geometry
struct vector4 {
    float x, y, z, w;
    vector4() {}
    vector4(float a, float b, float c) : x(a), y(b), z(c) {}   // w is left uninitialised
    float normalize();                                         // 0x407d60, returns the old length
};
struct matrix { vector4 s, f, u, o; };                         // 0x40
matrix* identity_pos(matrix* out);                             // 0x40cd00 (cdecl, returns out)
void unpack_color(vector4* out, const unsigned* argb);          // 0x40e4e0 (cdecl)

// ---------------------------------------------------------------- records touched by name
struct Wind {                       // 0x7c-byte effect/wind object handed out by Scene::spawn()
    int f0;                         // +0x00
    vector4 pos;                    // +0x04
    vector4 f14;                    // +0x14
    vector4 f24;                    // +0x24
    char pad34[0x44 - 0x34];
    vector4 f44;                    // +0x44
    float f54;                      // +0x54
    char pad58[0x6c - 0x58];
    int f6c;                        // +0x6c flags (bit 1 = "too weak / flagged")
    char pad70[0x7c - 0x70];
};

struct Scene;                       // Mission::grid, the cell/effect grid at Mission+0
struct Row {                        // 0x4c-byte table row at g_snd (hourly cell/ambient spec)
    char pad0[0x10];
    int f10;                        // +0x10
    char pad14[0x20 - 0x14];
    float f20, f24, f28;            // +0x20 wind strength per axis
    vector4 base;                   // +0x30
    vector4 get_dir(int idx, int day);          // 0x463610 (returns by hidden pointer)
};
struct Level {                      // 0x494-byte terrain row at g_game->levels
    char pad0[0x30];
    unsigned char f30;              // +0x30 (bit 0 selects the flat "fog sheet" variant)
};

struct RecZ; struct RecJ;           // the two id-pool record types
template<class T> struct Pool {
    int next_id;                    // +0x00
    std::vector<int> free_ids;      // +0x04
    std::vector<T*> pages;          // +0x14
    int serial;                     // +0x24
    void reset();
};
// 0x477f00
template<> void Pool<RecZ>::reset();
// 0x477f90
template<> void Pool<RecJ>::reset();

struct TerrainDesc {                // 0x1c8-byte terrain layer set at Mission+0x63d0
    char body[0x1c8];
    void init();                    // 0x464400
};
struct tstring {                    // 0x10
    int pos; char* buf; int cap; int len;
    void clear();                   // 0x42be50
};

struct GameData {
    char pad0[0x13a50];
    Level* levels;                  // +0x13a50 (0x494-byte terrain rows)
    char pad13a54[0x13b18 - 0x13a54];
    int f13b18;                     // +0x13b18 tick index (8-byte entries)
    unsigned f13b20;                // +0x13b20 low word of the mission tick clock
    int f13b24;                     // +0x13b24 high word
};

extern GameData* g_game;            // 0x5e2930
extern Row* g_snd;                  // 0x5e28f4 (0x4c-byte hourly rows)
extern __int64 g_ticks[];           // 0x5e2948
extern __int64 g_time1;             // 0x5e2950
extern int g_dinput_enabled;        // 0x5e28fc

struct Scene {
    char pad0[0x68];
    vector4 f68;                    // +0x68 colour the cell hands to the emitters (its w at +0x74 = strength)
    char pad78[8];
    ~Scene();                       // 0x475d50
    Wind* spawn();                  // 0x478f30
};

// ---------------------------------------------------------------- the mission object
struct Mission {
    Scene* grid;                                 // +0x0000
    Pool<RecZ> poolZ;                            // +0x0004
    char pad2c[0x30 - 0x2c];
    Pool<RecJ> poolJ;                            // +0x0030
    char pad58[0x5c - 0x58];
    int f5c;                                     // +0x005c
    int f60;                                     // +0x0060
    char pad64[0x5dfc - 0x64];
    std::vector<int> v5dfc;                      // +0x5dfc (retire list)
    char pad5e0c[0x63d0 - 0x5e0c];
    TerrainDesc terrain;                         // +0x63d0
    vector4 f6598;                               // +0x6598
    float f65a8;                                 // +0x65a8 (sky object / sky time)
    float f65ac;                                 // +0x65ac
    Wind* f65b0;                                 // +0x65b0 ambient wind A
    Wind* f65b4;                                 // +0x65b4 ambient wind B
    int f65b8;                                   // +0x65b8 camera node
    int f65bc;
    int f65c0;                                   // +0x65c0 camera node
    int f65c4;
    int f65c8;
    int f65cc;
    int f65d0;
    int f65d4;
    int f65d8;
    int f65dc;
    int f65e0;
    int f65e4;
    char pad65e8[0x6664 - 0x65e8];
    int f6664;                                   // +0x6664
    char pad6668[0x6678 - 0x6668];
    std::vector<int> v6678;                      // +0x6678 (dead/culled list)
    char pad6688[0x672c - 0x6688];
    int f672c;                                   // +0x672c
    int f6730;
    int f6734;
    int f6738;
    int f673c;
    int f6740;
    int f6744;
    void* f6748;                                 // +0x6748 camera record
    matrix frame_a;                              // +0x674c
    char pad678c[0x67cc - 0x678c];
    float blend_from;                            // +0x67cc
    float blend_to;                              // +0x67d0
    float blend_time;                            // +0x67d4
    char pad67d8[0x67e8 - 0x67d8];
    int f67e8;                                   // +0x67e8 (frame clock origin, -1 = unset)
    char pad67ec[0x67f0 - 0x67ec];
    int blend_n;                                 // +0x67f0
    float blend2_from;                           // +0x67f4
    char pad67f8[0x6810 - 0x67f8];
    float blend2_time;                           // +0x6810
    tstring str;                                 // +0x6814

    void reset();                                                            // 0x479160
    void setup_ambient(int level_idx, int spec_idx, float t);                 // 0x4792d0
};

typedef char assert_grid[offsetof(Mission, poolZ) == 0x0004 ? 1 : -1];
typedef char assert_pool_size[sizeof(Pool<RecZ>) == 0x28 ? 1 : -1];
typedef char assert_poolJ[offsetof(Mission, poolJ) == 0x0030 ? 1 : -1];
typedef char assert_v5dfc[offsetof(Mission, v5dfc) == 0x5dfc ? 1 : -1];
typedef char assert_terrain[offsetof(Mission, terrain) == 0x63d0 ? 1 : -1];
typedef char assert_f6598[offsetof(Mission, f6598) == 0x6598 ? 1 : -1];
typedef char assert_windA[offsetof(Mission, f65b0) == 0x65b0 ? 1 : -1];
typedef char assert_windB[offsetof(Mission, f65b4) == 0x65b4 ? 1 : -1];
typedef char assert_f6664[offsetof(Mission, f6664) == 0x6664 ? 1 : -1];
typedef char assert_v6678[offsetof(Mission, v6678) == 0x6678 ? 1 : -1];
typedef char assert_f672c[offsetof(Mission, f672c) == 0x672c ? 1 : -1];
typedef char assert_frame_a[offsetof(Mission, frame_a) == 0x674c ? 1 : -1];
typedef char assert_f67e8[offsetof(Mission, f67e8) == 0x67e8 ? 1 : -1];
typedef char assert_blend2[offsetof(Mission, blend2_time) == 0x6810 ? 1 : -1];
typedef char assert_str[offsetof(Mission, str) == 0x6814 ? 1 : -1];

// @ 0x479160
void Mission::reset()
{
    if (grid) {
        delete grid;
        grid = 0;
    }

    // vector<int>::resize(0, 0) below is the 2-argument overload; the original materialises the
    // value temporary in a stack slot and passes its address (cl 13.10 elides the store and passes
    // whatever is in the slot), so the source-level form is kept but cl builds its own temp.
    v6678.resize(0, 0);
    blend_time = 0.0f;
    blend2_time = 0.0f;
    f67e8 = -1;
    poolZ.reset();
    poolJ.reset();
    f5c = -1;
    f60 = -1;
    // The store order below is the original's (the compiler keeps it), so it is reproduced verbatim.
    f65ac = 0;
    f65a8 = 0;
    f65b8 = 0;
    f65bc = 0;
    f65c0 = 0;
    f65c4 = 0;
    f65c8 = 0;
    f65d0 = 0;
    f65d4 = 0;
    f65d8 = 0;
    f65dc = 0;
    f65e4 = 0;
    f65e0 = 0;
    f65cc = 0;
    f65b0 = 0;
    f6664 = -1;
    {
        vector4 v(10.0f, 10.0f, 10.0f);
        f6598 = v;
    }
    terrain.init();
    f672c = 0;
    f6730 = 0;
    f6734 = 0;
    f673c = 0;
    f6738 = 0;
    f6740 = 0;
    f6748 = 0;
    blend2_from = 1.0f;
    {
        matrix tmp;
        frame_a = *identity_pos(&tmp);
    }
    str.clear();
    v5dfc.resize(0, 0);
}
// @ 0x4792d0
void Mission::setup_ambient(int level_idx, int spec_idx, float t)
{
    Level* lv = g_game->levels + level_idx;
    Row* sp = g_snd + spec_idx;

    vector4 col;
    unpack_color(&col, (const unsigned*)((char*)sp + 0x44));

    float k = t * 60.0f;
    if (k < 0.0f)
        k = 0.0f;
    else if (k >= 1.0f)
        k = 1.0f;

    // ---- first wind object: a horizontal "sheet" whose normal comes from the hourly table
    Wind* a = grid->spawn();
    f65b0 = a;
    a->f0 = 2;

    double hour = fmod((double)((float)g_time1 * 1e-5f), 24.0);
    int day = (4.0 <= hour && hour < 20.0);

    int slot = (int)((float)(g_ticks[g_game->f13b18] - *(__int64*)&g_game->f13b20) * 1e-5f) % 24;
    vector4 dir = sp->get_dir(slot, day);
    a->f44 = dir;
    a->f24 = vector4(dir.x * -10000.0f, dir.y * -10000.0f, dir.z * -10000.0f);
    a->f54 = 20000.0f;
    a->f6c = 0;

    if (lv->f30 & 1) {
        // flat variant: the wind object just sits at the origin, the cell gets a uniform colour
        a->pos = vector4(0.0f, 0.0f, 0.0f);
        float s = g_dinput_enabled ? 0.03f : 0.15f;
        grid->f68 = vector4(s, s, s);
        grid->f68.w = 1.0f;
        a->f14 = a->pos;
        return;
    }

    // ---- variant B: an anisotropically scaled wind gust
    float p = (float)pow((double)(1.0f - k), 1.5);
    float w1 = p * sp->f20;
    float w2 = p * sp->f24;
    float w3 = p * sp->f28;
    if ((sp->f10 & 0x10000000) || sqrtf(w1 * w1 + w2 * w2 + w3 * w3) < 0.1f)
        a->f6c |= 2;
    if (!g_dinput_enabled) {
        w1 = sqrtf(w1);
        w2 = sqrtf(w2);
        w3 = sqrtf(w3);
    }
    a->pos = vector4(w1, w2, w3);
    a->f14 = a->pos;

    // the cell colour: the unpacked table colour faded by k, offset by the table base position
    float g1 = col.x * k * 0.2f + sp->base.x;
    float g2 = col.y * k * 0.2f + sp->base.y;
    float g3 = col.z * k * 0.2f + sp->base.z;
    if (!g_dinput_enabled) {
        g1 = sqrtf(g1);
        g2 = sqrtf(g2);
        g3 = sqrtf(g3);
    }
    grid->f68 = vector4(g1, g2, g3);
    grid->f68.w = 1.0f;

    // ---- second wind object: the table's direction rotated by 3.5 rad, normalised, half-size
    Wind* b = grid->spawn();
    f65b4 = b;
    b->f0 = 2;

    double hour2 = fmod((double)((float)g_time1 * 1e-5f), 24.0);
    int day2 = (4.0 <= hour2 && hour2 < 20.0);
    int slot2 = (int)((float)(g_ticks[g_game->f13b18] - *(__int64*)&g_game->f13b20) * 1e-5f) % 24;
    vector4 d2 = sp->get_dir(slot2, day2);

    float len = sqrtf(d2.x * d2.x + d2.y * d2.y);
    float cx, cy;
    if (len == 0.0f) {
        cx = 0.0f;
        cy = 1.0f;
    } else {
        float inv = 1.0f / len;
        cx = d2.x * inv;
        cy = d2.y * inv;
    }
    vector4 rd(cos(3.5) * cx - sin(3.5) * cy, sin(3.5) * cx + cos(3.5) * cy, -0.9f);
    rd.normalize();
    b->f44 = rd;

    b->pos = sp->base;
    b->pos.x *= 0.5f;
    b->pos.y *= 0.5f;
    b->pos.z *= 0.5f;
    b->f14 = b->pos;
    b->f24 = vector4(rd.x * -10000.0f, rd.y * -10000.0f, rd.z * -10000.0f);
    b->f54 = 20000.0f;
    b->f6c = 1;
}

// ---------------------------------------------------------------- STL
// 0x38-byte record: three words, two vector<int> (+0xc, +0x1c) and three words (see s0043e514).
struct E56 {
    int a, b, c;
    std::vector<int> v1;            // +0x0c
    std::vector<int> v2;            // +0x1c
    int d, e, f;
};

// @ 0x479930
template std::vector<E56>::iterator std::vector<E56>::erase(std::vector<E56>::iterator, std::vector<E56>::iterator);
