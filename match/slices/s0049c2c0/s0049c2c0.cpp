// Slice s0049c2c0: Mission::order (0x49c2c0, ~2.4 KB, `ret 8`). One `case` per army order: each of
// the three order slots belonging to the selected order leader is armed with a per-order state,
// the order's localised caption is fetched from the string table, and (only for the local agent's
// own order group) the caption plus the leader's order text is pushed to the message log. Finally
// every agent in the same order group gets its three order-parameter fields re-seeded from the
// global order table. `index` selects a leader (0..2), `order` the order (0..0xd).
// Names, and the two frame slots the compiler reads before writing on the advance/fall-back paths,
// are provisional.
#include <windows.h>
#include <string.h>

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() MB_NOTHROW;
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
};
struct StrTable { tstring* lookup(const tstring& key); };   // 0x5e2a18
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };        // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

struct vector4 { float x, y, z, w; };

// an order slot inside a leader record (0x6c bytes)
struct SubOrder {
    int     state;          // +0x00  armed flag (written by the hold/follow/charge/advance orders)
    char    pad04[0xc];
    vector4 target;         // +0x10  move target
    vector4 extra;          // +0x20  (zeroed, w = order group)
    int     face;           // +0x30  nearest face
    float   ox, oy, oz;     // +0x34  per-slot offset
    int     mode;           // +0x44  sub-order mode (0 = unused)
    char    pad48[0x18];
    int     field5c;        // +0x5c
};

struct Leader {             // this + 0x527c + index*0x16c
    int      pad0;
    int      agent_id;      // +4
    SubOrder sub[3];        // +8, stride 0x6c
    char     pad14c[0x20];
};

struct AgentTable {         // mission + 4
    unsigned count;                         // +0
    unsigned first_used();                  // 0x43b1b0
    void     next_used(unsigned* id);       // 0x43b160
};

struct Agent {
    char    pad0[0x50];
    vector4 pos;            // +0x50
    char    pad60[0x1c4 - 0x60];
    int     slot_no;        // +0x1c4  order group (clamped to 7)
    char    pad1c8[0x2b0 - 0x1c8];
    int     unit;           // +0x2b0  unit/order-table index
    int     form_a;         // +0x2b8
    int     form_b;         // +0x2bc
    float   form_delay;     // +0x2c0
    char    pad2c4[0x56b4 - 0x2c4];
    vector4 alt;            // +0x56b4
    char    pad56c4[0x5928 - 0x56c4];
    int     nav_face();     // 0x43cb30 (thiscall) -> eax
};

struct Level { int nearest_face(vector4* p); };     // 0x439a50 (thiscall) -> face index

struct Mission {
    Level*      level;                      // +0x00
    AgentTable  agents;                     // +4
    char        pad8[0x1c - 8];
    void**      agent_pages;                // +0x1c  page table, dir[id >> 4] then + (id&0xf)*0x5928
    char        pad20[0x5c - 0x20];
    int         local_id;                   // +0x5c  local agent id (-1 = none)
    char        pad60[0x527c - 0x60];
    Leader      leaders[3];                 // +0x527c
    char        pad6c20[0x40];

    int  bounds_test(vector4* p);           // 0x48a990 (thiscall) -> al
    void set_order(int index, int on);      // 0x492230 (ret 8)
    void set_fire(int index, int mode);     // 0x492350 (ret 8)
    void order(int index, int order);
};
extern Mission* g_mission;                  // 0x5e292c

// order-leader helpers (all thiscall on the Leader record)
void leader_set_targets(Leader* L, vector4* pos, vector4* a, vector4* b);   // 0x4898e0
void leader_set_mode(Leader* L, int v);                                     // 0x489860
void leader_set_mode2(Leader* L, int v);                                    // 0x4899e0
void order_text(Leader* L, char* buf);                                      // 0x48d940

struct TickPair { int a; int b; };
extern TickPair g_ticks[];                  // 0x5e2948
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f
extern "C" int __cdecl rand(void);                         // 0x536a9f

static Agent* agent_by_id(Mission* m, int id) {
    void** dir = m->agent_pages;
    return (Agent*)((char*)dir[id >> 4] + (id & 0xf) * 0x5928);
}

// look a localisation key up in the mission string table; the held buffer, or "" when absent
static const char* order_string(const char* key) {
    tstring tmp(key);
    tstring* r = g_strtable.lookup(tmp);
    return r->buf ? r->buf : (const char*)&r->buf;
}

