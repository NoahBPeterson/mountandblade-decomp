//-----------------------------------------------------------------------------
// File: party_create.cpp  (0x500ad0..0x501a20)
//
// Desc: the inventory window's builder and its money label: PartyWindow::create picks the troop to show from the mode,
//       builds the 0x190-byte window with its hinges, the character icons, the two scrolling item lists with their labels,
//       the leave button, the total-cost label and the armour labels, registers the window with the gui manager and shows
//       the inventory tutorial once; PartyWindow::money_label writes the formatted money amount of one side.
//       The class is the party window of src/game/party/party_window.h (code 0x501a20..0x502cc0 follows this object):
//       the members these two functions prove are added there as pure additions (the former padding).
//       "T regime" region (docs/tu_map.md): binary order is kept. 0x500ad0 holds two WinLicense marker blobs (an 18-byte
//       `EB 10 57 4C ..` sequence at 0x500b10 and 0x500bbf) that no C++ source can emit, so it can never be byte-exact.
//       Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include "../party/party_window.h"
// the shared class headers again, directly (the resolution audit scans only the source's own includes)
#include "../int_vec.h"
#include "../str_table.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../view_state.h"
#include "../../include/mesh_parts.h"
#include "../../rgl/mesh.h"
#include "../config_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// ---- layout proofs of the members this object added to PartyWindow ---------------------------------------------------------
typedef char chk_page_widget[offsetof(PartyWindow, page_widget) == 0x2c ? 1 : -1];
typedef char chk_view[offsetof(PartyWindow, view) == 0x34 ? 1 : -1];
typedef char chk_gold[offsetof(PartyWindow, gold) == 0x44 ? 1 : -1];
typedef char chk_name[offsetof(PartyWindow, name) == 0x4c ? 1 : -1];
typedef char chk_hinge[offsetof(PartyWindow, hinge) == 0x54 ? 1 : -1];
typedef char chk_total[offsetof(PartyWindow, total_cost) == 0xa60 ? 1 : -1];
typedef char chk_leave[offsetof(PartyWindow, leave) == 0xa90 ? 1 : -1];
typedef char chk_ab0[offsetof(PartyWindow, f_ab0) == 0xab0 ? 1 : -1];
typedef char chk_abc[offsetof(PartyWindow, f_abc) == 0xabc ? 1 : -1];
typedef char chk_ac0[offsetof(PartyWindow, spec_ac0) == 0xac0 ? 1 : -1];
typedef char chk_bg[offsetof(PartyWindow, equip_bg) == 0xac8 ? 1 : -1];

// the config lookup every key goes through: a missing key yields 0 (the original inlines the table scan at every use)
static __forceinline float cfg(const tstring& n)
{
    float r = 0.0f;
    int i = g_config.find(n);
    if (i >= 0)
        r = (float)atof(CONFIG_ENTRIES[i].value ? CONFIG_ENTRIES[i].value : (char*)&CONFIG_ENTRIES[i].value);
    return r;
}

// (x, y) pair handed to the widget methods: Vec2 of include/vec.h (was Vec2 here)

extern "C" int sprintf(char* dst, const char* fmt, ...);    // 0x536f8f

// ---- objects of other translation units ---------------------------------------------------------------------
// Callees declared under the name of the class or function that defines them (the modules that define them name the class after another of
// their users, and the linker folds identical bodies); the object is cast at the call, each says why it is the one the definition works on.
struct gui_manager { void reset(); };           // 0x42d1b0 (strat/gui_manager.cpp): the gui manager at 0x5d7080
extern gui_manager g_gui;                       // 0x5d7080
struct NodeA;
struct NodeVec { void push_back(NodeA* const& n); };   // 0x49b120 (widget_node.cpp): "make this window the active root"; takes the address of a pointer to the window
struct ContainerW { void* construct(); };       // 0x4931a0 (world_data.cpp): the window base constructor body, returns this
struct Map10007 { int lookup(unsigned id); };   // 0x48ad00 (ui/mission.h): the party table of the game object (Game::scene) starts with this 0x2717-entry table
struct FieldPanel { void update(); };           // 0x48d470 (mission_ui2/mission_3.cpp): lays the list out
struct resmgr { rgl::mesh* get_object(const rgl::string* name); };   // 0x419540 (rgl/core/resmgr.cpp): the resource manager at 0x5d6fa0
struct WidgetB;
WidgetB* __cdecl make_scrollbar(void* parent, const tstring* name);    // 0x4f6160 (cdecl; char_sheet/editor_ui.cpp): the scroll widget of a list
void __cdecl label_refresh(MissionUi* w, tstring name);                // 0x4e4c70 (cdecl, the name by value; cls_map/progress_bar.cpp)
struct GameData { tstring string_value(int index); };                  // 0x48e9e0 (include/game_data.h; ret 8: hidden return)
extern char g_gui_dirty;                        // 0x5d8788 (set whenever a window was installed)
extern int g_tutorial_off;                      // 0x5a9fac (non-zero: tutorials are switched off)
extern void* g_windows[];                       // 0xa73980 (the window manager: its first screen is the message box)
struct WindowOpener {                           // the window manager (world_data.h WindowMgrW)
    void open(int window);                      // 0x494260 (ret 4)
};
extern float g_row_unit;                        // 0x599ae8 (0.088f: the height of one row of the 3-column item grid)
MissionUi* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
                            const tstring& name, const tstring& down, float k1, float k2, float k3);   // 0x4dcc50

