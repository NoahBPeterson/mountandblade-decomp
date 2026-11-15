//-----------------------------------------------------------------------------
// File: exchange_screens.cpp  (0x531590..0x533a60, TU 119 party_3 + TU 120 screen; 0x533a60.. is list_screens.cpp)
//
// Desc: the screens at the end of the game code: the mission start menu (select / setup_entry / weight / update), the
//       party exchange window (per-frame update, grid selection, stack moves, row swap, compaction), the sprite
//       refresh of the party that is left, the key / menu event handler and the two scroll-list screens
//       (on_tick / on_input with their list helpers), the vector-iterator helper and the two item-source hooks.
//       "T regime" region (docs/tu_map.md): cl emitted these objects in source order, so the file keeps the binary
//       order and every member that another function of the file calls is __declspec(noinline) in exchange_screens.h.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "../../include/image_addrs.h"
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): listed directly (the resolution audit reads only the source's own includes)
#include "../game_globals.h"   // g_trial_mode (canonical)
#include <stdlib.h>
#include <math.h>
#include "./exchange_screens.h"
#include "../config/config_block.h"      // g_cfg (0x5a9f98); cheat_mode is +0x24 = 0x5a9fbc
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../options_panel/hud.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../strat/gui_manager.h"
#include "../ui/ui_common.h"
#include "../int_vec.h"
#include "../str_table.h"
#include "../../include/dialog_rec.h"
#include "../window_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../party_x.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission_source.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../menu_rec.h"   // g_menus 0x5e29b0
#include "../shared_vars.h"
#include "../key_names.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../slot.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// @ 0x531590
void Rec118::frame_update(float)
{
    int idx = -1;
    for (int i = 0; i < count; i++)
        if (((MissionUi*)(void*)row[i])->clear_mask(0x10000))
            idx = i;
    if (idx != -1)
    {
        g_mission_ctx.setup_entry(&entries[idx]);
        char b;
        g_map_flags.commit((char*)&b);
    }
}

// @ 0x531600
void MissionCtx::setup_entry(MenuEntry* e)
{
    type = e->id;
    menu_value = e->value;
    option = -1;
    mission_type* t = &g_mission_types[e->id];
    if (e->id <= 6)
    {
        setup(t->f10, 0);   // 0x513f80
        return;
    }
    int r = rand() % 100;
    int sum = 0;
    for (int i = 0; i < t->count; i++)
    {
        if (option != -1)
            break;
        sum += weight(t->list[i].flags);
        if (r < sum)
            option = i;
    }
    if (option != -1)
    {
        setup(t->list[option].setup_arg, 0);   // 0x513f80
        key_mask |= 0x10;
    }
    else
        key_mask |= 4;
    mission_flags |= 0x20;
}

static __forceinline float clamp01(float k) { if (k < 0.0f) return 0.0f; if (k >= 1.0f) return 1.0f; return k; }
// @ 0x5316f0
int MissionCtx::weight(unsigned flags)
{
    float f = (float)(int)(flags & 0x7f);
    if (flags & 0x100)
        f *= clamp01((float)(advantage + 0x20) * 0.04736842f - 0.4f);
    else if (flags & 0x200)
        f *= clamp01((float)(6 - advantage) * 0.04736842f - 0.4f);
    return (int)f;
}

// @ 0x531780
void Rec34::frame_update(float)
{
    g_gui.flush();
    if (w04 == 0)
    {
        char b;
        if (((MissionUi*)(void*)f30)->clear_mask(0x10000) || (g_keyboard.down[0x1c] == 0 && g_keyboard.prev[0x1c] != 0))
            g_map_flags.commit((char*)&b);
    }
    else
    {
        char b;
        if (((MissionUi*)(void*)f30)->clear_mask(0x10000) || (g_keyboard.down[0x1c] == 0 && g_keyboard.prev[0x1c] != 0))
            g_map_flags.commit((char*)&b);
    }
}

// @ 0x531800
void PartyScreen::frame_update(float)
{
    int extra = 0;
    int slot;
    int pid;
    int cmp_party;
    int count;
    int flag;
    int out;
    int i;
    int w14;
    int v;
    bool c48;
    bool c4c;
    bool c54;
    bool c58;
    int rows;
    int r;
    int c;
    bool found;
    int limit;
    int off;
    int j;
    int nfree;
    int old10;
    int rnd;
    bool overflow;
    int tmp[8];                             // remove_row's 0x20-byte out record
    Stack* sel;
    Troop* tsel;
    int a;
    int b;
    int action;
    char commit_out[4];

    if (skills_list != 0 && ((TargetW*)skills_list)->screen[1] < skills_base->screen[1]) {
        if (g_gui.mouse[0] < 0.5f) {
            ((TargetW*)skills_list)->src->on_value_changed((float)g_gui.wheel);
            ((TargetW*)skills_list)->src->set_key_flags(0x10000);
            goto after_scroll;
        }
    } else {
        if (g_gui.mouse[0] < 0.5f && f8c8 == 2)
            extra = 1;
    }

    slot = 0x26 + extra * 2;
    if (g_gui.mouse[1] < 0.33f)
        slot += 1;
    if (g_gui.wheel != 0) {
        TargetW** pt = &((TargetW**)this)[slot];
        ValueSource* s = (*pt)->src;
        float* pe = &s->slot[s->idx];
        float ratio = (s->f168 - s->f16c) / (s->f170 - s->f16c);
        float fv = (*pe - s->step) * ratio;
        float t = (float)g_gui.wheel / *pe;
        t = t * s->step;
        t = t * 0.0005f;
        fv += t;
        s->set_value(fv);
        (*pt)->src->set_key_flags(0x10000);
    }

after_scroll:
    if (sel_row >= 0 && (g_keyboard.down[0x1d] || g_keyboard.down[0x9d]) && g_keyboard.down[0x2c] &&
        !g_keyboard.prev[0x2d] && g_cfg.cheat_mode) {
        WL_MARKER(12)
        if (g_trial_mode) {
            slot = ((int*)slots)[(sel_slot + sel_type * 2) * 0x40 + sel_row];
            pid = sel_type != 0 ? w10 : g_game->main_party;
            // the original pushes the three words once: 0x51ee50 (ret 4) consumes `pid`, the two left on the stack are add_xp_to_stack's (ret 8)
            Party* p = party_mgr()->get(pid);   // 0x51ee50
            p->add_xp_to_stack(slot, 0xd2);                              // 0x4b1d80
            ((PartyScreen*)(void*)this)->refresh_selection();
        }
    }
    WL_MARKER(13)

    rows = 2;
    if (w04 == 0 || w04 == 5)
        rows = 1;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < 2; c++) {
            for (i = 0; i < 64; i++) {
                if (buttons[r * 2 + c][i]->clear_mask(0x10000))
                    set_selection(r, c, i);
            }
        }
    }

    if (action_btn->clear_mask(0x10000)) {
        slot = ((int*)slots)[(sel_slot + sel_type * 2) * 0x40 + sel_row];
        pid = sel_type != 0 ? w10 : g_game->main_party;
        cmp_party = sel_type != 0 ? g_game->main_party : w10;
        count = 1;
        if (g_keyboard.down[0x1d] || g_keyboard.down[0x9d])
            count = party_index(pid)->stacks[slot].count;
        flag = 0;
        for (i = 0; i < count; i++) {
            if (action_btn->f104 & 3)
                break;
            w14 = party_index(pid)->stacks[slot].troop;
            v = 0;
            if (w04 == 3) {
                v = g_game->sl_party_price(w14);
            } else if (w04 == 2 && w14 >= 0) {
                g_script.trigger_result.store0 = 0;
                g_script.trigger_result.store1 = 0;
                bool ok = g_game->emit_event(10, w14, g_game->main_party);   // 0x435790
                v = g_script.trigger_result.store0;
                if (!ok)
                    v = g_game->f13a20;
            }
            if (cmp_party == g_game->player)
                v = -v;
            if (w04 == 3 || w04 == 2)
                move_men(v);
            stack_remove(pid, slot, cmp_party, &out, &flag);
            ((PartyScreen*)(void*)this)->refresh_selection();
            if (flag)
                clear_selection();
            ((PartyScreen*)(void*)this)->refresh_party_members(0);
            ((PartyScreen*)(void*)this)->refresh_party_members(1);
        }
        ((PartyScreen*)(void*)this)->refresh_selection();
        if (flag)
            clear_selection();
        ((PartyScreen*)(void*)this)->refresh_party_members(0);
        ((PartyScreen*)(void*)this)->refresh_party_members(1);
        ((PartyScreen*)(void*)this)->refresh_morale();
    }

    c48 = upgrade1->clear_mask(0x10000);
    c4c = upgrade2->clear_mask(0x10000);
    if (c48 || c4c) {
        slot = ((int*)slots)[(sel_slot + sel_type * 2) * 0x40 + sel_row];
        pid = sel_type != 0 ? w10 : g_game->main_party;
        sel = &party_index(pid)->stacks[slot];
        tsel = &g_game->troops[sel->troop];
        found = false;
        limit = 1;
        off = 0;
        j = 0;
        overflow = false;

        for (;;) {
            PartyX* p = party_index(pid);
            int n = p->nstacks;
            int k = 0;
            nfree = 0;
            if (n > 0) {
                char* pf = (char*)p + 0xb0;
                do {
                    if (*pf & 1)
                        break;
                    nfree++;
                    k++;
                    pf += 0x20;
                } while (k < n);
            }
            if (j >= nfree)
                break;

            if (c48) {
                if (*(int*)((char*)party_index(pid) + off + 0xac) == tsel->upgrade_troop[0])
                    found = true;
            }
            if (c4c) {
                if (*(int*)((char*)party_index(pid) + off + 0xac) == second_upgrade(tsel))
                    found = true;
            }
            off += 0x20;
            j++;
        }

        if ((sel->flags & 1) || sel->f14 < sel->count) {
            if (found)
                goto overflow_path;
            goto merge;
        } else {
            if (found)
                goto overflow_path;
            {
                PartyX* p = party_index(pid);
                int n = p->nstacks;
                int k = 0;
                if (n > 0) {
                    char* pf = (char*)p + 0xb0;
                    do {
                        if (*pf & 1)
                            break;
                        k++;
                        pf += 0x20;
                    } while (k < n);
                }
                if (k == ((Party*)party_index(pid))->max_stacks()) {
                    limit = sel->f14;
                    if (limit > sel->count)
                        limit = sel->count;
                }
            }
        }

merge:
        if ((sel->flags & 1) || limit < sel->count)
            goto overflow_path;
        if (c48)
            sel->troop = tsel->upgrade_troop[0];
        else
            sel->troop = second_upgrade(tsel);
        sel->f14 = 0;
        sel->f18 = 0;
        goto tail;

overflow_path:
        if (limit >= sel->count)
            overflow = true;
        int base = tsel->upgrade_troop[0];
        if (c4c)
            base = second_upgrade(tsel);
        for (int k = 0; k < limit; k++) {
            if (locked)
                break;
            sel->f14--;
            PartyX* p = party_index(pid);
            old10 = p->stacks[slot].wounded;
            rnd = rand();
            p = party_index(pid);
            if (p->stacks[slot].count < 0x20)
                rnd = g_game->tbl[slot][party_index(g_game->main_party)->stacks[slot].count - 1];
            p = party_index(pid);
            int cc = p->stacks[slot].count - 1;
            p->stacks[slot].count = cc;
            if (cc < p->stacks[slot].wounded)
                p->stacks[slot].wounded = cc;
            if (cc < p->stacks[slot].f14)
                p->stacks[slot].f14 = cc;
            if (p->stacks[slot].count < 1)
                p->remove_row(tmp, slot, g_game->tbl[0]);
            p->dirty = 1;
            p = party_index(pid);
            int after = p->stacks[slot].wounded;
            p = party_index(pid);
            int ni = p->add_members(base, 1, 0, 0, 1);
            p = party_index(pid);
            if (after < old10)
                p->stacks[ni].wounded++;
            p = party_index(pid);
            if (p->stacks[ni].count < 0x20)
                g_game->tbl[ni][p->stacks[ni].count - 1] = rnd;
            ((PartyScreen*)(void*)this)->refresh_selection();
        }
        if (overflow)
            clear_selection();

tail:
        ((PartyScreen*)(void*)this)->refresh_selection();
        ((PartyScreen*)(void*)this)->refresh_party_members(0);
        ((PartyScreen*)(void*)this)->refresh_morale();
    }

    c54 = move_up->clear_mask(0x10000);
    c58 = move_down->clear_mask(0x10000);
    if (c54 || c58) {
        int d = c54 ? -1 : 1;
        swap_rows(d);
        set_selection(sel_type, sel_slot, sel_row + d);
        ((PartyScreen*)(void*)this)->refresh_party_members(0);
        ((PartyScreen*)(void*)this)->refresh_selection();
    }

    if (!done_btn->clear_mask(0x10000) && (g_keyboard.down[0x1] != 0 || g_keyboard.prev[0x1] == 0)) {
        if (reset_btn->clear_mask(0x10000)) {
            apply(0);
            apply(1);
            clear_selection();
            ((PartyScreen*)(void*)this)->refresh_party_members(0);
            ((PartyScreen*)(void*)this)->refresh_party_members(1);
            ((PartyScreen*)(void*)this)->refresh_selection();
            ((PartyScreen*)(void*)this)->refresh_morale();
            return;
        }
        if (!talk_btn->clear_mask(0x10000))
            return;                             // nothing pressed: stay on the screen (0x532384 -> the epilogue)
        if (sel_row < 0)
            return;
        get_cell(sel_type, sel_slot, sel_row, &a, &b);
        action = 0xd;
        if (party_index(a)->stacks[b].flags & 1)
            action = 0xe;
        w14 = party_index(a)->stacks[b].troop;
        g_mission_ctx.build_hero_table(w14, g_game->tbl[b][0],
                    g_game->main_party, action);
        slot_as<int>(16)[6] = 0xc;
        g_windows.set_state(0x10);
        return;
    }

