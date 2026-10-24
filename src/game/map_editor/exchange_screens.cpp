//-----------------------------------------------------------------------------
// File: exchange_screens.cpp  (0x531590..0x5342a6, TU 119 party_3 + TU 120 screen)
//
// Desc: the screens at the end of the game code: the mission start menu (select / setup_entry / weight / update), the
//       party exchange window (per-frame update, grid selection, stack moves, row swap, compaction), the sprite
//       refresh of the party that is left, the key / menu event handler and the two scroll-list screens
//       (on_tick / on_input with their list helpers), the vector-iterator helper and the two item-source hooks.
//       "T regime" region (docs/tu_map.md): cl emitted these objects in source order, so the file keeps the binary
//       order and every member that another function of the file calls is __declspec(noinline) in exchange_screens.h.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <math.h>
#include "./exchange_screens.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../options_panel/hud.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/msg_log.h"
#include "../ui/ui_common.h"
#include "../int_vec.h"
#include "../str_table.h"
#include "../../include/dialog_rec.h"
#include "../mission_source.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)

#define F(p, off, type) (*(type*)((char*)(p) + (off)))
#define W(p, off) (*(ListWidget**)((char*)(p) + (off)))
#define V(p, off) ((OutIntVec*)((char*)(p) + (off)))
#define GIF(off, type) (*(type*)((char*)g_game + (off)))
#define STACK_IDS ((int*)((char*)g_game + 0x14058))   // GameData +0x14058: the per-stack random ids (0x20 per stack); +0x1405c is the tbl of game_data.h

// @ 0x531590
void StartMenu::select(int arg)
{
    int idx = -1;
    for (int i = 0; i < count; i++)
        if (buttons[i]->changed(0x10000))
            idx = i;
    if (idx != -1)
    {
        g_mui.setup_entry(&entries[idx]);
        char b;
        g_flags.commit(&b);
    }
}

// @ 0x531600
void MenuCtl::setup_entry(MenuEntry* e)
{
    f2c = e->id;
    f30 = e->value;
    f160 = -1;
    TplRec* t = &g_templates[e->id];
    if (e->id <= 6)
    {
        setup(t->f10, 0);
        return;
    }
    int r = rand() % 100;
    int sum = 0;
    for (int i = 0; i < t->count; i++)
    {
        if (f160 != -1)
            break;
        sum += weight(t->list[i].flags);
        if (r < sum)
            f160 = i;
    }
    if (f160 != -1)
    {
        setup(t->list[f160].setup_arg, 0);
        f3c |= 0x10;
    }
    else
        f3c |= 4;
    f10 |= 0x20;
}

// @ 0x5316f0
int MenuCtl::weight(unsigned flags)
{
    float f = (float)(int)(flags & 0x7f);
    float x;
    if (flags & 0x100)
        x = (float)(f1c + 0x20);
    else if (flags & 0x200)
        x = (float)(6 - f1c);
    else
        return (int)f;
    float k = x * g_k59c48c - 0.4f;
    if (k < 0.0f)
        k = 0.0f;
    else if (k >= 1.0f)
        k = 1.0f;
    f *= k;
    return (int)f;
}

// @ 0x531780
void ContinueMenu::update(int arg)
{
    g_msglog.flush();
    if (f4 == 0)
    {
        char b;
        if (buttons[0]->changed(0x10000) || (g_a17144 == 0 && g_a17544 != 0))
            g_flags.commit(&b);
    }
    else
    {
        char b;
        if (buttons[0]->changed(0x10000) || (g_a17144 == 0 && g_a17544 != 0))
            g_flags.commit(&b);
    }
}

