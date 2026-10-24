// game_tick.h: the campaign map's frame tick and its helpers (code 0x516ac0..0x5191e0): the game data object (GameData,
// src/include/game_data.h; the slices called it Game and GameData) with the methods of this region, the paged pool of
// 0x98-byte map records, the global marker lists and the small records the tick walks. The methods are members of GameG,
// a view of GameData that adds them (the canonical header has no room for them: it is being edited by other modules); the
// fields they read are reached by offset (the .cpp names them with macros). Names provisional.
#ifndef GAME_INVENTORY_PANEL_GAME_TICK_H
#define GAME_INVENTORY_PANEL_GAME_TICK_H
#include "./inv_common.h"

// the paged pool of 0x98-byte map records at game+0xcc (0x517960, 0x5179b0, 0x516ac0)
struct MapRec {                         // 0x98 bytes
    char pad0[4];
    int used;                           // +0x04
    char pad8[0x10 - 8];
    int type;                           // +0x10
    char pad14[0x24 - 0x14];
    float x;                            // +0x24
    float y;                            // +0x28
    char pad2c[0x44 - 0x2c];
    float opacity;                      // +0x44 (visibility 0..1)
    char pad48[0x50 - 0x48];
    void* obj;                          // +0x50
    int f54;                            // +0x54
    int f58;                            // +0x58
    char pad5c[0x80 - 0x5c];
    int anim;                           // +0x80
    char pad84[4];
    __int64 stamp;                      // +0x88
    int sound;                          // +0x90
    char pad94[4];
    void update_nav_anim(float speed);  // 0x4b71e0
};
struct GamePool {
    int count;                          // +0x00
    char pad4[0x14];
    char** pages;                       // +0x18 (page = 16 records of 0x98 bytes)
    void next(int* index);              // 0x517960
    int first();                        // 0x5179b0
    MapRec* slot(unsigned index);       // 0x48ef20
};

// a tracking record of the party-track ribbons (0x5c bytes, vector at game+0x13a60)
struct Rec5c {
    float x, y, z;                      // +0x00
    char pad0c[0x14 - 0x0c];
    float f14;                          // +0x14 (age / timer)
    union {
        unsigned int flags;             // +0x18 (0x20000000 visible, 0x10000000 battle track)
        struct {
            unsigned short w18;         // +0x18
            unsigned char f1a;          // +0x1a
            unsigned char b1b;          // +0x1b
        };
    };
    char pad1c[0x5c - 0x1c];
};
struct RecArray {                       // the iterator helper on a bare base pointer (0x4b22b0)
    Rec5c* base;
    Rec5c** at(Rec5c** out, int idx);   // 0x4b22b0 (ret 8, returns out)
};
struct Vec5c {                          // vector of Rec5c at game+0x13a60
    int pad0;                           // +0x0
    Rec5c* _Myfirst;                    // +0x4
    Rec5c* _Mylast;                     // +0x8
    Rec5c* _Myend;                      // +0xc
    unsigned size() const
    {
        if (!_Myfirst)
            return 0;
        return (unsigned)(_Mylast - _Myfirst);
    }
    __declspec(noinline) Rec5c** shift_over(Rec5c** out, Rec5c* dst, Rec5c* src);   // 0x518990 (ret 0xc)
};

// global marker lists (pairs of begin / end pointers; the elements are scene objects with the floats at +0x1f4..+0x208)
struct Marker {
    char pad0[0x1f4];
    float f1f4, f1f8, f1fc, f200, f204, f208;   // +0x1f4
};
struct MarkerList {
    Marker** first;
    Marker** last;
    int count() const { return first ? (int)(last - first) : 0; }
    Marker* operator[](unsigned i) const { return first[i]; }
};

struct MapModel { void set_fade(const float* v); };   // 0x4b1800 (the map entity at Party+0x59c0: PartyModel::set_fade, party2/party_2.h)
struct UpdateObj { void update(int flags); };   // game+0x139e8 (0x462760)
struct SoundObj { void update(int flags); };    // game+0x139f4 (0x48add0)

struct GameG : GameDataV {
    void tick(float dt);                            // 0x516ac0 (ret 4)
    __declspec(noinline) void resume_strings(tstring* a, tstring* b);   // 0x517110 (ret 8)
    void update_markers();                          // 0x517140
    void update_colors();                           // 0x517350
    rgl::vector4* sun_color(rgl::vector4* out);     // 0x5176e0 (ret 4)
    rgl::vector4* sky_color(rgl::vector4* out);     // 0x517770 (ret 4)
    rgl::vector4* fog_color(rgl::vector4* out);     // 0x517820 (ret 4)
    void set_grid_color(unsigned* c);               // 0x5178d0 (ret 4)
    void update_cursor(float dt);                   // 0x5179f0 (ret 4: party / settlement visibility)
    void update_weather();                          // 0x517e70
    void weekly_report(bool arg);                   // 0x5181c0 (ret 4)
    void track_parties(float dt);                   // 0x518680 (ret 4)
    __declspec(noinline) float skill_plus_8();      // 0x518940
    void update_tracks();                           // 0x5189e0
    void train_troops();                            // 0x518d70
    // helpers of the game object this region calls
    bool is_daytime();                              // 0x435700
    void dissolve(int id);                          // 0x4915d0 (ret 4: GameData::remove_party)
    Party* player_party()
    {
        PartyMgr* mgr = &parties;
        int i = mgr->index_of(f78);
        Party** pp = mgr->pages._Myfirst + (i >> 4);
        return *pp + (i & 0xf);
    }
};

// the map scene object (global 0xa732b4): a colour at +0x68 (its w lane is the alpha at +0x74) and the per-frame step
#include "../scene_world.h"                  // World: the scene world (colour +0x68, alpha +0x74; step 0x427f40)
extern World* g_map_scene;           // 0xa732b4
// a scene object with a colour (0x517350): the sun and the entities of the cell grid
struct ColorEnt {
    char pad0[0x3c];
    unsigned color;                     // +0x3c
};
struct ColorTarget {                    // game+0x139dc
    char pad0[4];
    rgl::vector4 v;                     // +0x04
};
struct GridModel {                      // a grid cell: a scene object with an entity list
    char pad0[0x11c];
    ColorEnt** ents;                    // +0x11c
    ColorEnt** ents_end;                // +0x120
};

#endif // GAME_INVENTORY_PANEL_GAME_TICK_H
