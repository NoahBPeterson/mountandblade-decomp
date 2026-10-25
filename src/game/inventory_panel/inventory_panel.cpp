//-----------------------------------------------------------------------------
// File: inventory_panel.cpp  (0x50fe20..0x513710)
//
// Desc: the inventory / trade screen's window object: the per-frame step (scroll-bar follow, trial-mode money, character
//       strip clicks, the pending trade value and its pay / receive / dump questions), the slot state and value tables,
//       the grid refresh, the item tooltip, the item and attribute names, the price rules, and the slot-transfer engine.
//       "T regime" region (docs/tu_map.md): the file keeps the binary order. Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./inventory_panel.h"
#include "./inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../item_ref.h"
#include "../../include/item_kind.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/msg_log.h"

#define TROOP(i) (g_game->troops + (i))
// the slot arrays are indexed with a 0xa1 (= 0x508 / 8) stride, exactly as the original does
#define EQUIP(t, k)  (((ItemRef*)((char*)g_game->troops + 0xb4))[(t) * 0xa1 + (k)])
#define GRID(t, k)   (((ItemRef*)((char*)g_game->troops + 0x4a4))[(t) * 0xa1 + (k)])
#define ITYPE(i) (*(unsigned char*)&g_items[i].type_flags)

#include "../../include/sound_mgr.h"                      // SoundMgr (play 0x442650)
#define SOUNDMGR ((SoundMgr*)0x5aa128)                  // 0x5aa128

// ---- globals and callees of other translation units -----------------------------------------------------------------
extern MsgLog g_msglog;                                 // 0x5d7080
extern int   g_snd_id_a;                                // 0x5aa350
extern int   g_snd_id_b;                                // 0x5aa2fc

extern int   g_a17148;                                  // 0xa17148
extern int   g_a17348;                                  // 0xa17348
extern int   g_a17188;                                  // 0xa17188
extern int   g_a17588;                                  // 0xa17588
extern int   g_a171b4;                                  // 0xa171b4
extern int   g_a173b4;                                  // 0xa173b4

extern int   g_trial_mode;                              // 0x5e29c4
extern int   g_a170d8;                                  // 0xa170d8
extern int   g_a174d8;                                  // 0xa174d8
extern float g_ui_alpha;                                // 0x5d7094
extern int   g_ui_tick;                                 // 0x5d8790 (integer tick counter)
extern int   g_5d8760;                                  // 0x5d8760
extern int   g_5d8764;                                  // 0x5d8764
extern tstring g_5d7850[0x50];                          // 0x5d7850 .. 0x5d7d50
extern int   g_5d8020;                                  // 0x5d8020
extern int   g_5d8024;                                  // 0x5d8024
extern float g_5d8028;                                  // 0x5d8028
extern int   g_5d8780;                                  // 0x5d8780
extern void* g_5d75e8;                                  // 0x5d75e8 (== the UI root's object at +0x568)
extern const char g_empty[];                            // 0x54a480 ("")
extern int   g_disable_food_slot;                               // 0xa181a8
extern tstring g_num;                                   // 0xa70464
extern int   g_num_valid;                               // 0xa70470
extern __int64 g_result64;                              // 0xa70458 (read as one qword by the price rule)
extern char  g_skill_tbl[];                           // 0x8ee3100 (skill records of 0x44 bytes)
void mb_snprintf(char* buf, unsigned n, const char* fmt, ...);   // 0x415100
int round_float(float* v);                              // 0x432540
void item_events(int item_no, int* a, int* b, int* c);  // 0x4ef730
void weapon_class_caption(int cls, char* out);          // 0x4f04f0
void attribute_name(int type, char* out);               // 0x512840
extern "C" int __cdecl sprintf(char*, const char*, ...);           // 0x536f8f

struct GuiObjV { virtual void d0(int which); };         // the UI root's object at +0x568: slot 0 takes one int

struct SkillNames {                                     // a skill record seen from its two names
    char pad000[0x10];
    tstring f10;                                        // +0x10
    tstring f20;                                        // +0x20
    char pad30[0x44 - 0x30];
};

// modifier record, 0x28 bytes (array at 0xa75f70): format string, translation key, value weight
struct Modifier {
    tstring fmt;                                        // +0
    tstring key;                                        // +0x10
    float weight;                                       // +0x20
    char pad24[0x4];
};
extern Modifier g_item_modifiers[];                          // 0xa75f70

// script emitter record (0x34 bytes, array pointer at 0xa75eb0, parallel index array at 0xa75ebc)
#include "../emitter_rec.h"
#include "../main_menu.h"
#include "../ui/named_rec.h"      // NamedRec::setup2 0x48a3d0 (main_menu::ask is the same function)      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../config/config.h"