// @ 0x531800
void XferWin::update(int)
{
    int extra = 0;
    int slot;
    int pid;
    int cmp_party;
    int count;
    int flag;
    int out;
    int i;
    int troop;
    int v;
    bool c48;
    bool c4c;
    bool c54;
    bool c58;
    int rows;
    int r;
    int c;
    int found;
    int limit;
    int off;
    int j;
    int nfree;
    int old10;
    int rnd;
    bool overflow;
    int tmp[2];
    Stack* sel;
    Troop* tsel;
    int a;
    int b;
    int action;
    char commit_out[4];

    if (f94 != 0 && f94->f_f4 < f90->f_f4) {
        if (g_mouse_x < g_half) {
            f94->src->on_value_changed((float)g_mission_clock);
            f94->src->set_key_flags(0x10000);
            goto after_scroll;
        }
    } else {
        if (g_mouse_x < g_half && f8c8 == 2)
            extra = 1;
    }

    slot = 0x26 + extra * 2;
    if (g_mouse_y < g_033)
        slot += 1;
    if (g_mission_clock != 0) {
        TargetW** pt = &((TargetW**)this)[slot];
        ValueSource* s = (*pt)->src;
        float* pe = &s->slot[s->idx];
        float ratio = (s->f168 - s->f16c) / (s->f170 - s->f16c);
        float fv = (*pe - s->step) * ratio;
        float t = (float)g_mission_clock / *pe;
        t = t * s->step;
        t = t * g_char_scale;
        fv += t;
        s->set_value(fv);
        (*pt)->src->set_key_flags(0x10000);
    }

after_scroll:
    if (f8e4 >= 0 && (g_a17148 || g_a17348) && g_a17188 &&
        !g_a17588 && g_a5a9fbc && g_trial_mode) {
        slot = slots[(f8e0 + f8dc * 2) * 0x40 + f8e4];
        pid = f8dc != 0 ? sel_party : g_game->f78;
        PartyX* p = party_mgr()->get3(pid, slot, 0xd2);
        p->fn_4b1d80();
        refresh_party_window();
    }

    rows = 2;
    if (state == 0 || state == 5)
        rows = 1;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < 2; c++) {
            for (i = 0; i < 64; i++) {
                if (grid[r][c][i]->changed(0x10000))
                    set_selection(r, c, i);
            }
        }
    }

    if (w50->changed(0x10000)) {
        slot = slots[(f8e0 + f8dc * 2) * 0x40 + f8e4];
        pid = f8dc != 0 ? sel_party : g_game->f78;
        cmp_party = f8dc != 0 ? g_game->f78 : sel_party;
        count = 1;
        if (g_a17148 || g_a17348)
            count = party_index(pid)->stacks[slot].count;
        flag = 0;
        for (i = 0; i < count; i++) {
            if (w50->f104 & 3)
                break;
            troop = party_index(pid)->stacks[slot].troop;
            v = 0;
            if (state == 3) {
                v = gx()->fn_48a320(troop);
            } else if (state == 2 && troop >= 0) {
                g_a70458 = 0;
                g_a7045c = 0;
                bool ok = g_game->emit_event(10, troop, g_game->f78);
                v = g_a70458;
                if (!ok)
                    v = GI(0x13a20);
            }
            if (cmp_party == g_game->player)
                v = -v;
            if (state == 3 || state == 2)
                move_men(v);
            stack_remove(pid, slot, cmp_party, &out, &flag);
            refresh_party_window();
            if (flag)
                clear_selection();
            fn_503ed0(0);
            fn_503ed0(1);
        }
        refresh_party_window();
        if (flag)
            clear_selection();
        fn_503ed0(0);
        fn_503ed0(1);
        fn_503a50();
    }

    c48 = w48->changed(0x10000);
    c4c = w4c->changed(0x10000);
    if (c48 || c4c) {
        slot = slots[(f8e0 + f8dc * 2) * 0x40 + f8e4];
        pid = f8dc != 0 ? sel_party : g_game->f78;
        sel = &party_index(pid)->stacks[slot];
        tsel = &g_game->troops[sel->troop];
        found = 0;
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
                if (*(int*)((char*)party_index(pid) + off + 0xac) == tsel->f50)
                    found = 1;
            }
            if (c4c) {
                if (*(int*)((char*)party_index(pid) + off + 0xac) == troop_f54(tsel))
                    found = 1;
            }
            off += 0x20;
            j++;
        }

        if ((sel->flags & 1) || sel->f14 < sel->count) {
            if (!found)
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
                if (k == party_index(pid)->ext_4b1700()) {
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
            sel->troop = tsel->f50;
        else
            sel->troop = troop_f54(tsel);
        sel->f14 = 0;
        sel->f18 = 0;
        goto tail;

overflow_path:
        if (limit >= sel->count)
            overflow = true;
        int base = tsel->f50;
        if (c4c)
            base = troop_f54(tsel);
        for (int k = 0; k < limit; k++) {
            if (f3908)
                break;
            sel->f14--;
            PartyX* p = party_index(pid);
            old10 = p->stacks[slot].f10;
            rnd = rand();
            p = party_index(pid);
            if (p->stacks[slot].count < 0x20)
                rnd = STACK_IDS[party_index(g_game->f78)->stacks[slot].count +
                                        slot * 0x20];
            p = party_index(pid);
            int cc = p->stacks[slot].count - 1;
            p->stacks[slot].count = cc;
            if (cc < p->stacks[slot].f10)
                p->stacks[slot].f10 = cc;
            if (cc < p->stacks[slot].f14)
                p->stacks[slot].f14 = cc;
            if (p->stacks[slot].count < 1)
                p->remove_row(tmp, slot, &STACK_IDS[1]);
            p->dirty = 1;
            p = party_index(pid);
            int after = p->stacks[slot].f10;
            p = party_index(pid);
            int ni = p->add_row(base, 1, 0, 0, 1);
            p = party_index(pid);
            if (after < old10)
                p->stacks[ni].f10++;
            p = party_index(pid);
            if (p->stacks[ni].count < 0x20)
                STACK_IDS[ni * 0x20 + p->stacks[ni].count] = rnd;
            refresh_party_window();
        }
        if (overflow)
            clear_selection();

tail:
        refresh_party_window();
        fn_503ed0(0);
        fn_503a50();
    }

    c54 = w54->changed(0x10000);
    c58 = w58->changed(0x10000);
    if (c54 || c58) {
        int d = c54 ? -1 : 1;
        swap_rows(d);
        set_selection(f8dc, f8e0, f8e4 + d);
        fn_503ed0(0);
        refresh_party_window();
    }

    if (!w30->changed(0x10000) && (g_a170d8 != 0 || g_a174d8 == 0)) {
        if (w34->changed(0x10000)) {
            apply(0);
            apply(1);
            clear_selection();
            fn_503ed0(0);
            fn_503ed0(1);
            refresh_party_window();
            fn_503a50();
            return;
        }
        if (!w44->changed(0x10000))
            goto close_menu;
        if (f8e4 < 0)
            return;
        get_cell(f8dc, f8e0, f8e4, &a, &b);
        action = 0xd;
        if (party_index(a)->stacks[b].flags & 1)
            action = 0xe;
        troop = party_index(a)->stacks[b].troop;
        g_mui.show_item(troop, STACK_IDS[1 + b * 0x80],
                    g_game->f78, action);
        g_menu_data[6] = 0xc;
        g_main_menu.choose(0x10);
        return;
    }

close_menu:
    if (state == 4 && g_a735c4 < 1)
        gx()->update_party_sprites();
    if (f18 == 100) {
        g_flags.commit(commit_out + 3);
        return;
    }
    g_main_menu.choose(f18);
}

