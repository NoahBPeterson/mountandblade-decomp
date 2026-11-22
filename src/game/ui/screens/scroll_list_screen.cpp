//-----------------------------------------------------------------------------
// File: scroll_list_screen.cpp  (0x533a60..0x5342a6)
//
// Desc: the scroll-list screen (MWidget::get3, ItemD::frame_update), the vector-iterator helper and the two
//       item-source hooks. Split from mission_menu_and_exchange_screens.cpp at the original's file boundary 0x533a60: Rec58::frame_update
//       0x5337a0 matches only when get3's body is not in its unit (codegen.md 16, "a same-unit callee body can hide a
//       translation-unit boundary"). Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "platform/image_addrs.h"
#include "game/mission/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): listed directly (the resolution audit reads only the source's own includes)
#include "game/common/game_globals.h"   // g_trial_mode (canonical)
#include <stdlib.h>
#include <math.h>
#include "game/ui/screens/mission_menu_and_exchange_screens.h"
#include "game/config/config_block.h"      // g_cfg (0x5a9f98); cheat_mode is +0x24 = 0x5a9fbc
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "game/ui/hud/mission_hud_buttons.h"
#include "game/data/game_data.h"
#include "game/party/party.h"
#include "game/data/troop.h"
#include "game/ui/gui_manager.h"
#include "game/ui/widgets/ui_common.h"
#include <vector>
#include "game/data/str_table.h"
#include "game/dialog/dialog_rec.h"
#include "game/ui/window_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/party/party_x.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/presentation.h"      // widget_mgr::pick 0x4d5410
#include "game/ui/widgets/ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "platform/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "game/data/menu_rec.h"   // g_menus 0x5e29b0
#include "game/common/shared_vars.h"
#include "game/input/key_names.h"
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "game/input/slot.h"
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object

// the animation helpers of mission_menu_and_exchange_screens.cpp (file-local there too)
static __forceinline float anim_value(UiD* vs)
{
    float* base = &vs->rect_max[vs->f178];
    float a = vs->v168 - vs->f16c;
    float b = vs->f170 - vs->f16c;
    return (a / b) * (base[0] - vs->f17c)
         + ((float)g_gui.wheel / base[0]) * vs->f17c * 0.0005f;
}

// the first term of anim_value alone (0x533bda..0x533c09: no clock term)
static __forceinline float anim_ratio(UiD* vs)
{
    float* base = &vs->rect_max[vs->f178];
    float a = vs->v168 - vs->f16c;
    float b = vs->f170 - vs->f16c;
    return (a / b) * (base[0] - vs->f17c);
}

// one step through the page history: push the page shown onto one list, drop the top page of the other
static __forceinline void history_drop(std::vector<int>& from)
{
    from.resize(from.size() - 2, 0);   // 0x43f310
}
static __forceinline void step_back(ItemD* s)
{
    s->fwd_pages.push_back(s->page);   // 0x440b30
    s->fwd_pages.push_back(s->arg);
    history_drop(s->back_pages);
}
static __forceinline void step_forward(ItemD* s)
{
    s->back_pages.push_back(s->page);
    s->back_pages.push_back(s->arg);
    history_drop(s->fwd_pages);
}
static __forceinline void release_pages(std::vector<int>& v)
{
    std::vector<int>().swap(v);   // frees the block and zeroes the triple
}
static __forceinline void clear_second(ItemD* s)
{
    release_pages(s->fwd_pages);
}
static __forceinline void clear_anim(ItemD* s)
{
    release_pages(s->back_pages);
    release_pages(s->fwd_pages);
}



// @ 0x533a60
void MWidget::get3(int* a, int* b, int* c)
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
            && data_panel->rect_max.y > data_scroll->rect_max.y) {
            UiD* vs = data_scroll->follow[1];
            vs->on_value_changed((float)g_gui.wheel);
            UiD* vs2 = data_scroll->follow[1];
            vs2->set_key_flags(0x10000);
        } else {
            UiD* vs = keys_scroll->follow[1];
            vs->set_value(anim_value(vs));
            UiD* vs2 = keys_scroll->follow[1];
            vs2->set_key_flags(0x10000);
        }
    }

    if (btn_back->clear_mask(0x10000)) {
        step_back(this);
        int a98 = back_sel_a;
        if (page == a98) {
            UiD* vs = keys_scroll->follow[1];
            scroll_pos = anim_ratio(vs);
        }
        build(a98, back_sel_b);
    }

    if (btn_forward->clear_mask(0x10000)) {
        step_forward(this);
        int aa0 = fwd_sel_a;
        if (page == aa0) {
            UiD* vs = keys_scroll->follow[1];
            scroll_pos = anim_ratio(vs);
        }
        build(aa0, fwd_sel_b);
    }

    if (report->clear_mask(0x10000)) {
        MWidget* w = report;
        int iv2 = w->pages[w->sel].page;
        int iv1 = w->page_args[w->sel];
        clear_second(this);
        if (page >= 0
            && (page != iv2 || arg != iv1)) {
            back_pages.push_back(page);
            back_pages.push_back(arg);
        }
        {
            UiD* vs = keys_scroll->follow[1];
            scroll_pos = anim_ratio(vs);
        }
        build(iv2, iv1);
    }

    if (links->clear_mask(0x10000)) {
        MWidget* w = links;
        int iv1 = w->page_args[w->sel];
        int iv2 = w->pages[w->sel].page;
        clear_second(this);
        if (page >= 0
            && (page != iv2 || arg != iv1)) {
            back_pages.push_back(page);
            back_pages.push_back(arg);
        }
        if (page == iv2) {
            UiD* vs = keys_scroll->follow[1];
            scroll_pos = anim_ratio(vs);
        }
        build(iv2, iv1);
    }

    if (btn_troops->clear_mask(0x10000)) {
        clear_anim(this);
        build(1, 0);
    }
    if (btn_factions->clear_mask(0x10000)) {
        clear_anim(this);
        build(2, 0);
    }
    if (btn_locations->clear_mask(0x10000)) {
        clear_anim(this);
        build(3, 0);
    }
    if (btn_quests->clear_mask(0x10000)) {
        clear_anim(this);
        build(4, 0);
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
// defined here, not in game/ui/hud/agent_hud.cpp, because a body visible there makes cl treat the call in _Buy 0x4fd490 as noreturn
template<> void std::vector<NotePage>::_Xlen() const
{
    _THROW(length_error, "vector<T> too long");
}

// @ 0x534180
void ItemB::frame_update(float arg)
{
    pres.pick((int*)&arg);
    if ((float)(g_ticks[pres.tick.clock] - pres.tick.start) * 1e-5f > pres.duration) {
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
    pres.pick((int*)&arg);
    if ((float)(g_ticks[pres.tick.clock] - pres.tick.start) * 1e-5f > pres.duration) {
        int id = w18;
        if (id == 100) {
            g_map_flags.commit((char*)&arg);
            return;
        }
        g_windows.set_state(id);
    }
}
