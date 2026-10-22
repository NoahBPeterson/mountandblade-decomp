// Slice s004b3420: the party map-order helpers of the world map.
//   0x4b3420  WorldMap::can_move (walk the formation-record graph from `from` to `to` and test every record
//             edge for a crossing of the move segment, honouring the order-target mask and keep distance)
//   0x4b36a0  WorldMap::fixup (place a party order record at its target agent and refit its frame)
//   0x4b38f0  WorldMap::dismiss (move order records out of the range [first, last) -> shifts the table)
//   0x4b3910  Party::collect_targets (scan the parties that may receive the current order and emit the order
//             point event for the main party when it arrives nearby)
//   0x4b3bc0  Party::reissue_order (re-send the current map order of a party)
// Names provisional; layouts from the disassembly and the neighbouring slices s00457b40 (Party/GameData),
// s004608d0 (PartyMgr pages/first_party_id/next_party_id) and s004b1160 (0x4b1780).
#include <math.h>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- map records ---------------------------------------------------------------------------------------
struct NavVert {                                    // 0x24 bytes
    float x, y, z, w, a, b, c, d, e;
};

struct BigRec {                                     // 0x88-byte formation record of the world map
    int count;                                      // +0x00
    int* verts;                                     // +0x04 (mesh vertex indices)
    char pad8[0x14 - 8];
    int* agents;                                    // +0x14
    char pad18[0x24 - 0x18];
    unsigned mask;                                  // +0x24
    int* targets;                                   // +0x28
    char pad2c[0x38 - 0x2c];
    float* dists;                                   // +0x38 (keep distance per agent)
    char pad3c[0x54 - 0x3c];
    vector4 u;                                      // +0x54
    char pad64[0x88 - 0x64];
};

struct Rec {                                        // 0x5c-byte party order record
    vector4 target;                                 // +0x00
    float angle;                                    // +0x10
    float f14;                                      // +0x14
    unsigned scale;                                 // +0x18
    matrix pos;                                     // +0x1c
};

struct WorldAgent {                                 // 0x48ea80 result (the finer map agent)
    char pad0[4];
    int m4;                                         // +0x04 (formation record index)
    char pad8[0x20 - 8];
    float x;                                        // +0x20
    float y;                                        // +0x24
    float z;                                        // +0x28
    char pad2c[0xa0 - 0x2c];
    float f_a0;                                     // +0xa0
    float f_a4;                                     // +0xa4
    float f_a8;                                     // +0xa8
};

struct MapAgent {                                   // 0x98-byte agent-table page entry (0x4b39b5 / 0x4b3bfa)
    char pad0[0x2c];
    vector4 pos;                                    // +0x2c
    char pad3c[0x4c - 0x3c];
    int f4c;                                        // +0x4c
    char pad50[0x58 - 0x50];
    int party;                                      // +0x58
    char pad5c[0x98 - 0x5c];
};

struct MapGlobal {                                  // object at 0xa732b4
    char pad0[0xb0];
    struct WorldMap* world;                         // +0xb0
};
extern MapGlobal* g_map;
extern matrix* identity_pos(matrix* out);           // 0x40cd00 (cdecl, returns out)
extern void move_recs(Rec* dst, Rec* first, Rec* last, void* owner, Rec* extra);  // 0x4b2670

struct WorldMap {
    char pad0[4];
    NavVert* verts;                                 // +0x04
    char pad8[0xc];
    BigRec* recs;                                   // +0x14
    bool linked(const int* from, const int* to);    // 0x48efa0
    WorldAgent* find_agent(const float* p);         // 0x48ea80
    bool can_move(int from, float x, float y, int to, const float* p, unsigned flags, int max);
    void fixup(Rec* r);                             // 0x4b36a0
    void dismiss(Rec* dst, Rec* first, Rec* last);  // 0x4b38f0
};

// ---- parties -------------------------------------------------------------------------------------------
struct PartyMgr;
struct GameData;
struct Party {
    char pad0[4];
    int type;                                       // +0x04
    int id;                                         // +0x08
    char padc[0x30 - 0xc];
    vector4 pos;                                    // +0x30
    char pad40[0x88 - 0x40];
    int agent;                                      // +0x88
    char pad8c[0xa0 - 0x8c];
    unsigned flags_a0;                              // +0xa0
    int f_a4;                                       // +0xa4
    char pada8[0x8c8 - 0xa8];
    int f8c8;                                       // +0x8c8
    char pad8cc[0x918 - 0x8cc];
    int order;                                      // +0x918
    int order_target;                               // +0x91c
    int f920;                                       // +0x920
    vector4 order_pos;                              // +0x924
    int order_state;                                // +0x934
    char pad938[0x5974 - 0x938];
    int f5974;                                      // +0x5974
    unsigned group;                                 // +0x5978
    char pad597c[0x5ac8 - 0x597c];