// @ 0x5324d0
void XferWin::clear_selection()
{
    if (f8e4 != -1) {
        int idx = (f8e0 + f8dc * 2) * 0x40 + f8e4;
        Widget* w = ((Widget**)grid)[idx];
        w->activate(0);
    }
    f8e4 = -1;
    f8e0 = -1;
    f8dc = -1;
}

// @ 0x532520
void XferWin::get_cell(int a, int b, int c, int* main, int* cell)
{
    *cell = slots[(b + a * 2) * 0x40 + c];
    *main = g_game->f78;
    if (a != 0)
        *main = sel_party;
}

// @ 0x532560
void XferWin::move_men(int amount)
{
    g_game->troops[g_game->player].f3b4 += amount;
    g_game->troops[troop].f3b4 -= amount;
}

struct Row { int v[8]; };
typedef char assert_row[sizeof(Row) == 0x20 ? 1 : -1];

// @ 0x5325b0
void XferWin::apply(int which)
{
    GameData* g = g_game;
    int mainp = g->f78;
    int owner = g->player;
    if (which != 0) {
        mainp = sel_party;
        if (state == 3 || state == 2) {
            owner = troop;
        } else {
            owner = party_mgr()->get(mainp)->stacks[0].troop;
        }
    }
    if (owner >= 0) {
        g->troops[owner].f3b4 = ((int*)((char*)this + 0x18f0))[which];
    }
    if (mainp >= 0) {
        party_index(mainp)->nstacks = ((int*)((char*)this + 0x18e8))[which];
        if (((int*)((char*)this + 0x18e8))[which] > 0) {
            Stack* src = (Stack*)((char*)this + 0x8e8 + which * 0x800);
            int i = 0;
            do {
                party_index(mainp)->stacks[i] = *src;
                i++;
                src++;
            } while (i < ((int*)((char*)this + 0x18e8))[which]);
        }
    }
    if (which == 0) {
        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 32; j++)
                ((int*)((char*)g_game + 0x1405c))[i * 32 + j] =
                ((int*)((char*)this + 0x1908))[i * 32 + j];
    }
}

