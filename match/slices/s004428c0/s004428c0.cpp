// Slice s004428c0: GameData::add_experience (0x4428c0: level-up bookkeeping and its UI messages), the effect spawn
// helper (0x442ca0), the dead-agent queue (0x442dc0) and three agent-vs-agent scans (0x442ed0 / 0x443020 / 0x443140).
// All names provisional. Layouts reused from s0043a280 (tstring / translation table), s004333c0 (MsgLog, Obj3),
// s00427900 (world::alloc_emitter), s00440a30 / s0043b5d0 (Agent / Mission), s00442180 (Emit, EmitTable, SoundMgr).
#include <stddef.h>
#include <stdio.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const char* s) MB_NOTHROW;           // 0x42bbe0
    ~tstring() MB_NOTHROW;                       // 0x42bc40
    void operator=(const tstring& o) MB_NOTHROW; // 0x42bc50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;      // 0x43a1e0 (key -> translated string)
    tstring* find(const tstring* key) MB_NOTHROW;        // 0x4c6010 (null when absent)
};
extern StrTable g_strtable;                      // 0x5e2a18

// message log (object at 0x5d7080, see s004333c0): enabled byte at +0x1714, 32-entry ring at +0xb0b8
struct MsgLog {
    char pad0[0x1714];
    char enabled;                                // +0x1714
    char pad1715[0xb0b8 - 0x1715];
    struct Ring {
        int count;                               // +0xb0b8
        int head;                                // +0xb0bc
        tstring names[32];                       // +0xb0c0
        int vals[32];                            // +0xb2c0
    } ring;
    void push(const tstring& s, int v);          // 0x4334b0
};
extern MsgLog g_msglog;                          // 0x5d7080

// the ring-buffer append that this TU inlines (colour 0xfff000)
static __forceinline void log_add(const tstring& s)
{
    if (g_msglog.enabled && g_msglog.ring.count < 32) {
        int idx = (g_msglog.ring.head + g_msglog.ring.count) % 32;
        g_msglog.ring.names[idx] = s;
        g_msglog.ring.vals[idx] = 0xfff000;
        g_msglog.ring.count++;
    }
}

// ---- the sound player (s00442180) ----------------------------------------------------------------------------------
struct SoundMgr {
    int play(int idx, int flags, const float* pos, float freq, int* out_id);   // 0x442650
};
extern SoundMgr g_sound;                         // 0x5aa128
extern int g_levelup_sound;                      // 0x5aa17c
extern float g_levelup_pos[];                    // 0x5aa168
extern int g_show_messages;                      // 0x5aa080
extern int g_xp_table[];                         // 0x590974: experience needed for each level

// ---- game data: troop records and parties ---------------------------------------------------------------------------
struct TroopRec {                                // 0x508 bytes
    tstring id;                                  // +0
    char pad10[0x20 - 0x10];
    tstring name;                                // +0x20
    char pad30[0x9c - 0x30];
    int exp;                                     // +0x9c
    char pada0[4];
    int fa4, fa8, fac;                           // +0xa4 (fac += 10 per level)
    int level;                                   // +0xb0
    char padb4[0x508 - 0xb4];
};
struct Slot { int f0; int troop; char pad8[0x18]; };      // 0x20 bytes
struct Party {                                   // 0x5ac8 bytes
    char pad0[0xa8];
    Slot m[64];                                  // +0xa8
    int count;                                   // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0 (index >> 4 -> page of 16)
    int index_of(int id);                        // 0x48ad00
};
struct GameData {
    char pad0[0x78];
    int f78;                                     // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                  // +0x8c
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                            // +0xf8 (embedded; pages pointer at +0x139c8)
    char pad_x[0x13a00 - 0xf8 - sizeof(PartyMgr)];
    TroopRec* chars;                             // +0x13a00
    char pad13a04[0x13a34 - 0x13a04];
    int f13a34;                                  // +0x13a34
    void add_experience(int troop, int amount);  // 0x4428c0 (ret 8)
};

