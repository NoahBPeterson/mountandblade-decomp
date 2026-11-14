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

#define F(p, off, type) (*(type*)((char*)(p) + (off)))
#define W(p, off) (*(ChoiceList**)((char*)(p) + (off)))
#define V(p, off) ((OutIntVec*)((char*)(p) + (off)))
#define GIF(off, type) (*(type*)((char*)g_game + (off)))
#define STACK_IDS ((int*)((char*)g_game + 0x14058))   // GameData +0x14058: the per-stack random ids (0x20 per stack); +0x1405c is the tbl of game_data.h

// the animation helpers of exchange_screens.cpp (file-local there too)
static __forceinline float anim_value(ValueSource* vs)
{
    float* base = (float*)((char*)vs + 0xf0 + F(vs, 0x178, int) * 4);
    float a = F(vs, 0x168, float) - F(vs, 0x16c, float);
    float b = F(vs, 0x170, float) - F(vs, 0x16c, float);
    return (a / b) * (base[0] - F(vs, 0x17c, float))
         + ((float)g_gui.wheel / base[0]) * F(vs, 0x17c, float) * 0.0005f;
}

// the first term of anim_value alone (0x533bda..0x533c09: no clock term)
static __forceinline float anim_ratio(ValueSource* vs)
{
    float* base = (float*)((char*)vs + 0xf0 + F(vs, 0x178, int) * 4);
    float a = F(vs, 0x168, float) - F(vs, 0x16c, float);
    float b = F(vs, 0x170, float) - F(vs, 0x16c, float);
    return (a / b) * (base[0] - F(vs, 0x17c, float));
}

static __forceinline void set_anim(TickScreen* s, int push_off, int resize_off)
{
    ((std::vector<int>*)V(s, push_off))->push_back(F(s, 0xa8, int));   // 0x440b30
    ((std::vector<int>*)V(s, push_off))->push_back(F(s, 0xac, int));
    OutIntVec* d = V(s, resize_off);
    unsigned n = d->_Myfirst == 0 ? 0 : (unsigned)(d->_Mylast - d->_Myfirst);
    ((std::vector<int>*)d)->resize(n - 2, 0);   // 0x43f310
}

static __forceinline void clear_vec(void** p)
{
    if (p[0] != 0)
        operator delete(p[0]);
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
}
static __forceinline void clear_second(TickScreen* s)
{
    if (F(s, 0x8c, void*) != 0)
        operator delete(F(s, 0x8c, void*));
    F(s, 0x8c, void*) = 0;
    F(s, 0x90, void*) = 0;
    F(s, 0x94, void*) = 0;
}
static __forceinline void clear_anim(TickScreen* s)
{
    clear_vec((void**)((char*)s + 0x7c));
    clear_vec((void**)((char*)s + 0x8c));
}



// @ 0x533a60
void ChoiceList::get3(int* a, int* b, int* c)
{
    // the two arrays are std::vector<int>s (+0x198 / +0x1a8: pad, first at +0x19c / +0x1ac): operator[] gives the lea + load
    *a = (*(std::vector<int>*)((char*)this + 0x198))[F(this, 0x1cc, int)];
    *b = (*(std::vector<int>*)((char*)this + 0x1a8))[F(this, 0x1cc, int)];
    *c = F(this, 0x1cc, int);
}


