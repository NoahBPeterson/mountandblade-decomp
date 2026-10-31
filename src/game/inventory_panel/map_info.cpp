// ====================================================================================================
// Slice s0051abd0 -- 0x51abd0..0x51be1e   (ret 8, stack frame 0x6598 allocated through __chkstk)
//
//   void __thiscall MapPanel::update(int type, int id)
//
// The map info panel's repaint.  (type, id) describe the current selection:
//   0 = nothing (just remember the selection), 1 = a party, 3 = a fleet/battle-table slot,
//   4 = a party looked up by id.
// The panel is re-anchored to the selection's world position, then the whole text block is painted:
// the capital-battle two-column split, each side's name and relation colour, the collected sub-party
// list with troop strengths, the faction tag, the current order line ("travelling to ...",
// "fighting against ...") and the garrison contents (defenders / troops / prisoners).
//
// Offsets were recovered from this function plus the neighbouring verified slices
// (s004b5c30 = RecR/Stack layout, s0048f8f0 = party pages & party-page indexing, s0046bc70 = tstring).
// This is a complete behavioural reconstruction; see nonmatching.txt for why it is not byte-exact.
// ====================================================================================================
#include <stdio.h>
#include <stddef.h>
#include "../tstring.h"

extern "C" int _fltused = 0;                 // static-CRT float marker

// ---------------------------------------------------------------------------------- record layouts --

#include "map_panel.h"                      // MapPanel / MapView / Pos4 (shared with map_update.cpp)

struct RecR;
struct FactionRec;

#include "../../include/game_data.h"
#include "../../include/party.h"      // Party::prepare / speed_factor (listed directly: the resolution audit reads only the source's own includes)

struct RecR {                                // 0x5ac8  party / roster record
    RecR();                                  // 0x4934d0
    ~RecR();                                 // 0x431b90
    // prepare (0x4b1eb0) and speed_factor (0x4b20b0) are Party's (include/party.h); dist_is_zero is the free function below
    tstring* display_name();                 // 0x48d7b0  (the name used when appending an order)

    char     pad00[0x0c];
    tstring  name;                           // +0x0c
    char     f1c;                            // +0x1c  "key is a string-table token"
    char     pad1d[3];
    tstring  key;                            // +0x20
    Pos4     pos;                            // +0x30
    char     pad40[0x54 - 0x40];
    tstring  text;                           // +0x54  free-form note (len at +0x60)
    char     pad64[0x6c - 0x64];
    int      f6c;                            // +0x6c  nonzero = roster with sub-parties
    int      faction;                        // +0x70  index into the faction table
    char     pad74[0x88 - 0x74];
    int      agent;                          // +0x88  map agent / battle side, -1 if none
    char     pad8c[0xa0 - 0x8c];
    unsigned flags;                          // +0xa0  (0x400, 0x40000, 0x200000, 0x400000 used here)
    unsigned color;                          // +0xa4
    Stack    stacks[64];                     // +0xa8
    int      count;                          // +0x8a8
    char     pad8ac[0x918 - 0x8ac];
    int      state;                          // +0x918  order state
    int      target;                         // +0x91c  party the order refers to
    char     pad920[0x5ac8 - 0x920];
};



#include "../rec98.h"                       // Rec98 (this slice called it MapAgent): the 0x98-byte record, a = f54, b = f58

struct PageTable98 {                         // lives at GameData+0xcc (the page table slot_api.cpp defines slot() for)
    char      pad00[0x18];
    void**    pages;                         // +0x18  (GameData+0xe4)
    void* slot(unsigned idx);             // 0x48ef20
};
typedef PageTable98 AgentPages;


struct gui_obj;
struct gui_manager {
    void draw_string(tstring* s, float height, int arg, char wrap);        // 0x4e43e0 (ret 0x10; game/strat/gui_manager.h)
    __forceinline void draw_string(const tstring& s, float arg, unsigned color, int centered)   // the callers' by-reference form
    {
        draw_string((tstring*)&s, arg, (int)color, (char)centered);
    }
    void set_o568(gui_obj* obj, bool on);                                  // 0x42cdb0
};
struct gui_view {
    void set_view(const float* v);   // 0x4c5190 (rgl/mesh2/gui_edit.h): the same object as g_gui
};
struct MapWidget {
    void project(rgl::vector4* out, const rgl::vector4* in);   // 0x51a690 (inventory_panel/map_picker.cpp)
};
struct GameW {
    void list_members(unsigned id, int* out, int* n);   // 0x490fa0 (game/world_data)
};
struct DistObj;
bool __fastcall dist_is_zero(DistObj* o);                                 // 0x51be20 (map_tooltip.cpp)
int __stdcall draw_party_line(int id);                                    // 0x51c090 (map_tooltip.cpp)

