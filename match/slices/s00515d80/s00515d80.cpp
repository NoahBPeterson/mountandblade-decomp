// Slice s00515d80: the mission camera's per-frame update (0x515d80), the 2x2 matrix helper it uses
// (0x516220), the agent-view record constructor (0x516250) and its per-frame dispatch (0x516370), and
// the party record's "data ready" test (0x5166a0).
// Layouts are provisional; every offset comes from this slice's disassembly.
#include <windows.h>
#include <math.h>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- shared .rdata constants (declared as globals so the original addresses are referenced) ----------
extern const float g_zero;          // 0x549ca8  0.0f
extern const float g_ten;           // 0x549d74  10.0f
extern const float g_oneone;        // 0x54b888  1.1f
extern const float g_pttwo;         // 0x54a6e4  0.2f
extern const float g_three;         // 0x54a710  3.0f
extern const float g_quarter;       // 0x54b868  0.25f
extern const float g_tick_scale;    // 0x54a6d0  1e-5f
extern const float g_four;          // 0x5935c8  4.0f
extern const float g_003;           // 0x54b8b4  0.03f
extern const float g_axis_x;        // 0xa178d4
extern const float g_axis_y;        // 0xa178dc
extern const float g_mouse_dx;      // 0x5d70a4
extern const float g_mouse_dy;      // 0x5d70a8
extern const float g_exp_arg;       // 0x5aa018
extern __int64 g_ticks[];           // 0x5e2948  100 kHz clock table (two dwords per entry)

// key state: indices at 0xa73730..0xa7374c, down-flags at 0xa170d4
extern int g_key_1730;              // 0xa73730
extern int g_key_1734;              // 0xa73734
extern int g_key_1738;              // 0xa73738
extern int g_key_173c;              // 0xa7373c
extern int g_key_1740;              // 0xa73740
extern int g_key_1744;              // 0xa73744
extern int g_key_1748;              // 0xa73748
extern int g_key_174c;              // 0xa7374c
extern int g_keydown[];             // 0xa170d4
extern int g_in_17204;              // 0xa17204
extern int g_in_17210;              // 0xa17210
extern int g_in_17214;              // 0xa17214
extern int g_in_17218;              // 0xa17218
extern int g_in_17458;              // 0xa17458
extern int g_in_17858;              // 0xa17858

// ---- the 0x98-byte mission agent record and the 0x5ac8-byte party record ------------------------------
struct AgentRec {
    char pad0[0x98];
};
struct Party {
    char pad0[0x70];
    int      f70;                   // +0x70
    char pad74[0xa0 - 0x74];
    __int64 flags;                  // +0xa0
    char pad_a8[0x59b0 - 0xa8];
    int      clock;                 // +0x59b0
    __int64  start;                 // +0x59b8
    char pad_c0[0x5ac8 - 0x59c0];
    int data_ready();               // 0x5166a0
};
struct PartyMgr {
    char pad0[0x8a8];
    int  count;                     // +0x8a8
    int index_of(int id);           // 0x48ad00
};
struct GameData {
    char pad0[0x78];
    int  cur_agent;                 // +0x78
    char pad7c[0x84 - 0x7c];
    int  f84;                       // +0x84
    char f88;                       // +0x88
    char pad89[0xe4 - 0x89];
    AgentRec** agent_pages;         // +0xe4
    char pad_e8[0x139c8 - 0xe8];
    Party** party_pages;            // +0x139c8
    char pad_9cc[0x13a00 - 0x139cc];
    char* f13a00;                   // +0x13a00
    char pad_a04[0x13a18 - 0x13a04];
    char f13a18;                    // +0x13a18
    char pad_a19[0x13a48 - 0x13a19];
    char* f13a48;                   // +0x13a48
};
extern GameData* g_game;            // 0x5e2930

// The party pages: index -> Party* (0x5ac8 stride; page table at GameData+0x139c8).
static Party* party_by_id(int id)
{
    PartyMgr* mgr = (PartyMgr*)((char*)g_game + 0xf8);
    int idx = mgr->index_of(id);
    return (Party*)((char*)g_game->party_pages[idx >> 4] + (idx & 0xf) * 0x5ac8);
}