// @ 0x532740
void XferWin::swap_rows(int which)
{
    int slot = slots[(f8e0 + f8dc * 2) * 0x40 + f8e4];
    int owner = g_game->f78;
    if (f8dc != 0)
        owner = sel_party;
    PartyX* p = party_index(owner);

    Stack tmp = p->stacks[slot + which];
    p->stacks[slot + which] = p->stacks[slot];
    p->stacks[slot] = tmp;

    int o1 = 0x1405c + slot * 0x80;
    int o2 = 0x1405c + (slot + which) * 0x80;
    for (int i = 0; i < 0x20; i++) {
        int t = *(int*)((char*)g_game + o2);
        *(int*)((char*)g_game + o2) = *(int*)((char*)g_game + o1);
        *(int*)((char*)g_game + o1) = t;
        o1 += 4;
        o2 += 4;
    }
}

// @ 0x532830
void XferWin::stack_remove(int party_id, int slot, int dst_id, int* was_present, int* removed)
{
    int* table;
    Row local;
    int dummy[8];
    PartyMgrX* m = party_mgr();
    local = *(Row*)((char*)party_at(m, m->index_of(party_id)) + 0xa8 + slot * 0x20);

    *was_present = *(int*)((char*)party_at(m, m->index_of(party_id)) + 0xa8 + slot * 0x20 + 0xc) > 0;
    if (*was_present)
        (*(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20 + 0xc))--;
    bool had = *(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20 + 0x10) > 0;
    if (had)
        (*(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20 + 0x10))--;
    *removed = 0;
    (*(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20))--;
    if (*(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20) < 1) {        *removed = 1;
        table = (party_id == g_game->f78)
              ? (int*)((char*)g_game + 0x1405c) : 0;
        party_index(party_id)->remove_row(dummy, slot, table);
    }

    int found = party_index(dst_id)->find_row(local.v[1], local.v[2], 1);
    if (found == *(int*)((char*)party_index(dst_id) + 0x8a8)) {
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
        (*(int*)((char*)party_index(dst_id) + 0x8a8))++;
    }
    if (dst_id == g_game->f78) {
        if (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20) < 0x20) {
            int* row = (int*)((char*)party_index(g_game->f78) + 0xa8 + found * 0x20);
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
        party_index(dst_id)->total_value(0x10);
    }
}

// @ 0x532db0
void PartyX::compact()
{
    int i = 1;
    if (*(int*)((char*)this + 0x8a8) > 1) {
        do {
            remove_slot(i);
            i++;
        } while (i < *(int*)((char*)this + 0x8a8));
    }
    *(char*)((char*)this + 0x8b4) = 1;
}

// @ 0x532de0
void GameDataX::update_party_sprites()
{
    PartyMgrX* mgr = (PartyMgrX*)&parties;
    PartyX* p = party_index(this, *(int*)((char*)this + 0x7c));
    if (p->total_value(0x10) != 0) {
        PartyX* q = party_index(this, *(int*)((char*)this + 0x78));
        int spr = add_sprite(*(int*)((char*)q + 0x30), *(int*)((char*)q + 0x34), 1);
        PartyX* r = party_index(this, spr);
        int n = *(int*)((char*)p + 0x8a8);
        int i = 0;
        if (n > 0) {
            int* row = (int*)((char*)p + 0xb0);
            do {
                if ((row[0] & 1) == 0)
                    r->add_row(row[-1], row[-2], row[0], 0, 0);
                i++;
                row += 8;
            } while (i < *(int*)((char*)p + 0x8a8));
        }
    }
    int n = *(int*)((char*)p + 0x8a8);
    int i = 0;
    if (n > 0) {
        int* row = (int*)((char*)p + 0xac);
        do {
            Troop* rec = &troops[row[0]];
            if ((rec->flags32 & 0x10) != 0) {
                int kind = (row[1] & 1) ? 2 : 1;
                PartyX* q = party_index(this, *(int*)((char*)this + 0x78));
                int spr = add_sprite(*(int*)((char*)q + 0x30), *(int*)((char*)q + 0x34), kind);
                PartyX* d = party_index(this, spr);
                Troop* src = &troops[row[0]];
                tstring* s = STR_TABLE->find((const tstring*)((char*)src + 0x20));
                if (s == 0)
                    s = (tstring*)src;
                ((tstring*)((char*)d + 0xc))->operator=(*s);
                *(char*)((char*)d + 0x1c) = 1;
                d->add_row(row[0], 1, 0, 0, 0);
            }
            i++;
            row += 8;
        } while (i < *(int*)((char*)p + 0x8a8));
    }
    Party* e = mgr->get(*(int*)((char*)this + 0x7c));
    *(int*)((char*)e + 0x8a8) = 0;
}