close_menu:
    if (w04 == 4 && g_mission_ctx.passage_menu < 1)
        gx()->update_party_sprites();
    if (w18 == 100) {
        g_map_flags.commit((char*)commit_out + 3);
        return;
    }
    g_windows.set_state(w18);
}

// @ 0x5324d0
void PartyScreen::clear_selection()
{
    if (sel_row != -1) {
        int idx = (sel_slot + sel_type * 2) * 0x40 + sel_row;
        MissionUi* w = ((MissionUi**)buttons)[idx];
        w->set15c(0);
    }
    sel_row = -1;
    sel_slot = -1;
    sel_type = -1;
}

// @ 0x532520
void PartyScreen::get_cell(int a, int b, int c, int* main, int* cell)
{
    *cell = ((int*)slots)[(b + a * 2) * 0x40 + c];
    *main = g_game->main_party;
    if (a != 0)
        *main = w10;
}

// @ 0x532560
void PartyScreen::move_men(int amount)
{
    g_game->troops[g_game->player].gold += amount;
    g_game->troops[w14].gold -= amount;
}

struct Row { int v[8]; };
typedef char assert_row[sizeof(Row) == 0x20 ? 1 : -1];

// @ 0x5325b0
void PartyScreen::apply(int which)
{
    GameData* g = g_game;
    int mainp = g->main_party;
    int owner = g->player;
    if (which != 0) {
        mainp = w10;
        if (w04 == 3 || w04 == 2) {
            owner = w14;
        } else {
            owner = g->parties.get(mainp)->stacks[0].troop;
        }
    }
    if (owner >= 0) {
        g->troops[owner].gold = owner_troop[which];
    }
    if (mainp >= 0) {
        party_index(g, mainp)->nstacks = nstacks[which];
        if (nstacks[which] > 0) {
            Stack* src = (Stack*)rows[which];
            int i = 0;
            do {
                party_at(party_mgr(), party_mgr()->lookup(mainp))->stacks[i] = *src;     // party_index without its named idx: the page is added before the row offset
                i++;
                src++;
            } while (i < nstacks[which]);
        }
    }
    if (which == 0) {
        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 32; j++)
                g_game->tbl[i][j] = table[i][j];
    }
}