// @ 0x50fe20
void InventoryPanel::refresh_transaction(int unused)
{
    g_msglog.flush();

    int page = 0;
    if (g_ui_alpha < 0.5f && view_count == 2)
        page = 1;

    if (g_ui_tick != 0) {
        InvBar* bar = view[page]->bar;
        int stop = bar->stop;
        bar->set_value((g_ui_tick / bar->stops[stop]) * bar->scale * 0.0005f +
                       (bar->stops[stop] - bar->scale) *
                       ((bar->thumb - bar->track_start) / (bar->track_end - bar->track_start)));
        PSO(bar)->set_key_flags(0x10000);
    }

    if ((g_a17148 != 0 || g_a17348 != 0) && g_a17188 != 0 && g_a17588 == 0 &&
        g_cfg.cheat_mode != 0 && g_trial_mode != 0) {
        g_game->add_gold(1000, g_game->player);
        money_label(0);
    }

    if (icon_a84 != 0 && PSO(icon_a84)->clicked_on(0x10000)) {
        spec_ac0 = (spec_ac0 + 1) % 4;
        add_strip_icon((char*)this + 0xa84, tstring("inv_i_character"), g_game->player, spec_ac0);
    }
    if (icon_a88 != 0 && PSO(icon_a88)->clicked_on(0x10000)) {
        spec_ac4 = (spec_ac4 + 1) % 4;
        add_strip_icon((char*)this + 0xa88, tstring("inv_i_character2"), troop, spec_ac4);
    }

    refresh_lists(0);
    if (mode == 4)
        refresh_lists(1);

    char eaten = 0;
    if (PSO(leave)->clicked_on(0x10000) || (g_a170d8 == 0 && g_a174d8 != 0 && pending == 0)) {
        if (locked == 0) {
            int worn = 0;
            if (mode == 0 && troop == GD_I(0x90)) {
                ItemRef* it = &TROOP(troop)->inv[0];
                for (int i = 0; i < 9; i++) {
                    if (it[i].item >= 0)
                        worn++;
                }
            }

            if (pending < 0) {
                if (-pending <= TROOP(g_game->player)->f3b4) {
                    g_game->remove_gold(-pending, g_game->player);
                    TROOP(troop)->f3b4 -= pending;
                    eaten = 1;
                } else {
                    tstring key("ui_you_dont_have_value");
                    tstring_buffer vb;
                    tstring* val = GV->string_value(vb.get(), -pending);
                    tstring* f = g_strtable.lookup(key);
                    char buf[0x200];
                    sprintf(buf, f->c_str(), val->c_str());
                    vb.destroy();
                    key.~tstring();
                    tstring msg(buf);
                    char out[0x10];
                    PSO(total_cost)->set_text((float*)out, &msg);
                    msg.~tstring();
                }
            } else if (pending > 0) {
                if (pending <= TROOP(troop)->f3b4) {
                    g_game->add_gold(pending, g_game->player);
                    TROOP(troop)->f3b4 -= pending;
                    eaten = 1;
                } else {
                    q_pay = -1;
                    tstring key("ui_merchant_cant_afford_value");
                    tstring_buffer vb1;
                    tstring* val = GV->string_value(vb1.get(), TROOP(this->troop)->f3b4);
                    tstring_buffer vb2;
                    tstring* val2 = GV->string_value(vb2.get(), pending);
                    Troop* rec = TROOP(troop);
                    tstring* nm = g_strtable.find(&rec->key);
                    if (nm == 0)
                        nm = &rec->key;
                    tstring* f = g_strtable.lookup(key);
                    char buf[0x200];
                    mb_snprintf(buf, 0x200, f->c_str(), nm->c_str(), val2->c_str(), val->c_str());
                    vb2.destroy();
                    vb1.destroy();
                    key.~tstring();
                    tstring think("ui_merchant_think_of_something_else");
                    tstring pay("ui_merchant_pay_whatever");
                    tstring msg(buf);
                    ((NamedRec*)&g_main_menu)->setup2(&msg, g_strtable.lookup(pay), g_strtable.lookup(think), (int)&q_pay);
                    msg.~tstring();
                    pay.~tstring();
                    think.~tstring();
                    g_main_menu.f4 = 2;
                    g_main_menu.set_state(0);
                }
            } else if (worn > 0) {
                q_dump = -1;
                char buf[0x200];
                if (worn == 1) {
                    tstring key("ui_dumping_value_item");
                    tstring* f = g_strtable.lookup(key);
                    mb_snprintf(buf, 0x200, f->c_str(), 1);
                    key.~tstring();
                } else {
                    tstring key("ui_dumping_value_items");
                    tstring* f = g_strtable.lookup(key);
                    mb_snprintf(buf, 0x200, f->c_str(), worn);
                    key.~tstring();
                }
                tstring no("ui_no");
                tstring yes("ui_yes");
                tstring msg(buf);
                ((NamedRec*)&g_main_menu)->setup2(&msg, g_strtable.lookup(yes), g_strtable.lookup(no), (int)&q_dump);
                msg.~tstring();
                yes.~tstring();
                no.~tstring();
                g_main_menu.f4 = 2;
                g_main_menu.set_state(0);
            } else {
                eaten = 1;
            }

            int food = g_game->troops->equip[9].item;
            if (food >= 0 && ITYPE(food) == 1) {
                q_food = -1;
                tstring q("ui_question_slaughter_food_and_eat");
                ItemKind* rec = &g_items[food];
                tstring* nm = g_strtable.find(&rec->key);
                if (nm == 0)
                    nm = &rec->key;
                tstring* f = g_strtable.lookup(q);
                char buf[0x200];
                mb_snprintf(buf, 0x200, f->c_str(), nm->c_str());
                q.~tstring();
                tstring no("ui_no");
                tstring yes("ui_yes");
                tstring msg(buf);
                ((NamedRec*)&g_main_menu)->setup2(&msg, g_strtable.lookup(yes), g_strtable.lookup(no), (int)&q_food);
                msg.~tstring();
                yes.~tstring();
                no.~tstring();
                g_main_menu.set_state(0);
                eaten = 0;
            }
        }
    }

    if (q_food == 0) {
        q_food = -1;
        Troop* rec = TROOP(g_game->player);
        rec->equip[9].item = 1;
        rec->equip[9].m4 = 0;
        rec->equip[9].reduce(0);
        set_slot_flag(2, 9, 2, 1);
        SOUNDMGR->play(g_snd_id_a, 1, ((float*)&g_sound_mgr.listener), 1.0f, 0);
        SOUNDMGR->play(g_snd_id_b, 1, ((float*)&g_sound_mgr.listener), 1.0f, 0);
        init_window();
    }

    if (!eaten) {
        if (q_pay != 0) {
            if (q_dump != 0)
                return;
        }
    } else if (q_pay != 0) {
        goto equip;
    }
    g_game->add_gold(TROOP(troop)->f3b4, g_game->player);
    TROOP(troop)->f3b4 = 0;

equip:
    if (troop >= 3) {
        Troop* rec = TROOP(troop);
        unsigned f = rec->flags32;
        int spare = *(int*)((char*)rec + 0x34);
        spare = spare;
        if ((f & 0x10) == 0 || (f & 0x1000) == 0) {
            if (mode == 4)
                ((TroopRec*)rec)->auto_equip(-1, 0x100e, 0, 0);
        }
    }

    if (next_menu != 0x64)
        g_main_menu.set_state(next_menu);
    else
        g_map_flags.commit(&eaten);
}