// @ 0x533020
void XferWin::set_selection(int a, int b, int c)
{
    clear_selection();
    f8dc = a;
    f8e0 = b;
    f8e4 = c;
    if (c != -1) {
        Widget* w = ((Widget**)grid)[(b + a * 2) * 0x40 + c];
        w->activate(1);
    }
    refresh_party_window();
}

static __forceinline float anim_value(ValueSource* vs)
{
    float* base = (float*)((char*)vs + 0xf0 + F(vs, 0x178, int) * 4);
    return ((float)g_mission_clock / base[0]) * F(vs, 0x17c, float) * g_char_scale
         + (base[0] - F(vs, 0x17c, float))
         * ((F(vs, 0x168, float) - F(vs, 0x16c, float))
          / (F(vs, 0x170, float) - F(vs, 0x16c, float)));
}

static __forceinline void set_anim(TickScreen* s, int push_off, int resize_off)
{
    OutIntVec* v = V(s, push_off);
    v->push_back_call(F(s, 0xa8, int));
    v->push_back_call(F(s, 0xac, int));
    OutIntVec* d = V(s, resize_off);
    unsigned n = d->_Myfirst ? (unsigned)(d->_Mylast - d->_Myfirst) : 0;
    d->resize(n - 2, 0);
}

static __forceinline void clear_anim(TickScreen* s)
{
    if (F(s, 0x7c, void*) != 0)
        operator delete(F(s, 0x7c, void*));
    F(s, 0x7c, void*) = 0;
    F(s, 0x80, void*) = 0;
    F(s, 0x84, void*) = 0;
    if (F(s, 0x8c, void*) != 0)
        operator delete(F(s, 0x8c, void*));
    F(s, 0x8c, void*) = 0;
    F(s, 0x90, void*) = 0;
    F(s, 0x94, void*) = 0;
}


