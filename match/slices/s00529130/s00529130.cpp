// Slice s00529130: the mission HUD's per-frame marker update (0x529130), its per-frame tick
// (0x5295a0), the marker enable/disable pass (0x529600) and the party-stack hero reset (0x529670).
// Everything here is a __thiscall member of the HUD object; the marker widgets are reached through
// their vtables (slot 0x58 = set position, 0xb0 = hide, 0xb4 = show).  Names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- vtable-bearing marker widget (slots by byte offset: 0x58 pos, 0xb0 hide, 0xb4 show) ---------------
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(const float* p);                     // +0x58
    virtual void v23(); virtual void v24(); virtual void v25(); virtual void v26();
    virtual void v27(); virtual void v28(); virtual void v29(); virtual void v30();
    virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38();
    virtual void v39(); virtual void v40(); virtual void v41(); virtual void v42();
    virtual void v43();
    virtual void hide();                                      // +0xb0
    virtual void show();                                      // +0xb4
};

// secondary widget with an ARGB colour dword at +0x3c
struct Mark {
    char pad[0x3c];
    unsigned color;                                           // +0x3c
};

// ---- agent record (0x5928 bytes; layout from s0043ac40 / s0043bdb0 / s0043b5d0) -------------------------
struct Agent;
struct AgentTable {
    int count;                                                // +0x00
    char pad4[0x18 - 4];
    Agent** pages;                                            // +0x18 (id >> 4 -> page of 16)
};
struct ItemRef { int item; unsigned m4; };
struct Agent {
    char pad0[0x10];
    int f10;                                                  // +0x10
    int f14;                                                  // +0x14
    char pad18[0x2c - 0x18];
    int mount;                                                // +0x2c
    char pad30[0x50 - 0x30];
    int f50;                                                  // +0x50
    char pad54[0x60 - 0x54];
    char blk60[0x88 - 0x60];                                  // +0x60 (aim block, blended with +0x55d8)
    int troop;                                                // +0x88
    char pad8c[0x94 - 0x8c];
    int f94;                                                  // +0x94
    char pad98[0x1bc - 0x98];
    int f1bc;                                                 // +0x1bc (party id)
    int f1c0;                                                 // +0x1c0 (party stack index)
    char pad1c4[0x55d8 - 0x1c4];
    vector4 facing;                                           // +0x55d8
    char pad55e8[0x560c - 0x55e8];
    int f560c;
    char pad5610[0x5624 - 0x5610];
    vector4 prev_facing;                                      // +0x5624
    float aim_error;                                          // +0x5634
    char pad5638[0x56c4 - 0x5638];
    int f56c4;                                                // +0x56c4 (range-table index)
    char pad56c8[0x571c - 0x56c8];
    float f571c;                                              // +0x571c
    int pad5720;                                              // +0x5720
    float f5724;                                              // +0x5724
    int pad5728[2];                                           // +0x5728
    int f5730;                                                // +0x5730
    char pad5734[0x5928 - 0x5734];

    ItemRef slot_pair(int idx) const;                         // 0x432790 (thiscall, ret 8)
    bool charged_0x48() const;                                // 0x432470 (thiscall)
    int find_target();                                        // 0x43b9e0 (fastcall)
    int pick_attack_dir(unsigned target);                     // 0x43f970 (thiscall, ret 4)
};

// ---- mission (the object at 0x5e292c: agent table at +4, current agent at +0x5c) ------------------------
struct Mission {
    int f0;                                                   // +0x00
    AgentTable agents;                                        // +0x04
    char pad20[0x5c - 0x20];
    unsigned cur_agent;                                       // +0x5c
    char pad60[0x6524 - 0x60];
    int f6524;                                                // +0x6524
    void fn_529840(int mode);                                 // 0x529840 (thiscall, ret 4)
};

