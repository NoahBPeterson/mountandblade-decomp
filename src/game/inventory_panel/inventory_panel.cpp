//-----------------------------------------------------------------------------
// File: inventory_panel.cpp  (0x50fe20..0x513710)
//
// Desc: the inventory / trade screen's window object: the per-frame step (scroll-bar follow, trial-mode money, character
//       strip clicks, the pending trade value and its pay / receive / dump questions), the slot state and value tables,
//       the grid refresh, the item tooltip, the item and attribute names, the price rules, and the slot-transfer engine.
//       "T regime" region (docs/tu_map.md): the file keeps the binary order. Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../../include/image_addrs.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../game_globals.h"   // g_trial_mode (canonical)
#include "../party/party_window_class.h"      // PartyWindow: the one class of this object
#include "../party2/camera_class.h"            // Camera: the scrolling item list (its scroll bar is follow[1])
#include "../ui/container.h"
#include "../ui/ui_widgets.h"                  // UiD (the scroll bar's class)
#include "./inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../item_ref.h"
#include "../../include/item_kind.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../strat/gui_manager.h"

#define TROOP(i) (g_game->troops + (i))
// the slot arrays are indexed with a 0xa1 (= 0x508 / 8) stride, exactly as the original does
#define EQUIP(t, k)  (((ItemRef*)((char*)g_game->troops + 0xb4))[(t) * 0xa1 + (k)])
#define GRID(t, k)   (((ItemRef*)((char*)g_game->troops + 0x4a4))[(t) * 0xa1 + (k)])
#define ITYPE(i) (*(unsigned char*)&g_items[i].type_flags)

#include "../../include/sound_mgr.h"                      // SoundMgr (play 0x442650)
#include "../shared_vars.h"
#include "../key_names.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----

#define SOUNDMGR ((SoundMgr*)&orig::g_sound_mgr)                  // 0x5aa128

// ---- globals and callees of other translation units -----------------------------------------------------------------



#include "../game_globals.h"      // g_module_settings.show_quest_notes, g_module_settings.player_wounded_treshold, g_module_settings.disable_food_slot, g_display_wp (canonical, defined in triggers/trigger_vec.cpp)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
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
#include "../../include/item_mod.h"                  // item_mod, g_item_mods (0xa75f70)

// script emitter record (0x34 bytes, array pointer at 0xa75eb0, parallel index array at 0xa75ebc)
#include "../../include/rec34.h"                 // rec34_table, g_scripts (0xa75eb0)
#include "../window_mgr.h"
#include "../ui/named_rec.h"      // NamedRec::setup2 0x48a3d0 (main_menu::ask is the same function)      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../config/config.h"
#include "../key_names.h"      // g_keyboard (0xa170c8)
#include "../slot.h"
#include "../skill_cap.h"
#include "../ui_globals.h"      // UI / core globals
#include "../float_consts.h"     // g_one

// the translated name of a record, else the record's own name (0x5102d2, 0x510604: the record itself, not its key)
template <class T> static __forceinline const char* shown_name(T* rec)
{
    tstring* nm = g_strtable.find(&rec->key);
    if (nm == 0)
        nm = &rec->name;
    return nm->c_str();
}

