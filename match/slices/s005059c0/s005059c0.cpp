// Slice s005059c0: the party window's skills page (0x5059c0): the skills panel with its caption, one
// caption line per skill-holding member, one line per skill (with the best member's value), the scroll
// list that shows them, and the party-skills information bar.
// Names and layouts are provisional; every offset comes from this slice's disassembly.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---------------------------------------------------------------------------------------------------------
// tstring (layout from slice s004bd170).
struct tstring {
    int pos;                                            // +0
    char* buf;                                          // +4
    int cap;                                            // +8
    int len;                                            // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const char* s, int n) MB_NOTHROW;           // 0x42bc60
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    void capitalize();                                  // 0x42c060
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct ini_entry {
    tstring key;                                        // +0
    char    pad10[0xc];                                 // +0x10
    tstring value;                                      // +0x1c
};

extern "C" double __cdecl _atof(const char*);           // 0x537352

struct ini_file {
    ini_entry entries[0x1000];                          // +0
    int       count;                                    // +0x2c000
    int   find(const tstring& key);                     // 0x4bd5a0
    // 0x4bd5e0, inlined at every use site below exactly as the original does it
    float get_float(const tstring& key)
    {
        float ret = 0.0f;
        int idx = find(key);
        if (idx >= 0)
            ret = (float)_atof(entries[idx].value.c_str());
        return ret;
    }
};

ini_file g_ini;                                         // 0xa44240
extern const char g_empty_string[];                     // 0x54a480
struct string_table { char pad0[0x10]; };
string_table g_strings;                                 // 0x5e2a18
tstring* string_table_lookup(string_table* t, const tstring* key);  // 0x43a1e0
tstring* translate_text(string_table* t, const tstring* key);       // 0x4c6010

// ---------------------------------------------------------------------------------------------------------
// rgl resource (ctor 0x48ae20): rect at +0xe0, size at +0xf0, flags at +0x148.
struct resource {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54(); virtual void v58(const float* a); virtual void v5c();
    virtual void v60(); virtual void v64(const float* ab); virtual void v68();
    virtual void v6c(const float* ab); virtual void v70(const float* abc); virtual void v74(const int* v);
    virtual void v78(); virtual void v7c(void* p); virtual void v80(int n, void* p);
    virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90();
    virtual void v94(); virtual void v98(const int* v); virtual void v9c(); virtual void va0();
    virtual void va4(const float* pos, const tstring* text);            // set_text(pos, text)
    virtual void va8(); virtual void vac(const tstring* text, const float* pos);
    virtual void vb0(); virtual void vb4(); virtual void vb8(); virtual void vbc();
    virtual void vc0(); virtual void vc4();
    virtual void vc8(resource* o);                                      // add_child(o)
    char  pad04[0xe0 - 4];
    float x;                                            // +0xe0
    float y;                                            // +0xe4
    char  pad_e8[0xf0 - 0xe8];
    float w;                                            // +0xf0
    float h;                                            // +0xf4
    char  pad_f8[0x148 - 0xf8];
    unsigned flags;                                     // +0x148
    char  pad14c[0x178 - 0x14c];
    resource() MB_NOTHROW;                              // 0x48ae20
};

// The 0x188-byte frame widget: its ctor (0x4931a0) is the one the `new` sites call out of line, and it
// zeroes +0x17c/+0x180/+0x184 after installing vptr 0x5924a0 itself.  A derived class whose ctor is
// implicit then has its (derived) vptr stored by the caller after the base call.
struct frame : resource {
    int f178;                                           // +0x178
    int f17c;                                           // +0x17c
    int f180;                                           // +0x180
    unsigned f184;                                      // +0x184
    frame() MB_NOTHROW;                                 // 0x4931a0
};
struct list_widget : frame {                            // vptr 0x597050 (stored by the caller)
};

// A 0x4a8-byte scroll list (ctor 0x49deb0).
struct scroll_list : resource {
    char pad_f8[0x104 - 0xf8];
    unsigned f104;                                      // +0x104
    char pad108[0x484 - 0x108];
    resource* f484;                                     // +0x484 (scrollbar)
    char pad488[0x490 - 0x488];
    resource* f490;                                     // +0x490 (list)
    scroll_list() MB_NOTHROW;                           // 0x49deb0
};

// The party window object (the container whose vptr this function is part of).
struct party_window {
    char       pad0[0x2c];
    resource*  members_base;                            // +0x2c
    char       pad30[0x78 - 0x30];
    resource*  party_skills_bar;                        // +0x78
    char       pad7c[0x90 - 0x7c];
    resource*  skills_base;                             // +0x90
    scroll_list* skills_list;                           // +0x94

