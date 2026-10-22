// Slice s005181c0: the weekly party report and the map's party-tracking update of the game object
// at 0x5e2930 (g_game).
//   0x5181c0  GameData::weekly_report(bool)  - build the desertion/companions/prisoner/morale report,
//             store it in +0x13aa8 / +0x13ab8 and raise +0x13ac8.
//   0x518680  GameData::track_parties(float dt) - age the 0x5c-byte tracking records at +0x13a60, mark
//             the ones near the main party (0x20000000), then drop the ones whose timer has run out.
//   0x518940  GameData::skill_plus_8()       - party skill(0x10) + 8, as a float.
//   0x518990  Vec5c::shift_over(out, dst, src) - copy [src, _Mylast) down to dst, return dst in *out.
// Names and layouts provisional; the party manager / party / troop layouts follow the neighbouring
// campaign slices s0051be20 (same object) and s004b1b20 / s0048ca30 (the two shared iterator helpers).
// flags /O2 /MT /Gy /TP
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "rgl.h"

using rgl::vector4;

// ---- rgl string (16 bytes) ---------------------------------------------------------------------------
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    ~tstring();                                         // 0x42bc40
    tstring& operator=(const tstring& o);               // 0x42bc50
    void append(const tstring& o);                      // 0x42c1e0
    void push_back(char c);                             // 0x42be70
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* lookup(const tstring* key);                // 0x43a1e0 (never null)
    tstring* find(const tstring* key);                  // 0x4c6010 (null when absent)
};
extern StrTable g_strtable;                             // 0x5e2a18

// ---- party records -----------------------------------------------------------------------------------
struct Stack {                                          // 0x20 bytes, array at party+0xa8
    int count;                                          // +0x00
    int troop;                                          // +0x04
    int flags;                                          // +0x08
    int spawned;                                        // +0x0c
    int wounded;                                        // +0x10
    int f14, f18, f1c;
};

struct Party {                                          // 0x5ac8 bytes
    char pad0[0x30];
    vector4 pos;                                        // +0x30 (campaign map position)
    char pad40[0x7c - 0x40];
    float morale;                                       // +0x7c
    char pad80[0xa8 - 0x80];
    Stack stacks[64];                                   // +0xa8
    int nstacks;                                        // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
    int regular_limit();                                // 0x4b22d0
    int prisoner_limit();                               // 0x4b2330
    int skill(int n);                                   // 0x4b2370 (ret 4)
    int slot_consume(int idx, int amount, int extra, char merge);   // 0x48c3c0 (ret 0x10)
};

struct Troop {                                          // 0x508 bytes, table at game+0x13a00
    char pad0[0x20];
    tstring name;                                       // +0x20 (translated name)
    __int64 flags;                                      // +0x30 (0x10 = hero)
    char pad38[0x3b4 - 0x38];
    int f3b4;                                           // +0x3b4
    char pad3b8[0x508 - 0x3b8];
};

// one tracking record (0x5c bytes); +0x18 is a word/byte union
struct Rec5c {
    float x, y, z;                                      // +0x00
    char pad0c[0x14 - 0x0c];
    float f14;                                          // +0x14 (timer)
    union {
        unsigned int flags;                             // +0x18
        struct {
            unsigned short w18;                         // +0x18
            unsigned char f1a;                          // +0x1a
            unsigned char b1b;                          // +0x1b
        };
    };
    char pad1c[0x5c - 0x1c];
};

// the iterator helper: a bare base pointer, thiscall, out-param (0x4b22b0)
struct RecArray {
    Rec5c* base;
    Rec5c** at(Rec5c** out, int idx);                   // 0x4b22b0 (ret 8, returns out)
};

struct Vec5c {                                          // vector of Rec5c at game+0x13a60
    int pad0;                                           // +0x0
    Rec5c* _Myfirst;                                    // +0x4
    Rec5c* _Mylast;                                     // +0x8
    Rec5c* _Myend;                                      // +0xc
    unsigned size() const
    {
        if (!_Myfirst)
            return 0;
        return (unsigned)(_Mylast - _Myfirst);
    }
    __declspec(noinline) Rec5c** shift_over(Rec5c** out, Rec5c* dst, Rec5c* src);   // 0x518990 (ret 0xc)
};

struct PartyMgr {                                       // the manager at game+0xf8
    char pad0[0x138d0];
    Party** pages;                                      // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    int next_id;                                        // +0x138dc
    int index_of(int id);                               // 0x48ad00 (ret 4)
};

struct GameData {                                       // 0x5e2930
    char pad0[0x78];
    int main_party;                                     // +0x78
    char pad7c[0x8c - 0x7c];
    int f8c;                                            // +0x8c (troop index)
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                                   // +0xf8
    char pad139d8[0x13a00 - 0x139d8];
    Troop* troops;                                      // +0x13a00
    char pad13a04[0x13a60 - 0x13a04];
    Vec5c tracks;                                       // +0x13a60
    char pad13a70[0x13aa8 - 0x13a70];
    tstring report_text;                                // +0x13aa8
    tstring report_title;                               // +0x13ab8
    char report_pending;                                // +0x13ac8
    Party* get_party()
    {
        PartyMgr* mgr = &parties;
        int i = mgr->index_of(main_party);
        Party** pp = mgr->pages + (i >> 4);
        return *pp + (i & 0xf);
    }
    void f5181c0(bool);
    void f518680(float dt);
    __declspec(noinline) float skill_plus_8();          // 0x518940
    bool is_daytime();                                  // 0x435700
};
extern GameData* g_game;                                // 0x5e2930

extern float g_track_scale;                             // 0xa18118