// @ 0x50fe20
void PartyWindow::frame_update(float unused)
{
    g_gui.flush();

    int page = 0;
    if (g_gui.mouse[0] < 0.5f && pages == 2)
        page = 1;

    if (g_gui.wheel != 0) {
        UiD* bar = (UiD*)view[page]->follow[1];
        int stop = bar->f178;
        float& es = bar->screen[stop];
        bar->set_value((g_gui.wheel / es) * bar->f17c * 0.0005f +
                       (es - bar->f17c) *
                       ((bar->v168 - bar->f16c) / (bar->f170 - bar->f16c)));
        ((UiD*)view[page]->follow[1])->set_key_flags(0x10000);
    }

    if ((g_keyboard.down[0x1d] != 0 || g_keyboard.down[0x9d] != 0) && g_keyboard.down[0x2c] != 0 && g_keyboard.prev[0x2d] == 0 &&
        g_cfg.cheat_mode != 0) {
        WL_MARKER(12)
        if (g_trial_mode != 0) {
            g_game->add_gold(1000, g_game->player);
            money_label(0);
        }
        WL_MARKER(13)
    }

    if (icon_a84 != 0 && icon_a84->clear_mask(0x10000)) {
        spec_ac0 = (spec_ac0 + 1) % 4;
        add_strip_icon(&icon_a84, "inv_i_character", g_game->player, spec_ac0);
    }
    if (icon_a88 != 0 && icon_a88->clear_mask(0x10000)) {
        spec_ac4 = (spec_ac4 + 1) % 4;
        add_strip_icon(&icon_a88, "inv_i_character2", w14, spec_ac4);
    }

    refresh_lists(0);
    if (w04 == 4)
        refresh_lists(1);

    // the question boxes live on the dialog record g_main_menu.rec points at (0x51036e: ecx = [0xa73980]); the screen
    // state is then left through g_main_menu itself (0x510395: ecx = 0xa73980)
    char eaten = 0;
    if (leave->clear_mask(0x10000) || (g_keyboard.down[0x1] == 0 && g_keyboard.prev[0x1] != 0 && pending == 0)) {
        if (locked == 0) {
            int worn = 0;
            if (w04 == 0 && w14 == GD_I(0x90)) {
                ItemRef* it = &TROOP(w14)->inv[1];
                for (int i = 3; i; i--, it += 3) {
                    if (it[-1].item >= 0)
                        worn++;
                    if (it[0].item >= 0)
                        worn++;
                    if (it[1].item >= 0)
                        worn++;
                }
            }

            if (pending < 0) {
                int plr = g_game->player;
                int gold = TROOP(plr)->f3b4;
                int cost = -pending;
                if (cost <= gold) {
                    g_game->remove_gold(cost, plr);
                    TROOP(w14)->f3b4 -= pending;
                    eaten = 1;
                } else {
                    WL_MARKER(4)
                    char buf[0x200];
                    {
                        tstring key("ui_you_dont_have_value");
                        sprintf(buf, g_strtable.lookup(key)->c_str(), GV->GameData::string_value(-pending).c_str());
                    }
                    {
                        tstring msg(buf);
                        float out[2];
                        total_cost->set_text(out, &msg);
                    }
                    WL_MARKER(5)
                }
            } else if (pending > 0) {
                if (pending <= TROOP(w14)->f3b4) {
                    g_game->add_gold(pending, g_game->player);
                    TROOP(w14)->f3b4 -= pending;
                    eaten = 1;
                } else {
                    WL_MARKER(4)
                    int* q = &q_pay;
                    *q = -1;
                    char buf[0x200];
                    mb_snprintf(buf, 0x200, g_strtable.lookup("ui_merchant_cant_afford_value")->c_str(), shown_name(TROOP(w14)),
                                GV->GameData::string_value(pending).c_str(), GV->GameData::string_value(TROOP(w14)->f3b4).c_str());
                    {
                        tstring think("ui_merchant_think_of_something_else");
                        tstring pay("ui_merchant_pay_whatever");
                        tstring msg(buf);
                        g_windows.box()->setup2(&msg, g_strtable.lookup(pay), g_strtable.lookup(think), (int)q);
                    }                                   // msg, pay, think destroyed here (0x5106bd..0x5106cf)
                    g_windows.box()->f04 = 2;
                    g_windows.set_state(0);
                    WL_MARKER(5)
                }
            } else if (worn > 0) {
                int* q = &q_dump;
                *q = -1;
                char buf[0x200];
                if (worn == 1)
                    mb_snprintf(buf, 0x200, g_strtable.lookup("ui_dumping_value_item")->c_str(), worn);
                else
                    mb_snprintf(buf, 0x200, g_strtable.lookup("ui_dumping_value_items")->c_str(), worn);
                {
                    tstring no("ui_no");
                    tstring yes("ui_yes");
                    tstring msg(buf);
                    g_windows.box()->setup2(&msg, g_strtable.lookup(yes), g_strtable.lookup(no), (int)q);
                }
                g_windows.box()->f04 = 2;
                g_windows.set_state(0);
            } else {
                eaten = 1;
            }

            int food = g_game->troops->equip[9].item;
            if (food >= 0 && ITYPE(food) == 1) {
                WL_MARKER(4)
                int* q = &q_food;
                *q = -1;
                char buf[0x200];
                mb_snprintf(buf, 0x200, g_strtable.lookup("ui_question_slaughter_food_and_eat")->c_str(), shown_name(&g_items[food]));
                {
                    tstring no("ui_no");
                    tstring yes("ui_yes");
                    tstring msg(buf);
                    g_windows.box()->setup2(&msg, g_strtable.lookup(yes), g_strtable.lookup(no), (int)q);
                }
                g_windows.set_state(0);
                eaten = 0;
                WL_MARKER(5)
            }
        }
    }

    if (q_food == 0) {
        WL_MARKER(4)
        q_food = -1;
        ItemRef* food = &TROOP(g_game->player)->equip[9];
        food->item = 1;
        food->m4 = 0;
        food->reduce(0);
        set_slot_flag(2, 9, 2, 1);
        g_sound_mgr.play(g_sound_mgr.slot[0x76], 1, ((float*)&g_sound_mgr.frame.o), 1.0f, 0);
        g_sound_mgr.play(g_sound_mgr.slot[0x61], 1, ((float*)&g_sound_mgr.frame.o), 1.0f, 0);
        init_window();
        WL_MARKER(5)
    }

    // an unanswered question keeps the screen open (0x510479..0x51048f); the merchant's gold goes to the player
    // only when there is no pay question (0x510495)
    if (!eaten && q_pay != 0 && q_dump != 0)
        return;
    if (q_pay == 0) {
        g_game->add_gold(TROOP(w14)->f3b4, g_game->player);
        TROOP(w14)->f3b4 = 0;
    }

    if (w14 >= 3) {
        const unsigned __int64& f = TROOP(w14)->flags;
        if ((f & 0x10) == 0 || (f & 0x1000) == 0) {
            if (w04 == 4)
                ((TroopRec*)TROOP(w14))->auto_equip(-1, 0x100e, 0, 0);
        }
    }

    int state = w18;
    if (state == 0x64)
        g_map_flags.commit(&eaten);
    else
        g_windows.set_state(state);
}