// @ 0x533070
void EventScreen::on_event(int code)
{
    int slot = (int)this;
    ((MissionSource*)((char*)this + 0x30))->pick(&slot);

    char* rec = g_menu_rows + *(int*)((char*)g_game + 0x13aec) * 0x60;
    int local = -1;

    if (g_mission_clock != 0) {
        if (g_mouse_y < 0.3f) {
            ValueSource* vs = *(ValueSource**)((char*)F(this, 0xec, void*) + 0x484);
            vs->set_value(anim_value(vs));
            ValueSource* vs2 = *(ValueSource**)((char*)F(this, 0xec, void*) + 0x484);
            vs2->set_key_flags(0x10000);
        } else {
            ValueSource* vs = *(ValueSource**)((char*)F(this, 0xe8, void*) + 0x484);
            vs->set_value(anim_value(vs));
            ValueSource* vs2 = *(ValueSource**)((char*)F(this, 0xe8, void*) + 0x484);
            vs2->set_key_flags(0x10000);
        }
    }

    // the key flags of the menu row (a 64-bit word: the original tests the low dword and ors the zero high dword)
    unsigned __int64 k = *(unsigned __int64*)(rec + 0x20);

#define CTRL ((k & 0x200) == 0 && (k & 0x100) != 0)
    if (CTRL && g_a73730.down(0x10)) {
        g_a739a0->f18 = 0x17;
        g_a739a0->f4 = 0;
        g_main_menu.choose(8);
    }
    else if (CTRL && g_a73730.down(0x11)) {
        g_a739b0->f18 = 0x17;
        g_a739b0->f4 = 0;
        g_main_menu.choose(0xc);
    }
    else if (CTRL && g_a73730.down(0xf)) {
        g_a739bc->f18 = 0x17;
        g_a739bc->f4 = 0;
        g_a739bc->f14 = g_game->player;
        g_main_menu.choose(0xf);
    }
    else if (CTRL && g_a73730.down(0x12)) {
        if (g_a181b4 == 0) {
            g_a739b4->f18 = 0x17;
            g_a739b4->f4 = 0;
            g_main_menu.choose(0xd);
        } else {
            g_a739e8->f18 = 0x17;
            GI(0x13b08) = 4;
            GI(0x13b0c) = 0;
            g_main_menu.choose(0x1a);
        }
    }
    else if ((k & 0x200) == 0 && g_a73730.down(0x13)) {
        g_a739e4->f18 = 0x17;
        g_a739e4->f4 = 0;
        g_main_menu.choose(0x19);
    }
    else {
        // no hotkey: pick the row's option by key flag 0x10 (the first option) or by the pressed option widget
        // (0x5331a9..0x533409: both paths leave through the state test below)
        if ((k & 0x10) == 0) {
            if (g_game->fc8 == 0 && F(this, 0xa0, int) > 0) {
                int i = 0;
                ListWidget** ws = (ListWidget**)((char*)this + 0x60);
                do {
                    if (ws[i]->changed(0x10000))
                        local = i;
                    i++;
                } while (i < F(this, 0xa0, int));
                if (local >= 0)
                    goto emit_option;
            }
        } else {
            local = 0;
            if (F(this, 0xa0, int) > 0)
                goto emit_option;
        }
    }

    // after a hotkey or without a pick: nothing more to do when no command is queued
    if (g_game->fc8 == 0)
        return;
    goto dispatch;

emit_option:
    {
        int sel = ((int*)((char*)this + 0xa4))[local];
        GI(0x13af8) = 0;
        ((DialogBlock*)(*(char**)(rec + 0x58) + sel * 0x68 + 0x4c))->exec(8);
    }

dispatch:
    if (g_game == 0)
        return;
    {
        int st = g_game->fc8;
        if (st == 0) {
            if (F(this, 0x18, int) != 100)
                return;
            g_a7333c |= 0x400;
            g_flags.commit(&slot);
            return;
        }
        if (st == 1) {
            g_game->fc8 = 0;
            if (F(this, 0x18, int) == 100) {
                GI(0xa8) |= 1;
            } else {
                g_mui.reset();
                g_main_menu.choose(F(this, 0x18, int));
                return;
            }
            g_flags.commit(&slot);
            return;
        }
        if (st == 0x11) {
            g_game->fc8 = 0;
            g_mui.reset();
            g_main_menu.choose(7);
            return;
        }
        if (st == 0xd) {
            g_game->fc8 = 0;
            g_a7333c |= 8;
            g_flags.commit(&slot);
            return;
        }
        if (st == 2) {
            g_game->fc8 = 0;
            g_a7333c |= 0x80;
            g_flags.commit(&slot);
            return;
        }
        if (st == 0xe) {
            g_game->fc8 = 0;
            g_a7333c |= 0x800;
            g_flags.commit(&slot);
            return;
        }
        if (st == 0xf) {
            g_game->fc8 = 0;
            if (F(this, 0x18, int) != 100) {
                g_a739ec->f18 = 0x17;
                g_main_menu.choose(0x1b);
                return;
            }
            g_a7333c |= 0x1000;
            g_flags.commit(&slot);
            return;
        }
        if (st == 0x10) {
            g_game->fc8 = 0;
            if (F(this, 0x18, int) != 100) {
                g_a739e8->f18 = 0x17;
                g_main_menu.choose(0x1a);
                return;
            }
            g_a7333c |= 0x2000;
            g_flags.commit(&slot);
            return;
        }
        if (st == 0x12) {
            g_game->fc8 = 0;
            GI(0x13af8) = 0;
            if (F(this, 0x18, int) != 100) {
                g_main_menu.choose(0x17);
                return;
            }
            g_a7333c |= 0x400;
            g_flags.commit(&slot);
            return;
        }
        if (st == 4) {
            g_a739a0->f18 = 0x17;
            g_a739a0->f14 = GI(0x13af0);
            g_a739a0->f4 = 2;
            g_main_menu.choose(8);
            return;
        }
        if (st == 8) {
            g_game->fc8 = 0;
            g_a7333c |= 0x100;
            g_flags.commit(&slot);
            return;
        }
        if (st == 5 || st == 7 || st == 6) {
            g_a739b0->f18 = 0x17;
            g_a739b0->f10 = GI(0x13b00);
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
            g_a739b0->f4 = v;
            g_main_menu.choose(0xc);
            g_game->fc8 = 0;
            return;
        }
    }
#undef CTRL
}

