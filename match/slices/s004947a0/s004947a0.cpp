// Slice s004947a0: party-quest-bookkeeping helpers of the game data (quest concluded / cancelled value,
// set-quest-objective, switch-objective, experience share) plus the inline logging / translation helpers
// they use. Names provisional.
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <math.h>

// ---- tstring (see s0042aaf0/tstring.h) -----------------------------------------------------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring() {}
    tstring(const char* s) throw();              // 0x42bbe0
    tstring(const tstring& o) throw();           // 0x42c110
    ~tstring() throw();                          // 0x42bc40
    void operator=(const tstring& o) throw();    // 0x42bc50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
struct StrTable {
    tstring* lookup(const tstring& key) throw(); // 0x43a1e0
};
extern StrTable g_strtable;                      // 0x5e2a18
void mb_snprintf(char* out, unsigned size, const char* fmt, ...);   // 0x415100

// message log object at 0x5d7080 (see s004333c0): enabled byte at +0x1714, 32-entry ring at +0xb0b8
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

// the ring-buffer append inlined by this TU
static __forceinline void log_add(const tstring& s)
{
    if (g_msglog.enabled && g_msglog.ring.count < 32) {
        int idx = (g_msglog.ring.head + g_msglog.ring.count) % 32;
        g_msglog.ring.names[idx] = s;
        g_msglog.ring.vals[idx] = 0xfff000;
        g_msglog.ring.count++;
    }
}

struct SoundMgr { int play(int idx, int flags, const float* pos, float freq, int* out_id); };   // 0x442650
extern SoundMgr g_sound;                         // 0x5aa128
extern float g_sound_pos[];                      // 0x5aa168
extern int g_quest_sound_concluded;              // 0x5aa18c
extern int g_quest_sound_cancelled;              // 0x5aa194

// ---- game data (object at 0x5e2930; see s00473a60) -----------------------------------------------------------------
struct Troop {                                   // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    unsigned flags;                              // +0x30 (0x10 = hero)
    char pad34[0x508 - 0x34];
};
// int vector that can call the protected _Insert_n directly (the original calls it); layout = std::vector<int>
typedef std::vector<int>::iterator VecIt;
typedef std::vector<int>::const_iterator VecCIt;
struct int_vector : std::vector<int> {
    void _Insert_n(iterator where, size_type count, const int& value) { std::vector<int>::_Insert_n(where, count, value); }
};
struct Stack {                                   // 0x20 bytes, array at party+0xa8
    int count;                                   // +0x00
    int troop;                                   // +0x04
    int flags;                                   // +0x08 (bit 0 = end-of-list marker)
    int f0c;                                     // +0x0c
    int f10;                                     // +0x10
    int f14;                                     // +0x14
    int f18;                                     // +0x18
    int f1c;                                     // +0x1c
};
struct Party {                                   // 0x5ac8 bytes
    char pad0[0x4];
    int state;                                   // +0x04 (1 = in mission)
    char pad8[0xa8 - 0x8];
    Stack stacks[64];                            // +0xa8
    int nstacks;                                 // +0x8a8
    char pad8ac[0x4];
    unsigned char f8b0;                          // +0x8b0
    char pad8b1[0x3];
    unsigned char f8b4;                          // +0x8b4 (objective list dirty)
    int_vector list;                             // +0x8b8 (std::vector<int>: begin, end, capacity)
    int f8c8;                                    // +0x8c8 (assigned objective, -1 = none)
    unsigned char f8cc;                          // +0x8cc
    char pad8cd[0x5ac8 - 0x8cd];
};
struct PartyMgr {                                // embedded in the game data at +0xf8
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0 (index >> 4 -> page of 16)
    int index_of(int id);                        // 0x48ad00 (thiscall, ret 4)
    Party* get_party(int id);                    // 0x51ee50 (thiscall, ret 4)
};
struct QuestRec {                                // 0x138 bytes, array at game+0x13a58
    char pad0[0x10];
    tstring title;                               // +0x10
    char pad20[0x40 - 0x20];
    tstring value;                               // +0x40
    char pad50[0x64 - 0x50];
    unsigned flags;                              // +0x64 (0x1000 = concluded)
    char pad68[0x138 - 0x68];
};
struct GameData {
    char pad0[0x78];
    int f78;                                     // +0x78 (default party)
    char pad7c[0xf8 - 0x7c];
    PartyMgr parties;                            // +0xf8 (pages pointer at +0x139c8)
    char pad139cc[0x139d4 - (0xf8 + sizeof(PartyMgr))];
    unsigned nquests;                            // +0x139d4
    char pad139d8[0x13a00 - 0x139d8];
    Troop* troops;                               // +0x13a00
    char pad13a04[0x13a0c - 0x13a04];
    int f13a0c;                                  // +0x13a0c
    int f13a10;                                  // +0x13a10
    char pad13a14[0x4];
    char f13a18v;                                // +0x13a18 (objectives enabled)
    char pad13a19[0x13a58 - 0x13a19];
    QuestRec* quests;                            // +0x13a58
    void set_quest_concluded(int quest, int value);  // 0x4947a0
    void set_quest_cancelled(int quest);         // 0x4948c0
    void set_objective(int party, int value);    // 0x4949d0
    int clear_objective(int party);              // 0x490a30 (ret 4)
    int add_party_exp(int party, int amount);    // 0x4428c0 (ret 8)
    int clear_exp_share(Party* p, int slot, int n);  // 0x4b1d80 (ret 8)
    void switch_objective(int party, int mode);  // 0x494ad0
    void share_experience(int party, float amount);  // 0x494b60
};