// @ 0x510850
__declspec(noinline) void PartyWindow::set_slot_flag(int page, int idx, unsigned mask, char on)
{
    if (page < 2) {
        if (on)
            flags[page][idx] |= mask;
        else
            flags[page][idx] &= ~mask;
    } else {
        if (on)
            row_state[page - 2][idx] |= mask;
        else
            row_state[page - 2][idx] &= ~mask;
    }
}

// set_overlay(0, true) written out (0x42cdb0's body; the object's slot 0 is its deleting destructor)
static __forceinline void close_tooltip(gui_manager* g)
{
    void* o = g->overlay;
    if (o != 0)
        ((GuiObjV*)o)->d0(1);
    g->panel_color = -1;
    g->panel_bg = 0xff101010;
    for (int i = 0; i < 0x50; i++)
        g->lines[i].assign("");
    g->fa4 = 0;
    g->fa8 = -0.03f;
    g->fa0 = 0;
    g->overlay = 0;
    g->sel_ent = 0;
}

// @ 0x5108d0
void PartyWindow::refresh_lists(int unused)
{
    for (int page = 0; page < pages; page++) {
        int troop = g_game->player;
        if (page != 0)
            troop = w14;
        int count;
        if (troop == GD_I(0x90) && w04 == 0)
            count = 9;
        else
            count = TROOP(troop)->inv_count();
        for (int i = 0; i < count; i++)
            if (widgets[page][i]->clear_mask(4))
                g_gui.set_overlay(0, true);
        for (int i = 0; i < count; i++)
            if (widgets[page][i]->clear_mask(2) && locked == 0)
                place_slot(widgets[page][i], &EQUIP(troop, i), page, i);
    }

    for (int page = 0; page < rows; page++) {
        int troop = g_game->player;
        if (page != 0)
            troop = w14;
        for (int i = 0; i < 10; i++)
            if (row_slot[page][i] != 0 && row_slot[page][i]->clear_mask(4) && locked == 0)
                g_gui.set_overlay(0, true);
        for (int i = 0; i < 10; i++)
            if (row_slot[page][i] != 0 && row_slot[page][i]->clear_mask(2) && locked == 0)
                place_slot(row_slot[page][i], &GRID(troop, i), 2, i);
    }

    if (locked != 0)
        close_tooltip(&g_gui);

    for (int page = 0; page < pages; page++) {
        if (!page_widget[page]->clear_mask(0x10000))
            continue;
        int troop = g_game->player;
        if (page != 0)
            troop = w14;
        int count;
        if (troop == GD_I(0x90) && w04 == 0)
            count = 9;
        else
            count = TROOP(troop)->inv_count();
        if (count <= 0)
            continue;
        for (int i = 0; i < count; i++) {
            if (!widgets[page][i]->clear_mask(0x10000))
                continue;
            int idx = i;
            if (locked != 0 && page != 0) {
                int r = TROOP(troop)->find_free();
                if (!(troop == GD_I(0x90) && w04 == 0 && r >= 9) && r >= 0)
                    idx = r;
            }
            transfer_slot(&EQUIP(troop, idx), page, idx);
            place_slot(widgets[page][i], &EQUIP(troop, i), page, i);
            if ((g_keyboard.down[0x1d] != 0 || g_keyboard.down[0x9d] != 0 || g_keyboard.down[0x38] != 0 || g_keyboard.down[0xb8] != 0) &&
                locked != 0 && pages == 2) {
                int other = g_game->player;
                if (page == 0)
                    other = w14;
                int r = TROOP(other)->find_free();
                if (!(other == GD_I(0x90) && w04 == 0 && r >= 9) && r >= 0)
                    transfer_slot(&EQUIP(other, r), 1 - page, r);
            }
        }
    }

    for (int page = 0; page < rows; page++) {
        if (!panels[page]->clear_mask(0x10000))
            continue;
        for (int i = 0; i < 10; i++) {
            if (row_slot[page][i] == 0)
                continue;
            if (!row_slot[page][i]->clear_mask(0x10000))
                continue;
            int troop = g_game->player;
            if (page != 0)
                troop = w14;
            transfer_slot(&GRID(troop, i), page + 2, i);
            place_slot(row_slot[page][i], &GRID(troop, i), 2, i);
            if ((g_keyboard.down[0x1d] != 0 || g_keyboard.down[0x9d] != 0 || g_keyboard.down[0x38] != 0 || g_keyboard.down[0xb8] != 0) &&
                locked != 0 && rows == 2 && page == 1) {
                int t = g_game->player;
                Troop* base = g_game->troops;
                Troop* tr = base + t;
                int idx = -1;
                bool found = false;
                int count = tr->inv_count();
                for (int k = 0; !found && k < count; k++) {
                    if (tr->inv[k].item == -1) {
                        idx = k;
                        found = true;
                    }
                }
                if (idx >= 0)
                    transfer_slot(&((ItemRef*)((char*)base + 0xb4))[t * 0xa1 + idx], 0, idx);
            }
        }
    }

    init_window();
}