// @ 0x5337a0
void TickScreen::on_tick(int code)
{
    if (W(this, 0x40)->changed(0x10000))
        method_4fc820(1);
    if (W(this, 0x44)->changed(0x10000))
        method_4fc820(0);

    if (g_mission_clock != 0) {
        ValueSource* vs = *(ValueSource**)((char*)W(this, 0x38) + 0x484);
        vs->set_value(anim_value(vs));
        ValueSource* vs2 = *(ValueSource**)((char*)W(this, 0x38) + 0x484);
        vs2->set_key_flags(0x10000);
    }

    if (W(this, 0x30)->changed(0x10000)) {
        int a, b, c;
        W(this, 0x30)->get3(&a, &b, &c);
        g_a739e8->f18 = g_a739e4->f18;
        GIF(0x13b08, int) = a;
        GIF(0x13b0c, int) = b;
        g_main_menu.choose(0x1a);
        return;
    }
    if (W(this, 0x48)->changed(0x10000)) {
        g_a739e8->f18 = g_a739e4->f18;
        GIF(0x13b08, int) = 1;
        GIF(0x13b0c, int) = 0;
        g_main_menu.choose(0x1a);
        return;
    }
    if (W(this, 0x4c)->changed(0x10000)) {
        g_a739e8->f18 = g_a739e4->f18;
        GIF(0x13b08, int) = 3;
        GIF(0x13b0c, int) = 0;
        g_main_menu.choose(0x1a);
        return;
    }
    if (W(this, 0x50)->changed(0x10000)) {
        g_a739e8->f18 = g_a739e4->f18;
        GIF(0x13b08, int) = 2;
        GIF(0x13b0c, int) = 0;
        g_main_menu.choose(0x1a);
        return;
    }
    if (W(this, 0x54)->changed(0x10000)) {
        g_a739e8->f18 = g_a739e4->f18;
        GIF(0x13b08, int) = 4;
        GIF(0x13b0c, int) = 0;
        g_main_menu.choose(0x1a);
        return;
    }
    if (W(this, 0x3c)->changed(0x10000) || (g_a170d8 == 0 && g_a174d8 != 0)) {
        if (F(this, 0x18, int) == 100) {
            char slot;
            g_flags.commit(&slot);
            return;
        }
        g_main_menu.choose(F(this, 0x18, int));
    }
}


// @ 0x533a60
void ListWidget::get3(int* a, int* b, int* c)
{
    int* p = F(this, 0x19c, int*);
    p += F(this, 0x1cc, int);
    *a = *p;
    p = F(this, 0x1ac, int*);
    p += F(this, 0x1cc, int);
    *b = *p;
    *c = F(this, 0x1cc, int);
}


