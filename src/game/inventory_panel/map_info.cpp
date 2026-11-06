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
#include "../shared_vars.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_sel_id = -1;                            // 0x5aa3b0
int g_sel_type = -1;                          // 0x5aa3b4 last painted selection


extern "C" int _fltused = 0;                 // static-CRT float marker

// ---------------------------------------------------------------------------------- record layouts --

#include "map_panel.h"                      // MapPanel / MapView / PartyV4 (shared with map_update.cpp)

struct FactionRec;

#include "../../include/game_data.h"
#include "../../include/party.h"      // Party::prepare / speed_factor (listed directly: the resolution audit reads only the source's own includes)

// the party record (include/party.h Party; the roster built for the selected party is a Party too: constructor 0x4934d0, destructor 0x431b90)



#include "../rec98.h"                       // Rec98 (this slice called it MapAgent): the 0x98-byte record, a = f54, b = f58
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../rgl/text_stream.h"

#include "../battle_rec.h"                   // BattleTable (GameData+0xcc; get 0x48ef20)


struct MapWidget {
    void project(rgl::vector4* out, const rgl::vector4* in);   // 0x51a690 (inventory_panel/map_picker.cpp)
};
struct DistObj;
bool __fastcall dist_is_zero(DistObj* o);                                 // 0x51be20 (map_tooltip.cpp)
int __stdcall draw_party_line(int id);                                    // 0x51c090 (map_tooltip.cpp)

#include "../str_table.h"
#include "../skill_cap.h"
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)

// ---- layout guards (MSVC 13 has no static_assert: a mismatch is a negative array size) -------------
// Sizes: every record's stride appears in the original as an `imul ..., <stride>` immediate.
typedef char assert_Faction_300  [(sizeof(FactionRec)    == 0x300)   ? 1 : -1];
// Field offsets: taken straight from the original's displacements (see the header comment).
typedef char assert_Faction_f10  [(offsetof(FactionRec, f10)    == 0x10)    ? 1 : -1];
typedef char assert_Faction_key  [(offsetof(FactionRec, key)    == 0x14)    ? 1 : -1];
typedef char assert_Faction_f24  [(offsetof(FactionRec, f24)    == 0x24)    ? 1 : -1];
typedef char assert_Faction_color[(offsetof(FactionRec, color)  == 0x28)    ? 1 : -1];
typedef char assert_Faction_rel  [(offsetof(FactionRec, rel)    == 0x2c)    ? 1 : -1];
typedef char assert_Rec98_a      [(offsetof(Rec98, f54)      == 0x54)    ? 1 : -1];
typedef char assert_Rec98_b      [(offsetof(Rec98, f58)      == 0x58)    ? 1 : -1];
typedef char assert_Panel_view   [(offsetof(MapPanel, view)         == 0x30)    ? 1 : -1];
typedef char assert_Panel_timer  [(offsetof(MapPanel, timer)        == 0x2cc)   ? 1 : -1];
typedef char assert_View_matrix  [(offsetof(MapView, pad04)         == 0x4)     ? 1 : -1];

// ------------------------------------------------------------------------------------- globals ------



// ------------------------------------------------------------------------------------- helpers ------

// a record's display string: the key's string-table entry when the key is a token (0x51afb6, 0x51b2cb)
static tstring* record_name(Party* r)
{
    if (g_module_settings.show_party_ids_instead_of_names) return &r->key;
    if (r->flag == 0) {
        tstring* s = g_strtable.find(&r->key);
        if (s) return s;
    }
    return &r->name;
}
static const char* record_name_c(Party* r) { return record_name(r)->c_str(); }

// a troop's display string (no key-mode check, see 0x51bc05)
static const char* troop_name_c(Troop* t)
{
    tstring* s = g_strtable.find(&t->key);
    return (s ? s : &t->name)->c_str();
}

// index of the first prisoner/enemy stack, rescanned on every iteration (0x51bb56)
static int first_enemy_stack(Party* r)
{
    int i = 0;
    while (i < r->count && !(r->stacks[i].flags & 1)) i++;
    return i;
}