// ---- party stack (0x20 bytes, array at party+0xa8; layout from s00490a30) --------------------------------
struct Stack {
    int count;                                                // +0x00
    int troop;                                                // +0x04
    int flags;                                                // +0x08 (byte at +0xb0, bit 0)
    int f0c, f10, f14, f18, f1c;
};
struct Party {
    char pad0[0xa8];
    Stack stacks[64];                                         // +0xa8
    int nstacks;                                              // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                                            // +0x138d0
    int index_of(unsigned id);                                // 0x48ad00 (thiscall, ret 4)
};
struct Troop {
    char pad0[0x30];
    unsigned flags;                                           // +0x30 (0x10 = hero)
    char pad34[0xa0 - 0x34];
    float f_a0;                                               // +0xa0
    char pad_a4[0x508 - 0xa4];
};
struct GameData {
    char pad0[0x78];
    int cur_party;                                            // +0x78
    char pad7c[0x8c - 0x7c];
    int cur_troop;                                            // +0x8c
    void fn_51f800();                                         // 0x51f800 (thiscall)
};

// ---- the HUD object -----------------------------------------------------------------------------------
struct MissionHud {
    int f0;                                                   // +0x00
    int f4;                                                   // +0x04
    char pad8[0x14 - 8];
    int f14;                                                  // +0x14 (state: 1, 3, 4 tested here)
    char pad18[0x68 - 0x18];
    Widget* w[7];                                             // +0x68
    float color[7];                                           // +0x84
    Mark* marks[7];                                           // +0xa0
    char padbc[0x154 - 0xbc];
    int f154;                                                 // +0x154 (party id)
    char pad158[0x288 - 0x158];
    float f288;                                               // +0x288 (field of view, degrees)
    char pad28c[0x2c8 - 0x28c];
    char f2c8;                                                // +0x2c8

    void update_marks();                                      // 0x529130
    void tick();                                              // 0x5295a0
    void refresh_markers();                                   // 0x529600
    void reset_party_heroes(int party);                       // 0x529670
};

// ---- externals -----------------------------------------------------------------------------------------
extern Mission* g_scene;                                      // 0x5e292c
extern GameData* g_game;                                      // 0x5e2930
extern char* g_party_tbl;                                     // 0x5e2a00 (0x58-byte records)
extern void* g_objs[];                                        // 0xa170d4
extern int g_sel0;                                            // 0xa73798
extern int g_sel1;                                            // 0xa7379c
extern int g_flag5aa06c;                                      // 0x5aa06c
extern int g_flag5aa070;                                      // 0x5aa070
extern int g_flag5aa074;                                      // 0x5aa074
struct ItemKind { char pad[0x1c8]; int type; char pad1cc[0x238 - 0x1cc]; };
extern ItemKind* g_items;                                     // 0x5e29d0
extern float g_deg2rad;                                       // 0x590ce4 (0.017453292)
extern float g_half;                                          // 0x549d70 (0.5f)
extern float g_0375;                                          // 0x59c310 (0.375f)
extern float g_2p5;                                           // 0x590c4c (2.5f)
extern float g_1p2;                                           // 0x590b94 (1.2f)
extern float g_step01;                                        // 0x5a9c20 (0.01f)
extern float g_255;                                           // 0x590cec (255.0f)
extern float g_0;                                             // 0x549ca8 (0.0f)
extern float g_1;                                             // 0x549cdc (1.0f)
extern float g_range_tbl[];                                   // 0x59390c (lo/hi pairs)

float __cdecl blend_target(void* a, void* b);                 // 0x4381e0

inline PartyMgr* party_mgr() { return (PartyMgr*)((char*)g_game + 0xf8); }
inline Troop* troop_tbl() { return *(Troop**)((char*)g_game + 0x13a00); }
inline Agent* cur_agent() { return &g_scene->agents.pages[g_scene->cur_agent >> 4][g_scene->cur_agent & 0xf]; }