// @ 0x533aa0
void ItemD::frame_update(float)
{
    if (g_gui.wheel != 0) {
        if (g_gui.mouse[0] < 0.7f
            && F(W(this, 0x44), 0xf4, float) > F(W(this, 0x4c), 0xf4, float)) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x4c) + 0x484);
            vs->on_value_changed((float)g_gui.wheel);
            ValueSource* vs2 = *(ValueSource**)((char*)W(this, 0x4c) + 0x484);
            vs2->set_key_flags(0x10000);
        } else {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            vs->set_value(anim_value(vs));
            ValueSource* vs2 = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            vs2->set_key_flags(0x10000);
        }
    }

    if (W(this, 0x6c)->clear_mask(0x10000)) {
        set_anim((TickScreen*)this, 0x88, 0x78);
        int a98 = F(this, 0x98, int);
        if (F(this, 0xa8, int) == a98) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_ratio(vs);
        }
        ((note_window*)(void*)this)->build(a98, F(this, 0x9c, int));
    }

    if (W(this, 0x70)->clear_mask(0x10000)) {
        set_anim((TickScreen*)this, 0x78, 0x88);
        int aa0 = F(this, 0xa0, int);
        if (F(this, 0xa8, int) == aa0) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_ratio(vs);
        }
        ((note_window*)(void*)this)->build(aa0, F(this, 0xa4, int));
    }

    if (W(this, 0x34)->clear_mask(0x10000)) {
        ChoiceList* w = W(this, 0x34);
        VecIter it = { F(w, 0x19c, int*) };
        int* second;
        int** r = it.advance(&second, F(w, 0x1cc, int));
        int iv2 = **r;
        int iv1 = (*(std::vector<int>*)((char*)w + 0x1a8))[F(w, 0x1cc, int)];
        clear_second((TickScreen*)this);
        if (F(this, 0xa8, int) >= 0
            && (F(this, 0xa8, int) != iv2 || F(this, 0xac, int) != iv1)) {
            ((std::vector<int>*)V(this, 0x78))->push_back(F(this, 0xa8, int));
            ((std::vector<int>*)V(this, 0x78))->push_back(F(this, 0xac, int));
        }
        {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_ratio(vs);
        }
        ((note_window*)(void*)this)->build(iv2, iv1);
    }

    if (W(this, 0x3c)->clear_mask(0x10000)) {
        ChoiceList* w = W(this, 0x3c);
        VecIter it = { F(w, 0x19c, int*) };
        int* second;
        int** r = it.advance(&second, F(w, 0x1cc, int));
        int iv1 = F(w, 0x1ac, int*)[F(w, 0x1cc, int)];
        int iv2 = **r;
        clear_second((TickScreen*)this);
        if (F(this, 0xa8, int) >= 0
            && (F(this, 0xa8, int) != iv2 || F(this, 0xac, int) != iv1)) {
            ((std::vector<int>*)V(this, 0x78))->push_back(F(this, 0xa8, int));
            ((std::vector<int>*)V(this, 0x78))->push_back(F(this, 0xac, int));
        }
        if (F(this, 0xa8, int) == iv2) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_ratio(vs);
        }
        ((note_window*)(void*)this)->build(iv2, iv1);
    }

    if (W(this, 0x58)->clear_mask(0x10000)) {
        clear_anim((TickScreen*)this);
        ((note_window*)(void*)this)->build(1, 0);
    }
    if (W(this, 0x60)->clear_mask(0x10000)) {
        clear_anim((TickScreen*)this);
        ((note_window*)(void*)this)->build(2, 0);
    }
    if (W(this, 0x64)->clear_mask(0x10000)) {
        clear_anim((TickScreen*)this);
        ((note_window*)(void*)this)->build(3, 0);
    }
    if (W(this, 0x5c)->clear_mask(0x10000)) {
        clear_anim((TickScreen*)this);
        ((note_window*)(void*)this)->build(4, 0);
    }
    if (W(this, 0x50)->clear_mask(0x10000)) {
        slot_as<MenuItem>(25)->f18 = slot_as<MenuItem>(26)->f18;
        slot_as<MenuItem>(25)->f4 = 1;
        g_windows.set_state(0x19);
        return;
    }
    if (W(this, 0x74)->clear_mask(0x10000)) {
        GIF(0x88, char) = 1;
        GIF(0x84, int) = F(this, 0xac, int);
        g_windows.set_state(F(this, 0x18, int));
        return;
    }
    if (W(this, 0x54)->clear_mask(0x10000)) {
        slot_as<MenuItem>(25)->f18 = slot_as<MenuItem>(26)->f18;
        slot_as<MenuItem>(25)->f4 = 0;
        g_windows.set_state(0x19);
        return;
    }
    if (W(this, 0x68)->clear_mask(0x10000) || (g_keyboard.down[0x1] == 0 && g_keyboard.prev[0x1] != 0)) {
        if (F(this, 0x18, int) == 100) {
            char slot;
            g_map_flags.commit((char*)&slot);
            return;
        }
        g_windows.set_state(F(this, 0x18, int));
    }
}

// @ 0x534160
int** VecIter::advance(int** out, int n)
{
    *out = ptr + n;
    return out;
}

// @ 0x534180
void ItemB::frame_update(float arg)
{
    src().pick((int*)&arg);
    if ((float)(g_ticks[src().slot] - *(__int64*)&src().tick_lo) * 1e-5f > f58) {
        int id = w18;
        if (id == 100) {
            g_map_flags.commit((char*)&arg);
            return;
        }
        g_windows.set_state(id);
        return;
    }
    if (GI(0xc8) == 0xf) {
        GI(0xc8) = 0;
        g_windows.set_state(0x1b);
    }
}

// @ 0x534230
void ItemC::frame_update(float arg)
{
    src().pick((int*)&arg);
    if ((float)(g_ticks[src().slot] - *(__int64*)&src().tick_lo) * 1e-5f > f58) {
        int id = w18;
        if (id == 100) {
            g_map_flags.commit((char*)&arg);
            return;
        }
        g_windows.set_state(id);
    }
}