    int skill(int s);                               // 0x4b2370
    float strength();                               // 0x4b1fa0
    void set_order(int* pos, int* b);               // 0x4b1780
    void set_order_agent(vector4* p);               // 0x4b2970
    void set_order_party(int* t);                   // 0x4b29d0
    void reissue_order();                           // 0x4b3bc0
    void collect_targets(int* out, int* max, int* count);   // 0x4b3910
};

struct AgentTable {                                 // at g_game + 0xcc
    char pad0[0x18];
    MapAgent** pages;                               // +0x18 (== g_game + 0xe4)
    char pad1c[0x2c - 0x1c];
    MapAgent* get(unsigned id);                     // 0x48ef20
};

struct PartyMgr {                                   // embedded in GameData at +0xf8
    char pad0[0x138b8];
    int count;                                      // +0x138b8
    char pad138bc[0x138d0 - 0x138bc];
    Party** pages;                                  // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    int deflt;                                      // +0x138dc
    int index_of(int id);                           // 0x48ad00
    Party* get(int id);                             // 0x51ee50
    int first_party_id();                           // 0x460d20
    void next_party_id(unsigned* id);               // 0x460ca0
    int arrived(int id);                            // 0x4903e0
};

struct GameData {
    char pad0[0x78];
    int main_party;                                 // +0x78
    char pad7c[0xcc - 0x7c];
    AgentTable atbl;                                // +0xcc
    PartyMgr parties;                               // +0xf8
    char pad139d8[0x13ad0 - 0x139d8];
    float f13ad0;                                   // +0x13ad0
    int f13ad4;                                     // +0x13ad4
    bool is_night();                                // 0x435740
    int emit_event(int evt, int a, int b);          // 0x435790
};
extern GameData* g_game;

extern float g_f_a18114;                            // 0xa18114
extern int g_ev_a;                                  // 0xa70458
extern int g_ev_b;                                  // 0xa7045c

static __forceinline PartyMgr* parties() { return (PartyMgr*)((char*)g_game + 0xf8); }