// the scrolling item list (constructor 0x49deb0, size 0x4a8): the flags word is MissionUi's; +0x484 the scroll bar, +0x490 the row count
namespace c49deb0 {                             // party2/camera.cpp names the class of the constructor 0x49deb0 Camera in this namespace
struct Camera : MissionUi {
    char pad178[0x484 - 0x178];
    MissionUi* scroll;                          // +0x484
    char pad488[0x490 - 0x488];
    int rows;                                   // +0x490
    char pad494[0x4a8 - 0x494];
    Camera();                                   // 0x49deb0
};
}
typedef c49deb0::Camera InvList;
// virtual +0xc8 of the list sets the row count (the widget base's slot at +0xc8 is add_child(child), which takes one stack word as well)
#define SET_ROWS(w, n) WOPS(w)->add_child((void*)(size_t)(n))

// the window (constructor 0x4931a0, vptr 0x592848, size 0x190)
struct InvWindow : UiWindow {
    char f188;                                  // +0x188
    char pad189[7];
private:
    InvWindow();                                // (never defined: built by new_inv_window)
};
// new InvWindow: the allocation, then the base constructor body 0x4931a0 (ContainerW::construct, which runs MissionUi's 0x48ae20 itself), the vptr and f188
static __forceinline InvWindow* new_inv_window()
{
    void* mem = operator new(sizeof(InvWindow));
    if (mem == 0)
        return 0;
    ((ContainerW*)mem)->construct();
    *(void**)mem = (void*)0x592848;
    ((InvWindow*)mem)->f188 = 0;
    return (InvWindow*)mem;
}