extern GameData* g_game;                         // 0x5e2930

// @ 0x4947a0
void GameData::set_quest_concluded(int quest, int value)
{
    if (quest < 0 || value < 0) return;
    char buf[512];
    QuestRec* q = &quests[quest];
    q->flags |= 0x1000;
    {
        tstring id(q->title);
        tstring key("ui_quest_concluded_value");
        mb_snprintf(buf, 0x200, g_strtable.lookup(key)->c_str(), id.c_str());
    }
    {
        tstring msg(buf);
        g_msglog.push(msg, 0xffd02028);
    }
    g_sound.play(g_quest_sound_concluded, 1, g_sound_pos, 1.0f, 0);
}

// @ 0x4948c0
void GameData::set_quest_cancelled(int quest)
{
    if (quest < 0) return;
    char buf[512];
    QuestRec* q = &quests[quest];
    q->flags = 0;
    {
        tstring id(q->title);
        tstring key("ui_quest_cancelled_value");
        mb_snprintf(buf, 0x200, g_strtable.lookup(key)->c_str(), id.c_str());
    }
    {
        tstring msg(buf);
        g_msglog.push(msg, 0xffd02028);
    }
    g_sound.play(g_quest_sound_cancelled, 1, g_sound_pos, 1.0f, 0);
}

static __forceinline Party* get_party(PartyMgr* pm, int id)
{
    int i = pm->index_of(id);
    return &pm->pages[i >> 4][i & 15];
}

// @ 0x4949d0
void GameData::set_objective(int party, int value)
{
    Party* pp = get_party(&parties, party);
    if (pp->f8c8 >= 0)
        clear_objective(party);
    Party* pp2 = get_party(&parties, party);
    pp2->f8c8 = value;
    Party* pv = get_party(&parties, value);
    pv->list.push_back(party);
    get_party(&parties, value)->f8b4 = 1;
}

// @ 0x494ad0
void GameData::switch_objective(int party, int mode)
{
    if (party == f13a0c)
        return;
    if (party == f13a10)
        return;
    if (party == f78)
        return;
    if (!f13a18v)
        return;
    if (parties.get_party(party)->f8c8 >= 0)
        clear_objective(party);
    if (mode == 0)
        set_objective(party, f78);
    else if (mode == 1)
        set_objective(party, f13a0c);
    else if (mode == 2)
        set_objective(party, f13a10);
    parties.get_party(party)->f8cc = 1;
}

// @ 0x494b60
void GameData::share_experience(int party, float amount)
{
    if (party >= nquests)
        return;
    PartyMgr* pm = &parties;
    int idx = pm->index_of(party);
    if (idx < 0)
        return;
    if (pm->pages[idx >> 4][idx & 15].state != 1)
        return;
    Party* p = get_party(pm, party);
    int total = 0;
    int i = 0;
    for (; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            break;
        int troop = s.troop;
        char hero = (troops[troop].flags & 0x10) != 0;
        if (hero)
            total = (int)((float)total + (i == 0 ? 10.0f : 3.0f));
        else
            total += s.count;
    }
    if (total > 0) {
        float per = amount / (float)total;
        for (int j = 0; j < p->nstacks; j++) {
            int idx2 = j;
            Stack& s = p->stacks[idx2];
            if (s.flags & 1)
                break;
            int troop = s.troop;
            char hero = (troops[troop].flags & 0x10) != 0;
            if (hero)
                add_party_exp(troop, (int)(per * (idx2 == 0 ? 10.0f : 3.0f)));
            else
                clear_exp_share(p, idx2, (int)((float)s.count * per));
        }
    }
    float fl = floorf(amount);
    int n = (int)fl;
    if (!(amount - fl <= 0.5f))
        n = (int)fl + 1;
    if (n != 0 && party == f78) {
        char buf[512];
        {
            tstring key("ui_shared_number_experience_within_party");
            sprintf(buf, g_strtable.lookup(key)->c_str(), n);
        }
        {
            tstring msg(buf);
            log_add(msg);
        }
    }
}