// ---- helpers -------------------------------------------------------------------------------------------
static __forceinline vector4 crossv(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// @ 0x4b3420
bool WorldMap::can_move(int from, float x, float y, int to, const float* p, unsigned flags, int max)
{
    if (!linked(&from, &to))
        return false;
    int cur = -1;
    if (from < 0 || to < 0)
        return cur == to;
    float dx = p[0] - x;
    float dy = p[1] - y;
    if (dx * dx + dy * dy < 0.0001f)
        return true;
    bool vert = true;
    if (-0.000001f < dx && dx < 0.000001f)
        vert = false;
    int hops = 0;
    int prev = -1;
    bool stop = false;
    for (;;) {
        cur = from;
        if (cur == to || cur < 0 || stop || hops >= max)
            return cur == to;
        hops++;
        int hit = -1;
        BigRec* r = &recs[cur];
        int i = 0;
        do {
            if (r->count <= i)
                break;
            if (r->agents[i] != prev) {
                NavVert* a = &verts[r->verts[i]];
                NavVert* b = &verts[r->verts[(i + 1) % r->count]];
                float ex = b->x - a->x;
                float ey = b->y - a->y;
                float t = ((x - a->x) * dy - (y - a->y) * dx) / (ex * dy - ey * dx);
                float s;
                if (vert)
                    s = ((ex * t + a->x) - x) / dx;
                else
                    s = ((ey * t + a->y) - y) / dy;
                if (0.01f <= t && t <= 0.99f && 0.0f <= s && s <= 1.0f)
                    hit = i;
            }
            i++;
        } while (hit == -1);
        if (hit < 0) {
            stop = true;
            continue;
        }
        from = r->targets[hit];
        prev = r->agents[hit];
        if (from < 0 || 100.0f < r->dists[hit] || (recs[from].mask & flags) != flags) {
            stop = true;
            from = cur;
        }
    }
}

// @ 0x4b36a0
void WorldMap::fixup(Rec* r)
{
    vector4 t = r->target;
    float angle = r->angle;
    matrix m;
    r->pos = *identity_pos(&m);
    r->pos.o = t;
    WorldAgent* e = find_agent(&t.x);
    if (e) {
        r->pos.o.z = e->z - ((t.x - e->x) * e->f_a0 + (t.y - e->y) * e->f_a4) / e->f_a8 + 0.01f;
        r->pos.u = *(vector4*)((char*)g_map->world->recs + e->m4 * 0x88 + 0x54);
        float c = cosf(angle);
        float s = sinf(angle);
        float fx = r->pos.f.x, fy = r->pos.f.y;
        r->pos.f.x = c * fx - s * fy;
        r->pos.f.y = s * fx + c * fy;
        r->pos.s = crossv(r->pos.f, r->pos.u);
        r->pos.s.normalize();
        r->pos.f = crossv(r->pos.u, r->pos.s);
        r->pos.f.normalize();
    }
    float k = (float)(r->scale & 0xff) * 0.01f + 0.5f;
    if (k > 2.5f)
        k = 2.5f;
    r->pos.s.x *= k; r->pos.s.y *= k; r->pos.s.z *= k;
    r->pos.f.x *= k; r->pos.f.y *= k; r->pos.f.z *= k;
    r->pos.u.x *= k; r->pos.u.y *= k; r->pos.u.z *= k;
}

// @ 0x4b38f0
void WorldMap::dismiss(Rec* dst, Rec* first, Rec* last)
{
    move_recs(dst, first, last, this, last);
}

// @ 0x4b3910
void Party::collect_targets(int* out, int* max, int* count)
{
    *count = 0;
    float radius = ((float)skill(0xd) * 0.1f + 1.0f) * g_f_a18114;
    if (g_game->is_night())
        radius = radius * 0.5f;
    unsigned id = (unsigned)parties()->first_party_id();
    while (id < (unsigned)parties()->deflt) {
        if (*count >= *max)
            return;
        int idx = parties()->index_of((int)id);
        Party* p = &parties()->pages[idx >> 4][idx & 0xf];
        if (p->f8c8 >= 0)
            goto next;
        unsigned f = p->flags_a0;
        if ((f & 0x1080100) != 0)
            goto next;
        if (id == (unsigned)this->id)
            goto next;
        {
            unsigned g1 = p->group & 7;
            char a = ((g1 >= 2 && g1 < 7) || p->group == 7);
            unsigned g2 = this->group & 7;
            char b = ((g2 >= 2 && g2 < 7) || this->group == 7);
            if (a != b)
                goto next;
        }
        if (p->id == g_game->main_party && 0.0f < g_game->f13ad0 && g_game->f13ad4 != 0)
            goto next;
        if ((f & 0x400) != 0)
            goto next;
        {
            int ag = p->agent;
            if (ag >= 0 && parties()->arrived(g_game->atbl.pages[ag >> 4][ag & 0xf].party)) {
                if (parties()->get(g_game->atbl.get((unsigned)ag)->party)->flags_a0 & 0x400)
                    goto next;
            }
        }
        if (p->order == 8)
            goto next;
        {
            vector4 d = p->pos - this->pos;
            float len = d.out_length();
            if (!(len < radius))
                goto next;
            if (len >= strength() * radius)
                goto next;
        }
        g_ev_a = 1;
        g_ev_b = 0;
        g_game->emit_event(0x19, this->id, (int)id);
        if ((g_ev_a | g_ev_b) != 0)
            out[(*count)++] = (int)id;
    next:
        parties()->next_party_id(&id);
    }
}

// @ 0x4b3bc0
void Party::reissue_order()
{
    if (order == 9) {
        set_order((int*)&g_game->atbl.pages[order_target >> 4][order_target & 0xf].pos,
                  (int*)g_game->atbl.pages[order_target >> 4][order_target & 0xf].f4c);
        order_state = 0;
    } else if (order == 6) {
        set_order_agent(&order_pos);
        order_state = 0;
    } else if (order == 4 || order == 1) {
        if (order_target >= 0 && parties()->arrived(order_target)) {
            if (parties()->get(order_target)->flags_a0 & 0x400)
                set_order((int*)&parties()->get(order_target)->pos,
                          (int*)parties()->get(order_target)->f5974);
            else
                set_order_party(&order_target);
        }
        order_state = 0;
    }
}