// @ 0x4428c0
void GameData::add_experience(int troop, int amount)
{
    char buf1[512], buf2[512], buf3[512];
    if (amount < 0)
        amount = 0;
    else if (amount >= 30000)
        amount = 29999;
    if (troop == player && amount > 0 && f13a34 != 0 && g_show_messages != 0) {
        {
            tstring key("ui_you_got_number_experience");
            sprintf(buf1, g_strtable.lookup(key)->c_str(), amount);
        }
        {
            tstring msg(buf1);
            g_msglog.push(msg, -1);
        }
    }
    TroopRec* c = &chars[troop];
    c->exp += amount;
    while (c->exp >= g_xp_table[c->level]) {
        c->fa4++;
        c->fa8++;
        c->fac += 10;
        c->level++;
        if (troop == player) {
            g_sound.play(g_levelup_sound, 1, g_levelup_pos, 1.0f, 0);
            {
                tstring key("ui_you_have_advanced_to_level_number");
                sprintf(buf2, g_strtable.lookup(key)->c_str(), c->level);
            }
            {
                tstring msg(buf2);
                log_add(msg);
            }
        } else {
            PartyMgr* pm = &parties;
            int p = pm->index_of(f78);
            int i = 0;
            if (pm->pages[p >> 4][p & 0xf].count > 0) {
                do {
                    int q = pm->index_of(f78);
                    if (pm->pages[q >> 4][q & 0xf].m[i].troop == troop) {
                        {
                            tstring key("ui_value_has_advanced_to_level_number");
                            tstring* nm = g_strtable.find(&c->name);
                            if (nm == 0)
                                nm = (tstring*)c;
                            sprintf(buf3, g_strtable.lookup(key)->c_str(), nm->c_str(), c->level);
                        }
                        {
                            tstring msg(buf3);
                            log_add(msg);
                        }
                    }
                    i++;
                    int r = pm->index_of(f78);
                    if (!(i < pm->pages[r >> 4][r & 0xf].count))
                        break;
                } while (1);
            }
        }
    }
}

// ---- spawning an effect emitter (0x442ca0) -------------------------------------------------------------------------
struct mat4 {                                           // 64-byte frame (rows s, f, u, o)
    vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;               // 0x40e870 (this = left operand; hidden return pointer)
};
struct EmitRes {                                        // resource record reached through Emit::res
    char pad0[0x1c];
    unsigned flags;                                     // +0x1c (0x40: follows the camera)
};
struct Emit {                                           // particle emitter (0x1bc bytes; see s00442180)
    char pad0[4];
    EmitRes* res;                                       // +4
    char pad8[0xd8 - 8];
    vector4 offs;                                       // +0xd8
    unsigned flags;                                     // +0xe8 (bit 0 cleared on spawn)
    char padec[0xf4 - 0xec];
    mat4 world;                                         // +0xf4
    mat4 local;                                         // +0x134
    char pad174[0x19c - 0x174];
    int f19c, f1a0, f1a4;                               // +0x19c
};
struct EmitTable {
    void* defs; int count;
    void setup(Emit* e, int idx, const vector4* pos);   // 0x4424c0
};
extern EmitTable g_emit_defs;                           // 0x8ed5090
struct World {
    Emit* alloc_emitter();                              // 0x427ea0
};
struct CamRec {                                         // 0x4c bytes, array behind the pointer at 0x5e28f4
    char pad0[0x20];
    float dx, dy, dz;                                   // +0x20
    char pad2c[4];
    float px, py, pz;                                   // +0x30
    char pad3c[0x4c - 0x3c];
};
extern CamRec* g_cams;                                  // 0x5e28f4

extern __int64 g_ticks[];                               // 0x5e2948
struct Agent;
struct Mission2 {
    World* world;                                       // +0
    char pad4[0x660c - 4];
    int cam;                                            // +0x660c
    char pad6610[0x6678 - 0x6610];
    std::vector<Agent*> dead;                           // +0x6678 (first at +0x667c)
    Emit* spawn_effect(int idx, const mat4* m, int duration);   // 0x442ca0
    void queue_dead(Agent* a);                          // 0x442dc0
};
// @ 0x442ca0
Emit* Mission2::spawn_effect(int idx, const mat4* m, int duration)
{
    Emit* e = world->alloc_emitter();
    vector4 v(0.0f, 0.0f, 0.0f);
    g_emit_defs.setup(e, idx, &v);
    e->world = *m * e->local;
    if (e->res != 0 && (e->res->flags & 0x40)) {
        CamRec* c = &g_cams[cam];
        vector4 t;
        t.x = c->dx * 0.3f;
        t.y = c->dy * 0.3f;
        t.z = c->dz * 0.3f;
        v.x = t.x + c->px;
        v.y = t.y + c->py;
        v.z = t.z + c->pz;
        e->offs = v;
    }
    e->flags &= ~1u;
    e->f19c = e->f1a0 = duration;
    e->f1a4 = 0;
    return e;
}

// ---- agents -----------------------------------------------------------------------------------------------------
extern unsigned g_max_dead;                             // 0x5aa044
struct Agent;
struct AgentTable {
    int count;                                          // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
};
struct Mission {
    char pad0[4];
    AgentTable agents;                                  // +4
    char pad_x[0x53c8 - 4 - sizeof(AgentTable)];
    int relation[8 * 0x5b];                             // +0x53c8 (team x team, stride 0x5b)
};
extern Mission* g_mission;                              // 0x5e292c

