//-----------------------------------------------------------------------------
// File: list_screens.cpp  (0x533a60..0x5342a6)
//
// Desc: the scroll-list screen (ChoiceList::get3, ItemD::frame_update), the vector-iterator helper and the two
//       item-source hooks. Split from exchange_screens.cpp at the original's file boundary 0x533a60: Rec58::frame_update
//       0x5337a0 matches only when get3's body is not in its unit (codegen.md 16, "a same-unit callee body can hide a
//       translation-unit boundary"). Flags: /O2 /MT /Gy /TP. Names provisional.
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

// the animation helpers of exchange_screens.cpp (file-local there too)
static __forceinline float anim_value(ValueSource* vs)
{
    float* base = &vs->slot[vs->idx];
    float a = vs->f168 - vs->f16c;
    float b = vs->f170 - vs->f16c;
    return (a / b) * (base[0] - vs->step)
         + ((float)g_gui.wheel / base[0]) * vs->step * 0.0005f;
}

// the first term of anim_value alone (0x533bda..0x533c09: no clock term)
static __forceinline float anim_ratio(ValueSource* vs)
{
    float* base = &vs->slot[vs->idx];
    float a = vs->f168 - vs->f16c;
    float b = vs->f170 - vs->f16c;
    return (a / b) * (base[0] - vs->step);
}

// one step through the page history: push the page shown onto one list, drop the top page of the other
static __forceinline void history_drop(std::vector<int>& from)
{
    from.resize(from.size() - 2, 0);   // 0x43f310
}
static __forceinline void step_back(ItemD* s)
{
    s->fwd().push_back(s->page);   // 0x440b30
    s->fwd().push_back(s->arg);
    history_drop(s->back());
}
static __forceinline void step_forward(ItemD* s)
{
    s->back().push_back(s->page);
    s->back().push_back(s->arg);
    history_drop(s->fwd());
}
static __forceinline void clear_vec(owned_triple& t)
{
    if (t.p0 != 0)
        operator delete(t.p0);
    t.p0 = 0;
    t.p1 = 0;
    t.p2 = 0;
}
static __forceinline void clear_second(ItemD* s)
{
    if (s->blk1.p0 != 0)
        operator delete(s->blk1.p0);
    s->blk1.p0 = 0;
    s->blk1.p1 = 0;
    s->blk1.p2 = 0;
}
static __forceinline void clear_anim(ItemD* s)
{
    clear_vec(s->blk0);
    clear_vec(s->blk1);
}



// @ 0x533a60
void ChoiceList::get3(int* a, int* b, int* c)
{
    *a = pages[sel].page;
    *b = page_args[sel];
    *c = sel;
}