// @ 0x500ad0
void PartyWindow::create()
{
    spec_ac0 = 0;
    spec_ac4 = 0;
    for (int k = 0; k < 0x60; k++) {
        flags[0][k] = 0;
        flags[1][k] = 0;
    }

    if (f4 == 1) {
        troop = g_game->f13af0;
        if (troop <= 0)
            troop = g_game->hero_troop;
    } else if (f4 == 0) {
        troop = g_game->hero_troop;
        ItemRef* inv = g_game->troops[troop].inv;
        for (int k = 0; k < 0x60; k++)
            inv[k].item = -1;
    }

    f_a8c = 0;
    f_a98 = 0;
    g_gui.reset();
    f_ab0 = -1;
    f_ab4 = -1;
    f_ab8 = -1;
    icon_a84 = 0;
    icon_a88 = 0;

    InvWindow* win = new_inv_window();
    container = win;
    {
        Vec2 p(0.0f, 0.0f);
        WOPS(win)->set_pos(&p.x);
    }
    {
        Vec2 s(1.0f, 0.75f);
        WOPS(win)->set_scale(&s.x);
    }
    {
        tstring n("inventory_window_b");
        ResMesh* m = ((ResMesh*)((resmgr*)&g_resmgr_party)->get_object((const rgl::string*)&n))->clone();
        WOPS(win)->add_image(0, m);
    }

    if (f4 == 4) {
        MissionUi* bg = new MissionUi;
        equip_bg = bg;
        label_refresh(bg, tstring("inv_i_equipment_pangel_bg"));
        {
            tstring n("inventory_equipment_panel");
            WOPS(bg)->add_image_a(0, ((ResMesh*)((resmgr*)&g_resmgr_party)->get_object((const rgl::string*)&n))->clone());
        }
        WOPS(container)->add_child(bg);
    }

    for (int k = 0; k < 4; k++) {
        MissionUi* h = new MissionUi;
        hinge[k] = h;
        char buf[0x58];
        sprintf(buf, "inv_hinge%d", k + 1);
        label_refresh(h, tstring(buf));
        {
            tstring n("inventory_window_hinge");
            WOPS(h)->add_image_a(0, ((ResMesh*)((resmgr*)&g_resmgr_party)->get_object((const rgl::string*)&n))->clone());
        }
        WOPS(container)->add_child(h);
    }
    {
        tstring e("");
        float out[2];
        WOPS(container)->set_text(out, &e);
    }
    add_strip_icon((char*)&icon_a84, tstring("inv_i_character"), g_game->cur_troop, spec_ac0);
    if (f4 == 4)
        add_strip_icon((char*)&icon_a88, tstring("inv_i_character2"), troop, spec_ac4);

    pages = 1;
    rows = 1;
    if (f4 == 0 || f4 == 2 || f4 == 1 || f4 == 3)
        pages = 2;
    if (f4 == 4)
        rows = 2;

    for (int i = 0; i < pages; i++) {
        page_widget[i] = build_page(i);
        int idx = g_game->cur_troop;
        if (i != 0)
            idx = troop;
        int n;
        if (idx == g_game->hero_troop && f4 == 0)
            n = 9;
        else
            n = g_game->troops[idx].inv_count();
        int nrows = (n + 2) / 3;
        float rows_h = (float)nrows * g_row_unit;
        float visible = cfg(tstring("inv_visible_inventory_area"));

        InvList* v;
        if (i == 0) {
            MissionUi* sc = (MissionUi*)make_scrollbar((MissionUi*)container, &tstring("inv_s_inventory1"));
            v = new InvList;
            v->f104 |= 0x20;
            if (sc != 0)
                v->scroll = sc;
            v->rows = nrows;
            SET_ROWS(v, nrows);
            label_refresh(v, tstring("inv_sc_inventory1_outer"));
        } else {
            MissionUi* sc = (MissionUi*)make_scrollbar((MissionUi*)container, &tstring("inv_s_inventory2"));
            v = new InvList;
            v->f104 |= 0x20;
            if (sc != 0)
                v->scroll = sc;
            v->rows = nrows;
            SET_ROWS(v, nrows);
            label_refresh(v, tstring("inv_sc_inventory2_outer"));
        }
        view[i] = v;

        float m = rows_h < visible ? rows_h : visible;
        {
            Vec2 p(v->fE8, v->fEC - m);
            WOPS(v)->set_pos(&p.x);
        }
        {
            Vec2 s(v->screen[0], m);
            WOPS(v)->set_scale(&s.x);
        }
        ((FieldPanel*)v)->update();
        WOPS(container)->add_child(view[i]);

        MissionUi* g = new MissionUi;
        gold[i] = g;
        if (i == 0)
            label_refresh(g, tstring("inv_t_gold1"));
        else
            label_refresh(g, tstring("inv_t_gold2"));
        WOPS(container)->add_child(g);
        if (f4 != 0 || idx != g_game->hero_troop)
            money_label(i);

        MissionUi* nm = new MissionUi;
        name[i] = nm;
        if (i == 0) {
            label_refresh(nm, tstring("inv_t_name1"));
            Troop* t0 = g_game->troops;
            tstring* s = g_strtable.find(&t0->key);
            if (s == 0)
                s = &t0->name;
            float out[2];
            WOPS(nm)->set_text(out, s);
        } else {
            label_refresh(nm, tstring("inv_t_name2"));
            if (f4 == 2 || f4 == 4) {
                Troop* t = &g_game->troops[troop];
                tstring* s = g_strtable.find(&t->key);
                if (s == 0)
                    s = &t->name;
                float out[2];
                WOPS(nm)->set_text(out, s);
            } else if (f4 == 0) {
                float out[2];
                WOPS(nm)->set_text(out, g_strtable.lookup(&tstring("ui_dump")));
            }
        }
        WOPS(container)->add_child(nm);
    }
    for (int i = 0; i < rows; i++)
        make_inventory_menu(i);

    {
        float y = cfg(tstring("inv_b_leave_h"));
        tstring* label = g_strtable.lookup(&tstring("ui_return"));
        leave = new_sized_button((MissionUi*)container, *label, 0.0f, 0.0f, y, tstring("short_button"), tstring("short_button_down"), 3.0f, 0.072f, 0.085f);
    }
    label_refresh((MissionUi*)leave, tstring("inv_b_leave_h"));
    {
        MissionUi* tc = new MissionUi;
        total_cost = tc;
        label_refresh(tc, tstring("inv_t_total_cost"));
        float out[2];
        WOPS(tc)->set_text(out, g_strtable.lookup(&tstring("ui_no_cost")));
        WOPS(container)->add_child(tc);
    }

    for (int r = 0; r < 4; r++) {
        MissionUi* a = new MissionUi;
        armor_labels[0][r] = a;
        label_refresh(a, tstring("inv_t_armor_base"));
        float step = cfg(tstring("inv_t_armor_y_a"));
        float ry = (float)r;
        {
            Vec2 p(a->fE8, a->fEC - step * ry);
            WOPS(a)->set_pos(&p.x);
        }
        WOPS(container)->add_child(a);
        if (f4 == 4) {
            MissionUi* a2 = new MissionUi;
            armor_labels[1][r] = a2;
            label_refresh(a2, tstring("inv_t_armor_base2"));
            float step2 = cfg(tstring("inv_t_armor_y_a"));
            {
                Vec2 p(a2->fE8, a2->fEC - step2 * ry);
                WOPS(a2)->set_pos(&p.x);
            }
            WOPS(container)->add_child(a2);
        }
    }

    f_abc = 0;
    if (f4 == 2) {
        // the player's party records (0x5ac8 bytes each, the scene's party table): the hero of the other side is on its list
        int n = 1;
        int off = 0x20;
        for (;;) {
            int party = g_game->f78;
            int slot = ((Map10007*)&g_game->scene)->lookup(party);
            char** pg = *(char***)((char*)&g_game->scene + 0x138d0);
            char* rec = pg[slot >> 4] + (slot & 0xf) * 0x5ac8;
            int count = *(int*)(rec + 0x8a8);
            int c = 0;
            if (count > 0) {
                char* p = rec + 0xb0;
                int j = 0;
                do {
                    if ((*p & 1) != 0)
                        break;
                    c++;
                    j++;
                    p += 0x20;
                } while (j < count);
            }
            if (n >= c)
                break;
            slot = ((Map10007*)&g_game->scene)->lookup(party);
            pg = *(char***)((char*)&g_game->scene + 0x138d0);
            rec = pg[slot >> 4] + (slot & 0xf) * 0x5ac8;
            if (troop == *(int*)(rec + 0xac + off))
                f_abc = 1;
            n++;
            off += 0x20;
        }
    }

    {
        MissionUi* root = (MissionUi*)container;
        g_gui_dirty = 1;
        ((NodeVec*)&g_gui)->push_back(*(NodeA**)&root);
    }
    init_window();

    if ((g_game->f13ae8 & 1) == 0 && f4 == 2 && g_tutorial_off == 0) {
        tstring t1("ui_tutorial_inventory");
        tstring t2("ui_tutorial");
        tstring* text = g_strtable.lookup(&t1);
        tstring* title = g_strtable.lookup(&t2);
        char* box = (char*)g_windows[0];
        *(tstring*)(box + 0x68) = *title;
        *(tstring*)(box + 0x1c) = *text;
        *(int*)(box + 0x4c) = 0;
        *(int*)(box + 0x50) = 0;
        *(int*)(box + 4) = 0;
        ((WindowOpener*)g_windows)->open(0);
        g_game->f13ae8 |= 1;
    }
}

// @ 0x501940
void PartyWindow::money_label(int which)
{
    int idx = g_game->cur_troop;
    if (which != 0)
        idx = troop;
    int money = g_game->troops[idx].f3b4;

    char buf[512];
    {
        tstring key("ui_money_value");
        sprintf(buf, g_strtable.lookup(&key)->c_str(), ((GameData*)g_game)->string_value(money).c_str());   // the callee builds the string in place through the hidden return pointer
    }
    {
        tstring s(buf);
        float out[2];
        WOPS(gold[which])->set_text(out, &s);
    }
}
