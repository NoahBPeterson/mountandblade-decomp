// Slice s004b6180: party committal to a mission -- detach a party (its members leave), the
// "commit party" wrapper, and the per-party "leave mission" virtual entry.
// Names are provisional; offsets come from the callees here and from s00431960 / s004428c0 / s00490a30.
#include <windows.h>
#include <stdlib.h>
#include <vector>

// ---- the 0x5ac8-byte world party record (see s00490a30 for the verified layout) -----------------------
struct Stack {                                  // 0x20 bytes, one troop stack in a party
    int count;                                  // +0
    int troop;                                  // +4
    int flags;                                  // +8 (bit 0: count is not a number)
    int f0c, f10, f14, f18, f1c;
};
struct PartyRec {                               // 0x5ac8-byte party record
    int f0;                                     // +0
    int type;                                   // +4 (== 1 when the party is in the mission)
    int f8;                                     // +8 (party unique id)
    char padc[0x88 - 0xc];
    int f88;                                    // +0x88 (== GameData::cur_party when active)
    int f8c;                                    // +0x8c (leader/stat index)
    char pad90[0xa0 - 0x90];
    int f_a0;                                   // +0xa0 (flags; 0x400 tested)
    char pada4[0xa8 - 0xa4];
    Stack stacks[64];                           // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    char f8b4;                                  // +0x8b4
    char pad8b5[0x8bc - 0x8b5];
    int* followers;                             // +0x8bc (vector<int>: member agent ids)
    int* followers_end;                         // +0x8c0
    int* followers_cap;                         // +0x8c4
    char pad8c8[0x8cc - 0x8c8];
    char in_mission;                            // +0x8cc
    char pad8cd[0x8d0 - 0x8cd];
    int anim;                                   // +0x8d0 (animation index)
    char pad8d4[0x8d8 - 0x8d4];
    __int64 anim_time;                          // +0x8d8 (animation start time, from g_ticks)
    float f8e0;                                 // +0x8e0 (speed)
    char pad8e4[0x918 - 0x8e4];
    int f918;                                   // +0x918
    char pad91c[0x5ac8 - 0x91c];
    int sum(int kind);                          // 0x4b1cf0 (thiscall, ret 4): troop count
};

// ---- the party manager (GameData+0xf8, pages at +0x138d0) -------------------------------------------
struct PartyMgr {
    char pad0[0x138d0];
    PartyRec** pages;                           // +0x138d0 (index >> 4 -> page of 16)
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                             // +0x138dc
    int index_of(unsigned id);                  // 0x48ad00 (thiscall, ret 4)
    int has(unsigned id);                       // 0x4903e0 (thiscall, ret 4)
};

// a page of the party manager's own 0x98-byte mission slots
struct PMPage { char b[0x98]; };

// ---- the party in the mission (the mission's copy of a party) ---------------------------------------
struct MissionObj { char pad0[8]; int id; };    // object released on leave (+8 = id)
struct MissionParty {
    int f0;                                     // +0
    int f4;                                     // +4
    int f8;                                     // +8 (party unique id of the corresponding record)
    char padc[0x10 - 0xc];
    int type;                                   // +0x10 (== 1 when the party is in the mission)
    char pad14[0x24 - 0x14];
    float fx24;                                 // +0x24 (position used when re-entering)
    float fy28;                                 // +0x28
    char pad2c[0x3c - 0x2c];
    float fz3c;                                 // +0x3c
    char pad40[0x50 - 0x40];
    MissionObj* f50;                            // +0x50 (object released on leave)
    int leader;                                 // +0x54 (leader party id)
    int f58;                                    // +0x58 (party id)
    char pad5c[0x78 - 0x5c];
    int f78;                                    // +0x78
};

// party statistics table at 0x5e29f8: 0xb0-byte records, visit counter at +0x3c
struct PartyStat { char pad0[0x3c]; int val; char pad40[0xb0 - 0x40]; };

struct CommitState {
    int begin(int flag);                        // 0x48add0 (thiscall, ret 4)
};

// ---- object manager that releases mission objects ----------------------------------------------------
struct MissionObjMgr {
    void release(int id);                       // 0x41f0d0 (thiscall, ret 4)
};

extern __int64 g_ticks[];                       // 0x5e2948: g_ticks[A] = start time of animation A
extern int g_cur_party_state;                   // 0xa73438 (0: player's party, 1: the other party)
extern PartyStat* g_party_stats;                // 0x5e29f8
extern std::vector<int> g_party_ids;            // the world's party-id list
extern MissionObjMgr* g_obj_mgr;                // 0xa732b4