// @ 0x49c2c0
void Mission::order(int index, int order) {
    char ordstr[0x200];                 // this+0x38: the localised order caption
    char ordtxt[0x200];                 // this+0x238: the leader's order text
    char msg[0x200];                    // this+0x438: the composed log line
    Leader* L = &leaders[index];

    ordstr[0] = 0;

    switch (order) {
    case 0: {                                   // hold this position
        if (L->agent_id == -1)
            return;
        Agent* a = agent_by_id(this, L->agent_id);
        vector4 zero = { 0, 0, 0, 0 };
        int f = a->nav_face();
        (void)f;
        leader_set_targets(L, &a->pos, &zero, &zero);
        if (L->sub[0].mode) L->sub[0].state = 1;
        if (L->sub[1].mode) L->sub[1].state = 1;
        if (L->sub[2].mode) L->sub[2].state = 1;
        strcpy(ordstr, order_string("ui_hold_this_position_e_"));
        break;
    }
    case 1: {                                   // follow me
        if (L->agent_id == -1)
            return;
        if (L->sub[0].mode) L->sub[0].state = 2;
        if (L->sub[1].mode) L->sub[1].state = 2;
        if (L->sub[2].mode) L->sub[2].state = 2;
        Agent* a = agent_by_id(this, L->agent_id);
        vector4 zero = { 0, 0, 0, 0 };
        int f = a->nav_face();
        (void)f;
        leader_set_targets(L, &a->pos, &a->alt, &zero);
        strcpy(ordstr, order_string("ui_follow_me_e_"));
        break;
    }
    case 2: {                                   // charge
        if (L->sub[0].mode) L->sub[0].state = 0;
        if (L->sub[1].mode) L->sub[1].state = 0;
        if (L->sub[2].mode) L->sub[2].state = 0;
        strcpy(ordstr, order_string("ui_charge_e_"));
        break;
    }
    case 11: {                                  // stand ground
        if (L->sub[0].mode) L->sub[0].state = 3;
        if (L->sub[1].mode) L->sub[1].state = 3;
        if (L->sub[2].mode) L->sub[2].state = 3;
        strcpy(ordstr, order_string("ui_stand_ground_e_"));
        break;
    }
    case 3: {                                   // mount horses
        leader_set_mode(L, 1);
        strcpy(ordstr, order_string("ui_mount_horses_e_"));
        break;
    }
    case 4: {                                   // dismount
        leader_set_mode(L, 2);
        strcpy(ordstr, order_string("ui_dismount_e_"));
        break;
    }
    case 7: {                                   // stand closer
        leader_set_mode2(L, -1);
        strcpy(ordstr, order_string("ui_stand_closer_e_"));
        break;
    }
    case 8: {                                   // spread out
        leader_set_mode2(L, 1);
        strcpy(ordstr, order_string("ui_spread_out_e_"));
        break;
    }
    case 5: {                                   // advance
        for (int k = 0; k < 3; ++k) {
            SubOrder* s = &L->sub[k];
            if (s->mode == 0)
                continue;
            if (s->state == 0)
                continue;
            vector4 p;
            p.x = s->ox * 7.0f + s->target.x;
            p.y = s->oy * 7.0f + s->target.y;
            p.z = s->oz * 7.0f + s->target.z;
            p.w = 0.0f;
            if (!bounds_test(&p))
                continue;
            s->target = p;
            s->target.w = 0.0f;
            s->face = level->nearest_face(&p);
            s->extra.x = 0.0f;
            s->extra.y = 0.0f;
            s->extra.z = 0.0f;
            s->extra.w = 0.0f;
            s->state = 1;
        }
        strcpy(ordstr, order_string("ui_advance_e_"));
        break;
    }
    case 6: {                                   // fall back
        for (int k = 0; k < 3; ++k) {
            SubOrder* s = &L->sub[k];
            if (s->mode == 0)
                continue;
            if (s->state == 0) {
                s->target.x = s->extra.x;
                s->target.y = s->extra.y;
                s->target.z = s->extra.z;
                s->target.w = s->extra.w;
            }
            vector4 p;
            p.x = s->target.x - s->ox * 7.0f;
            p.y = s->target.y - s->oy * 7.0f;
            p.z = s->target.z - s->oz * 7.0f;
            p.w = 0.0f;
            if (!bounds_test(&p))
                continue;
            s->target = p;
            s->target.w = 0.0f;
            s->face = level->nearest_face(&p);
            s->extra.x = 0.0f;
            s->extra.y = 0.0f;
            s->extra.z = 0.0f;
            s->extra.w = 0.0f;
            s->state = 1;
        }
        strcpy(ordstr, order_string("ui_fall_back_e_"));
        break;
    }
    case 9: {                                   // use blunt weapons
        set_order(index, 1);
        strcpy(ordstr, order_string("ui_use_blunt_weapons_e_"));
        break;
    }
    case 10: {                                  // use any weapon
        set_order(index, 0);
        strcpy(ordstr, order_string("ui_use_any_weapon_e_"));
        break;
    }
    case 12: {                                  // hold fire
        set_fire(index, 1);
        strcpy(ordstr, order_string("ui_hold_fire_e_"));
        break;
    }
    case 13: {                                  // fire at will
        set_fire(index, 0);
        strcpy(ordstr, order_string("ui_fire_at_will_e_"));
        break;
    }
    default:
        return;
    }

    // inform the local agent, but only about its own order group
    if (local_id >= 0) {
        Agent* la = agent_by_id(this, local_id);
        int g = la->slot_no;
        if (g < 0 || g >= 8)
            g = 7;
        if (g == index) {
            order_text(L, ordtxt);
            sprintf(msg, ordstr, ordtxt);
            g_msglog.add(tstring(msg), 0xfffffefb);
        }
    }

    // re-seed the order parameters of every agent in this order group
    AgentTable* tbl = &agents;
    unsigned id = tbl->first_used();
    while (id < tbl->count) {
        Agent* a = agent_by_id(this, id);
        int g = a->slot_no;
        if (g < 0 || g >= 8)
            g = 7;
        if (g == index) {
            a->form_a = g_ticks[a->unit].a;
            a->form_b = g_ticks[a->unit].b;
            a->form_delay = (float)(rand() % 0x3dc9) * 6.3223117e-05f + 1.0f;
        }
        tbl->next_used(&id);
    }
}