// @ 0x510850
void InventoryPanel::set_slot_flag(int page, int idx, unsigned mask, char on)
{
    if (page < 2) {
        if (on)
            ((int*)((char*)this + 0x364))[page * 0x60 + idx] |= mask;
        else
            ((int*)((char*)this + 0x364))[page * 0x60 + idx] &= ~mask;
    } else {
        if (on)
            ((int*)((char*)this + 0x970))[page * 10 + idx] |= mask;
        else
            ((int*)((char*)this + 0x970))[page * 10 + idx] &= ~mask;
    }
}

// @ 0x5108d0
void InventoryPanel::refresh_lists(int unused)
{
    for (int page = 0; page < view_count; page++) {
        int troop = g_game->player;
        if (page != 0)
            troop = this->troop;
        int count;
        if (troop == GD_I(0x90) && mode == 0)
            count = 9;
        else
            count = TROOP(troop)->inv_count();
        if (count > 0) {
            MissionUi** w = (MissionUi**)((char*)this + 0x64 + page * 0x180);
            for (int i = 0; i < count; i++)
                if (PSO(w[i])->clicked_on(4))
                    g_gui_root.set_o568(0, true);
            for (int i = 0; i < count; i++)
                if (PSO(w[i])->clicked_on(2) && locked == 0)
                    place_slot(w[i], &EQUIP(troop, i), page, i);
        }
    }

    for (int page = 0; page < view_count2; page++) {
        int troop = g_game->player;
        if (page != 0)
            troop = this->troop;
        MissionUi** w = (MissionUi**)((char*)this + 0x970 + page * 0x28);
        for (int i = 0; i < 10; i++)
            if (w[i] != 0 && PSO(w[i])->clicked_on(4) && locked == 0)
                g_gui_root.set_o568(0, true);
        for (int i = 0; i < 10; i++)
            if (w[i] != 0 && PSO(w[i])->clicked_on(2) && locked == 0)
                place_slot(w[i], &GRID(troop, i), 2, i);
    }

    if (locked != 0) {
        void* o = g_5d75e8;
        if (o != 0)
            ((GuiObjV*)o)->d0(1);
        g_5d8760 = -1;
        g_5d8764 = 0xff101010;
        for (tstring* s = g_5d7850; s < g_5d7850 + 0x50; s++)
            s->assign(g_empty);
        g_5d8024 = 0;
        g_5d8028 = -0.03f;
        g_5d8020 = 0;
        g_5d75e8 = 0;
        g_5d8780 = 0;
    }

    for (int page = 0; page < view_count; page++) {
        if (!PSO(btn[page])->clicked_on(0x10000))
            continue;
        int troop = g_game->player;
        if (page != 0)
            troop = this->troop;
        int count;
        if (troop == GD_I(0x90) && mode == 0)
            count = 9;
        else
            count = TROOP(troop)->inv_count();
        if (count <= 0)
            continue;
        MissionUi** w = (MissionUi**)((char*)this + 0x64 + page * 0x180);
        for (int i = 0; i < count; i++) {
            if (!PSO(w[i])->clicked_on(0x10000))
                continue;
            int idx = i;
            if (locked != 0 && page != 0) {
                int r = TROOP(troop)->find_free();
                if (r >= 0 && !(troop == GD_I(0x90) && mode == 0 && r >= 9))
                    idx = r;
            }
            transfer_slot(&EQUIP(troop, idx), page, idx);
            place_slot(w[i], &EQUIP(troop, i), page, i);
            if ((g_a17148 != 0 || g_a17348 != 0 || g_a171b4 != 0 || g_a173b4 != 0) &&
                locked != 0 && view_count == 2) {
                int other = (page == 0) ? this->troop : g_game->player;
                int r = TROOP(other)->find_free();
                if (r >= 0 && !(other == GD_I(0x90) && mode == 0 && r >= 9))
                    transfer_slot(&EQUIP(other, r), 1 - page, r);
            }
        }
    }

    for (int page = 0; page < view_count2; page++) {
        if (!PSO(btn2[page])->clicked_on(0x10000))
            continue;
        MissionUi** w = (MissionUi**)((char*)this + 0x970 + page * 0x28);
        for (int i = 0; i < 10; i++) {
            if (w[i] == 0)
                continue;
            if (!PSO(w[i])->clicked_on(0x10000))
                continue;
            int troop = (page != 0) ? this->troop : g_game->player;
            ItemRef* slot = &GRID(troop, i);
            transfer_slot(slot, page + 2, i);
            place_slot(w[i], slot, 2, i);
            if ((g_a17148 != 0 || g_a17348 != 0 || g_a171b4 != 0 || g_a173b4 != 0) &&
                locked != 0 && view_count2 == 2 && page == 1) {
                int t = g_game->player;
                int count = TROOP(t)->inv_count();
                int idx = -1;
                bool found = false;
                ItemRef* it = &EQUIP(t, 0);
                for (int k = 0; k < count && !found; k++, it++) {
                    if (it->item == -1) {
                        idx = k;
                        found = true;
                    }
                }
                if (idx >= 0)
                    transfer_slot(&EQUIP(t, idx), 0, idx);
            }
        }
    }

    init_window();
}