#include "../str_table.h"
#include "../skill_cap.h"

// ---- layout guards (MSVC 13 has no static_assert: a mismatch is a negative array size) -------------
// Sizes: every record's stride appears in the original as an `imul ..., <stride>` immediate.
typedef char assert_Faction_300  [(sizeof(FactionRec)    == 0x300)   ? 1 : -1];
typedef char assert_RecR_5ac8    [(sizeof(RecR)       == 0x5ac8)  ? 1 : -1];
typedef char assert_AgentPages   [(sizeof(AgentPages) == 0x1c)    ? 1 : -1];
// Field offsets: taken straight from the original's displacements (see the header comment).
typedef char assert_RecR_name    [(offsetof(RecR, name)      == 0x0c)    ? 1 : -1];
typedef char assert_RecR_f1c     [(offsetof(RecR, f1c)       == 0x1c)    ? 1 : -1];
typedef char assert_RecR_key     [(offsetof(RecR, key)       == 0x20)    ? 1 : -1];
typedef char assert_RecR_pos     [(offsetof(RecR, pos)       == 0x30)    ? 1 : -1];
typedef char assert_RecR_text    [(offsetof(RecR, text)      == 0x54)    ? 1 : -1];
typedef char assert_RecR_f6c     [(offsetof(RecR, f6c)       == 0x6c)    ? 1 : -1];
typedef char assert_RecR_faction [(offsetof(RecR, faction)   == 0x70)    ? 1 : -1];
typedef char assert_RecR_agent   [(offsetof(RecR, agent)     == 0x88)    ? 1 : -1];
typedef char assert_RecR_flags   [(offsetof(RecR, flags)     == 0xa0)    ? 1 : -1];
typedef char assert_RecR_color   [(offsetof(RecR, color)     == 0xa4)    ? 1 : -1];
typedef char assert_RecR_stacks  [(offsetof(RecR, stacks)    == 0xa8)    ? 1 : -1];
typedef char assert_RecR_count   [(offsetof(RecR, count)     == 0x8a8)   ? 1 : -1];
typedef char assert_RecR_state   [(offsetof(RecR, state)     == 0x918)   ? 1 : -1];
typedef char assert_RecR_target  [(offsetof(RecR, target)    == 0x91c)   ? 1 : -1];
typedef char assert_Faction_f10  [(offsetof(FactionRec, f10)    == 0x10)    ? 1 : -1];
typedef char assert_Faction_key  [(offsetof(FactionRec, key)    == 0x14)    ? 1 : -1];
typedef char assert_Faction_f24  [(offsetof(FactionRec, f24)    == 0x24)    ? 1 : -1];
typedef char assert_Faction_color[(offsetof(FactionRec, color)  == 0x28)    ? 1 : -1];
typedef char assert_Faction_rel  [(offsetof(FactionRec, rel)    == 0x2c)    ? 1 : -1];
typedef char assert_Rec98_a      [(offsetof(Rec98, f54)      == 0x54)    ? 1 : -1];
typedef char assert_Rec98_b      [(offsetof(Rec98, f58)      == 0x58)    ? 1 : -1];
typedef char assert_AgPages_ptr  [(offsetof(AgentPages, pages)   == 0x18)    ? 1 : -1];
typedef char assert_Panel_view   [(offsetof(MapPanel, view)         == 0x30)    ? 1 : -1];
typedef char assert_Panel_timer  [(offsetof(MapPanel, timer)        == 0x2cc)   ? 1 : -1];
typedef char assert_View_matrix  [(offsetof(MapView, pad04)         == 0x4)     ? 1 : -1];

// ------------------------------------------------------------------------------------- globals ------
extern gui_manager g_gui;                    // 0x5d7080
extern StrTable   g_strtable;                // 0x5e2a18
extern int        g_sel_type;                // 0x5aa3b4  last painted selection
extern int        g_sel_id;                  // 0x5aa3b0
extern int        g_show_faction_color;      // 0xa181b0
extern char       g_show_party_ids_instead_of_names;               // 0xa181b8

