//-----------------------------------------------------------------------------
// File: party_window.cpp  (0x501a20..0x502cc0)
//
// Desc: the party screen's inventory window: the equipment menu builder, the slot factory, the page builder, the
//       armour / encumbrance refresh and the slot placement. Names provisional.
//       "T regime" region (docs/tu_map.md): cl emitted this object in source order, so the file keeps the binary
//       order and the helpers that follow their caller (refresh_page -> place_slot -> make_mesh) are
//       __declspec(noinline), as the original calls them. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../../include/image_addrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include "./party_window.h"
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../int_vec.h"
#include "../str_table.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../view_state.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../item_sound.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../config_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui/ui_frame.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// the definition (cls_map/progress_bar.cpp) takes the widget: this module calls it under that signature (the header keeps the void* spelling other modules use)
void __cdecl label_refresh(MissionUi* w, tstring name);      // 0x4e4c70 (cdecl, the name by value)

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

#define EQUIP(t, k) (g_game->troops[(t)].equip[(k)])
#define INV(t, k) (g_game->troops[(t)].inv[(k)])

struct ini_file;
extern ini_file g_game_vars;                // 0xa44240 (viewed as the ConfigTable of config_table.h)
// the config lookup every equipment key goes through: a missing key yields 0. The key is a temporary of the
// caller's expression (it is destroyed after the value is stored).
static __forceinline float cfg(const tstring& n)
{
    float r = 0.0f;
    int i = ((ConfigTable&)g_game_vars).find(n);
    if (i >= 0)
        r = (float)atof(CONFIG_ENTRIES[i].value ? CONFIG_ENTRIES[i].value : (char*)&CONFIG_ENTRIES[i].value);
    return r;
}

// the string-table lookup with a named key (a temporary of the caller's expression)
static inline tstring* lookup(const tstring& k)
{
    return g_strtable.lookup(&k);
}

// (x, y) pair handed to the widget methods
// (x, y) pair handed to the widget methods: Vec2 of include/vec.h (was Vec2 here)

