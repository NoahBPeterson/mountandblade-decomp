//-----------------------------------------------------------------------------
// File: party_window.cpp  (0x501a20..0x502cc0)
//
// Desc: the party screen's inventory window: the equipment menu builder, the slot factory, the page builder, the
//       armour / encumbrance refresh and the slot placement. Names provisional.
//       "T regime" region (docs/tu_map.md): cl emitted this object in source order, so the file keeps the binary
//       order and the helpers that follow their caller (refresh_page -> place_slot -> make_mesh) are
//       __declspec(noinline), as the original calls them. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#define CONTAINER_DTOR_EXPANDED   // ??_GPartyPanel 0x5021f0, see ui/container.h
#define UIA_CTOR_OUT_OF_LINE   // add_strip_icon calls the constructor 0x48b1c0 (StripIcon is built out of line, then stores its vptr)
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40; listed directly for the resolution audit)
#include "../../include/image_addrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include "./party_window.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)

// ---- data this module owns (docs/standalone_data.md) ----
int g_equip_page;                                    // 0xa75f68 (.bss) the equipment page shown by the party window (negative: nothing selected)
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
#include "../ini_file.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui/ui_frame.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/item_kind.h"   // g_items 0x5e29d0

// the definition (cls_map/progress_bar.cpp) takes the widget: this module calls it under that signature (the header keeps the void* spelling other modules use)
void __cdecl label_refresh(MissionUi* w, tstring name);      // 0x4e4c70 (cdecl, the name by value)

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

#define EQUIP(t, k) (g_game_G->troops[(t)].equip[(k)])
#define INV(t, k) (g_game_G->troops[(t)].inv[(k)])

// the config lookup every equipment key goes through: a missing key yields 0. The key is a temporary of the
// caller's expression (it is destroyed after the value is stored).
static __forceinline float cfg(const tstring& n)
{
    float r = 0.0f;
    int i = g_game_vars.find(n);
    if (i >= 0)
        r = (float)atof(g_game_vars.entries[i].value.c_str());
    return r;
}

// the string-table lookup with a named key (a temporary of the caller's expression)
static inline tstring* lookup(const tstring& k)
{
    return g_strtable.lookup(k);
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
        panel->move_a(&v.x);
    }
    {
        tstring e("");
        float out[2];
        panel->set_text(out, &e);
    }

    {
        MissionUi* w = new MissionUi;
        {
            Vec2 v(pos[4].x - 0.044f, pos[4].y + 0.095f);
            w->move_a(&v.x);
        }
        {
            Vec2 v(0.088f, 0.04f);
            w->set_xy(&v.x);
        }
        {
            int f = 0x10;          // the option flags (+0x148)
            w->set148(&f);
        }
        {
            float out[2];
            w->set_text(out, lookup(tstring("ui_outfit")));
        }
        panel->add_child(w);
    }
    {
        MissionUi* w = new MissionUi;
        {
            Vec2 v(pos[0].x, pos[0].y + 0.095f);
            w->move_a(&v.x);
        }
        {
            Vec2 v(0.088f, 0.04f);
            w->set_xy(&v.x);
        }
        {
            int f = 0x10;          // the option flags (+0x148)
            w->set148(&f);
        }
        {
            float out[2];
            w->set_text(out, lookup(tstring("ui_arms")));
        }
        panel->add_child(w);
    }
    {
        MissionUi* w = new MissionUi;
        {
            Vec2 v(pos[8].x, pos[8].y + 0.095f);
            w->move_a(&v.x);
        }
        {
            Vec2 v(0.088f, 0.04f);
            w->set_xy(&v.x);
        }
        {
            int f = 0x10;          // the option flags (+0x148)
            w->set148(&f);
        }
        {
            float out[2];
            w->set_text(out, lookup(tstring("ui_horse")));
        }
        panel->add_child(w);
    }
    if (idx == 0 && g_module_settings.disable_food_slot == 0) {
        MissionUi* w = new MissionUi;
        {
            Vec2 v(pos[9].x, pos[9].y + 0.095f);
            w->move_a(&v.x);
        }
        {
            Vec2 v(0.088f, 0.04f);
            w->set_xy(&v.x);
        }
        {
            int f = 0x10;          // the option flags (+0x148)
            w->set148(&f);
        }
        {
            float out[2];
            w->set_text(out, lookup(tstring("ui_food")));
        }
        panel->add_child(w);
    }

    int troop_idx = g_game_G->cur_troop;
    if (idx != 0)
        troop_idx = w14;
    for (int i = 0; i < 10; i++) {
        if ((idx < 1 && g_module_settings.disable_food_slot == 0) || i != 9) {
            MissionUi* slot = make_slot(&EQUIP(troop_idx, i));
            row_slot[idx][i] = slot;
            if (i != 9)
                slot->b11c[0] = 0;
            {
                Vec2 v(pos[i].x, pos[i].y);
                slot->move_a(&v.x);
            }
            panels[idx]->add_child(slot);
            row_state[idx][i] = 2;
        } else {
            row_slot[idx][9] = 0;
            row_state[idx][9] = 0;
        }
    }
    container->add_child(panels[idx]);
    return 0;
}