    void setup_skills();                                // 0x5059c0 (thiscall, `ret`)
};

// ---------------------------------------------------------------------------------------------------------
// The 0x5ac8-byte party object that holds the members of one party.  It is reached through the two-level
// block table at party_manager+0x138d0 (pages[id >> 4] + (id & 0xf) * 0x5ac8).  Its member slots are 0x20
// bytes each and start at +0xac; the member count lives at +0x8a8.
struct member_slot {
    int  party;                                         // +0
    int  flag;                                          // +4 (bit 0 = not a member of this army)
    char pad8[0x20 - 8];
};

struct party_obj {
    char pad00[0xac];
    // the 0x20-byte member slots follow at +0xac, the count is int at +0x8a8
    int          member_count() const { return *(const int*)((const char*)this + 0x8a8); }
    member_slot* members() { return (member_slot*)((char*)this + 0xac); }
};

// The 0x508-byte party-information record, indexed by a member's party id: name at +0x20, flags at +0x30
// (bit 4 = the party is shown in this window), skill points (a float) at +0xa0.
struct party_rec {
    char     pad00[0x20];
    tstring  name;                                      // +0x20
    unsigned flags;                                     // +0x30
    char     pad34[0xa0 - 0x34];
    float    skill_points;                              // +0xa0
    char     padA4[0x508 - 0xa4];
};

// The 0x44-byte skill table entry: the lookup key at +0, the name at +0x10, flags at +0x30 (bit 4 = the
// skill is shown on this page).
struct skill_entry {
    tstring  key;                                       // +0
    tstring  name;                                      // +0x10
    char     pad20[0x30 - 0x20];
    unsigned flags;                                     // +0x30
    char     pad34[0x44 - 0x34];
};

extern skill_entry g_skills[];                          // 0x8ee3110 (42 0x44-byte entries)

// The party manager at World+0xf8 (the object `index_of` is called on).
struct party_manager {
    char         pad0[0x138d0];
    void**       pages;                                 // +0x138d0 (party-object block table)
    char         pad138d4[0x13908 - 0x138d4];
    party_rec*   parties;                               // +0x13908 (flat party-information records)
    int          index_of(int party_id);                // 0x48ad00
};

struct World {
    char          pad0[0x78];
    int           party_id;                             // +0x78 (the party this page shows)
    char          pad7c[0x8c - 0x7c];
    int           player_party;                         // +0x8c (the player's own party)
    char          pad90[0xf8 - 0x90];
    party_manager party;                                // +0xf8
};

extern World* g_game;                                   // 0x5e2930
extern float g_skill_points_ok;                         // 0xa18138
extern float g_skill_points_full;                       // 0xa1813c

// engine callees
void party_bar_setup(resource* w, tstring s);                         // 0x4e4c70
list_widget* make_scrollbar(void* parent, const tstring* name);       // 0x4f6160
int  member_skill(int member, int skill);                             // 0x437b20
int  party_skill(party_obj* party, int skill);                        // 0x4b2370
void quarter_update(resource* r);                                     // 0x488d50

// The 0x508-byte record of the party a member slot refers to.
static inline party_rec* rec_of(World* g, int party)
{
    return (party_rec*)((char*)g->party.parties + party * 0x508);
}

// Is the member slot on a party that this window shows?
static inline bool shown(World* g, const member_slot* ms)
{
    return (ms->flag & 1) == 0 && (rec_of(g, ms->party)->flags & 0x10) != 0;
}

// One member slot's skill value for `skill`: members of a party that has not trained the skill (skill
// points below the threshold) count as zero.  The threshold is the higher one for the player's party.
static inline int skill_of(World* g, const member_slot* ms, int skill)
{
    int v = member_skill(ms->party, skill);
    party_rec* r = rec_of(g, ms->party);
    if (ms->party == g->player_party) {
        if (r->skill_points < g_skill_points_full)
            v = 0;
    } else {
        if (r->skill_points < g_skill_points_ok)
            v = 0;
    }
    return v;
}

// @ 0x5059c0
void party_window::setup_skills()
{
    // ---- the skills panel ----
    list_widget* w = new list_widget;                   // 0x188, ctor 0x4931a0, vptr 0x597050
    skills_base = w;
    party_bar_setup(skills_base, tstring("party_bo_skills"));
    {
        tstring empty(g_empty_string);
        float pos[2];                                   // passed uninitialised by the original
        skills_base->va4(pos, &empty);
    }
    resource* bar = new resource;                       // 0x178, ctor 0x48ae20
    float d[2];
    d[1] = bar->y - skills_base->y;
    d[0] = bar->x - skills_base->x;
    bar->v58(d);
    {
        tstring key("ui_party_skills");
        float pos[2];
        bar->va4(pos, string_table_lookup(&g_strings, &key));
    }
    skills_base->vc8(bar);

    // ---- the party this page shows and its members ----
    World* g = g_game;
    int idx = g->party.index_of(g->party_id);
    party_obj* members = (party_obj*)((char*)g->party.pages[idx >> 4] + (idx & 0xf) * 0x5ac8);
    int count = members->member_count();

    // ---- the page's layout steps ----
    float t_y = g_ini.get_float(tstring("party_bo_skills_t_y"));
    float t_y_a = g_ini.get_float(tstring("party_bo_skills_t_y_a"));
    float t_y_a_short = g_ini.get_float(tstring("party_bo_skills_t_y_a_short"));
    float t_x = g_ini.get_float(tstring("party_bo_skills_t_x"));
    float t_x_deep = g_ini.get_float(tstring("party_bo_skills_t_x_deep"));

    // ---- pass 1: the content height ----
    float y = 0.0f;
    if (count > 0) {
        member_slot* ms = members->members();
        for (int m = 0; m < count; m++, ms++) {
            if (!shown(g, ms))
                continue;
            y += t_y_a;
            for (int k = 0; &g_skills[k] < (skill_entry*)0x8ee3c38; k++) {
                if (((g_skills[k].flags >> 4) & 1) == 0)
                    continue;
                int best = 0;
                int best_i = -1;
                if (count > 0) {
                    member_slot* ms2 = members->members();
                    for (int i = 0; i < count; i++, ms2++) {
                        if (!shown(g, ms2))
                            continue;
                        int v = skill_of(g, ms2, k);
                        if (best < v) {
                            best = v;
                            best_i = i;
                        }
                    }
                }
                if (best_i == m)
                    y += t_y_a_short;
            }
        }
    }
    y += t_y_a;
    {
        float pos[2];
        pos[0] = skills_base->w;
        pos[1] = y + t_y;
        skills_base->v64(pos);
    }

    // ---- pass 2: a caption line per member and a line per skill ----
    if (count > 0) {
        member_slot* ms = members->members();
        char buf[0x224];
        for (int mi = 0; mi < count; mi++, ms++) {
            if (!shown(g, ms))
                continue;
            party_rec* r = rec_of(g, ms->party);
            {
                tstring* nm = translate_text(&g_strings, &r->name);
                if (nm == 0)
                    nm = &r->name;
                tstring cap(*nm);
                cap.capitalize();
                sprintf(buf, "%s:", cap.c_str());
            }
            {
                tstring cap(buf);
                float pos[2];
                pos[0] = t_x;
                pos[1] = y;
                skills_base->vac(&cap, pos);
            }
            y -= t_y_a;

            for (int k = 0; &g_skills[k] < (skill_entry*)0x8ee3c38; k++) {
                skill_entry* se = &g_skills[k];
                if (((se->flags >> 4) & 1) == 0)
                    continue;
                int best = 0;
                int best_i = -1;
                if (count > 0) {
                    member_slot* ms2 = members->members();
                    for (int i = 0; i < count; i++, ms2++) {
                        if (!shown(g, ms2))
                            continue;
                        int v = skill_of(g, ms2, k);
                        if (best < v) {
                            best = v;
                            best_i = i;
                        }
                    }
                }
                if (best_i == mi) {
                    int pq = party_skill(members, k);
                    {
                        tstring* nm = translate_text(&g_strings, &se->name);
                        if (nm == 0)
                            nm = &se->name;
                        if (pq == best)
                            sprintf(buf, "%s (%d)", nm->c_str(), best);
                        else
                            sprintf(buf, "%s (%d)(+%d)", nm->c_str(), best, pq - best);
                    }
                    {
                        tstring line(buf);
                        float pos[2];
                        pos[0] = t_x_deep;
                        pos[1] = y;
                        skills_base->vac(&line, pos);
                    }
                    y -= t_y_a_short;
                }
            }
        }
    }

    // ---- the scroll list ----
    list_widget* sb;
    {
        tstring name("party_s_skills");
        sb = make_scrollbar(members_base, &name);
    }
    {
        tstring name("party_sc_skills");
        scroll_list* list = new scroll_list;            // 0x4a8, ctor 0x49deb0
        list->f104 |= 0x20;
        if (sb != 0)
            list->f484 = sb;
        list->f490 = skills_base;
        list->vc8(skills_base);
        party_bar_setup(list, name);
        skills_list = list;
    }

    // ---- place the list in the panel ----
    scroll_list* list = skills_list;
    if (skills_base->h > list->h)
        d[1] = 0.0f;
    else
        d[1] = list->h - skills_base->h;
    d[0] = list->x;
    d[1] = d[1] + list->y;
    list->v58(d);
    {
        resource* lower = skills_base->h > list->h ? skills_base : list;
        float pos[2];
        pos[0] = list->w;
        pos[1] = lower->h;
        list->v64(pos);
    }
    quarter_update(list);
    members_base->vc8(skills_list);

    // ---- the party-skills information bar ----
    party_skills_bar = new resource;
    party_bar_setup(party_skills_bar, tstring("party_t_party_skills"));
    {
        tstring key("ui_party_skills");
        float pos[2];
        party_skills_bar->va4(pos, string_table_lookup(&g_strings, &key));
    }
    members_base->vc8(party_skills_bar);
}