// @ 0x501a20
void* PartyWindow::make_inventory_menu(int idx)
{
    float f_1 = 0.0f;
    if (idx == 1)
        f_1 = -cfg(tstring("inv_equipment_menu_x_a"));
    float f_x = cfg(tstring("inv_equipment_x"));
    float f_y = cfg(tstring("inv_equipment_y"));
    float f_ax = cfg(tstring("inv_armor_x"));
    float f_hy = cfg(tstring("inv_horse_y"));

    // one position per equipment slot (0x501be7..0x501d34 fill esp+0x50..0x9c): four arms, three armour, then the
    // three around the horse slot
    Vec2 pos[10];
    pos[0] = Vec2(f_x, f_y);
    pos[1] = Vec2(f_x, f_y - 0.088f);
    pos[2] = Vec2(f_x, f_y - 0.176f);
    pos[3] = Vec2(f_x, f_y - 0.264f);
    pos[4] = Vec2(f_ax, f_y);
    pos[5] = Vec2(f_ax, f_y - 0.088f);
    pos[6] = Vec2(f_ax, f_y - 0.176f);
    pos[7] = Vec2(f_ax - 0.088f, f_y - 0.088f);
    pos[8] = Vec2(f_ax - 0.088f, f_hy);
    pos[9] = Vec2(f_ax, f_hy - 0.088f);

    PartyPanel* panel = new PartyPanel;
    panels[idx] = panel;
    label_refresh((MissionUi*)panel, tstring("inv_im_equipment_menu"));
    {
        Vec2 v(f_1 + panel->fE0, panel->fE4);
        WOPS(panel)->set_pos(&v.x);
    }
    {
        tstring e("");
        float out[2];
        WOPS(panel)->set_text(out, &e);
    }

    {
        MissionUi* w = new MissionUi;
        {
            Vec2 v(pos[4].x - 0.044f, pos[4].y + 0.095f);
            WOPS(w)->set_pos(&v.x);
        }
        {
            Vec2 v(0.088f, 0.04f);
            WOPS(w)->set_scale(&v.x);
        }
        {
            unsigned f = 0x10;          // the option flags (+0x148), an integer
            WOPS(w)->set_flags(&f);
        }
        {
            float out[2];
            WOPS(w)->set_text(out, lookup(tstring("ui_outfit")));
        }
        WOPS(panel)->add_child(w);
    }
    {
        MissionUi* w = new MissionUi;
        {
            Vec2 v(pos[0].x, pos[0].y + 0.095f);
            WOPS(w)->set_pos(&v.x);
        }
        {
            Vec2 v(0.088f, 0.04f);
            WOPS(w)->set_scale(&v.x);
        }
        {
            unsigned f = 0x10;          // the option flags (+0x148), an integer
            WOPS(w)->set_flags(&f);
        }
        {
            float out[2];
            WOPS(w)->set_text(out, lookup(tstring("ui_arms")));
        }
        WOPS(panel)->add_child(w);
    }
    {
        MissionUi* w = new MissionUi;
        {
            Vec2 v(pos[8].x, pos[8].y + 0.095f);
            WOPS(w)->set_pos(&v.x);
        }
        {
            Vec2 v(0.088f, 0.04f);
            WOPS(w)->set_scale(&v.x);
        }
        {
            unsigned f = 0x10;          // the option flags (+0x148), an integer
            WOPS(w)->set_flags(&f);
        }
        {
            float out[2];
            WOPS(w)->set_text(out, lookup(tstring("ui_horse")));
        }
        WOPS(panel)->add_child(w);
    }
    if (idx == 0 && g_disable_food_slot == 0) {
        MissionUi* w = new MissionUi;
        {
            Vec2 v(pos[9].x, pos[9].y + 0.095f);
            WOPS(w)->set_pos(&v.x);
        }
        {
            Vec2 v(0.088f, 0.04f);
            WOPS(w)->set_scale(&v.x);
        }
        {
            unsigned f = 0x10;          // the option flags (+0x148), an integer
            WOPS(w)->set_flags(&f);
        }
        {
            float out[2];
            WOPS(w)->set_text(out, lookup(tstring("ui_food")));
        }
        WOPS(panel)->add_child(w);
    }

    int troop_idx = g_game->cur_troop;
    if (idx != 0)
        troop_idx = troop;
    for (int i = 0; i < 10; i++) {
        if ((idx < 1 && g_disable_food_slot == 0) || i != 9) {
            MissionUi* slot = make_slot(&EQUIP(troop_idx, i));
            row_slot[idx][i] = slot;
            if (i != 9)
                slot->b11c[0] = 0;
            {
                Vec2 v(pos[i].x, pos[i].y);
                WOPS(slot)->set_pos(&v.x);
            }
            WOPS(panels[idx])->add_child(slot);
            row_state[idx][i] = 2;
        } else {
            row_slot[idx][9] = 0;
            row_state[idx][9] = 0;
        }
    }
    WOPS(container)->add_child(panels[idx]);
    return 0;
}

// @ 0x5021b0
char PartyWindow::mouse_event(unsigned mask)
{
    bool r = false;
    if (mask == 0x10) {
        r = true;
        on_input(0x10000);
    }
    if (base_mouse_event(mask))
        return true;
    return r;
}