// @ 0x5021b0
char PartyPanel::key(unsigned mask)
{
    bool r = false;
    if (mask == 0x10) {
        r = true;
        set_key_flags(0x10000);
    }
    if (ContainerBase::key(mask))
        return true;
    return r;
}

// @ 0x5021f0 ??_GPartyPanel is the compiler's deleting destructor of PartyPanel (implicit; text_box's, slot 0 of 0x597050, is the
// same code folded by ICF).
// @ 0x502230
MissionUi* PartyWindow::make_slot(ItemRef* slot)
{
    PartySlot* w = new PartySlot;
    {
        float v[2];
        v[0] = 0.088f;
        v[1] = 0.088f;
        w->set_xy(v);
    }
    {
        tstring n("inv_slot");
        w->set_mesh(0, g_resmgr.get_object(&n)->clone());
    }
    return w;
}

// @ 0x5022c0
char PartySlot::key(unsigned mask)
{
    char r = 0;
    if (mask == 0x10) {
        r = 1;
        set_key_flags(0x10000);
    }
    set_key_flags(mask);
    refresh();
    return r;
}

// @ 0x502320
PartyPanel* PartyWindow::build_page(int page)
{
    Game* g = g_game_G;
    int t = g->cur_troop;
    if (page != 0)
        t = w14;
    int n;
    if (t == g->hero_troop && w04 == 0)
        n = 9;
    else
        n = g->troops[t].inv_count();
    float h = (float)((n + 2) / 3) * 0.088f;
    PartyPanel* p = new PartyPanel;
    { Vec2 v(0.0f, 0.0f); p->move_a(&v.x); }
    { Vec2 v(0.264f, h); p->set_xy(&v.x); }
    int col = 0, row;
    int i = 0;
    if (i < n) {            // the original tests n before it sets row (0x502402)
        row = 1;
        for (; i < n; i++) {
            MissionUi* w = make_slot(&INV(t, i));
            Vec2 v((float)col * 0.088f, h - (float)row * 0.088f);
            widgets[page][i] = w;
            w->move_a(&v.x);
            col++; if (col >= 3) { col = 0; row++; }
            p->add_child(widgets[page][i]);
            flags[page][i] = 2; aux[page][i] = 0x12fd1;
        }
    }
    return p;
}

// @ 0x5024e0
void PartyWindow::add_strip_icon(StripIcon** slot, const tstring& name, int troop, int kind)
{
    int sel = g_equip_page;
    if (sel < 0)
        return;
    if (*slot == 0) {
        *slot = new StripIcon;
        label_refresh(*slot, name);
        container->add_child(*slot);
    }
    (*slot)->set_track(0, 0);
    int idx = kind + troop * 4;
    (*slot)->set_track(0, g_tableaus.make_mesh_from(&sel, &idx));
}