extern unsigned   g_panel_color;             // 0x5d8760  name colour of the left column
extern unsigned   g_panel_bg;                // 0x5d8764  background tint for the left column

void mb_snprintf(char* buf, size_t n, const char* fmt, ...);   // 0x415100

// ------------------------------------------------------------------------------------- helpers ------
// party page lookup, exactly as inlined at 0x51ac38 / 0x51aed3: pages[idx >> 4] + (idx & 15)
static RecR* party_at(PartyMgr* mgr, int idx)
{
    return (RecR*)mgr->pages._Myfirst[idx >> 4] + (idx & 0xf);
}
static RecR* party_by_id(int id)
{
    return party_at(&g_game->parties, g_game->parties.index_of(id));
}
static AgentPages* agents_of() { return (AgentPages*)((char*)g_game + 0xcc); }   // GameData +0xcc (0x48ef20 is its slot())
static Rec98* agent_at(int idx)
{
    return (Rec98*)g_game->agent_pages[idx >> 4] + (idx & 0xf);
}

// a record's display string: the key's string-table entry when the key is a token (0x51afb6, 0x51b2cb)
static tstring* record_name(RecR* r)
{
    if (g_show_party_ids_instead_of_names) return &r->key;
    if (r->f1c == 0) {
        tstring* s = g_strtable.find(&r->key);
        if (s) return s;
    }
    return &r->name;
}
static const char* record_name_c(RecR* r) { return record_name(r)->c_str(); }

// a troop's display string (no key-mode check, see 0x51bc05)
static const char* troop_name_c(Troop* t)
{
    tstring* s = g_strtable.find(&t->key);
    return (s ? s : &t->name)->c_str();
}

// index of the first prisoner/enemy stack, rescanned on every iteration (0x51bb56)
static int first_enemy_stack(RecR* r)
{
    int i = 0;
    while (i < r->count && !(r->stacks[i].flags & 1)) i++;
    return i;
}

// append the name of the party an order refers to
static void append_order_target(tstring& label, int target)
{
    label.append(*record_name(party_by_id(target)));
}