// ---- the game data (0x5e2930) ------------------------------------------------------------------------
struct GameData {
    char pad0[0x78];
    int cur_party;                              // +0x78 (current party id)
    char pad7c[0xe4 - 0x7c];
    PMPage** pm_pages;                          // +0xe4 (mission slot page table, 0x98-byte slots)
    char padE8[0xf8 - 0xe8];
    PartyMgr parties;                           // +0xf8 (count lives at +0x139d4)
    char pad139d8[0x139f4 - 0x139d8];
    int f139f4;                                 // +0x139f4

    void list_members(int id, unsigned* out, unsigned* n);   // 0x490fa0 (thiscall, ret 0xc)
    void on_member_killed(int id);              // 0x490a30 (thiscall, ret 4)
    void kill_member(int id);                   // 0x4915d0 (thiscall, ret 4)
    void party_commit(int id);                  // 0x4b4700 (thiscall, ret 4)
    void commit_mission(const void* v, int id); // 0x4b5fe0 (thiscall, ret 8)
    void show_party_refresh(int which, int a, int b);   // 0x435790 (thiscall, ret 0xc)
};
extern GameData* g_game;                        // 0x5e2930

// ---- the mission-level commit state the game keeps around (0xa70448..) --------------------------------
extern __int64 g_commit_id;                     // 0xa70448
extern __int64 g_commit_leader;                 // 0xa70450
extern __int64 g_commit_ok;                     // 0xa70458

void __fastcall detach_party(MissionParty* self);     // 0x4b6180