// @ 0x510f50
void PartyWindow::place_slot(MissionUi* w, ItemRef* slot, int page, int index)
{
    float pt[2];
    if (slot->item >= 0) {
        pt[0] = w->fE0 + 0.044f;
        pt[1] = w->fE4 + 0.044f;
        place_slot_mesh(slot, pt, page, index);
    }
}


static __forceinline float item_weight(const ItemRef* r) { return r->item < 0 ? 0.0f : g_items[r->item].f208; }
static __forceinline int item_food(const ItemRef* r) { return r->item < 0 ? 0 : g_items[r->item].f1fc; }
static __forceinline int item_accuracy(const ItemRef* r)
{
    if (r->item < 0)
        return 100;
    int v = g_items[r->item].f204;
    if (v == 0)
        v = 99;
    return v;
}
static __forceinline int slot_aux(PartyWindow* w, int page, int index)
{
    return page < 2 ? w->aux[page][index] : w->row_value[page - 2][index];
}
static __forceinline int cap_100(int v) { return v > 100 ? 100 : v; }
static __forceinline int round_near(float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

// @ 0x510fa0
// ------------------------------------------------------------------------------------------------------------
void PartyWindow::place_slot_mesh(ItemRef* slot, float* pt, int page, int index)
{
    ItemKind* it = &g_items[slot->item];
    g_gui.set_overlay(0, 1);
    g_gui.set_view(pt);

    char buf[512];
    slot->name(buf);
    { tstring s(buf); g_gui.draw_string(&s, 0.025f, 0xfff0e0d0, 1); }

    {
    unsigned int flags = it->type_flags;
    unsigned int kind = flags & 0xff;
    if (kind == 2 || kind == 3 || kind == 4) {
        if (kind == 4) {
            if (flags & 0x200000)
                sprintf(buf, g_strtable.lookup("ui_polearm_no_shield")->c_str());
            else
                sprintf(buf, g_strtable.lookup("ui_polearm")->c_str());
        } else if (kind == 3) {
            if (flags & 0x200000)
                sprintf(buf, g_strtable.lookup("ui_two_handed")->c_str());
            else
                sprintf(buf, g_strtable.lookup("ui_two_handed_one_handed")->c_str());
        } else
            sprintf(buf, g_strtable.lookup("ui_one_handed")->c_str());
        { tstring s(buf); g_gui.draw_string(&s, 0.021f, 0xffc0b0e0, 1); }
    }

    }

    char buyable = test_slot_flag(page, index, 1);

    {
    unsigned int flags = it->type_flags;
    unsigned int kind = flags & 0xff;
    if (kind == 10 || kind == 5 || kind == 6 || kind == 0x12) {
        int amt = slot->m4 & 0xff;
        int cur = slot->max_amount() - amt;
        sprintf(buf, "(%d / %d)", cur, slot->max_amount());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
    } else if (kind == 7) {
        sprintf(buf, "(%d / %d)", slot->m4 & 0xffff, slot->stat_1e8());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
    } else if (kind == 0xb && (flags & 0x2000000)) {
        int amt = slot->m4 & 0xff;
        int cur = slot->max_amount() - amt;
        sprintf(buf, "(%d / %d)", cur, slot->max_amount());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
    }

    }
    if (page != 1) {
        if (buyable)
            sprintf(buf, g_strtable.lookup("ui_return_price")->c_str(), slot_aux(this, page, index));
        else
            sprintf(buf, g_strtable.lookup("ui_sell_price")->c_str(), compute_price((int*)slot, 0));
    } else {
        if (buyable)
            sprintf(buf, g_strtable.lookup("ui_reclaim_price")->c_str(), slot_aux(this, 1, index));
        else
            sprintf(buf, g_strtable.lookup("ui_buying_price")->c_str(), compute_price((int*)slot, 1));
    }
    { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xfffffdf0, 1); }

    if (item_weight(slot) > 1e-5f) {
        sprintf(buf, g_strtable.lookup("ui_weight")->c_str(), (double)item_weight(slot));
        { tstring s(buf); g_gui.draw_string(&s, 0.022f, 0xffffffff, 1); }
    }

    {
    unsigned int kind = it->type_flags & 0xff;
    if (kind == 12 || kind == 13 || kind == 14 || kind == 15) {
        if (slot->stat_1fc()) {
            sprintf(buf, g_strtable.lookup("ui_plus_value_to_head_armor")->c_str(), slot->stat_1fc());
            { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        }
        if (slot->stat_200()) {
            sprintf(buf, g_strtable.lookup("ui_plus_value_to_body_armor")->c_str(), slot->stat_200());
            { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        }
        if (slot->stat_204()) {
            sprintf(buf, g_strtable.lookup("ui_plus_value_to_leg_armor")->c_str(), slot->stat_204());
            { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        }
    }

    }
    {
    unsigned int kind = it->type_flags & 0xff;
    if (kind == 8 || kind == 9 || kind == 10 || kind == 16 || kind == 17 ||
        kind == 2 || kind == 3 || kind == 4) {
        if (slot->stat_21x(0)) {
            int t = ((int)it->f218 >> 8) & 3;
            sprintf(buf, g_strtable.lookup("ui_swing")->c_str(), slot->stat_21x(0),
                    t == 0 ? g_strtable.lookup("ui_cut_short")->c_str()
                           : t == 1 ? g_strtable.lookup("ui_pierce_short")->c_str()
                                    : g_strtable.lookup("ui_blunt_short")->c_str());
            { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffb0a0, 1); }
        }
        if (slot->stat_21x(1)) {
            int t = ((int)it->f214 >> 8) & 3;
            unsigned int kind = it->type_flags & 0xff;
            bool direct = kind == 8 || kind == 9 || kind == 10 || kind == 16 || kind == 17;
            if (direct) {
                sprintf(buf, g_strtable.lookup("ui_damage")->c_str(), slot->stat_21x(1),
                        t == 0 ? g_strtable.lookup("ui_cut_short")->c_str()
                               : t == 1 ? g_strtable.lookup("ui_pierce_short")->c_str()
                                        : g_strtable.lookup("ui_blunt_short")->c_str());
            } else {
                sprintf(buf, g_strtable.lookup("ui_thrust")->c_str(), slot->stat_21x(1),
                        t == 0 ? g_strtable.lookup("ui_cut_short")->c_str()
                               : t == 1 ? g_strtable.lookup("ui_pierce_short")->c_str()
                                        : g_strtable.lookup("ui_blunt_short")->c_str());
            }
            { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffb0a0, 1); }
        }
        unsigned int kind = it->type_flags & 0xff;
        if (kind == 8 || kind == 9 || kind == 16 || kind == 17) {
            int v = item_accuracy(slot);
            sprintf(buf, g_strtable.lookup("ui_accuracy")->c_str(), cap_100(v));
            { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffeebbfe, 1); }
        }
        if (slot->stat_1f0a()) {
            sprintf(buf, g_strtable.lookup("ui_speed_rating")->c_str(), slot->stat_1f0a());
            { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        }
    } else if (kind == 5 || kind == 6 || kind == 0x12) {
        if (slot->stat_21x(1)) {
            sprintf(buf, g_strtable.lookup("ui_value_to_damage")->c_str(), slot->stat_21x(1) > 0 ? '+' : ' ', slot->stat_21x(1));
            { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffb0a0, 1); }
        }
    } else if (kind == 7) {
        sprintf(buf, g_strtable.lookup("ui_resistance")->c_str(), slot->stat_200());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        sprintf(buf, g_strtable.lookup("ui_size")->c_str(), (int)((float)it->weight * g_one));
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        sprintf(buf, g_strtable.lookup("ui_speed_rating")->c_str(), slot->stat_1f0a());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
    } else if (kind == 0xb && g_module_settings.disable_food_slot == 0 && item_food(slot) > 0) {
        sprintf(buf, g_strtable.lookup("ui_value_to_morale")->c_str(), (double)((float)item_food(slot) * 0.02f));
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffa0f0b0, 1); }
    }

    }
    {
    unsigned int kind = it->type_flags & 0xff;
    if (kind == 2 || kind == 3 || kind == 4) {
        float reach = (float)it->weight * g_one;
        sprintf(buf, g_strtable.lookup("ui_weapon_reach")->c_str(), round_near(&reach));
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
    }

    }
    if (*(char*)&it->type_flags == 1) {
        g_game->skill_level(g_game->player, 0x18);
        sprintf(buf, g_strtable.lookup("ui_armor")->c_str(), slot->stat_200());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffddddff, 1); }
        sprintf(buf, g_strtable.lookup("ui_speed")->c_str(), slot->stat_1f4());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        sprintf(buf, g_strtable.lookup("ui_maneuver")->c_str(), slot->stat_1f0b());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        sprintf(buf, g_strtable.lookup("ui_charge")->c_str(), slot->stat_214());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
        sprintf(buf, g_strtable.lookup("ui_hit_points")->c_str(), slot->stat_1e8(), slot->stat_1e8());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffffffff, 1); }
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
        } else if (b >= 0) {
            SkillNames* sk = (SkillNames*)&g_skill_tbl[b];
            const tstring* name = g_strtable.find(&sk->f20);
            if (!name)
                name = &sk->f10;
            sprintf(need, name->c_str());
        } else if (c >= 0) {
            weapon_class_caption(c, need);
        }
        sprintf(buf, g_strtable.lookup("ui_requires_value_difficulty")->c_str(), need, slot->stat_1ec());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, color, 1); }
    }

    if (it->type_flags & 0x4000000) {
        sprintf(buf, g_strtable.lookup("ui_bonus_against_shields")->c_str());
        { tstring s(buf); g_gui.draw_string(&s, 0.024f, 0xffbbff22, 1); }
    }
    if (it->type_flags & 0x4000)
        g_gui.draw_string(g_strtable.lookup("ui_cant_be_used_to_block"), 0.024f, 0xffccbb33, 1);
    if (it->type_flags & 0x10100000)
        g_gui.draw_string(g_strtable.lookup("ui_cannot_be_used_on_horseback"), 0.024f, 0xffccbb33, 1);

    for (int i = 0; i < 8; i++) {
        g_script.trigger_result.store0 = 0;
        g_script.trigger_result.store1 = 0;
        g_script.result_string.assign("");
        GV->emit_event4(0x13, slot->item, i, *(signed char*)((char*)slot + 7));
        if (g_script.result_string.len > 0) {
            unsigned int color = 0xffffffff;
            if (g_script.trigger_result.value != 0)
                color = (unsigned int)g_script.trigger_result.value | 0xff000000;
            g_gui.draw_string(&g_script.result_string, 0.024f, color, 1);
        }
    }
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
    if (g_scripts.used[id] >= 0)
        r = g_scripts[g_scripts.used[id]].body.emit(0x200000, 0, &count, vals, tag);
    return r;
}