// @ 0x532740
void PartyScreen::swap_rows(int which)
{
    int slot = ((int*)slots)[(sel_slot + sel_type * 2) * 0x40 + sel_row];
    int owner = g_game->main_party;
    if (sel_type != 0)
        owner = w10;
    PartyX* p = party_index(owner);

    int other = slot + which;
    Stack tmp = p->stacks[other];
    p->stacks[other] = p->stacks[slot];
    p->stacks[slot] = tmp;

    for (int i = 0; i < 0x20; i++) {
        int t = g_game->tbl[other][i];
        g_game->tbl[other][i] = g_game->tbl[slot][i];
        g_game->tbl[slot][i] = t;
    }
}

// @ 0x532830
void PartyScreen::stack_remove(int party_id, int slot, int dst_id, int* was_present, int* removed)
{
    int* table;
    Row local;
    int dummy[8];
    int off = slot * 0x20;
    int pid = party_id;
    local = *(Row*)((char*)party_index(pid) + 0xa8 + off);

    *was_present = *(int*)((char*)party_index(pid) + 0xa8 + off + 0xc) > 0;
    if (*was_present)
        (*(int*)((char*)party_index(pid) + 0xa8 + off + 0xc))--;
    bool had = *(int*)((char*)party_index(pid) + 0xa8 + off + 0x10) > 0;
    if (had)
        (*(int*)((char*)party_index(pid) + 0xa8 + off + 0x10))--;
    *removed = 0;
    (*(int*)((char*)party_index(pid) + 0xa8 + off))--;
    if (*(int*)((char*)party_index(pid) + 0xa8 + off) < 1) {        *removed = 1;
        table = (pid == g_game->main_party)
              ? (int*)((char*)g_game + 0x1405c) : 0;
        party_index(pid)->remove_row(dummy, slot, table);
    }

    int found = ((Party*)party_index(dst_id))->find_or_add(local.v[1], local.v[2], 1);
    if (found == party_index(dst_id)->nstacks) {
        int* row = (int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20);
        row[0] = 0;
        row[2] = 0;
        row[5] = 0;
        row[3] = 0;
        row[4] = 0;
        row[6] = 0;
        row[1] = -1;
        *(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20 + 4) = local.v[1];
        *(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20 + 8) = local.v[2];
        party_index(dst_id)->nstacks++;
    }
    if (dst_id == g_game->main_party) {
        if (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20) < 0x20) {
            int* row = (int*)((char*)party_index(g_game->main_party) + 0xa8 + found * 0x20);
            int rv = rand();
            ((int*)((char*)g_game + 0x1405c))[row[0] + found * 0x20] = rv;
        }
    }
    (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20))++;
    (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20 + 0xc)) += 1 - *was_present;
    if (had) {
        (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20 + 0x10))++;
    }
    party_index(dst_id)->compact();
    if ((local.v[2] & 1) == 0) {
        ((Party*)party_index(dst_id))->sum_stat(0x10);
    }
}