// @ 0x533aa0
void ItemD::frame_update(float)
{
    if (g_gui.wheel != 0) {
        if (g_gui.mouse[0] < 0.7f
            && data_panel->screen[1] > data_scroll->screen[1]) {
            ValueSource* vs = data_scroll->src;
            vs->on_value_changed((float)g_gui.wheel);
            ValueSource* vs2 = data_scroll->src;
            vs2->set_key_flags(0x10000);
        } else {
            ValueSource* vs = keys_scroll->src;
            vs->set_value(anim_value(vs));
            ValueSource* vs2 = keys_scroll->src;
            vs2->set_key_flags(0x10000);
        }
    }

    if (btn_back->clear_mask(0x10000)) {
        step_back(this);
        int a98 = back_sel_a;
        if (page == a98) {
            ValueSource* vs = keys_scroll->src;
            scroll_pos = anim_ratio(vs);
        }
        ((note_window*)(void*)this)->build(a98, back_sel_b);
    }

    if (btn_forward->clear_mask(0x10000)) {
        step_forward(this);
        int aa0 = fwd_sel_a;
        if (page == aa0) {
            ValueSource* vs = keys_scroll->src;
            scroll_pos = anim_ratio(vs);
        }
        ((note_window*)(void*)this)->build(aa0, fwd_sel_b);
    }

    if (report->clear_mask(0x10000)) {
        ChoiceList* w = report;
        int iv2 = w->pages[w->sel].page;
        int iv1 = w->page_args[w->sel];
        clear_second(this);
        if (page >= 0
            && (page != iv2 || arg != iv1)) {
            back().push_back(page);
            back().push_back(arg);
        }
        {
            ValueSource* vs = keys_scroll->src;
            scroll_pos = anim_ratio(vs);
        }
        ((note_window*)(void*)this)->build(iv2, iv1);
    }

    if (links->clear_mask(0x10000)) {
        ChoiceList* w = links;
        int iv1 = w->page_args[w->sel];
        int iv2 = w->pages[w->sel].page;
        clear_second(this);
        if (page >= 0
            && (page != iv2 || arg != iv1)) {
            back().push_back(page);
            back().push_back(arg);
        }
        if (page == iv2) {
            ValueSource* vs = keys_scroll->src;
            scroll_pos = anim_ratio(vs);
        }
        ((note_window*)(void*)this)->build(iv2, iv1);
    }

    if (btn_troops->clear_mask(0x10000)) {
        clear_anim(this);
        ((note_window*)(void*)this)->build(1, 0);
    }
    if (btn_factions->clear_mask(0x10000)) {
        clear_anim(this);
        ((note_window*)(void*)this)->build(2, 0);
    }
    if (btn_locations->clear_mask(0x10000)) {
        clear_anim(this);
        ((note_window*)(void*)this)->build(3, 0);
    }
    if (btn_quests->clear_mask(0x10000)) {
        clear_anim(this);
        ((note_window*)(void*)this)->build(4, 0);
    }
    if (btn_game_log->clear_mask(0x10000)) {
        slot_as<head_rec>(25)->w18 = slot_as<head_rec>(26)->w18;
        slot_as<head_rec>(25)->w04 = 1;
        g_windows.set_state(0x19);
        return;
    }
    if (btn_map->clear_mask(0x10000)) {
        g_game->map_live = 1;
        g_game->sel_party = arg;
        g_windows.set_state(w18);
        return;
    }
    if (btn_recent->clear_mask(0x10000)) {
        slot_as<head_rec>(25)->w18 = slot_as<head_rec>(26)->w18;
        slot_as<head_rec>(25)->w04 = 0;
        g_windows.set_state(0x19);
        return;
    }
    if (btn_return->clear_mask(0x10000) || (g_keyboard.down[0x1] == 0 && g_keyboard.prev[0x1] != 0)) {
        if (w18 == 100) {
            char slot;
            g_map_flags.commit((char*)&slot);
            return;
        }
        g_windows.set_state(w18);
    }
}


// @ 0x534160  (the out-of-line instance of the page list's iterator + n; the original's frame_update above calls it for pages[sel])
template std::vector<NotePage>::iterator std::vector<NotePage>::iterator::operator+(difference_type) const;

// vector<NotePage>::_Xlen, <vector>'s body. The original's copy is folded into 0x43e2a0 with every other vector's; it is
// defined here, not in agent5/agent_hud.cpp, because a body visible there makes cl treat the call in _Buy 0x4fd490 as noreturn
template<> void std::vector<NotePage>::_Xlen() const
{
    _THROW(length_error, "vector<T> too long");
}

// @ 0x534180
void ItemB::frame_update(float arg)
{
    src().pick((int*)&arg);
    if ((float)(g_ticks[src().slot] - *(__int64*)&src().tick_lo) * 1e-5f > duration) {
        int id = w18;
        if (id == 100) {
            g_map_flags.commit((char*)&arg);
            return;
        }
        g_windows.set_state(id);
        return;
    }
    if (g_game->mode == 0xf) {
        g_game->mode = 0;
        g_windows.set_state(0x1b);
    }
}

// @ 0x534230
void ItemC::frame_update(float arg)
{
    src().pick((int*)&arg);
    if ((float)(g_ticks[src().slot] - *(__int64*)&src().tick_lo) * 1e-5f > duration) {
        int id = w18;
        if (id == 100) {
            g_map_flags.commit((char*)&arg);
            return;
        }
        g_windows.set_state(id);
    }
}