// ---- callees -----------------------------------------------------------------------------------------
struct mat2 {
    float m[4];
    mat2(const float* a, const float* b);   // 0x516220
    void rotate(float angle);               // 0x4f3b40 (thiscall: pointer in ecx, angle on the stack)
};

void refresh_rating();                  // 0x492470

// ======================================================================================================
// The camera object: yaw at +0x2b0, speed +0x2b4, height +0x2b8, position x/z +0x2bc/+0x2c0,
// vertical velocity +0x2c4, y +0x2c8.
struct Camera {
    char  pad0[0x2b0];
    float yaw;              // +0x2b0
    float speed;            // +0x2b4
    float height;           // +0x2b8
    float x;                // +0x2bc
    float z;                // +0x2c0
    float vy;               // +0x2c4
    float y;                // +0x2c8
    char  pad2cc[8];
    char  f2d4;             // +0x2d4
    void update(float dt);  // 0x515d80
};

// @ 0x515d80
void Camera::update(float dt)
{
    float src[2][2] = { { 0.0f, 1.0f }, { 1.0f, 0.0f } };
    mat2 m(&src[0][0], &src[1][0]);
    m.rotate(-yaw);

    bool b = g_key_1740 >= 0 && g_keydown[g_key_1740] != 0;
    if ((g_key_1744 >= 0 && g_keydown[g_key_1744] != 0) || b) {
        float h = height + g_ten;
        x -= m.m[0] * h * dt;
        z -= m.m[1] * h * dt;
        g_game->f88 = 0;
        g_game->f84 = g_game->cur_agent;
    }
    b = g_key_1748 >= 0 && g_keydown[g_key_1748] != 0;
    if ((g_key_174c >= 0 && g_keydown[g_key_174c] != 0) || b) {
        float h = height + g_ten;
        x += m.m[0] * h * dt;
        z += m.m[1] * h * dt;
        g_game->f88 = 0;
        g_game->f84 = g_game->cur_agent;
    }
    b = g_key_1730 >= 0 && g_keydown[g_key_1730] != 0;
    if ((g_key_1734 >= 0 && g_keydown[g_key_1734] != 0) || b) {
        float h = height + g_ten;
        x += m.m[2] * h * dt;
        z += m.m[3] * h * dt;
        g_game->f88 = 0;
        g_game->f84 = g_game->cur_agent;
    }
    b = g_key_1738 >= 0 && g_keydown[g_key_1738] != 0;
    if ((g_key_173c >= 0 && g_keydown[g_key_173c] != 0) || b) {
        float h = height + g_ten;
        x -= m.m[2] * h * dt;
        z -= m.m[3] * h * dt;
        g_game->f88 = 0;
        g_game->f84 = g_game->cur_agent;
    }

    if (g_in_17204) speed += dt * g_oneone;
    if (g_in_17214) speed -= dt * g_oneone;
    if (g_in_17210) vy += dt * g_oneone;
    if (g_in_17218) vy -= dt * g_oneone;

    if (g_in_17458 && !g_in_17858)
        f2d4 = 0;

    float v = g_mouse_dx * 0.1f;
    if (!(v > -1.0f)) v = -1.0f;
    else if (v > 1.0f) v = 1.0f;

    float w = g_mouse_dy * 0.1f;
    if (!(w > -2.0f)) w = -2.0f;
    else if (w > 2.0f) w = 2.0f;

    if (g_axis_x != 0.0f)
        f2d4 = 1;

    vy = v * dt + vy;
    if (g_in_17458) {
        vy = expf(g_four * g_exp_arg) * g_003 * g_axis_x * dt + vy;
    }

    if (w != 0.0f || g_axis_y != 0.0f) {
        y = y - w * height * dt;
        if (g_axis_y > 0.0f)
            y = y - height * g_pttwo;
        else if (g_axis_y < 0.0f)
            y = height * g_pttwo + y;
    }
}

// @ 0x516220
mat2::mat2(const float* a, const float* b)
{
    m[0] = a[0];
    m[1] = a[1];
    m[2] = b[0];
    m[3] = b[1];
}