// @ 0x532db0
void PartyX::compact()
{
    int i = 1;
    if (nstacks > 1) {
        do {
            remove_slot(i);
            i++;
        } while (i < nstacks);
    }
    dirty = 1;
}

// @ 0x532de0
void GameDataX::update_party_sprites()
{
    WL_MARKER(12)
    PartyMgr* mgr = &parties;
    PartyX* p = party_index(this, *(int*)((char*)this + 0x7c));
    if (((Party*)p)->sum_stat(0x10) != 0) {
        int qi = mgr->lookup(main_party);
        Party* page = mgr->pages[qi >> 4];
        int spr = add_sprite(page[qi & 0xf].pos.x, page[qi & 0xf].pos.y, 1);
        PartyX* r = party_index(this, spr);
        int n = p->nstacks;
        int i = 0;
        if (n > 0) {
            int* row = (int*)((char*)p + 0xb0);
            do {
                if ((row[0] & 1) == 0)
                    r->add_members(row[-1], row[-2], row[0], 0, 0);
                i++;
                row += 8;
            } while (i < p->nstacks);
        }
    }
    int n = p->nstacks;
    int i = 0;
    if (n > 0) {
        int* row = (int*)((char*)p + 0xac);
        do {
            Troop* rec = &troops[row[0]];
            if ((rec->flags30 & 0x10) != 0) {
                int kind = 1;
                if (row[1] & 1)
                    kind = 2;
                int qi = mgr->lookup(main_party);
                Party* pp = mgr->pages[qi >> 4] + (qi & 0xf);
                int spr = add_sprite(pp->pos.x, pp->pos.y, kind);
                PartyX* d = party_index(this, spr);
                Troop* src = &troops[row[0]];
                tstring* s = STR_TABLE->find(&src->key);
                if (s == 0)
                    s = &src->name;
                d->name = *s;
                d->flag = 1;
                d->add_members(row[0], 1, 0, 0, 0);
            }
            i++;
            row += 8;
        } while (i < p->nstacks);
    }
    Party* e = mgr->get(*(int*)((char*)this + 0x7c));
    e->nstacks = 0;
    WL_MARKER(13)
}