// @ 0x5126e0
char PartyWindow::test_slot_flag(int page_idx, int slot, unsigned mask)
{
    if (page_idx < 2)
        return (flags[page_idx][slot] & mask) != 0;
    return (row_state[page_idx - 2][slot] & mask) != 0;
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
        item_mod* m = &g_item_mods[mod];
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
void PartyWindow::compute(int* item, int price_mode)
{
    char trade = 0;
    char grid = 0;
    if (f_abc != 0 || w04 == 1 || w04 == 3 || w04 == 4)
        trade = 1;
    if (w04 == 2)
        grid = 1;
    char plain;
    if (price_mode != 1)
        plain = 1;
    else
        plain = 0;
    GV->price((ItemRef*)item, w14, plain, trade, grid);
}

// @ 0x5129d0
int GameDataV::price(ItemRef* item, int idx, char adjust, char plain, char scaled)
{
    if (item->item < 0)
        return 0;
    if (g_items[item->item].value <= 0)
        return 0;

    PartyMgr* pool = &parties;
    int pi = pool->lookup(f78);
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
        g_script.trigger_result.value = 100;
        int mode = 3;
        if (adjust)
            mode = 4;
        if (emit_event(mode, item->item, (int)value)) {
            result = (int)((float)g_script.trigger_result.value * value * 0.01f);
        } else {
            float v2 = value;
            if (scaled)
                v2 = value * g_items[item->item].f21c;
            int row = *(int*)((char*)troops + idx * 0x508 + 0x4c);
            float tf = *(float*)((char*)factions + row * 0x300 + 0x2c);
            // a bad relation weighs 3x, a good one 0.5x (0x512be7..0x512c75)
            if (tf < -1.0f)
                tf = -1.0f;
            else if (tf > 1.0f)
                tf = 1.0f;
            if (tf < 0.0f)
                tf *= 3.0f;
            else
                tf *= 0.5f;
            tf = (1.0f - tf) * f + 1.0f;
            if (adjust)
                tf = 1.0f / tf;
            float last = tf * v2;
            result = round_float(&last);
        }
    }
    if (result < 1)
        result = 1;
    return result;
}