// -----------------------------------------------------------------------------------------------------
// @ 0x51abd0
void MapPanel::update(int type, int id)
{
    tstring label;                           // [E+0x2c]  the accumulated order text
    tstring scratch;                         // [E+0x170] constructed and destroyed, otherwise unused
    Pos4    pos = { 0.0f, 0.0f, 0.0f, 0.0f };// [E+0x40]  world anchor of the selection
    RecR*   rec  = 0;                        // [E+0x60]  record currently being painted
    int     ncols = 1;                       // [E+0x28]
    int     agent;                           // [E+0x14]  battle side shown in the current column

    // ---- anchor the panel -------------------------------------------------------------------------
    if (type == 1) {
        rec = party_by_id(id);
        pos = rec->pos;
    } else if (type == 3) {
        pos = *(Pos4*)(g_game->fleet + id * 0x5c);
    } else if (type == 4) {
        rec = (RecR*)g_game->parties.get(id);
        pos = rec->pos;
    }

    // ---- the same selection is still on screen and the cooldown has not elapsed -------------------
    if (type == g_sel_type && id == g_sel_id && timer < 1.0f) return;

    g_gui.set_o568(0, true);
    timer = 0.0f;

    if (type != 0) {
        if (type == 1) {
            RecR roster;                     // [E+0xae0] the selected party's own roster (0x4934d0)

            ncols = 1;
            rec = party_by_id(id);
            if (rec->agent >= 0) {
                g_gui.draw_string(*g_strtable.lookup(tstring("ui_capital_battle")), 0.02f, 0xffffffff, 1);
                ncols = 2;
            }

            for (int c = 0; c < ncols; c++) {                       // 0x51adc0
                agent = id;
                if (ncols > 1) {
                    RecR*     r    = party_by_id(id);
                    Rec98* slot = agent_at(r->agent);
                    agent = (c == 0) ? slot->f58 : slot->f54;
                }
                if (c > 0) {
                    g_gui.draw_string(tstring(" "), 0.01f, 0xffffffff, 1);
                    g_gui.draw_string(*g_strtable.lookup(tstring("ui_capital_versus")), 0.024f, 0xffffffff, 1);
                }

                rec = party_by_id(agent);
                float    rel   = g_game->relation(agent, g_game->f78);
                unsigned rel_color = g_game->color_of(rel, 1);
                unsigned color = rel_color;
                if (g_show_faction_color)
                    color = g_game->factions[party_by_id(agent)->faction].color;
                if (c == 0) {
                    g_panel_color = rel_color;          // 0x51af67: the relation colour (edi), not the faction override (esi)
                    g_panel_bg    = rel < -0.001f ? 0xff200000u
                                  : rel >  0.001f ? 0xff002000u : 0xff101010u;
                }

                // ---- the party's own name (or its list of sub-parties) -----------------------------
                if (rec->f6c == 0) {
                    char buf[0x100];
                    _snprintf(buf, 0x100, "%s", record_name_c(rec));             // 0x535717
                    g_gui.draw_string(tstring(buf), 0.021f, color, 1);
                } else {
                    int members[128];                                        // [E+0x8e0]
                    int count = 0;                                           // [E+0x18]
                    ((GameW*)g_game)->list_members(agent, members, &count);                 // 0x490fa0
                    for (int i = 0; i < count; i++) {
                        int      m    = members[i];
                        float    mrel = g_game->relation(m, g_game->f78);
                        unsigned mcol = g_game->color_of(mrel, 1);
                        if (g_show_faction_color)
                            mcol = g_game->factions[party_by_id(m)->faction].color;
                        if (party_by_id(m)->flags & 0x40000)
                            mcol = mrel < -0.01f ? 0xff22aaffu : 0xffaa22ddu;

                        int extra = 0, wounded = 0, size = 0;
                        g_game->party_counts(m, &size, &wounded, &extra);    // 0x51be60: arg2 = troops, arg3 = wounded, arg4 = prisoners (the "+ N")

                        char buf[0x100];
                        RecR* mr = party_by_id(m);
                        if (mr->flags & 0x200000) {
                            _snprintf(buf, 0x100, "%s", record_name_c(mr));
                        } else if (wounded != 0) {
                            if (extra != 0)
                                _snprintf(buf, 0x100, "%s (%d/%d + %d)", record_name_c(mr), size - wounded, size, extra);
                            else
                                _snprintf(buf, 0x100, "%s (%d/%d)", record_name_c(mr), size - wounded, size);
                        } else {
                            if (extra != 0)
                                _snprintf(buf, 0x100, "%s (%d + %d)", record_name_c(mr), size, extra);
                            else
                                _snprintf(buf, 0x100, "%s (%d)", record_name_c(mr), size);
                        }
                        g_gui.draw_string(tstring(buf), 0.021f, mcol, 1);
                    }

                    roster.count = 0;
                    g_game->fill_parties(agent, (Party*)&roster);                    // 0x48fd60
                    ((Party*)&roster)->prepare();                                        // 0x4b1eb0
                }

                // ---- faction tag -------------------------------------------------------------------
                if ((rec->flags & 0x400000) && rec->faction >= 0) {
                    FactionRec* f = &g_game->factions[rec->faction];
                    if (f->f24 == 0) {
                        tstring* fn = &f->name;
                        if (f->f10 == 0) {
                            tstring* s = g_strtable.find(&f->key);
                            if (s) fn = s;
                        }
                        char buf[0x200];
                        mb_snprintf(buf, 0x200, "(%s)", fn->c_str());        // 0x415100
                        unsigned fcol = g_show_faction_color
                                      ? f->color
                                      : f->rel[0] < -0.001f ? 0xffff4422u
                                      : f->rel[0] >  0.001f ? 0xff44ff22u : 0xffffffffu;
                        g_gui.draw_string(tstring(buf), 0.022f, fcol, 1);     // 0x3cb43958
                    }
                }
                if (rec->text.len > 2)
                    g_gui.draw_string(rec->text, 0.023f, 0xffffffff, 1);

                // ---- the order line (single-column display only) -----------------------------------
                if (ncols == 1) {
                    switch (rec->state) {                                    // 0x51b5d1
                    case 0:
                        label = *g_strtable.lookup(tstring("ui_waiting"));
                        break;
                    case 1:
                        label = *g_strtable.lookup(tstring("ui_travelling_to"));
                        append_order_target(label, rec->target);
                        break;
                    case 2:
                        label = *g_strtable.lookup(tstring("ui_patrolling"));
                        break;
                    case 3:
                        label = *g_strtable.lookup(tstring("ui_patrolling_around"));
                        label.append(*g_game->parties.get(rec->target)->display_name());
                        break;
                    case 4: {
                        bool on_way = (agent == g_game->f78);
                        if (!on_way)
                            on_way = (party_by_id(rec->target)->flags & 0x400) != 0;
                        label = *(on_way ? g_strtable.lookup(tstring("ui_travelling_to"))
                                         : g_strtable.lookup(tstring("ui_following")));
                        append_order_target(label, rec->target);
                        break;
                    }
                    case 5:
                        label = *g_strtable.lookup(tstring("ui_running_from"));
                        label.append(*g_game->parties.get(rec->target)->display_name());
                        break;
                    case 6:
                        label = *(dist_is_zero((DistObj*)rec) ? g_strtable.lookup(tstring("ui_holding"))
                                                   : g_strtable.lookup(tstring("ui_travelling")));
                        break;
                    case 7: {
                        int a = rec->agent;
                        if (a >= 0) {
                            Rec98* slot = (Rec98*)agents_of()->slot(a);         // 0x48ef20
                            int other = (slot->f54 == agent) ? slot->f58 : slot->f54;
                            if (other >= 0) {
                                label = *g_strtable.lookup(tstring("ui_fighting_against"));
                                label.append(*g_game->parties.get(other)->display_name());
                            }
                        }
                        break;
                    }
                    case 10:
                        label = *g_strtable.lookup(tstring("ui_accompanying"));
                        label.append(*g_game->parties.get(rec->target)->display_name());
                        break;
                    default:
                        break;                       // unknown order: the label stays empty
                    }

                    if (!(rec->flags & 0x400)) {
                        g_gui.draw_string(label, 0.021f, 0xffc0b0a0, 1);
                        if (!dist_is_zero((DistObj*)rec)) {
                            char buf[0x200];
                            const char* fmt = g_strtable.lookup(tstring("ui_speed_equals"))->c_str();
                            sprintf(buf, fmt, (double)((Party*)rec)->speed_factor());
                            g_gui.draw_string(tstring(buf), 0.021f, 0xffb0a090, 1);
                        }
                    }
                }

                // ---- garrison contents -------------------------------------------------------------
                if (!(rec->flags & 0x200000) && rec->f6c != 0) {              // 0x51ba69
                    unsigned tcol = 0xffd0b0e0;
                    g_gui.draw_string(*g_strtable.lookup(tstring((rec->flags & 0x400) ? "ui_defenders"
                                                                                    : "ui_troops")),
                                      0.02f, 0xffc08020, 1);
                    for (int i = 0; i < roster.count; i++) {
                        if (i == first_enemy_stack(&roster)) {                // 0x51bb56
                            g_gui.draw_string(*g_strtable.lookup(tstring("ui_prisoners")), 0.02f, 0xffc08020, 1);
                            tcol = 0xffc0a060;
                        }
                        Stack* s = &roster.stacks[i];
                        Troop* t = &g_game->troops[s->troop];
                        char   buf[0x100];
                        if (t->flags32 & 0x10) {
                            int tier = t->nib_b[0x7a - 0x68] & 0xf;
                            if (tier < 0)                tier = 0;
                            else if (tier > (*(int*)&g_skill_tbl[36].cap)) tier = (*(int*)&g_skill_tbl[36].cap);
                            int pct = (int)(t->hp * 100.0f / (float)(t->f58[0] + tier * 2 + 0x23) + 1e-4f);
                            sprintf(buf, "%s (%d%%)", troop_name_c(t), pct);
                            g_gui.draw_string(tstring(buf), 0.015f, tcol | 0x0f0f0f, 0);
                        } else {
                            if (s->wounded == 0)
                                sprintf(buf, "%s (%d)", troop_name_c(t), s->count);
                            else
                                sprintf(buf, "%s (%d/%d)", troop_name_c(t), s->count - s->wounded, s->count);
                            g_gui.draw_string(tstring(buf), 0.015f, tcol, 0);
                        }
                    }
                }
            }                                    // for (c ...)
        } else if (type == 3) {
            ::draw_party_line(id);
        }

        // ---- move the panel to the selection's screen position --------------------------------------
        float xyv[4];                                           // project's output (a vector4; the original reads it back through eax)
        ((MapWidget*)view)->project((rgl::vector4*)xyv, (const rgl::vector4*)&pos);   // 0x51a690
        int* xy = (int*)xyv;
        int  screen[2] = { xy[0], xy[1] };
        ((gui_view*)&g_gui)->set_view((const float*)screen);      // 0x4c5190
    }

    g_sel_type = type;
    g_sel_id   = id;
}