// @ 0x533020
void PartyScreen::set_selection(int a, int b, int c)
{
    clear_selection();
    sel_type = a;
    sel_slot = b;
    sel_row = c;
    if (c != -1) {
        MissionUi* w = ((MissionUi**)buttons)[(b + a * 2) * 0x40 + c];
        w->set15c(1);
    }
    ((PartyScreen*)(void*)this)->refresh_selection();
}

static __forceinline float anim_value(ValueSource* vs)
{
    float* base = &vs->slot[vs->idx];
    float a = vs->f168 - vs->f16c;
    float b = vs->f170 - vs->f16c;
    return (a / b) * (base[0] - vs->step)
         + ((float)g_gui.wheel / base[0]) * vs->step * 0.0005f;
}


// @ 0x533070
void ItemA::frame_update(float)
{
    int slot = (int)this;
    src().pick(&slot);

    char* rec = (char*)g_menus + g_game->f13aec * 0x60;
    int local = -1;

    if (g_gui.wheel != 0) {
        if (g_gui.mouse[1] < 0.3f) {
            ValueSource* vs = ((TargetW*)slider)->src;
            vs->set_value(anim_value(vs));
            ValueSource* vs2 = ((TargetW*)slider)->src;
            vs2->set_key_flags(0x10000);
        } else {
            ValueSource* vs = ((TargetW*)tree)->src;
            vs->set_value(anim_value(vs));
            ValueSource* vs2 = ((TargetW*)tree)->src;
            vs2->set_key_flags(0x10000);
        }
    }

    // the key flags of the menu row (a 64-bit word: the original tests the low dword and ors the zero high dword)
    unsigned __int64 k = *(unsigned __int64*)(rec + 0x20);

#define CTRL ((k & 0x200) == 0 && (k & 0x100) != 0)
    if (CTRL && g_game_keys.clicked(0x10)) {
        slot_as<head_rec>(8)->w18 = 0x17;
        slot_as<head_rec>(8)->w04 = 0;
        g_windows.set_state(8);
    }
    else if (CTRL && g_game_keys.clicked(0x11)) {
        slot_as<head_rec>(12)->w18 = 0x17;
        slot_as<head_rec>(12)->w04 = 0;
        g_windows.set_state(0xc);
    }
    else if (CTRL && g_game_keys.clicked(0xf)) {
        slot_as<head_rec>(15)->w18 = 0x17;
        slot_as<head_rec>(15)->w04 = 0;
        slot_as<head_rec>(15)->w14 = g_game->player;
        g_windows.set_state(0xf);
    }
    else if (CTRL && g_game_keys.clicked(0x12)) {
        if (!(g_module_settings.show_quest_notes == 0)) {
            slot_as<head_rec>(26)->w18 = 0x17;
            g_game->f13b08 = 4;
            g_game->f13b0c = 0;
            g_windows.set_state(0x1a);
        } else {
            slot_as<head_rec>(13)->w18 = 0x17;
            slot_as<head_rec>(13)->w04 = 0;
            g_windows.set_state(0xd);
        }
    }
    else if ((k & 0x200) == 0 && g_game_keys.clicked(0x13)) {
        slot_as<head_rec>(25)->w18 = 0x17;
        slot_as<head_rec>(25)->w04 = 0;
        g_windows.set_state(0x19);
    }
    else {
        // no hotkey: pick the row's option by key flag 0x10 (the first option) or by the pressed option widget
        // (0x5331a9..0x533409: both paths leave through the state test below)
        if (!((k & 0x10) == 0)) {
            local = 0;
            if (count > 0)
                goto emit_option;
        } else {
            if (g_game->mode == 0 && count > 0) {
                int i = 0;
                MissionUi** ws = kids;
                do {
                    if (ws[i]->clear_mask(0x10000))
                        local = i;
                    i++;
                } while (i < count);
                if (local >= 0)
                    goto emit_option;
            }
        }
    }

    // after a hotkey or without a pick: nothing more to do when no command is queued
    if (g_game->mode == 0)
        return;
    goto dispatch;

emit_option:
    {
        int sel = ids[local];
        g_game->f13af8 = 0;
        ((DialogBlock*)(*(char**)(rec + 0x58) + sel * 0x68 + 0x4c))->play(8);
    }

dispatch:
    if (g_game == 0)
        return;
    {
        int st = g_game->mode;
        if (st == 0) {
            if (w18 != 100)
                return;
            g_mission_ctx.key_mask |= 0x400;
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 1) {
            g_game->mode = 0;
            if (w18 == 100) {
                GI(0xa8) |= 1;
            } else {
                g_mission_ctx.update_parties();
                g_windows.set_state(w18);
                return;
            }
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 0x11) {
            g_game->mode = 0;
            g_mission_ctx.update_parties();
            g_windows.set_state(7);
            return;
        }
        if (st == 0xd) {
            g_mission_ctx.key_mask |= 8;
            g_game->mode = 0;
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 2) {
            g_game->mode = 0;
            g_mission_ctx.key_mask |= 0x80;
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 0xe) {
            g_game->mode = 0;
            g_mission_ctx.key_mask |= 0x800;
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 0xf) {
            g_game->mode = 0;
            if (w18 != 100) {
                slot_as<head_rec>(27)->w18 = 0x17;
                g_windows.set_state(0x1b);
                return;
            }
            g_mission_ctx.key_mask |= 0x1000;
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 0x10) {
            g_game->mode = 0;
            if (w18 != 100) {
                slot_as<head_rec>(26)->w18 = 0x17;
                g_windows.set_state(0x1a);
                return;
            }
            g_mission_ctx.key_mask |= 0x2000;
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 0x12) {
            g_game->mode = 0;
            g_game->f13af8 = 0;
            if (w18 != 100) {
                g_windows.set_state(0x17);
                return;
            }
            g_mission_ctx.key_mask |= 0x400;
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 4) {
            slot_as<head_rec>(8)->w18 = 0x17;
            slot_as<head_rec>(8)->w14 = g_game->f13af0;
            slot_as<head_rec>(8)->w04 = 2;
            g_windows.set_state(8);
            return;
        }
        if (st == 8) {
            g_game->mode = 0;
            g_mission_ctx.key_mask |= 0x100;
            g_map_flags.commit((char*)&slot);
            return;
        }
        if (st == 5 || st == 7 || st == 6) {
            slot_as<head_rec>(12)->w18 = 0x17;
            slot_as<head_rec>(12)->w10 = GI(0x13b00);
            int v = 0;
            if (st == 5)
                v = 1;
            if (st == 7)
                v = st;
            else if (st == 6)
                v = st;
            else if (st == 9)
                v = 2;
            else if (st == 10)
                v = 3;
            slot_as<head_rec>(12)->w04 = v;
            g_windows.set_state(0xc);
            g_game->mode = 0;
            return;
        }
    }
#undef CTRL
}