struct Agent {                                          // 0x5928 bytes
    char pad0[8];
    int id;                                             // +8
    char padc[4];
    int f10;                                            // +0x10
    int f14;                                            // +0x14 (1: active)
    char pad18[0x2c - 0x18];
    int f2c;                                            // +0x2c (mount / rider id)
    char pad30[0x50 - 0x30];
    vector4 pos;                                        // +0x50
    char pad60[0x1c4 - 0x60];
    int team;                                           // +0x1c4
    char pad1c8[0x2c0 - 0x1c8];
    int f2c0;                                           // +0x2c0
    unsigned f2c4;                                      // +0x2c4
    char pad2c8[0x55d8 - 0x2c8];
    float fx, fy, fz;                                   // +0x55d8 (facing direction)
    char pad55e4[0x57e0 - 0x55e4];
    struct Timer {
        int clock; int pad; __int64 start;
        void reset() { start = g_ticks[clock]; }
    } tm;                                               // +0x57e0 (clock id, start tick)
    char pad57f0[0x5830 - 0x57f0];
    int state;                                          // +0x5830
    char pad5834[0x5928 - 0x5834];
    bool rel_a(const Agent* o);                         // 0x432350
    bool rel_b(const Agent* o);                         // 0x4323a0
    bool rel_c(const Agent* o);                         // 0x4323f0
    bool line_of_sight(unsigned id, void* ignore);      // 0x440e30
    bool can_target(unsigned id, unsigned flags, float maxdist, float* out_dist);   // 0x442ed0
    bool any_hostile_near();                            // 0x443020 (fastcall)
    void update_alarm();                                // 0x443140 (fastcall)
};

// @ 0x442dc0
void Mission2::queue_dead(Agent* a)
{
    dead.push_back(a);
    while (dead.size() > g_max_dead) {
        dead[0]->state = 4;
        Agent* o = dead[0];
        o->tm.reset();
        for (unsigned i = 1; i < dead.size(); i++)
            dead[i - 1] = dead[i];
        dead.pop_back();
    }
}

// @ 0x442ed0
bool Agent::can_target(unsigned aid, unsigned flags, float maxdist, float* out_dist)
{
    Agent* o = &g_mission->agents.pages[aid >> 4][aid & 0xf];
    if (o->f14 == 1
        && (!(flags & 1) || !rel_a(o))
        && (!(flags & 2) || !rel_b(o))
        && (!(flags & 4) || !rel_c(o))
        && (!(flags & 0x10) || o->f10 == 1)
        && (!(flags & 0x20) || o->f10 != 1)
        && (!(flags & 0x200) || o->id > id)
        && o->id != id
        && (!(flags & 0x2000) || line_of_sight(aid, 0))
        && o->id != f2c) {
        vector4 d = o->pos - pos;
        float len = d.normalize();
        *out_dist = len;
        if (maxdist >= 0.0f && len > maxdist)
            return false;
        if (flags & 0x1000) {
            float dot = d.x * fx + (d.z * fz + d.y * fy);
            if (dot < -0.1f)
                return false;
        }
        return true;
    }
    return false;
}

// @ 0x443020
bool Agent::any_hostile_near()
{
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned i = t->first_used();
    while (i < (unsigned)t->count) {
        Agent* o = &m->agents.pages[i >> 4][i & 0xf];
        if (o->f14 == 1) {
            bool friendly = false;
            if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
                friendly = m->relation[team * 0x5b + o->team] == 1;
            if (!friendly && !rel_c(o) && o->f10 != 1 && o->id != id && o->id != f2c) {
                vector4 d;
                d.x = o->pos.x - pos.x;
                d.y = o->pos.y - pos.y;
                d.z = o->pos.z - pos.z;
                float len = d.normalize();
                if (!(300.0f < len) && line_of_sight(i, 0))
                    return true;
                m = g_mission;
                t = &m->agents;
            }
        }
        t->next_used(&i);
    }
    return false;
}

// @ 0x443140
void Agent::update_alarm()
{
    if (f2c4 & 1)
        return;
    bool hit = false;
    AgentTable* t = &g_mission->agents;
    unsigned i = t->first_used();
    unsigned n = t->count;
    while (i < n) {
        if (hit)
            break;
        Agent** pg = g_mission->agents.pages._Myfirst;
        Agent* o = pg[i >> 4] + (i & 0xf);
        if (o->f14 == 1 && !rel_a(o) && !rel_c(o) && o->f10 != 1 && o->id != id && o->id != f2c) {
            vector4 d = o->pos - pos;
            if (!(10.0f < d.normalize()))
                hit = (pg[i >> 4][i & 0xf].f2c4 & 1) != 0;
        }
        g_mission->agents.next_used(&i);
    }
    if (hit || any_hostile_near()) {
        f2c4 |= 1;
        f2c0 = 0;
    }
}