// @ 0x5025a0
void PartyWindow::init_window()
{
    for (int i = 0; i < pages; i++)
        refresh_page(i);
    for (int i = 0; i < rows; i++)
        refresh_stash(i);
    refresh_armor(armor_labels[0], g_game_G->cur_troop);
    if (rows == 2)
        refresh_armor(armor_labels[1], w14);
}

// @ 0x502620
void PartyWindow::refresh_armor(MissionUi** labels, int t)
{
    int legs = 0;
    int head = 0, body = 0;
    float enc = 0.0f;
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
        labels[0]->set_text(out, &s);
    }
    {
        sprintf(buf, lookup(tstring("ui_body_armor"))->c_str(), body);
    }
    {
        tstring s(buf);
        float out[2];
        labels[1]->set_text(out, &s);
    }
    {
        sprintf(buf, lookup(tstring("ui_leg_armor"))->c_str(), legs);
    }
    {
        tstring s(buf);
        float out[2];
        labels[2]->set_text(out, &s);
    }
    {
        sprintf(buf, lookup(tstring("ui_encumbrance"))->c_str(), enc);
    }
    {
        tstring s(buf);
        float out[2];
        labels[3]->set_text(out, &s);
    }
}

// @ 0x502920
void PartyWindow::refresh_page(int page)
{
    Game* g = g_game_G;
    int t = g->cur_troop;
    if (page != 0)
        t = w14;
    int count;
    if (t == g->hero_troop && w04 == 0)
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
        int t = g_game_G->cur_troop;
        if (page == 1) {
            if ((fl & 1) == 0)
                t = w14;
        } else if ((fl & 1) != 0) {
            t = w14;
        }
        float sc = 0.088f;
        rgl::mesh* m = make_mesh(item, &sc, t);
        rgl::vector4 off(0.044f, 0.044f, 0.0f);
        m->translate(&off);
        w->set_track(2, m);
        w->b11c[2] = 1;
        if (flag != 0 && idx < 9)
            w->b11c[0] = 1;
    }
    state[idx] &= ~2u;
    if (icon_a84 != 0 && flag != 0) {
        tstring n("inv_i_character");
        add_strip_icon(&icon_a84, n, g_game_G->cur_troop, spec_ac0);
    }
    if (icon_a88 != 0 && flag != 0) {
        tstring n("inv_i_character2");
        add_strip_icon(&icon_a88, n, w14, spec_ac4);
    }
}

// @ 0x502b80
MB_NOINLINE rgl::mesh* PartyWindow::make_mesh(ItemRef* item, void* unused, int t)
{
    rgl::mesh* m = 0;
    if (item->item >= 0) {
        float s = *(float*)unused;              // read, then overridden: the read is dead, but it makes the callee summary say
        s = 0.088f;                             // "reads through parameter 2", so place_slot keeps its store of the scale
        m = make_item_mesh(item, &s);
        g_game_G->f1a1f4 = -1;
        g_game_G->f1a1f8 = t;
        g_game_G->f1a204 = -10000;
        g_game_G->f1a200 = (int)m;
        g_game_G->f1a1fc = 0;
        g_script.param1_lo = -1;
        g_script.param1_hi = -1;
        g_script.param2 = t;
        item_sound(&g_items[item->item])->update(-0x32);
    }
    return m;
}

// @ 0x502c30  (the last function of the object; the slice listed 0x502c30 as exact in s00502300)
void PartyWindow::refresh_stash(int row)
{
    int t = g_game_G->cur_troop;
    if (row != 0)
        t = w14;
    for (int n = 0; n < 10; n++) {
        if (row_slot[row][n] != 0)
            place_slot((MissionUi*)row_slot[row][n], &EQUIP(t, n), n, row_state[row], row, 1);
    }
}

#pragma inline_depth(2)   // for ??_GPartyPanel, generated at the end of the object