// ======================================================================================================
// The agent-view record. Fields used by the constructor and the two dispatchers.
struct Unit {
    int  f00;               // +0x00
    int  f04;               // +0x04
    int  id;                // +0x08
    char pad0c[4];
    unsigned flags;         // +0x10
    int  f14;               // +0x14
    char pad18[8];
    char f20;               // +0x20
    char f21;               // +0x21
    char pad22[2];
    int  f24;               // +0x24
    char pad28[0xc];
    char f34;               // +0x34
    char f35;               // +0x35
    char pad36[2];
    int  f38;               // +0x38
    unsigned f3c;           // +0x3c
    char f40;               // +0x40
    char pad41[0x138 - 0x41];
    int  f138;              // +0x138
    char pad13c[0x144 - 0x13c];
    int  f144;              // +0x144
    char pad148[0x164 - 0x148];
    int  f164;              // +0x164
    int  f168;              // +0x168
    int  f16c;              // +0x16c
    int  f170;              // +0x170
    char pad174[0x274 - 0x174];
    int  f274;              // +0x274
    int  f278;              // +0x278
    char pad27c[0x2c0 - 0x27c];
    int  f2c0;              // +0x2c0
    int  f2c4;              // +0x2c4
    int  f2c8;              // +0x2c8
    Unit(int id_);          // 0x516250
    void frame(int tick);   // 0x516370
    void base_init();       // 0x4be730
    void update_anim();     // 0x516700
};

// @ 0x516250
Unit::Unit(int id_)
{
    base_init();
    id = id_;
    unsigned urow = (id_ & 0xf) * 0x98;
    f40 = 1;
    f138 = 0;
    f144 = 1;
    flags = 1;
    f14 = 3;

    char* pages = (char*)g_game->agent_pages;
    AgentRec* rec = *(AgentRec**)(pages + ((unsigned)id_ >> 4) * 4);
    rec = (AgentRec*)((char*)rec + urow);
    if (g_game->cur_agent == *(int*)((char*)rec + 0x54)) {
        f00 = *(int*)((char*)rec + 0x58);
        f04 = -1;
        f24 = 1;
        f144 = 1;
    } else if (g_game->cur_agent == *(int*)((char*)rec + 0x58)) {
        f00 = *(int*)((char*)rec + 0x54);
        f04 = -1;
        f24 = 0;
        f144 = 1;
    } else if (*(int*)((char*)rec + 0x10) == 1) {
        f00 = *(int*)((char*)rec + 0x58);
        f04 = *(int*)((char*)rec + 0x54);
        flags = 0x11;
        f144 = 0;
        f24 = 2;
        f14 = 4;
    }
    g_game->f13a18 = 1;
}

// @ 0x516370
void Unit::frame(int tick)
{
    f2c4 = tick;
    if (f24 == 2 && tick <= 0) {
        int aid = id;
        AgentRec* rec = (AgentRec*)((char*)g_game->agent_pages[aid >> 4] + (aid & 0xf) * 0x98);
        f00 = *(int*)((char*)rec + 0x58);
        rec = (AgentRec*)((char*)g_game->agent_pages[aid >> 4] + (aid & 0xf) * 0x98);
        f04 = *(int*)((char*)rec + 0x54);

        bool a = !(*(float*)(g_game->f13a48 + party_by_id(f00)->f70 * 0x300 + 0x2c) > 0.0f);
        bool b = *(float*)(g_game->f13a48 + party_by_id(f04)->f70 * 0x300 + 0x2c) < 0.0f;
        if (a != b) {
            f138 = b ? 1 : 0;
            flags &= ~0x10;
            f144 = 1;
        } else {
            flags |= 0x10;
            f138 = 0;
            f144 = 0;
        }
    }
    if (f144 != 0) {
        refresh_rating();
        if (f2c4 < 1)
            f3c |= 1;
    }
    Party* p = party_by_id(g_game->cur_agent);
    if (p->data_ready())
        flags |= 2;

    AgentRec* self = (AgentRec*)((char*)g_game->agent_pages[id >> 4] + (id & 0xf) * 0x98);
    if (g_game->cur_agent == *(int*)((char*)self + 0x58))
        flags |= 4;
    else if (g_game->cur_agent == *(int*)((char*)self + 0x54))
        flags |= 8;

    update_anim();
}

// ======================================================================================================
// @ 0x5166a0  Party::data_ready
int Party::data_ready()
{
    if ((flags & 0x1000000) == 0) {
        __int64 d = g_ticks[clock] - start;
        if (!((float)d * g_tick_scale < g_quarter))
            return 0;
    }
    return 1;
}