// @ 0x533aa0
void InputScreen::on_input(int code)
{
    if (g_mission_clock != 0) {
        if (g_mouse_x < 0.7f
            && F(W(this, 0x4c), 0xf4, float) < F(W(this, 0x44), 0xf4, float)) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x4c) + 0x484);
            vs->on_value_changed((float)g_mission_clock);
            ValueSource* vs2 = *(ValueSource**)((char*)W(this, 0x4c) + 0x484);
            vs2->set_key_flags(0x10000);
        } else {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            vs->set_value(anim_value(vs));
            ValueSource* vs2 = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            vs2->set_key_flags(0x10000);
        }
    }

    if (W(this, 0x6c)->changed(0x10000)) {
        set_anim((TickScreen*)this, 0x88, 0x78);
        if (F(this, 0xa8, int) == F(this, 0x98, int)) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_value(vs);
        }
        refresh_anim(F(this, 0xa8, int), F(this, 0x9c, int));
    }

    if (W(this, 0x70)->changed(0x10000)) {
        set_anim((TickScreen*)this, 0x78, 0x88);
        if (F(this, 0xa8, int) == F(this, 0xa0, int)) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_value(vs);
        }
        refresh_anim(F(this, 0xa8, int), F(this, 0xa4, int));
    }

    if (W(this, 0x34)->changed(0x10000)) {
        ListWidget* w = W(this, 0x34);
        VecIter it;
        int* second;
        it.ptr = F(w, 0x19c, int*);
        second = 0;
        int** r = it.advance(&second, F(w, 0x1cc, int));
        int iv2 = **r;
        int iv1 = F(w, 0x1ac, int*)[F(w, 0x1cc, int)];
        if (F(this, 0x8c, void*) != 0)
            operator delete(F(this, 0x8c, void*));
        F(this, 0x8c, void*) = 0;
        F(this, 0x90, void*) = 0;
        F(this, 0x94, void*) = 0;
        if (F(this, 0xa8, int) >= 0
            && (F(this, 0xa8, int) != iv2 || F(this, 0xac, int) != iv1)) {
            V(this, 0x78)->push_back_call(F(this, 0xa8, int));
            V(this, 0x78)->push_back_call(F(this, 0xac, int));
        }
        if (F(this, 0xa8, int) == iv2) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_value(vs);
        }
        refresh_anim(iv2, iv1);
    }

    if (W(this, 0x3c)->changed(0x10000)) {
        ListWidget* w = W(this, 0x3c);
        VecIter it;
        int* second;
        it.ptr = F(w, 0x19c, int*);
        second = 0;
        int** r = it.advance(&second, F(w, 0x1cc, int));
        int iv2 = **r;
        int iv1 = F(w, 0x1ac, int*)[F(w, 0x1cc, int)];
        if (F(this, 0x8c, void*) != 0)
            operator delete(F(this, 0x8c, void*));
        F(this, 0x8c, void*) = 0;
        F(this, 0x90, void*) = 0;
        F(this, 0x94, void*) = 0;
        if (F(this, 0xa8, int) >= 0
            && (F(this, 0xa8, int) != iv2 || F(this, 0xac, int) != iv1)) {
            V(this, 0x78)->push_back_call(F(this, 0xa8, int));
            V(this, 0x78)->push_back_call(F(this, 0xac, int));
        }
        if (F(this, 0xa8, int) == iv2) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_value(vs);
        }
        refresh_anim(iv2, iv1);
    }

    if (W(this, 0x58)->changed(0x10000)) {
        clear_anim((TickScreen*)this);
        refresh_anim(1, 0);
    }
    if (W(this, 0x60)->changed(0x10000)) {
        clear_anim((TickScreen*)this);
        refresh_anim(2, 0);
    }
    if (W(this, 0x64)->changed(0x10000)) {
        clear_anim((TickScreen*)this);
        refresh_anim(3, 0);
    }
    if (W(this, 0x5c)->changed(0x10000)) {
        clear_anim((TickScreen*)this);
        refresh_anim(4, 0);
    }
    if (W(this, 0x50)->changed(0x10000)) {
        g_a739e4->f18 = g_a739e8->f18;
        g_a739e4->f4 = 1;
        g_main_menu.choose(0x19);
        return;
    }
    if (W(this, 0x74)->changed(0x10000)) {
        GIF(0x88, char) = 1;
        GIF(0x84, int) = F(this, 0xac, int);
        g_main_menu.choose(F(this, 0x18, int));
        return;
    }
    if (W(this, 0x54)->changed(0x10000)) {
        g_a739e4->f18 = g_a739e8->f18;
        g_a739e4->f4 = 0;
        g_main_menu.choose(0x19);
        return;
    }
    if (W(this, 0x68)->changed(0x10000) || (g_a170d8 == 0 && g_a174d8 != 0)) {
        if (F(this, 0x18, int) == 100) {
            char slot;
            g_flags.commit(&slot);
            return;
        }
        g_main_menu.choose(F(this, 0x18, int));
    }
}

// @ 0x534160
int** VecIter::advance(int** out, int n)
{
    *out = ptr + n;
    return out;
}

// @ 0x534180
void SourceScreen::hook_a(int arg)
{
    src.pick(&arg);
    if ((float)(g_ticks[src.slot] - *(__int64*)&src.tick_lo) * 1e-5f > interval) {
        int id = f18;
        if (id == 100) {
            g_flags.commit(&arg);
            return;
        }
        g_main_menu.choose(id);
        return;
    }
    if (GI(0xc8) == 0xf) {
        GI(0xc8) = 0;
        g_main_menu.choose(0x1b);
    }
}

// @ 0x534230
void SourceScreen::hook_b(int arg)
{
    src.pick(&arg);
    if ((float)(g_ticks[src.slot] - *(__int64*)&src.tick_lo) * 1e-5f > interval) {
        int id = f18;
        if (id == 100) {
            g_flags.commit(&arg);
            return;
        }
        g_main_menu.choose(id);
    }
}