// party lookup with the id by const reference: a by-value id takes one more scratch pick and rotates what follows (0x51b87e, 0x51b930)
static inline Party* pget(PartyMgr* m, const int& id) { int idx = m->lookup(id); return (Party*)((char*)m->pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x5ac8); }
struct FleetRec { rgl::vector4 pos; char pad10[0x5c - 0x10]; };   // the 0x5c-byte fleet / battle record: only its position is read here

// -----------------------------------------------------------------------------------------------------
// @ 0x51abd0
void MapPanel::update(int type, int id)
{
    tstring label;                           // [esp+0x2c]  the accumulated order text
    tstring scratch;                         // [esp+0x170] constructed and destroyed, otherwise unused
    rgl::vector4 pos(0.0f, 0.0f, 0.0f);      // [esp+0x40]  world anchor of the selection (w stays unset)

    // ---- anchor the panel -------------------------------------------------------------------------
    if (type == 1) {
        pos = *(rgl::vector4*)&g_game->parties.get_w(id)->pos;
    } else if (type == 3) {
        FleetRec& fr = ((FleetRec*)g_game->fleet)[id];
        pos = fr.pos;
    } else if (type == 4) {
        pos = *(rgl::vector4*)&g_game->parties.get(id)->pos;
    }

    // ---- the same selection is still on screen and the cooldown has not elapsed -------------------
    if (type == g_sel_type && id == g_sel_id && timer < 1.0f) return;

    timer = 0.0f;
    g_gui.set_overlay(0, true);

    if (type != 0) {
        if (type == 1) {
            Party roster;                     // [esp+0xae0] the selected party's own roster (0x4934d0)
            int ncols = 1;                    // [esp+0x28]

            if (g_game->parties.get_party(id)->battle >= 0) {
                g_gui.draw_string(g_strtable.lookup("ui_capital_battle"), 0.02f, -1, 1);
                ncols = 2;
            }

            for (int c = 0; c < ncols; c++) {                       // 0x51adc0
                int agent = id;               // [esp+0x14]  battle side shown in the current column
                char buf[0x100];
                if (ncols > 1) {
                    GameData* g = g_game;
                    int b = g->parties.get_party(agent)->battle;
                    Rec98* slot = (Rec98*)g->agent_pages[(unsigned)b >> 4] + (b & 0xf);
                    if (c == 0) agent = slot->f58;
                    else agent = slot->f54;
                }
                if (c > 0) {
                    g_gui.draw_string(" ", 0.01f, -1, 1);
                    g_gui.draw_string(g_strtable.lookup("ui_capital_versus"), 0.024f, -1, 1);
                }

                PartyMgr& mg = g_game->parties;
                GameData* g = g_game;
                Party* rec = mg.get_party(agent);        // [esp+0x60]  record being painted
                float    rel   = g->relation(agent, g->f78);
                unsigned rel_color = g->color_of(rel, 1);
                unsigned color;
                if (g_module_settings.show_faction_color)
                    color = g_game->factions[mg.get_party(agent)->faction].color;
                else
                    color = rel_color;
                if (c == 0) {
                    g_gui.panel_color = rel_color;          // 0x51af67: the relation colour (edi), not the faction override (esi)
                    if (rel < -0.001f) g_gui.panel_bg = 0xff200000u;
                    else if (rel > 0.001f) g_gui.panel_bg = 0xff002000u;
                    else g_gui.panel_bg = 0xff101010u;
                }

                // ---- the party's own name (or its list of sub-parties) -----------------------------
                if (rec->f6c == 0) {
                    _snprintf(buf, 0x100, "%s", record_name_c(rec));             // 0x535717
                    g_gui.draw_string(buf, 0.021f, color, 1);
                } else {
                    int members[128];                                        // [esp+0x8e0]
                    int count = 0;                                           // [esp+0x18]
                    g_game->list_members(agent, members, &count);                 // 0x490fa0
                    for (int i = 0; i < count; i++) {
                        GameData* g = g_game;
                        int      m    = members[i];
                        float mrel = g->relation(m, g->f78);
                        unsigned mcol = g->color_of(mrel, 1);
                        if (g_module_settings.show_faction_color)
                            mcol = g->factions[g->parties.get_party(m)->faction].color;
                        if (g->parties.get_party(m)->flags & 0x40000)
                            mcol = mrel < -0.01f ? 0xffaa22ddu : 0xff22aaffu;

                        int extra, wounded, size;
                        g->party_counts(m, &size, &wounded, &extra);    // 0x51be60: arg2 = troops, arg3 = wounded, arg4 = prisoners (the "+ N")

                        PartyMgr& pm = g_game->parties;
                        if (pm.get_party(m)->flags & 0x200000) {
                            _snprintf(buf, 0x100, "%s", record_name_c(pm.get_party(m)));
                        } else if (wounded != 0) {
                            if (extra != 0)
                                _snprintf(buf, 0x100, "%s (%d/%d + %d)", record_name_c(pm.get_party(m)), size - wounded, size, extra);
                            else
                                _snprintf(buf, 0x100, "%s (%d/%d)", record_name_c(pm.get_party(m)), size - wounded, size);
                        } else {
                            if (extra != 0)
                                _snprintf(buf, 0x100, "%s (%d + %d)", record_name_c(pm.get_party(m)), size, extra);
                            else
                                _snprintf(buf, 0x100, "%s (%d)", record_name_c(pm.get_party(m)), size);
                        }
                        g_gui.draw_string(buf, 0.021f, mcol, 1);
                    }

                    roster.count = 0;
                    g_game->fill_parties(agent, &roster);                    // 0x48fd60
                    roster.prepare();                                        // 0x4b1eb0
                }

                // ---- faction tag -------------------------------------------------------------------
                GameData* gd;                   // esi: re-read after each call that can kill it; it stays live into the party lookup below
                if (rec->flags & 0x400000) {
                    int fac = rec->faction;
                    if (fac >= 0) {
                        gd = g_game;
                        if (gd->factions[fac].f24 != 0) goto have_gd;
                        FactionRec* f = gd->factions;
                        f += fac;
                        if (f->f10 == 0) {
                            tstring* s = g_strtable.find(&f->key);
                            if (s) f = (FactionRec*)s;
                        }
                        char buf[0x200];
                        mb_snprintf(buf, 0x200, "(%s)", f->name.c_str());        // 0x415100
                        unsigned fcol;
                        if (g_module_settings.show_faction_color) {
                            fcol = g_game->factions[fac].color;
                        } else {
                            const float& r = g_game->factions[fac].rel[0];
                            fcol = 0xffffffffu;
                            if (r < -0.001f) fcol = 0xffff4422u;
                            else if (r > 0.001f) fcol = 0xff44ff22u;
                        }
                        g_gui.draw_string(buf, 0.022f, fcol, 1);     // 0x3cb43958
                    }
                }
                gd = g_game;
            have_gd:
                if (rec->extra_text.len > 2) {
                    g_gui.draw_string(rec->extra_text, 0.023f, 0xffffffff, 1);
                    gd = g_game;
                }

                // ---- the order line (single-column display only) -----------------------------------
                if (ncols == 1) {
                    int action = rec->action;
                    if (action == 0) {
                        label = *g_strtable.lookup("ui_waiting");
                    } else if (action != 4 && action != 1) {
                        if (action == 10) {
                        label = *g_strtable.lookup("ui_accompanying");
                        label.append(*g_game->parties.get(rec->target)->display_name());
                    } else if (action == 5) {
                        label = *g_strtable.lookup("ui_running_from");
                        label.append(*g_game->parties.get(rec->target)->display_name());
                    } else if (action == 2) {
                        label = *g_strtable.lookup("ui_patrolling");
                    } else if (action == 3) {
                        label = *g_strtable.lookup("ui_patrolling_around");
                        label.append(*g_game->parties.get(rec->target)->display_name());
                    } else if (action == 6) {
                        if (dist_is_zero((DistObj*)rec))
                            label = *g_strtable.lookup("ui_holding");
                        else
                            label = *g_strtable.lookup("ui_travelling");
                    } else if (action == 7) {
                        int a = rec->battle;
                        if (a >= 0) {
                            Rec98* slot = (Rec98*)((BattleTable*)((char*)gd + 0xcc))->get(a);         // 0x48ef20
                            int other;
                            if (slot->f54 == agent) other = slot->f58;
                            else other = slot->f54;
                            if (other >= 0) {
                                label = *g_strtable.lookup("ui_fighting_against");
                                label.append(*g_game->parties.get(other)->display_name());
                            }
                        }
                        }
                    } else {
                        if (rec->action == 1 || agent == gd->f78
                            // gd becomes the manager here: one variable across the lookup keeps it in esi (add esi,0xf8), which sets the scratch picks of the whole order chain
                            || (pget((PartyMgr*)(gd = (GameData*)&gd->parties), rec->target)->flags & 0x400))
                            label = *g_strtable.lookup("ui_travelling_to");
                        else
                            label = *g_strtable.lookup("ui_following");
                        label.append(*record_name(pget(&g_game->parties, rec->target)));
                    }

                    if (!(rec->flags & 0x400)) {
                        g_gui.draw_string(label, 0.021f, 0xffc0b0a0, 1);
                        if (!dist_is_zero((DistObj*)rec)) {
                            char buf[0x200];
                            sprintf(buf, g_strtable.lookup("ui_speed_equals")->c_str(), (double)rec->speed_factor());
                            g_gui.draw_string(buf, 0.021f, 0xffb0a090, 1);
                        }
                    }
                }

                // ---- garrison contents -------------------------------------------------------------
                unsigned tcol = 0xffd0b0e0;
                if (!(rec->flags & 0x200000) && rec->f6c != 0) {              // 0x51ba69
                    if (rec->flags & 0x400)
                        g_gui.draw_string(g_strtable.lookup("ui_defenders"), 0.02f, 0xffc08020, 1);
                    else
                        g_gui.draw_string(g_strtable.lookup("ui_troops"), 0.02f, 0xffc08020, 1);
                    for (int i = 0; i < roster.count; i++) {
                        if (i == first_enemy_stack(&roster)) {                // 0x51bb56
                            g_gui.draw_string(g_strtable.lookup("ui_prisoners"), 0.02f, 0xffc08020, 1);
                            tcol = 0xffc0a060;
                        }
                        Stack& s = roster.stacks[i];
                        Troop* t = g_game->troops + s.troop;
                        char   buf[0x200];
                        if (t->flags30 & 0x10) {
                            int base = t->f58[0];
                            int tier = t->nib_b[0x7a - 0x68] & 0xf;
                            if (tier < 0)                tier = 0;
                            else if (tier > (*(int*)&g_skill_tbl[36].max_rank)) tier = (*(int*)&g_skill_tbl[36].max_rank);
                            const char* name = troop_name_c(t);
                            sprintf(buf, "%s (%d%%)", name, (int)(t->hp * 100.0f / (base + tier * 2 + 0x23) + 1e-4f));
                            g_gui.draw_string(buf, 0.015f, tcol | 0x0f0f0f, 0);
                        } else {
                            if (s.wounded == 0)
                                sprintf(buf, "%s (%d)", troop_name_c(t), s.count);
                            else
                                sprintf(buf, "%s (%d/%d)", troop_name_c(t), s.count - s.wounded, s.count);
                            g_gui.draw_string(buf, 0.015f, tcol, 0);
                        }
                    }
                }
            }                                    // for (c ...)
        } else if (type == 3) {
            ::draw_party_line(id);
        }

        // ---- move the panel to the selection's screen position --------------------------------------
        {
            float out[2];                  // eight bytes: it shares [esp+0x60] with rec, the garrison index and the capital-battle temporary
            ((MapWidget*)view)->project((rgl::vector4*)out, &pos);
            float screen[2];
            screen[0] = out[0];
            screen[1] = out[1];
        g_gui.set_view(screen);      // 0x4c5190
        }
    }

    g_sel_type = type;
    g_sel_id   = id;
}