// ---- the shared iterator helper on a bare base pointer ------------------------------------------------
static Rec5c* rec_at(Rec5c* base, int idx)
{
    RecArray a;
    a.base = base;
    Rec5c* r;
    return *a.at(&r, idx);
}

// @ 0x518990
Rec5c** Vec5c::shift_over(Rec5c** out, Rec5c* dst, Rec5c* src)
{
    if (dst != src) {
        Rec5c* end = _Mylast;
        Rec5c* p = dst;
        while (src != end) {
            *p = *src;
            ++p;
            ++src;
        }
        _Mylast = p;
    }
    *out = dst;
    return out;
}

// @ 0x518940
float GameData::skill_plus_8()
{
    int s = get_party()->skill(0x10);
    return (float)s + 8.0f;
}

// @ 0x518680
void GameData::f518680(float dt)
{
    vector4 pos = get_party()->pos;
    int skill = get_party()->skill(0x10);
    float radius = ((float)skill * 0.125f + 1.0f) * g_track_scale;
    if (!is_daytime())
        radius *= 0.3f;
    float threshold = radius * radius;
    if (get_party()->skill(0x10) == 0)
        threshold = -0.1f;

    const int SEEN = 0x20000000;
    Rec5c* rec = tracks._Myfirst;
    while (rec < tracks._Mylast) {
        rec->f14 += dt;
        if ((rec->flags & SEEN) == 0) {
            float dx = rec->x - pos.x;
            float dy = rec->y - pos.y;
            float dz = rec->z - pos.z;
            if (dx * dx + dz * dz + dy * dy < threshold)
                rec->flags |= SEEN;
        }
        rec++;
    }

    float sf = skill_plus_8();
    int last = tracks.size() - 1;
    int i = 0;
    while (i <= last) {
        Rec5c* cur = rec_at(tracks._Myfirst, i);
        unsigned char b = cur->f1a;
        float t = (float)((b & 0xf) + 0x10) * 0.03125f;
        t *= sf;
        if (rec_at(tracks._Myfirst, i)->f14 > t) {
            *rec_at(tracks._Myfirst, i) = *rec_at(tracks._Myfirst, last);
            last--;
        } else {
            i++;
        }
    }
    if (last < tracks.size() - 1) {
        Rec5c* dummy;
        tracks.shift_over(&dummy, tracks._Myfirst + (last + 1), tracks._Mylast);
    }
}

// @ 0x5181c0
void GameData::f5181c0(bool)
{
    Party* p = get_party();
    float factor1 = -0.01f;
    float factor2;
    Troop* troop = &troops[f8c];
    tstring body;

    int total = 0;
    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        total += p->stacks[i].count;
    }
    int limit = p->regular_limit();
    if (total > limit) {
        float f = (1.0f - (float)limit / (float)total) * 0.5f;
        if (f > -0.01f)
            factor1 = f;
        tstring key("ui_number_of_companions_exceeds_leadership_limit");
        body.append(*g_strtable.lookup(&key));
    }

    total = 0;
    for (int i = 0; i < p->nstacks; i++)
        if (p->stacks[i].flags & 1)
            total += p->stacks[i].count;
    limit = p->prisoner_limit();
    factor2 = -0.01f;
    if (total > limit) {
        factor2 = (1.0f - (float)limit / (float)total) * 0.5f;
        tstring key("ui_number_of_prisoners_exceeds_prisoner_management_limit");
        body.append(*g_strtable.lookup(&key));
    }

    if (p->morale < 0.2f) {
        tstring key("ui_party_morale_is_low");
        body.append(*g_strtable.lookup(&key));
        float f = 0.3f - p->morale;
        if (f < factor1)
            f = factor1;
        factor1 = f * 0.2f;
    }

    if (troop->f3b4 < 0)
        troop->f3b4 = 0;

    int counts[32];
    int ids[32];
    int nsel = 0;
    int i = p->nstacks - 1;
    Stack* s = &p->stacks[i];
    for (; i > 0; i--, s--) {
        int hits = 0;
        float thr = factor1;
        if (s->flags & 1)
            thr = factor2;
        if ((troops[s->troop].flags & 0x10) == 0) {
            counts[nsel] = 0;
            for (int k = 0; k < s->count; k++) {
                if ((float)(rand() % 0x3dc9) * 6.3223117e-5f < thr) {
                    counts[nsel]++;
                    hits++;
                }
            }
            if (counts[nsel] != 0) {
                ids[nsel] = s->troop;
                nsel++;
            }
            if (hits != 0)
                p->slot_consume(i, hits, (int)((char*)this + 0x1405c), 0);
        }
    }

    if (nsel != 0) {
        int before_last = nsel - 2;
        for (int j = 0; j < nsel; j++) {
            body.push_back(' ');
            Troop* tr = &troops[ids[j]];
            if ((tr->flags & 0x10) != 0) {
                tstring* name = g_strtable.find(&tr->name);
                if (name == 0)
                    name = (tstring*)tr;
                body.append(*name);
            } else {
                tstring* name = g_strtable.find(&tr->name);
                if (name == 0)
                    name = (tstring*)tr;
                char buf[4092];
                sprintf(buf, "%d %s ", counts[j], name->c_str());
                tstring part(buf);
                body.append(part);
            }
            if (j < before_last)
                body.push_back(',');
            else if (j == before_last) {
                tstring key("ui_and_one_space");
                body.append(*g_strtable.lookup(&key));
            }
        }
        {
            tstring key(nsel == 1 && counts[0] == 1 ? "ui_has_deserted_the_party"
                                                    : "ui_have_deserted_the_party");
            body.append(*g_strtable.lookup(&key));
        }
        {
            tstring key("ui_weekly_report");
            report_title = *g_strtable.lookup(&key);
            report_text = body;
            report_pending = 1;
        }
    }
    // body's destructor
}