// @ 0x4b6180
void __fastcall detach_party(MissionParty* self)
{
    unsigned list1[128], list2[128];
    int n1, n2;
    int total = 0;

    if (self->type != 1)
        goto tail;

    g_game->list_members(self->leader, list1, (unsigned*)&n1);
    g_game->list_members(self->f58, list2, (unsigned*)&n2);

    {
        PartyMgr* pm = &g_game->parties;
        int i, j, k, m;

        if (pm->has(self->leader)) {            i = pm->index_of(self->leader);
            PartyRec* q = pm->pages[i >> 4] + (i & 15);
            q->f918 = 0;
            q->f8e0 = 0.4f;
            q->anim_time = g_ticks[q->anim];
            q->f88 = -1;
            total = q->sum(0x10);

            for (k = (q->followers ? (int)(q->followers_end - q->followers) : 0) - 1; k >= 0; k--) {
                int mid = q->followers[k];
                j = pm->index_of(mid);
                if ((pm->pages[j >> 4] + (j & 15))->in_mission) {
                    g_game->on_member_killed(mid);
                    m = pm->index_of(q->followers[k]);
                    (pm->pages[m >> 4] + (m & 15))->f918 = 0;
                }
            }
        }

        {
            unsigned uid = self->f58;
            if (uid < g_game->parties.count) {
                j = pm->index_of(uid);
                if (j >= 0 && (pm->pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                    i = pm->index_of(self->f58);
                    PartyRec* q = pm->pages[i >> 4] + (i & 15);
                    q->f918 = 0;
                    q->f8e0 = 0.1f;
                    q->anim_time = g_ticks[q->anim];
                    q->f88 = -1;

                    m = 0;
                    if (q->nstacks > 0) {
                        for (int t = 0; t < q->nstacks; t++) {
                            int c = q->stacks[t].count;
                            if (((q->stacks[t].flags & 1) == 0) && c >= 0)
                                m += c;
                        }
                    }
                    total += m;

                    for (k = (q->followers ? (int)(q->followers_end - q->followers) : 0) - 1; k >= 0; k--) {
                        int mid = q->followers[k];
                        j = pm->index_of(mid);
                        if ((pm->pages[j >> 4] + (j & 15))->in_mission) {
                            g_game->on_member_killed(mid);
                            m = pm->index_of(q->followers[k]);
                            (pm->pages[m >> 4] + (m & 15))->f918 = 0;
                        }
                    }
                }
            }
        }

        if (total < self->f78) {
            float v[3];
            v[0] = self->fx24;
            v[1] = self->fy28;
            v[2] = self->fz3c;
            g_game->commit_mission(v, self->f78);
        }

        {
            int cur_id = g_game->cur_party;
            i = pm->index_of(cur_id);
            PartyRec* cur = pm->pages[i >> 4] + (i & 15);
            int st = g_cur_party_state;
            char is1, is2;

            if (self->leader == cur_id || (cur->f88 == self->f8 && st == 0))
                is1 = 1;
            else
                is1 = 0;
            if (self->f58 == cur_id || (cur->f88 == self->f8 && st == 1))
                is2 = 1;
            else
                is2 = 0;

            for (i = 0; i < n2; i++) {
                unsigned id = list2[i];
                if (id < g_game->parties.count) {
                    j = pm->index_of(id);
                    if (j >= 0 && (pm->pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                        j = pm->index_of(id);
                        PartyRec* p = pm->pages[j >> 4] + (j & 15);
                        if ((p->f_a0 & 0x400) == 0) {
                            m = 0;
                            for (int t = 0; t < p->nstacks; t++) {
                                int c = p->stacks[t].count;
                                if (c < 0)
                                    c = 0;
                                m += c;
                            }
                            if (m <= 0) {
                                if (is1 && p->f8c >= 0)
                                    g_party_stats[p->f8c].val++;
                                g_game->kill_member(list2[i]);
                            }
                        }
                    }
                }
            }

            for (i = 0; i < n1; i++) {
                unsigned id = list1[i];
                if (id < g_game->parties.count) {
                    j = pm->index_of(id);
                    if (j >= 0 && (pm->pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                        j = pm->index_of(id);
                        PartyRec* p = pm->pages[j >> 4] + (j & 15);
                        if ((p->f_a0 & 0x400) == 0) {
                            m = 0;
                            for (int t = 0; t < p->nstacks; t++) {
                                int c = p->stacks[t].count;
                                if (c < 0)
                                    c = 0;
                                m += c;
                            }
                            if (m <= 0) {
                                if (is2 && p->f8c >= 0)
                                    g_party_stats[p->f8c].val++;
                                g_game->kill_member(list1[i]);
                            }
                        }
                    }
                }
            }
        }
    }

tail:
    {
        MissionObj* obj = self->f50;
        g_obj_mgr->release(obj->id);
        self->f50 = 0;

        unsigned id = self->f8;
        char* page = (char*)g_game->pm_pages[id >> 4];
        *(int*)(page + (id & 15) * 0x98 + 4) = 0;
        g_party_ids.push_back(id);
    }
}

// @ 0x4b6870
void __fastcall commit_party(MissionParty* self)
{
    unsigned list1[128], list2[128];
    unsigned n1, n2;

    g_commit_id = self->f58;
    g_commit_leader = self->leader;
    g_commit_ok = 0;
    ((CommitState*)&g_game->f139f4)->begin(-5);
    if (g_commit_ok == 0)
        g_game->show_party_refresh(2, self->f58, self->leader);

    g_game->list_members(self->leader, list1, &n1);
    g_game->list_members(self->f58, list2, &n2);

    for (unsigned i = 0; i < n2; i++) {
        unsigned id = list2[i];
        if (id < g_game->parties.count) {
            int j = g_game->parties.index_of(id);
            if (j >= 0 && (g_game->parties.pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                g_game->party_commit(id);
                int m = g_game->parties.index_of(list2[i]);
                (g_game->parties.pages[m >> 4] + (m & 15))->f8b4 = 1;
            }
        }
    }

    for (unsigned i = 0; i < n1; i++) {
        unsigned id = list1[i];
        if (id < g_game->parties.count) {
            int j = g_game->parties.index_of(id);
            if (j >= 0 && (g_game->parties.pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                g_game->party_commit(id);
                int m = g_game->parties.index_of(list1[i]);
                (g_game->parties.pages[m >> 4] + (m & 15))->f8b4 = 1;
            }
        }
    }

    if (g_commit_ok != 0) {
        detach_party(self);
        g_game->show_party_refresh(0x1b, self->f58, self->leader);
    }
}

// the party record viewed through the object released by PartyObj::recover (leader at +0x8c8)
struct PartyObj {
    char pad0[8];
    int id;                                     // +8 (party id)
    char padc[0x88 - 0xc];
    int f88;                                    // +0x88
    char pad8c[0x8c8 - 0x8c];
    int leader;                                 // +0x8c8
    char in_mission;                            // +0x8cc
    char pad8cd[0x5ac8 - 0x8cd];

    void recover();                             // 0x4b6ae0
};

// @ 0x4b6ae0
void PartyObj::recover()
{
    if (leader < 0 || !in_mission) {
        if (f88 >= 0) {
            unsigned idx = (unsigned)f88;
            char* pg = (char*)g_game->pm_pages[idx >> 4] + (idx & 15) * 0x98;
            detach_party((MissionParty*)pg);
        }
    } else {
        g_game->on_member_killed(id);
    }
}