// @ 0x529130
void MissionHud::update_marks()
{
    Agent* a = &g_scene->agents.pages[g_scene->cur_agent >> 4][g_scene->cur_agent & 0xf];

    float y = (g_half / tanf(f288 * g_deg2rad * g_half)) * tanf(a->aim_error);

    float p[2];
    p[0] = 0.5f;
    p[1] = y + g_0375;
    w[0]->set_pos(p);

    float x = 0.0f;
    p[0] = x;
    p[1] = y;
    float c = (float)cos(2.268928f);
    float s = (float)sin(2.268928f);
    float nx = p[0] * c - p[1] * s;
    float ny = p[0] * s + p[1] * c;
    p[0] = nx + g_half;
    p[1] = ny + g_0375;
    w[1]->set_pos(p);

    p[0] = x;
    p[1] = y;
    c = (float)cos(-2.268928f);
    s = (float)sin(-2.268928f);
    nx = p[0] * c - p[1] * s;
    ny = p[0] * s + p[1] * c;
    p[0] = nx + g_half;
    p[1] = ny + g_0375;
    w[2]->set_pos(p);

    w[0]->hide();
    w[1]->hide();
    w[2]->hide();
    w[3]->hide();
    w[4]->hide();
    w[5]->hide();
    w[6]->hide();

    float target[7] = {0, 0, 0, 0, 0, 0, 0};

    ItemRef pair = a->slot_pair(0);
    float aim = blend_target(a->blk60, &a->facing);

    if (pair.item >= 0) {
        bool other = false;
        if (g_sel0 >= 0 && g_objs[g_sel0]) other = true;
        if ((g_sel1 < 0 || g_objs[g_sel1] == 0) && !other) {
            int type = g_items[pair.item].type & 0xff;
            if ((type == 8 || type == 9 || type == 10 || type == 16 || type == 17) && g_flag5aa06c) {
                if (!cur_agent()->charged_0x48()) {
                    Agent* a2 = cur_agent();
                    bool in_range = true;
                    if (a2->mount != -1) {
                        int idx = a2->f56c4 * 2;
                        in_range = (g_range_tbl[idx] <= aim && aim < g_range_tbl[idx + 1]);
                    }
                    if (in_range) {
                        target[0] = 0.9f;
                        target[1] = 0.9f;
                        target[2] = 0.9f;
                    }
                }
            } else if (type == 2 || type == 3 || type == 4) {
                if (g_flag5aa070 || !g_flag5aa074) {
                    int t = a->find_target();
                    if (t >= 0) {
                        int dir = a->pick_attack_dir(t);
                        if (dir >= 0 && dir < 4) {
                            if (dir == 3)
                                target[3] = 0.7f;
                            else if (dir == 1)
                                target[4] = 0.7f;
                            else if (dir == 0)
                                target[5] = 0.7f;
                            else if (dir == 2)
                                target[6] = 0.7f;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < 7; i++) {
        float step = g_step01 * (i < 3 ? 0.5f : 2.5f);
        if (target[i] > color[i]) {
            color[i] += step * g_1p2;
            if (color[i] > target[i]) color[i] = target[i];
        } else if (target[i] < color[i]) {
            color[i] -= step;
            if (color[i] < target[i]) color[i] = target[i];
        }
        if (i < 3)
            marks[i]->color = 0xffffff;
        else
            marks[i]->color = 0xffbb11;
        int alpha = (int)(color[i] * g_255);
        if (alpha) {
            if (i < 3)
                w[i]->show();
            else
                w[i]->show();
        }
        marks[i]->color |= (unsigned)alpha << 24;
    }
}

// @ 0x5295a0
void MissionHud::tick()
{
    g_game->fn_51f800();
    if (f2c8 != 0 || (f154 >= 0 && (g_party_tbl[f154 * 0x58 + 0x20] & 0x10))) {
        refresh_markers();
    }
}

// @ 0x529600
void MissionHud::refresh_markers()
{
    int n = 1;
    if (g_scene->f6524 > 0)
        n = 0x701;
    else if (g_scene->f6524 < 0)
        n = 0x7031;
    g_scene->fn_529840(n);
    if (f14 == 3 || f14 == 4 || f14 == 1) {
        reset_party_heroes(g_game->cur_party);
        if (f0 >= 0) reset_party_heroes(f0);
        if (f4 >= 0) reset_party_heroes(f4);
    }
}

// @ 0x529670
void MissionHud::reset_party_heroes(int party_id)
{
    PartyMgr* mgr = party_mgr();
    int idx = mgr->index_of(party_id);
    Party* party = &mgr->pages[idx >> 4][idx & 0xf];

    int n = 0;
    int i = 0;
    while (i < party->nstacks) {
        if (party->stacks[i].flags & 1) break;
        n++;
        i++;
    }

    for (int j = 0; j < n; j++) {
        Troop* t = (Troop*)(*(int*)((char*)g_game + 0x13a00) + party->stacks[j].troop * 0x508);
        if (t->flags & 0x10) {
            if (t->f_a0 < 0.0f) t->f_a0 = 0.0f;
        }
    }
}