// @ 0x512c80
void PartyWindow::transfer_slot(ItemRef* item, int page, int slot)
{
    if ((locked != 0 && can_transfer(&held, page, slot)) || (locked == 0 && item->item != -1)) {
        if (!(w04 == 2 && try_place(item, page, slot))) {
    if (locked != 0) {
        ItemRef* h = &held;
        if (h->item == item->item && item->item >= 0 && ITYPE(item->item) == 0x12 &&
            held.b.amount != 0 && item->b.amount != 0) {
            // same stackable: pour as much as fits from the hand into the held stack
            g_gui.play_sound();
            int n = item->m4 & 0xff;
            int have = h->m4 & 0xff;
            int room = h->max_amount() - have;
            if (n > room)
                n = room;
            item->reduce(n);
            h->add_amount(n);
            int have2 = h->m4 & 0xff;
            int left = h->max_amount() - have2;
            if (left > 0)
                return;
            h->item = -1;
            g_gui.set_drag(0);
            locked = 0;
            return;
        }
    }
    // pick the slot's contents up into the hand
    g_gui.play_sound();
    ItemRef tmp = *item;
    char fl = test_slot_flag(page, slot, 1);
    int sv = (page < 2) ? (int)aux[page][slot] : (int)row_value[page - 2][slot];
    item->item = -1;
    or_slot_flag(page, slot);
    int hv;
    if (locked != 0) {
        char hf = held_flag;
        hv = held_value;
        set_slot_flag(page, slot, 1, hf);
        set_slot_value(page, slot, hf ? hv : 0x1e61);
        if (w04 == 2 && (page == 1 ? held_page != page : held_page == 1)) {
            // trade mode: price the move, unless it stays on the same side
            int price = held_flag ? held_value : compute_price((int*)&held, held_page);
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
        // hand the held item back to the slot
        *item = held;
        or_slot_flag(page, slot);
        held.item = -1;
        delete g_gui.drag;
        g_gui.drag = 0;
        locked = 0;
    }
    // take the slot's contents into the hand
    if (tmp.item >= 0) {
        held = tmp;
        held_page = page;
        held_flag = fl;
        held_value = sv;
        int idx = g_game->player;
        if (page == 1 || page == 3) {
            if (fl == 0)
                idx = w14;
        } else {
            if (fl != 0)
                idx = w14;
        }
        rgl::mesh* m = make_mesh(&held, &hv, idx);
        m->scale(2.0f);
        delete g_gui.drag;
        g_gui.drag = m;
        locked = 1;
    }

            return;
        }
    }
    if (locked != 0) {
    char buf[0x100];
    if (page < 2)
        return;
    if (slot == 8 && ITYPE(held.item) == 1) {
        if (page == 2) {
            tstring key("ui_notification_riding_skill_not_enough");
            g_gui.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else if (slot >= 0 && slot < 4 &&
               ((g_items[held.item].type_flags & 0xff) == 2 || (g_items[held.item].type_flags & 0xff) == 3 ||
                (g_items[held.item].type_flags & 0xff) == 4 || (g_items[held.item].type_flags & 0xff) == 8 ||
                (g_items[held.item].type_flags & 0xff) == 9 || (g_items[held.item].type_flags & 0xff) == 10 ||
                (g_items[held.item].type_flags & 0xff) == 16 || (g_items[held.item].type_flags & 0xff) == 17)) {
        if (page == 2) {
            tstring key("ui_notification_requirements_not_met");
            g_gui.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else if (((g_items[held.item].type_flags & 0xff) == 12 && slot == 4) ||
               ((g_items[held.item].type_flags & 0xff) == 13 && slot == 5) ||
               ((g_items[held.item].type_flags & 0xff) == 15 && slot == 7) ||
               ((g_items[held.item].type_flags & 0xff) == 14 && slot == 6)) {
        if (page == 2) {
            tstring key("ui_notification_requirements_not_met");
            g_gui.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else {
        return;
    }
    {
        tstring key("ui_troop_cant_use_item");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                ((TroopRec*)((char*)g_game->chars + w14 * 0x508))->display_name()->c_str());
    }
    g_gui.add(tstring(buf), 0xffff7700);

    }
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x513230
char PartyWindow::can_transfer(ItemRef* item, int where, int flag)
{
    char r = 1;
    if (where != 0) {
        if (where >= 2) {
            int idx = g_game->player;
            if (where - 2 != 0)
                idx = w14;
            r = ((TroopRec*)((char*)g_game->chars + idx * 0x508))->fits_slot(item, flag);
            if (flag == 9 && (w04 == 2 || w04 == 4)) {
                char k = ITYPE(item->item);
                if (k == 1)
                    r = 0;
            }
        } else if (where == 1 && w04 == 3) {
            char k = ITYPE(item->item);
            if (k == 1)
                r = 0;
        }
    }
    return r;
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x5132e0
void PartyWindow::set_slot_value(int page, int slot, int value)
{
    if (page < 2)
        aux[page][slot] = (unsigned)value;
    else
        row_value[page - 2][slot] = (unsigned)value;
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x513320
void PartyWindow::or_slot_flag(int page, int slot)
{
    if (page < 2)
        flags[page][slot] |= 2;
    else
        row_state[page - 2][slot] |= 2;
}

// ---------------------------------------------------------------------------------------------------------

// @ 0x513360
void PartyWindow::add_cost(int delta)
{
    char buf[0x200];

    pending += delta;
    if (pending < 0) {
        tstring key("ui_notification_payment_value");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                GV->GameData::string_value(-pending).c_str());
    } else if (pending > 0) {
        tstring key("ui_notification_payment_receive_value");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                GV->GameData::string_value(pending).c_str());
    } else {
        tstring key("ui_no_cost");
        sprintf(buf, g_strtable.lookup(key)->c_str());
    }
    {
        tstring msg(buf);
        float sz[2];
        total_cost->set_text(sz, &msg);
    }
}


// @ 0x5134c0
bool PartyWindow::try_place(ItemRef* item, int page, int slot)
{
    bool found = false;
    int id = item->item;

    if (id == -1)
        goto done;
    if (test_slot_flag(page, slot, 1))
        goto done;
    if (ITYPE(id) != 0xb)
        goto done;

    if (page == 1) {
        GameData* gd = g_game;
        int idx = w14;
        int n;

        if (idx == *(int*)((char*)gd + 0x90) && w04 == 0)
            n = 9;
        else
            n = gd->troops[idx].inv_count();
        for (int i = 0; i < n; i++) {                       // (n <= 0: nothing found, the original returns at once: 0x513554)
            if (gd->troops[idx].inv[i].item == id && (flags[1][i] & 1))
                found = true;
        }
    }
    else {
        GameData* gd = g_game;
        int idx = gd->player;
        int n;

        if (idx == *(int*)((char*)gd + 0x90) && w04 == 0)
            n = 9;
        else
            n = gd->troops[idx].inv_count();
        for (int i = 0; i < n; i++) {                       // (the loop's own entry test is the original's `jle 0x51360b`)
            if (gd->troops[idx].inv[i].item == id && (flags[0][i] & 1))
                found = true;
        }
        for (int i = 0; i < 10; i++) {
            if (gd->troops[idx].equip[i].item == id && (row_state[0][i] & 1))
                found = true;
        }
    }

    if (found) {
        if (page == 1) {
            tstring msg("ui_reclaim_your_sold_goods");
            g_gui.add(*g_strtable.lookup(msg), -1);
        } else {
            tstring msg("ui_return_your_bought_goods");
            g_gui.add(*g_strtable.lookup(msg), -1);
        }
    }
done:
    return found;
}