// @ 0x5337a0
void Rec58::frame_update(float)
{
    if (tab_log->clear_mask(0x10000))
        build(1 != 0);
    if (tab_recent->clear_mask(0x10000))
        build(0 != 0);

    if (g_gui.wheel != 0) {
        ValueSource* vs = ((TargetW*)slider)->src;
        vs->set_value(anim_value(vs));
        ValueSource* vs2 = ((TargetW*)slider)->src;
        vs2->set_key_flags(0x10000);
    }

    if (log->clear_mask(0x10000)) {
        int a, b, c;
        static_cast<ChoiceList*>(log)->get3(&a, &b, &c);
        slot_as<head_rec>(26)->w18 = slot_as<head_rec>(25)->w18;
        g_game->f13b08 = a;
        g_game->f13b0c = b;
        g_windows.set_state(0x1a);
        return;
    }
    if (tab_troops->clear_mask(0x10000)) {
        slot_as<head_rec>(26)->w18 = slot_as<head_rec>(25)->w18;
        g_game->f13b08 = 1;
        g_game->f13b0c = 0;
        g_windows.set_state(0x1a);
        return;
    }
    if (tab_locations->clear_mask(0x10000)) {
        slot_as<head_rec>(26)->w18 = slot_as<head_rec>(25)->w18;
        g_game->f13b08 = 3;
        g_game->f13b0c = 0;
        g_windows.set_state(0x1a);
        return;
    }
    if (tab_factions->clear_mask(0x10000)) {
        slot_as<head_rec>(26)->w18 = slot_as<head_rec>(25)->w18;
        g_game->f13b08 = 2;
        g_game->f13b0c = 0;
        g_windows.set_state(0x1a);
        return;
    }
    if (tab_quests->clear_mask(0x10000)) {
        slot_as<head_rec>(26)->w18 = slot_as<head_rec>(25)->w18;
        g_game->f13b08 = 4;
        g_game->f13b0c = 0;
        g_windows.set_state(0x1a);
        return;
    }
    if (tab_return->clear_mask(0x10000) || (g_keyboard.down[0x1] == 0 && g_keyboard.prev[0x1] != 0)) {
        if (w18 == 100) {
            char slot;
            g_map_flags.commit((char*)&slot);
            return;
        }
        g_windows.set_state(w18);
    }
}