// @ 0x510f50
void InventoryPanel::place_slot(MissionUi* w, ItemRef* slot, int page, int index)
{
    float pt[2];
    if (slot->item >= 0) {
        pt[0] = w->fE0 + 0.044f;
        pt[1] = w->fE4 + 0.044f;
        place_slot_mesh(slot, pt, page, index);
    }
}


// @ 0x510fa0
// ------------------------------------------------------------------------------------------------------------
void InventoryPanel::place_slot_mesh(ItemRef* slot, float* pt, int page, int index)
{
    int zero = 0;
    ItemKind* it = &g_items[slot->item];
    g_gui_root.set_o568(0, 1);
    g_gui_root.set_view(pt);

    char buf[512];
    slot->name(buf);
    {
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.025f, 0xfff0e0d0, 1);
    }

    unsigned int flags = it->type_flags;
    unsigned int kind = flags & 0xff;
    if (kind == 2 || kind == 3 || kind == 4) {
        const char* key;
        if (kind == 4)
            key = (flags & 0x200000) ? "ui_polearm_no_shield" : "ui_polearm";
        else if (kind == 3)
            key = (flags & 0x200000) ? "ui_two_handed" : "ui_two_handed_one_handed";
        else
            key = "ui_one_handed";
        tstring k(key);
        sprintf(buf, "%s", g_strtable.lookup(k)->c_str());
        k.~tstring();
        {
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.021f, 0xffc0b0e0, 1);
        }
    }

    char buyable = test_slot_flag(page, index, 1);

    kind = it->type_flags & 0xff;
    if (kind == 10 || kind == 5 || kind == 6 || kind == 0x12) {
        int max = slot->max_amount();
        sprintf(buf, "(%d / %d)", max - (slot->m4 & 0xff), slot->max_amount());
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
    } else if (kind == 7) {
        int hp = slot->stat_1e8();
        sprintf(buf, "(%d / %d)", slot->m4 & 0xffff, hp);
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
    } else if (kind == 0xb && (it->type_flags & 0x2000000)) {
        int max = slot->max_amount();
        sprintf(buf, "(%d / %d)", max - (slot->m4 & 0xff), slot->max_amount());
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
    }

    if (page == 1) {
        if (buyable) {
            sprintf(buf, "%s", g_strtable.lookup(tstring("ui_reclaim_price"))->c_str(),
                     values[1][index]);
        } else {
            const tstring* v = g_strtable.lookup(tstring("ui_buying_price"));
            int p = compute_price((int*)slot, 1);
            sprintf(buf, v->c_str(), p);
        }
    } else {
        if (buyable) {
            int p;
            if (page < 2)
                p = values[page][index];
            else
                p = vstash[page - 2][index];
            const tstring* v = g_strtable.lookup(tstring("ui_return_price"));
            sprintf(buf, v->c_str(), p);
        } else {
            int p = compute_price((int*)slot, 0);
            const tstring* v = g_strtable.lookup(tstring("ui_sell_price"));
            sprintf(buf, v->c_str(), p);
        }
    }
    {
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.024f, 0xfffffdf0, 1);
    }

    if (slot->item >= 0 && it->f208 > 1e-5f) {
        float w = slot->item < 0 ? 0.0f : it->f208;
        const tstring* v = g_strtable.lookup(tstring("ui_weight"));
        sprintf(buf, v->c_str(), (double)w);
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.011f, 0xffffffff, 1);
    }

    kind = it->type_flags & 0xff;
    if (kind == 12 || kind == 13 || kind == 14 || kind == 15) {
        if (slot->stat_1fc()) {
            const tstring* v = g_strtable.lookup(tstring("ui_plus_value_to_head_armor"));
            sprintf(buf, v->c_str(), slot->stat_1fc());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        if (slot->stat_200()) {
            const tstring* v = g_strtable.lookup(tstring("ui_plus_value_to_body_armor"));
            sprintf(buf, v->c_str(), slot->stat_200());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        if (slot->stat_204()) {
            const tstring* v = g_strtable.lookup(tstring("ui_plus_value_to_leg_armor"));
            sprintf(buf, v->c_str(), slot->stat_204());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
    }

    kind = it->type_flags & 0xff;
    if (kind == 8 || kind == 9 || kind == 10 || kind == 16 || kind == 17 ||
        kind == 2 || kind == 3 || kind == 4) {
        // swing damage (type in flags2 at +0x218)
        int mask = 0;
        if (slot->stat_21x(0)) {
            unsigned int t = (it->f218 >> 8) & 3;
            const char* key;
            if (t == 0)      { key = "ui_cut_short";    mask = 1; }
            else if (t == 1) { key = "ui_pierce_short"; mask = 2; }
            else             { key = "ui_blunt_short";  mask = 4; }
            const tstring* v;
            {
                tstring k(key);
                v = g_strtable.lookup(k);
            }
            const tstring* dmg = g_strtable.lookup(tstring("ui_swing"));
            sprintf(buf, dmg->c_str(), slot->stat_21x(0), v->c_str());
            if (mask & 4) mask &= ~4;
            if (mask & 2) mask &= ~2;
            if (mask & 1) mask &= ~1;
            {
                tstring s(buf);
                g_gui_root.draw_string(&s, 0.024f, 0xffffb0a0, 1);
            }
        }
        // thrust damage (type in flags2 at +0x214)
        if (slot->stat_21x(1)) {
            unsigned int t = (it->f214 >> 8) & 3;
            const char* key;
            const char* fmtkey;
            if (kind == 8 || kind == 9 || kind == 10 || kind == 16 || kind == 17) {
                fmtkey = "ui_damage";
                if (t == 0)      { key = "ui_cut_short";    mask = 8; }
                else if (t == 1) { key = "ui_pierce_short"; mask = 0x10; }
                else             { key = "ui_blunt_short";  mask = 0x20; }
            } else {
                fmtkey = "ui_thrust";
                if (t == 0)      { key = "ui_cut_short";    mask = 0x40; }
                else if (t == 1) { key = "ui_pierce_short"; mask = 0x80; }
                else             { key = "ui_blunt_short";  mask = 0x100; }
            }
            const tstring* v;
            {
                tstring k(key);
                v = g_strtable.lookup(k);
            }
            const tstring* dmg = g_strtable.lookup(tstring(fmtkey));
            sprintf(buf, dmg->c_str(), slot->stat_21x(1), v->c_str());
            if (mask & 0x100) mask &= ~0x100;
            if (mask & 0x80)  mask &= ~0x80;
            if (mask & 0x40)  mask &= ~0x40;
            {
                tstring s(buf);
                g_gui_root.draw_string(&s, 0.024f, 0xffffb0a0, 1);
            }
        }
        if (kind == 8 || kind == 9 || kind == 16 || kind == 17) {
            int v = 100;
            if (slot->item >= 0) {
                v = it->f204;
                if (v == 0)
                    v = 99;
            }
            if (v > 100) v = 100;
            const tstring* p = g_strtable.lookup(tstring("ui_accuracy"));
            sprintf(buf, p->c_str(), v);
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffeebbfe, 1);
        }
        if (slot->stat_1f0a()) {
            const tstring* p = g_strtable.lookup(tstring("ui_speed_rating"));
            sprintf(buf, p->c_str(), slot->stat_1f0a());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
    } else if (kind == 5 || kind == 6 || kind == 0x12) {
        if (slot->stat_21x(1)) {
            const tstring* p = g_strtable.lookup(tstring("ui_value_to_damage"));
            int v = slot->stat_21x(1);
            sprintf(buf, p->c_str(), slot->stat_21x(1), v <= 0 ? '+' : ' ');
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffb0a0, 1);
        }
    } else if (kind == 7) {
        if (slot->stat_200()) {
            const tstring* p = g_strtable.lookup(tstring("ui_resistance"));
            sprintf(buf, p->c_str(), slot->stat_200());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        {
            const tstring* p = g_strtable.lookup(tstring("ui_size"));
            int v = (int)((float)it->weight * 1.0f);
            sprintf(buf, p->c_str(), v);
        }
        {
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        if (slot->stat_1f0a()) {
            const tstring* p = g_strtable.lookup(tstring("ui_speed_rating"));
            sprintf(buf, p->c_str(), slot->stat_1f0a());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
    } else if (kind == 0xb && g_disable_food_slot == 0 && slot->item >= 0 && it->f1fc > 0) {
        const tstring* p = g_strtable.lookup(tstring("ui_value_to_morale"));
        float v = slot->item < 0 ? 0.0f : (float)it->f1fc;
        sprintf(buf, p->c_str(), (double)(v * 0.02f));
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.024f, 0xffa0f0b0, 1);
    }

    kind = it->type_flags & 0xff;
    if (kind == 2 || kind == 3 || kind == 4) {
        float reach = (float)it->weight * 1.0f;
        int n = (int)floor(reach);
        if (reach - (float)n > 0.5f)
            n++;
        const tstring* p = g_strtable.lookup(tstring("ui_weapon_reach"));
        sprintf(buf, p->c_str(), n);
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
    }

    if (*(char*)&it->type_flags == 1) {
        g_game->skill_level(g_game->player, 0x18);
        {
            const tstring* p = g_strtable.lookup(tstring("ui_armor"));
            sprintf(buf, p->c_str(), slot->stat_200());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffddddff, 1);
        }
        {
            const tstring* p = g_strtable.lookup(tstring("ui_speed"));
            sprintf(buf, p->c_str(), slot->stat_1f4());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        {
            const tstring* p = g_strtable.lookup(tstring("ui_maneuver"));
            sprintf(buf, p->c_str(), slot->stat_1f0b());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        {
            const tstring* p = g_strtable.lookup(tstring("ui_charge"));
            sprintf(buf, p->c_str(), slot->stat_214());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        {
            const tstring* p = g_strtable.lookup(tstring("ui_hit_points"));
            sprintf(buf, p->c_str(), slot->stat_1e8(), slot->stat_1e8());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
    }

    if (slot->stat_1ec()) {
        int color = 0xffee4444;
        int have = ((Troop*)((char*)g_game->troops + g_game->player * 0x508))->slot_value(slot->item);
        if (have >= slot->stat_1ec())
            color = 0xff66ff44;

        char need[512];
        int a, b, c;
        item_events(slot->item, &a, &b, &c);
        if (a >= 0) {
            attribute_name(a, need);
        } else {
            if (b < 0) {
                if (c >= 0)
                    weapon_class_caption(c, need);
            } else {
                SkillNames* sk = (SkillNames*)(g_skill_tbl + b * 0x44);
                const tstring* name = g_strtable.find(&sk->f20);
                if (!name)
                    name = &sk->f10;
                sprintf(need, "%s", name->c_str());
            }
        }
        {
            const tstring* p = g_strtable.lookup(tstring("ui_requires_value_difficulty"));
            sprintf(buf, p->c_str(), need, slot->stat_1ec());
            tstring s(buf);
            g_gui_root.draw_string(&s, 0.024f, color, 1);
        }
    }

    if (it->type_flags & 0x4000000) {
        const tstring* p = g_strtable.lookup(tstring("ui_bonus_against_shields"));
        sprintf(buf, p->c_str());
        tstring s(buf);
        g_gui_root.draw_string(&s, 0.024f, 0xffbbff22, 1);
    }
    if (it->type_flags & 0x4000) {
        tstring k("ui_cant_be_used_to_block");
        g_gui_root.draw_string(g_strtable.lookup(k), 0.024f, 0xffccbb33, 1);
    }
    if (it->type_flags & 0x10100000) {
        tstring k("ui_cannot_be_used_on_horseback");
        g_gui_root.draw_string(g_strtable.lookup(k), 0.024f, 0xffccbb33, 1);
    }

    for (int i = 0; i < 8; i++) {
        g_store0 = 0;
        g_store1 = 0;
        g_num.assign("");
        GV->emit_event4(0x13, slot->item, i, *(signed char*)((char*)slot + 7));
        if (g_num_valid > 0) {
            unsigned int color = 0xffffffff;
            if (g_store0 != 0 || g_store1 != 0)
                color = g_store0 | 0xff000000;
            g_gui_root.draw_string(&g_num, 0.024f, color, 1);
        }
    }
    (void)zero;
}


// @ 0x512620
char GameDataV::emit_event4(int id, int a, int b, int c)
{
    __int64 vals[16];
    vals[0] = a;
    vals[1] = b;
    vals[2] = c;
    unsigned tag;
    if (*(int*)((char*)this + 0x1a068) && g_scripts[id].name.equals(*(tstring*)((char*)this + 0x1a06c)))
        tag = (id << 16) | *(int*)((char*)this + 0x1a07c);
    else
        tag = (unsigned)-1;
    char r = 0;
    int count = 3;
    if (g_emitter_idx[id] >= 0)
        r = g_scripts[g_emitter_idx[id]].sub.emit(0x200000, 0, &count, vals, tag);
    return r;
}

// @ 0x5126e0
char InventoryPanel::test_slot_flag(int page_idx, int slot, unsigned mask)
{
    if (page_idx < 2)
        return (flags[page_idx][slot] & mask) != 0;
    return (((unsigned*)&grid[0][0])[page_idx * 10 + slot] & mask) != 0;
}

// @ 0x512730
void ItemRef::name(char* out)
{
    if (item < 0) {
        strcpy(out, "ERROR: INVALID ITEM");
        return;
    }
    int mod = ((signed char*)this)[7];
    ItemKind* it = &g_items[item];
    tstring* t = g_strtable.find(&it->key);
    tstring nm(*(t ? t : (tstring*)it));
    nm.underscores_to_spaces();
    if (mod != 0) {
        const char* s = nm.c_str();
        Modifier* m = &g_item_modifiers[mod];
        tstring* mk = g_strtable.find(&m->key);
        const char* fmt = (mk ? mk : (tstring*)m)->c_str();
        _snprintf(out, 0x80, fmt, s);
    } else {
        _snprintf(out, 0x80, "%s", nm.c_str());
    }
}

// @ 0x512840
void attribute_name(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_strength");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_agility");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_intelligence");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else {
        tstring k("ui_charisma");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}

// @ 0x512960
void InventoryPanel::compute(int* item, int price_mode)
{
    char trade = 0;
    char grid = 0;
    if (fabc != 0 || mode == 1 || mode == 3 || mode == 4)
        trade = 1;
    if (mode == 2)
        grid = 1;
    char plain;
    if (price_mode != 1)
        plain = 1;
    else
        plain = 0;
    GV->price((ItemRef*)item, troop, plain, trade, grid);
}

// @ 0x5129d0
int GameDataV::price(ItemRef* item, int idx, char adjust, char plain, char scaled)
{
    if (item->item < 0)
        return 0;
    if (g_items[item->item].value <= 0)
        return 0;

    PartyMgr* pool = &parties;
    int pi = ((Map10007*)pool)->lookup(f78);
    Party* p = party_at(pool, pi);
    int skill = p->skill_level(0);
    float f = (1.0f - (float)skill * 0.06f) * 1.25f;
    if (adjust)
        f = f * 1.9f;
    if (ITYPE(item->item) == 0xb) {
        int skill2 = pool->get(f78)->skill_level(0);
        f = 0.25f - (float)skill2 * 0.01f;
    }

    float value = (float)g_items[item->item].value;
    if (g_items[item->item].type_flags & 0x2000000) {
        int n = item->m4 & 0xff;
        int c = item->max_amount();
        value = (float)(c - n) / (float)item->max_amount() * value;
    }
    int mod = ((signed char*)item)[7];
    if (mod > 0)
        value = value * g_sort_val[mod * 5].v;

    int result;
    if (plain) {
        result = (int)value;
    } else {
        g_result64 = 100;
        int mode = adjust ? 4 : 3;
        if (emit_event(mode, item->item, (int)value)) {
            result = (int)((float)g_result64 * value * 0.01f);
        } else {
            if (scaled)
                value = value * g_items[item->item].f21c;
            int row = *(int*)((char*)troops + idx * 0x508 + 0x4c);
            float tf = *(float*)((char*)factions + row * 0x300 + 0x2c);
            if (tf < -1.0f)
                tf = -1.0f * 3.0f;
            else {
                if (tf > 1.0f)
                    tf = 1.0f;
                else if (tf < 0.0f)
                    tf *= 3.0f;
                tf *= 0.5f;
            }
            tf = (1.0f - tf) * f + 1.0f;
            if (adjust)
                tf = 1.0f / tf;
            value = tf * value;
            result = round_float(&value);
        }
    }
    if (result < 1)
        result = 1;
    return result;
}


// @ 0x512c80
void InventoryPanel::transfer_slot(ItemRef* item, int page, int slot)
{
    ItemRef tmp;
    char fl;
    int sv;
    int hv;
    char hf;
    char buf[0x100];

    if (locked != 0) {
        if (can_transfer(&held, page, slot))
            goto L_cd4;
        if (locked != 0)
            goto L_304c;
    }
    if (item->item == -1)
        goto L_303e;
L_cd4:
    if (mode == 2 && try_place(item, page, slot))
        goto L_303e;
    if (locked == 0)
        goto L_de3;
    if (held.item == item->item && item->item >= 0 && ITYPE(item->item) == 0x12 &&
        (held.m4 & 0xff) != 0 && (item->m4 & 0xff) != 0) {
        // same stackable: pour as much as fits from the hand into the held stack
        g_gui_root.play_sound();
        int n = item->m4 & 0xff;
        int have = held.m4 & 0xff;
        int room = held.max_amount() - have;
        if (n > room)
            n = room;
        item->reduce(n);
        held.add_amount(n);
        if (held.max_amount() - (held.m4 & 0xff) > 0)
            return;
        held.item = -1;
        g_gui_root.set_o564(0);
        locked = 0;
        return;
    }
L_de3:
    // pick the slot's contents up into the hand
    g_gui_root.play_sound();
    tmp = *item;
    fl = test_slot_flag(page, slot, 1);
    sv = (page < 2) ? (int)values[page][slot] : (int)vstash[page - 2][slot];
    item->item = -1;
    or_slot_flag(page, slot);
    if (locked == 0)
        goto L_f9d;
    hf = held_flag;
    hv = held_value;
    set_slot_flag(page, slot, 1, hf);
    set_slot_value(page, slot, hf ? hv : 0x1e61);
    if (mode == 2) {
        // trade mode: price the move, unless it stays on the same side
        if (page == 1) {
            if (held_page == 1)
                goto L_f54;
        } else {
            if (held_page != 1)
                goto L_f54;
        }
        {
            int price = hf ? hv : compute_price((int*)&held, held_page);
            char empty = (held_flag == 0);
            set_slot_flag(page, slot, 1, empty);
            set_slot_value(page, slot, empty ? price : 0x1e61);
            int ev;
            if (page == 1) {
                add_cost(price);
                ev = 6;
            } else {
                add_cost(-price);
                ev = 5;
            }
            g_game->emit_event(ev, held.item, held_flag != 0);
        }
    }
L_f54:
    // hand the held item back to the slot
    *item = held;
    or_slot_flag(page, slot);
    held.item = -1;
    if (g_drag != 0)
        g_drag->release(1);
    g_drag = 0;
    locked = 0;
L_f9d:
    // take the slot's contents into the hand
    if (tmp.item < 0)
        return;
    held = tmp;
    held_page = page;
    held_flag = fl;
    held_value = sv;
    {
        int idx = g_game->player;
        if (page == 1 || page == 3) {
            if (fl == 0)
                idx = troop;
        } else {
            if (fl != 0)
                idx = troop;
        }
        DragMesh* m = make_mesh(&held, &hv, idx);
        m->scale(2.0f);
        if (g_drag != 0)
            g_drag->release(1);
        g_drag = m;
        locked = 1;
    }
    return;
L_303e:
    if (locked == 0)
        return;
L_304c:
    if (page < 2)
        return;
    if (slot == 8 && ITYPE(held.item) == 1) {
        if (page == 2) {
            tstring key("ui_notification_riding_skill_not_enough");
            g_msglog.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else if (slot >= 0 && slot < 4 &&
               ((g_items[held.item].type_flags & 0xff) == 2 || (g_items[held.item].type_flags & 0xff) == 3 ||
                (g_items[held.item].type_flags & 0xff) == 4 || (g_items[held.item].type_flags & 0xff) == 8 ||
                (g_items[held.item].type_flags & 0xff) == 9 || (g_items[held.item].type_flags & 0xff) == 10 ||
                (g_items[held.item].type_flags & 0xff) == 16 || (g_items[held.item].type_flags & 0xff) == 17)) {
        if (page == 2) {
            tstring key("ui_notification_requirements_not_met");
            g_msglog.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else if (((g_items[held.item].type_flags & 0xff) == 12 && slot == 4) ||
               ((g_items[held.item].type_flags & 0xff) == 13 && slot == 5) ||
               ((g_items[held.item].type_flags & 0xff) == 15 && slot == 7) ||
               ((g_items[held.item].type_flags & 0xff) == 14 && slot == 6)) {
        if (page == 2) {
            tstring key("ui_notification_requirements_not_met");
            g_msglog.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else {
        return;
    }
    {
        tstring key("ui_troop_cant_use_item");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                ((TroopRec*)((char*)g_game->chars + troop * 0x508))->display_name()->c_str());
    }
    g_msglog.add(tstring(buf), 0xffff7700);
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x513230
char InventoryPanel::can_transfer(ItemRef* item, int where, int flag)
{
    char r = 1;
    if (where == 0)
        return r;
    if (where >= 2) {
        int idx = (where == 2) ? g_game->player : troop;
        r = ((TroopRec*)((char*)g_game->chars + idx * 0x508))->fits_slot(item, flag);
        if (flag != 9)
            return r;
        if (mode != 2 && mode != 4)
            return r;
    } else {
        if (where != 1)
            return r;
        if (mode != 3)
            return r;
    }
    if (ITYPE(item->item) == 1)
        r = 0;
    return r;
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x5132e0
void InventoryPanel::set_slot_value(int page, int slot, int value)
{
    if (page < 2)
        values[page][slot] = (unsigned)value;
    else
        vstash[page - 2][slot] = (unsigned)value;
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x513320
void InventoryPanel::or_slot_flag(int page, int slot)
{
    if (page < 2)
        flags[page][slot] |= 2;
    else
        fstash[page - 2][slot] |= 2;
}

// ---------------------------------------------------------------------------------------------------------

// @ 0x513360
void InventoryPanel::add_cost(int delta)
{
    char buf[0x200];

    pending += delta;
    if (pending < 0) {
        tstring_buffer sv;
        tstring key("ui_notification_payment_value");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                GV->string_value(sv.get(), -pending)->c_str());
        sv.destroy();
    } else if (pending > 0) {
        tstring_buffer sv;
        tstring key("ui_notification_payment_receive_value");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                GV->string_value(sv.get(), pending)->c_str());
        sv.destroy();
    } else {
        tstring key("ui_no_cost");
        sprintf(buf, g_strtable.lookup(key)->c_str());
    }
    {
        tstring_buffer out;
        tstring msg(buf);
        PSO(total_cost)->set_text((float*)out.get(), &msg);
    }
}


// @ 0x5134c0
bool InventoryPanel::try_place(ItemRef* item, int page, int slot)
{
    bool found = false;
    int id = item->item;

    if (id == -1)
        return found;
    if (test_slot_flag(page, slot, 1))
        return found;
    if (ITYPE(id) != 0xb)
        return found;

    if (page == 1) {
        int idx = troop;
        int n;

        if (idx == GD_I(0x90) && mode == 0)
            n = 9;
        else
            n = g_game->troops[idx].inv_count();
        if (n <= 0)
            return found;
        for (int i = 0; i < n; i++) {
            if (g_game->troops[idx].inv[i].item == id && (flags[1][i] & 1))
                found = true;
        }
    }
    else {
        int idx = g_game->player;
        int n;

        if (idx == GD_I(0x90) && mode == 0)
            n = 9;
        else
            n = g_game->troops[idx].inv_count();
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                if (g_game->troops[idx].inv[i].item == id && (flags[0][i] & 1))
                    found = true;
            }
        }
        for (int i = 0; i < 10; i++) {
            if (g_game->troops[idx].equip[i].item == id && (fstash[0][i] & 1))
                found = true;
        }
    }

    if (found) {
        if (page == 1) {
            tstring msg("ui_reclaim_your_sold_goods");
            g_msglog.add(*g_strtable.lookup(msg), -1);
        } else {
            tstring msg("ui_return_your_bought_goods");
            g_msglog.add(*g_strtable.lookup(msg), -1);
        }
    }
    return found;
}