// @ 0x5021f0
void* UiFrame::destroy(unsigned flags)
{
    *(void**)this = (void*)vtbl_5924a0;
    release();
    ((vec<mface>*)&f178)->clear();               // 0x41f4b0
    ((WidgetDtor*)this)->base_dtor();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

// @ 0x502230
MissionUi* PartyWindow::make_slot(ItemRef* slot)
{
    PartySlot* w = new PartySlot;
    {
        float v[2];
        v[0] = 0.088f;
        v[1] = 0.088f;
        WOPS(w)->set_scale(v);
    }
    {
        tstring n("inv_slot");
        WOPS(w)->add_image_a(0, g_resmgr_party.get_object(&n)->clone());
    }
    return w;
}

// @ 0x5022c0
char PartyWindow::refresh_event(unsigned mask)
{
    char r = 0;
    if (mask == 0x10) {
        r = 1;
        on_input(0x10000);
    }
    on_input(mask);
    refresh();
    return r;
}

// @ 0x502300
void* WidgetDtor::destroy(unsigned char flag)
{
    base_dtor();
    if (flag & 1)
        ::operator delete(this);
    return this;
}

// @ 0x502320
void* PartyWindow::build_page(int page)
{
    Game* g = g_game;
    int t = g->cur_troop;
    if (page != 0)
        t = troop;
    int n;
    if (t == g->hero_troop && f4 == 0)
        n = 9;
    else
        n = g->troops[t].inv_count();
    float h = (float)((n + 2) / 3) * 0.088f;
    PartyPanel* p = new PartyPanel;
    { Vec2 v(0.0f, 0.0f); WOPS(p)->set_pos(&v.x); }
    { Vec2 v(0.264f, h); WOPS(p)->set_scale(&v.x); }
    int col = 0, row = 1;
    void** wp = widgets[page];
    for (int i = 0; i < n; i++, wp++) {
        MissionUi* w = make_slot(&INV(t, i));
        *wp = w;
        { Vec2 v((float)col * 0.088f, h - (float)row * 0.088f); WOPS(w)->set_pos(&v.x); }
        col++; if (col >= 3) { col = 0; row++; }
        WOPS(p)->add_child(*wp);
        wp[0xc0] = (void*)2; wp[0x180] = (void*)0x12fd1;
    }
    return p;
}

// @ 0x5024e0
void PartyWindow::add_strip_icon(char* slot, const tstring& name, int troop, int kind)
{
    int sel = g_equip_page;
    if (sel < 0)
        return;
    if (*(void**)slot == 0) {
        *(void**)slot = new StripIcon;
        label_refresh(*(MissionUi**)slot, name);
        WOPS(container)->add_child(*(void**)slot);
    }
    WOPS(*(void**)slot)->add_image(0, 0);
    int idx = *(volatile int*)&kind + troop * 4;
    WOPS(*(void**)slot)->add_image(0, g_tableaus.make_mesh_from(&sel, &idx));
}

// @ 0x5025a0
void PartyWindow::init_window()
{
    for (int i = 0; i < pages; i++)
        refresh_page(i);
    for (int i = 0; i < rows; i++)
        refresh_stash(i);
    refresh_armor(armor_labels[0], g_game->cur_troop);
    if (rows == 2)
        refresh_armor(armor_labels[1], troop);
}

// @ 0x502620
void PartyWindow::refresh_armor(void** labels, int t)
{
    int head = 0, body = 0;
    float enc = 0.0f;
    int legs = 0;
    for (int i = 0; i < 8; i++) {
        ItemRef* r = (ItemRef*)&EQUIP(t, i);
        if (r->item < 0)
            continue;
        ItemKind* it = &g_items[r->item];
        if (i >= 4) {
            head += r->stat_1fc();
            body += r->stat_200();
            legs += r->stat_204();
        }
        float v = item_f208(it);
        unsigned mod = it->type_flags & 0xff;
        if (mod == 5 || mod == 6 || mod == 10 || mod == 18) {
            int amt = (int)(r->m4 & 0xff);
            int mx = r->max_amount();
            v = (float)(mx - amt) / (float)r->max_amount() * v;
        } else if (mod == 7 && (int)(r->m4 & 0xffff) <= 0) {
            v = 0.0f;
        }
        enc += v;
    }
    char buf[0x210];
    {
        sprintf(buf, lookup(tstring("ui_head_armor"))->c_str(), head);
    }
    {
        tstring s(buf);
        float out[2];
        WOPS(labels[0])->set_text(out, &s);
    }
    {
        sprintf(buf, lookup(tstring("ui_body_armor"))->c_str(), body);
    }
    {
        tstring s(buf);
        float out[2];
        WOPS(labels[1])->set_text(out, &s);
    }
    {
        sprintf(buf, lookup(tstring("ui_leg_armor"))->c_str(), legs);
    }
    {
        tstring s(buf);
        float out[2];
        WOPS(labels[2])->set_text(out, &s);
    }
    {
        sprintf(buf, lookup(tstring("ui_encumbrance"))->c_str(), enc);
    }
    {
        tstring s(buf);
        float out[2];
        WOPS(labels[3])->set_text(out, &s);
    }
}

// @ 0x502920
void PartyWindow::refresh_page(int page)
{
    Game* g = g_game;
    int t = g->cur_troop;
    if (page != 0)
        t = troop;
    int count;
    if (t == g->hero_troop && f4 == 0)
        count = 9;
    else
        count = g->troops[t].inv_count();
    for (int i = 0; i < count; i++)
        place_slot((MissionUi*)widgets[page][i], &INV(t, i), i, flags[page], page, 0);
}

// @ 0x5029f0
MB_NOINLINE void PartyWindow::place_slot(MissionUi* w, ItemRef* item, int idx, int* state, int page, char flag)
{
    unsigned fl = state[idx];
    if ((fl & 2) == 0)
        return;
    if (item->item == -1) {
        w->b11c[2] = 0;
        if (flag != 0 && idx < 9)
            w->b11c[0] = 0;
    } else {
        int t = g_game->cur_troop;
        if (page == 1) {
            if ((fl & 1) == 0)
                t = troop;
        } else if ((fl & 1) != 0) {
            t = troop;
        }
        volatile float sc = 0.088f;     // (volatile: make_mesh ignores its scale argument, which would otherwise drop this store)
        ResMesh* m = make_mesh(item, (float*)&sc, t);
        rgl::vector4 off(0.044f, 0.044f, 0.0f);
        ((rgl::mesh*)m)->translate(&off);        // ResMesh is the view of rgl::mesh (rgl/mesh.h)
        WOPS(w)->add_image(2, m);
        w->b11c[2] = 1;
        if (flag != 0 && idx < 9)
            w->b11c[0] = 1;
    }
    state[idx] &= ~2u;
    if (icon_a84 != 0 && flag != 0) {
        tstring n("inv_i_character");
        add_strip_icon((char*)&icon_a84, n, g_game->cur_troop, spec_ac0);
    }
    if (icon_a88 != 0 && flag != 0) {
        tstring n("inv_i_character2");
        add_strip_icon((char*)&icon_a88, n, troop, spec_ac4);
    }
}

// @ 0x502b80
MB_NOINLINE ResMesh* PartyWindow::make_mesh(ItemRef* item, float* scale, int t)
{
    ResMesh* m = 0;
    if (item->item >= 0) {
        float s = 0.088f;
        m = (ResMesh*)make_item_mesh(item, &s);
        g_game->f1a1f4 = -1;
        g_game->f1a1f8 = t;
        g_game->f1a204 = -10000;
        g_game->f1a200 = (int)m;
        g_game->f1a1fc = 0;
        (*(SpawnSliders*)((char*)&g_script + 8)).lo = -1;
        (*(SpawnSliders*)((char*)&g_script + 8)).hi = -1;
        (*(SpawnSliders*)((char*)&g_script + 8)).pos = t;
        item_sound(&g_items[item->item])->play(-0x32);
    }
    return m;
}

// @ 0x502c30  (the last function of the object; the slice listed 0x502c30 as exact in s00502300)
void PartyWindow::refresh_stash(int row)
{
    int t = g_game->cur_troop;
    if (row != 0)
        t = troop;
    for (int n = 0; n < 10; n++) {
        if (row_slot[row][n] != 0)
            place_slot((MissionUi*)row_slot[row][n], &EQUIP(t, n), n, row_state[row], row, 1);
    }
}
