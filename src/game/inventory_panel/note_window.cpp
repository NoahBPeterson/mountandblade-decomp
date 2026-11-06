// 0x50a1c0
//
// note_window::build(page, arg) -- builds the "notes" page: a large panel with
// a title picture, a report text, a list of links, two scrolling sub panels
// and a row of short buttons.  The page number selects the report kind:
//   1 = troop, 2 = faction, 3 = map party (location), 4 = quest
// The text for the page is built from the troop/faction/party/quest tables and
// the "other pages that link here" back references are collected by scanning
// those tables for the localised key of the current page.
//
// __thiscall, cleans two stack arguments (ret 8).

#include "../../include/image_addrs.h"
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/rgl_string.h"

// ---------------------------------------------------------------------------
// string object (0x42bbd0 family)
// ---------------------------------------------------------------------------

#include "../tstring.h"
#include "../shared_vars.h"

extern "C" int sprintf(char* dst, const char* fmt, ...);     // 0x536f8f
extern "C" double atof(const char* s);                       // 0x537352
// ---------------------------------------------------------------------------
// widgets -- only the vtable slots touched by this function are named
// ---------------------------------------------------------------------------

#define TEXT_BOX_OUT_OF_LINE   // this file calls 0x50d0b0 (0x50c434, 0x50c493); see ui/container.h
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)
#include "../res_mgr.h"                   // g_resmgr: the resource manager 0x5d6fa0 (get_object 0x419540)
#include "../../rgl/mesh.h"             // rgl::mesh::clone 0x41c6d0
#include "../ui/container.h"             // NodeA (0x592848), text_box (0x597050)
#include "../ui/map_list.h"              // MWidget (0x5995c0): the report and link lists
#include "../party2/camera_class.h"      // Camera (draw 0x488d50)
#include "../ui/ui_widgets.h"             // UiD (scroll bar slider, vtable 0x592140): make_scrollbar 0x4f6160 returns one

// (each constructor on its own line inside its class: the resolution tools read a '// 0x' annotation in the class's scope)
// widget helper functions
void label_refresh(MissionUi* w, tstring name);                       // 0x4e4c70
UiA* make_short_button(MissionUi* parent, const tstring* text, float x, float y, float h);   // 0x4e5570
UiA* new_long_button(MissionUi* parent, const tstring* text, float x, float y, float h);     // 0x4dcbd0
UiD* make_scrollbar(void* parent, const tstring* name);                                      // 0x4f6160
Camera* make_editor_list(void* parent, const tstring* name, int child);                      // 0x4f62f0

#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------

#include "../strat/gui_manager.h"

#include "../str_table.h"                          // StrTable g_strtable (0x5e2a18): lookup 0x43a1e0, find 0x4c6010
#define UITEXT (&g_strtable)

#include "../ini_file.h"
#define CONFIG (&g_game_vars)

#include "../../include/tableau_table.h"            // TableauTbl g_tableaus (0xa75f40)

#include "../../include/game_data.h"             // GameData: the game object at 0x5e2930 (troops +0x13a00, factions +0x13a48, quests +0x13a58, parties +0xf8)
#define WORLD g_game

// map-party accessors (this = world + 0xf8: the party manager, include/party.h; the slice called it WorldMap)
#include "../../include/party.h"
#define WORLDMAP (&WORLD->parties)


// ---------------------------------------------------------------------------
// the party accessors: visible here too, as header inlines in the original (the same definitions as ui/mission_ui.cpp,
// mission5/troop_rec.cpp and dialog_manager.cpp; the row owners win the link). cl then knows the calls keep g_game, so
// `&g_game->parties` stays in a register across them (0x50b0d2, 0x50b28c, 0x50b4b6).
// ---------------------------------------------------------------------------
inline int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}
__declspec(noinline) Party* PartyMgr::get(int id)
{
    int k = this->lookup(id);
    return (Party*)((k & 0xf) * 0x5ac8 + (int)pages._Myfirst[k >> 4]);
}
__declspec(noinline) void PartyMgr::next_party_id(int* id)
{
    unsigned i = lookup(*id) + 1;
    while ((int)i < count && pages._Myfirst[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        *id = deflt;
    else
        *id = pages._Myfirst[i >> 4][i & 0xf].id;
}
__declspec(noinline) int PartyMgr::first_party_id()
{
    unsigned i = 0;
    while ((int)i < count && pages._Myfirst[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        return deflt;
    return pages._Myfirst[i >> 4][i & 0xf].id;
}

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------

// translated text of a key; the key is a temporary of the caller's full expression (0x50a84f..0x50a8b8)
#define TR(key) (UITEXT->lookup(key))

// config float: 0 when the key is absent. The key is a temporary of the caller's full expression (0x50c4f0..0x50c563).
static __forceinline float cfg_float(const tstring& key)
{
    float v = 0.0f;
    int i = CONFIG->find(key);
    if (i >= 0) {
        v = (float)atof(CONFIG->entries[i].value.c_str());
    }
    return v;
}

static __forceinline void cfg_get(float& v, const tstring& key)
{
    v = 0.0f;
    int i = CONFIG->find(key);
    if (i >= 0) {
        v = (float)atof(CONFIG->entries[i].value.c_str());
    }
}

// a troop's display name: the UITEXT entry for its key, else the record's own name (0x50a95e / 0x50ab3f)
static __forceinline tstring* troop_name(Troop& t)
{
    tstring* s = UITEXT->find(&t.key);
    return s ? s : &t.name;
}

// a faction's display name: its own name, replaced by the UITEXT entry for its key when the flag byte at +0x10 is clear
// and the entry exists (0x50ae3b..0x50ae5d)
static __forceinline tstring* faction_name(FactionRec& f)
{
    if (f.f10 == 0) {
        tstring* s = UITEXT->find(&f.key);
        if (s)
            return s;
    }
    return &f.name;
}

// name tstring of a map-party record
static __forceinline tstring* party_name(char* rec)
{
    if (*(char*)&g_module_settings.show_party_ids_instead_of_names != 0)
        return (tstring*)(rec + 0x20);
    if (*(char*)(rec + 0x1c) == 0) {
        tstring* s = UITEXT->find((tstring*)(rec + 0x20));
        if (s)
            return s;
    }
    return (tstring*)(rec + 0x0c);
}

// ---------------------------------------------------------------------------
// the window
// ---------------------------------------------------------------------------

// the original's callees take the name by reference (implicit temporaries of the literals, built before the arguments are
// loaded); the definitions in char_sheet/editor_ui.cpp take a pointer, the same ABI.
#define mk_scrollbar ((UiD* (__cdecl*)(void*, const tstring&))make_scrollbar)
#define mk_editor_list ((Camera* (__cdecl*)(void*, const tstring&, int))make_editor_list)
static inline float fmin2(const float& a, const float& b) { return a < b ? a : b; }

// set_text's vtable slot (+0xa4) as the source saw it: it returns the text's size by value through a hidden pointer, so
// every call that isn't assigned has its own private temporary ([esp+0x258]..[esp+0x2d0])
struct text_size { float x, y; text_size() {} };
struct text_slot {
#define S4(n) virtual void n##0() = 0; virtual void n##1() = 0; virtual void n##2() = 0; virtual void n##3() = 0;
    S4(a) S4(b) S4(c) S4(d) S4(e) S4(f) S4(g) S4(h) S4(i) S4(j)
    virtual void k0() = 0;
#undef S4
    virtual text_size set_text(const tstring* s) = 0;   // +0xa4
};
#define TEXTSET(w, s) (((text_slot*)(w))->set_text(s))

struct sel_list { void add(int val); };                // 0x50d0e0 (inventory_panel/widget_text.cpp)


// 0x50cc70 (inventory_panel/widget_text.cpp): a __thiscall member (`mov ecx, this` before each of the four calls; the body
// never reads ecx, ret 8).
#define SORT_LIST(l, m) this->sort_list(l, m)

struct note_window {
    void sort_list(sel_list* l, int mode);      // 0x50cc70 (inventory_panel/widget_text.cpp)
    char pad00[0x18];
    int  menu_id;              // +0x18
    char pad1c[0x2c - 0x1c];
    NodeA* panel;                  // +0x2c
    MissionUi* picture;           // +0x30
    MissionUi* report;            // +0x34
    MissionUi* title;             // +0x38
    MissionUi* links;             // +0x3c
    text_box* keys_panel;           // +0x40
    text_box* data_panel;           // +0x44
    MissionUi* keys_scroll;       // +0x48
    MissionUi* data_scroll;       // +0x4c
    MissionUi* btn_game_log;      // +0x50
    MissionUi* btn_recent;        // +0x54
    MissionUi* btn_troops;        // +0x58
    MissionUi* btn_quests;        // +0x5c
    MissionUi* btn_factions;      // +0x60
    MissionUi* btn_locations;     // +0x64
    MissionUi* btn_return;        // +0x68
    MissionUi* btn_back;          // +0x6c
    MissionUi* btn_forward;       // +0x70
    MissionUi* btn_map;           // +0x74
    std::vector<int> back;     // +0x78 (first +0x7c)
    std::vector<int> fwd;      // +0x88 (first +0x8c)
    int    back_sel_a;         // +0x98
    int    back_sel_b;         // +0x9c
    int    fwd_sel_a;          // +0xa0
    int    fwd_sel_b;          // +0xa4
    int    page;               // +0xa8
    int    arg;                // +0xac
    float  scroll_pos;         // +0xb0

    void build(int page, int arg);              // 0x50a1c0
};

// The party notes use get_party (raw page read) except the find() in the back-reference scan, which uses get_w
// (vector::operator[]: `lea; mov`, 0x50c0xx). get_w costs more inside the compiler even where both emit the same code: with
// get_w everywhere the troop scan's operand order flips (0x50bca6: `lea ecx,[eax+edi]`, original `lea ecx,[edi+esi]`).
// @ 0x50a1c0
void note_window::build(int page_, int arg_)
{
    char buf[0x100];
    char buf2[0x100];

    this->page = page_;
    this->arg = arg_;
    g_gui.reset();

    // ---- main panel ------------------------------------------------------
    this->panel = new NodeA((char)0);               // constructor 0x4931a0, then the container's vtable and f188 (0x50a20e)
    {
        float v[2];
        v[0] = 0.0f;
        v[1] = 0.0f;
        this->panel->move_a(v);
        v[0] = 1.0f;
        v[1] = 0.75f;
        this->panel->set_xy(v);
    }
    {
        tstring key("note_window");
        this->panel->set_track(0, g_resmgr.get_object(&key)->clone());   // 0x419540, 0x41c6d0
    }

    // ---- title picture ---------------------------------------------------
    this->picture = new MissionUi;
    label_refresh(this->picture, tstring("note_i_picture"));

    // [esp+0x14] / [esp+0x24]: the text heights of the links and the report; the 0.1 stands when a page sets neither
    float x_cur = 0.1f;
    float y_cur = 0.1f;

    // ---- report text -----------------------------------------------------
    this->report = new MWidget;
    {
        tstring empty("");
        float sz[2];
        this->report->set_text(sz, &empty);
    }
    label_refresh(this->report, tstring("note_t_keys"));

    // ---- links list (made before the title: 0x50a34b / 0x50a3b3) ------------------------------------------------
    this->links = new MWidget;
    {
        tstring empty("");
        float sz[2];
        this->links->set_text(sz, &empty);
    }
    label_refresh(this->links, tstring("note_t_data"));          // (it joins the data panel later, 0x50c5eb)

    // ---- report title: the one widget the main panel holds directly (0x50a3e8..0x50a41d) ----
    this->title = new MissionUi;
    label_refresh(this->title, tstring("note_t_title"));
    {
        tstring empty("");
        TEXTSET(this->title, &empty);
    }
    this->panel->add_child(this->title);

    // ---- back / forward / map buttons ------------------------------------
    {
        float small_h = cfg_float("note_button_sm_h");      // the config's small-button height (0x50a423..0x50a466)
        this->btn_back = make_short_button(this->panel, TR("ui_back"), 0.0f, 0.0f, small_h);
        label_refresh(this->btn_back, tstring("note_b_back"));
        this->btn_forward = make_short_button(this->panel, TR("ui_forward"), 0.0f, 0.0f, small_h);
        label_refresh(this->btn_forward, tstring("note_b_forward"));
        this->btn_map = new_long_button(this->panel, TR("ui_display_on_map"), 0.0f, 0.0f, small_h);
        label_refresh(this->btn_map, tstring("note_b_map_follow"));
        this->btn_map->hide();
    }

    // a button is enabled only when its history has an entry to go to (0x50a5db: slot 0xbc enable, 0x50a5e8: 0xb8 disable)
    if (this->back.size() > 0) {
        this->back_sel_a = this->back[this->back.size() - 2];
        this->back_sel_b = this->back[this->back.size() - 1];
        this->btn_back->flag2_clear();
    } else {
        this->btn_back->flag2_set();
    }
    if (this->fwd.size() > 0) {
        this->fwd_sel_a = this->fwd[this->fwd.size() - 2];
        this->fwd_sel_b = this->fwd[this->fwd.size() - 1];
        this->btn_forward->flag2_clear();
    } else {
        this->btn_forward->flag2_set();
    }

    // ---- page link key + candidate list ----------------------------------
    tstring acc;                // +0xa0   "<trp=%d>" for the current page
    std::vector<int> vec;       // +0x5c
    bool  any = true;           // [esp+0x1b]
    int   selected = -1;        // [esp+0x58]
    this->picture->hide();

    switch (page_) {
    case 0:
        x_cur = TEXTSET(this->links, TR("ui_click_button_to_view_note")).y;    // the returned size (0x50a6d8)
        break;


    case 1: {
        // ---- troop report -------------------------------------------------
        if (WORLD->troops[arg_].note_tableau >= 0) {
            this->picture->set_track(0, g_tableaus.make_mesh_from(&WORLD->troops[arg_].note_tableau, &arg_));
            this->picture->show();
        }
        sprintf(buf, "<trp=%d>", arg_);
        acc.assign(buf);

        tstring line;
        for (int k = 0; k < 8; k++) {
            if (WORLD->troops[arg_].notes[k].len > 0) {
                if (line.len > 0)
                    line.append("\n\n");
                line.append(WORLD->troops[arg_].notes[k]);
                if (WORLD->troops[arg_].note_times[k] >= 0) {
                    if (WORLD->troops[arg_].note_times[k] < WORLD->string_count) {
                        sprintf(buf2, TR("ui_report_is_value_days_old")->c_str(), WORLD->string_count - WORLD->troops[arg_].note_times[k]);
                        line.append(buf2);
                    } else {
                        line.append(*TR("ui_report_is_current"));
                    }
                }
            }
        }

        if (line.len > 0) {
            TEXTSET(this->title, troop_name(WORLD->troops[arg_]));
            x_cur = TEXTSET(this->links, &line).y;
            line.clear();
            any = false;
        } else {
            x_cur = TEXTSET(this->links, TR("ui_click_button_to_view_note")).y;
        }

        // all troops that carry at least one report item
        for (int i = 0; i < WORLD->ntroops; i++) {
            for (int k = 0; k < 8; k++) {
                if (WORLD->troops[i].notes[k].len > 0) {
                    vec.push_back(i);
                    break;
                }
            }
        }
        SORT_LIST((sel_list*)&vec, 1);

        for (unsigned n = 0; n < vec.size(); n++) {
            int v = vec[n];
            if (selected < 0)
                selected = v;
            if (v == arg_)
                sprintf(buf2, "%s\n", troop_name(WORLD->troops[v])->c_str());
            else
                sprintf(buf2, "<trp=%d>%s</>\n", v, troop_name(WORLD->troops[v])->c_str());
            line.append(buf2);
        }
        if (line.len > 0)
            line.erase(line.len - 1, 1);
        y_cur = TEXTSET(this->report, &line).y;
        break;
    }

    case 2: {
        // ---- faction report -----------------------------------------------
        if (WORLD->factions[arg_].note_tableau >= 0) {
            this->picture->set_track(0, g_tableaus.make_mesh_from(&WORLD->factions[arg_].note_tableau, &arg_));
            this->picture->show();
        }
        sprintf(buf, "<fac=%d>", arg_);
        acc.assign(buf);

        tstring line;
        for (int k = 0; k < 8; k++) {
            if (WORLD->factions[arg_].notes[k].len > 0) {
                if (line.len > 0)
                    line.append("\n\n");
                line.append(WORLD->factions[arg_].notes[k]);
                if (WORLD->factions[arg_].note_times[k] >= 0) {
                    if (WORLD->factions[arg_].note_times[k] < WORLD->string_count) {
                        sprintf(buf2, TR("ui_report_is_value_days_old")->c_str(), WORLD->string_count - WORLD->factions[arg_].note_times[k]);
                        line.append(buf2);
                    } else {
                        line.append(*TR("ui_report_is_current"));
                    }
                }
            }
        }

        if (line.len > 0) {
            TEXTSET(this->title, faction_name(WORLD->factions[arg_]));
            x_cur = TEXTSET(this->links, &line).y;
            line.clear();
            any = false;
        } else {
            x_cur = TEXTSET(this->links, TR("ui_click_button_to_view_note")).y;
        }

        for (int i = 0; i < WORLD->n_factions; i++) {
            for (int k = 0; k < 8; k++) {
                if (WORLD->factions[i].notes[k].len > 0) {
                    vec.push_back(i);
                    break;
                }
            }
        }
        SORT_LIST((sel_list*)&vec, 2);

        for (unsigned n = 0; n < vec.size(); n++) {
            int v = vec[n];
            if (selected < 0)
                selected = v;
            if (v == arg_)
                sprintf(buf2, "%s\n", faction_name(WORLD->factions[v])->c_str());
            else
                sprintf(buf2, "<fac=%d>%s</>\n", v, faction_name(WORLD->factions[v])->c_str());
            line.append(buf2);
        }
        if (line.len > 0)
            line.erase(line.len - 1, 1);
        y_cur = TEXTSET(this->report, &line).y;
        break;
    }

    case 3: {
        // ---- map party (location) report ----------------------------------
        if (WORLDMAP->get(arg_)->note_tableau >= 0) {
            this->picture->set_track(0, g_tableaus.make_mesh_from(&WORLDMAP->get(arg_)->note_tableau, &arg_));
            this->picture->show();
        }
        sprintf(buf, "<p=%d>", arg_);
        acc.assign(buf);

        tstring line;
        for (int k = 0; k < 8; k++) {
            if (WORLDMAP->get_party(arg_)->notes[k].len > 0) {
                if (line.len > 0)
                    line.append("\n\n");
                line.append(WORLDMAP->get_party(arg_)->notes[k]);
                if (WORLDMAP->get_party(arg_)->note_times[k] >= 0) {
                    if (WORLDMAP->get(arg_)->note_times[k] < WORLD->string_count) {
                        sprintf(buf2, TR("ui_report_is_value_days_old")->c_str(), WORLD->string_count - WORLDMAP->get(arg_)->note_times[k]);
                        line.append(buf2);
                    } else {
                        line.append(*TR("ui_report_is_current"));
                    }
                }
            }
        }

        if (line.len > 0) {
            if (this->menu_id == 7)
                this->btn_map->show();
            TEXTSET(this->title, party_name((char*)WORLDMAP->get_party(arg_)));
            x_cur = TEXTSET(this->links, &line).y;
            line.clear();
            any = false;
        } else {
            x_cur = TEXTSET(this->links, TR("ui_click_button_to_view_note")).y;
        }

        // live map parties
        {
            int p = WORLDMAP->first_party_id();
            while ((unsigned)p < WORLD->parties.n_parties) {
                for (int k = 0; k < 8; k++) {
                    if (WORLDMAP->get_party(p)->notes[k].len > 0) {
                        vec.push_back(p);
                        break;
                    }
                }
                WORLDMAP->next_party_id(&p);
            }
        }
        SORT_LIST((sel_list*)&vec, 3);

        for (unsigned n = 0; n < vec.size(); n++) {
            int v = vec[n];
            if (selected < 0)
                selected = v;
            if (v == arg_)
                sprintf(buf2, "%s\n", party_name((char*)WORLDMAP->get_party(v))->c_str());
            else
                sprintf(buf2, "<p=%d>%s</>\n", v, party_name((char*)WORLDMAP->get_party(v))->c_str());
            line.append(buf2);
        }
        if (line.len > 0)
            line.erase(line.len - 1, 1);
        y_cur = TEXTSET(this->report, &line).y;
        break;
    }

    case 4: {
        // ---- quest report -------------------------------------------------
        if (WORLD->quests[arg_].note_tableau >= 0) {
            this->picture->set_track(0, g_tableaus.make_mesh_from(&WORLD->quests[arg_].note_tableau, &arg_));
            this->picture->show();
        }
        sprintf(buf, "<qst=%d>", arg_);
        acc.assign(buf);

        tstring line;
        for (int k = 0; k < 8; k++) {
            if (WORLD->quests[arg_].notes[k].len > 0) {
                if (line.len > 0)
                    line.append("\n\n");
                line.append(WORLD->quests[arg_].notes[k]);
                if (WORLD->quests[arg_].note_times[k] >= 0) {
                    if (WORLD->quests[arg_].note_times[k] < WORLD->string_count) {
                        sprintf(buf2, TR("ui_report_is_value_days_old")->c_str(), WORLD->string_count - WORLD->quests[arg_].note_times[k]);
                        line.append(buf2);
                    } else {
                        line.append(*TR("ui_report_is_current"));
                    }
                }
            }
        }

        if (line.len > 0) {
            TEXTSET(this->title, &WORLD->quests[arg_].text.strip_tags());
            x_cur = TEXTSET(this->links, &line).y;
            line.clear();
            any = false;
        } else {
            x_cur = TEXTSET(this->links, TR("ui_click_button_to_view_note")).y;
        }

        for (int i = 0; i < WORLD->n_quests; i++) {
            for (int k = 0; k < 8; k++) {
                if (WORLD->quests[i].notes[k].len > 0) {
                    vec.push_back(i);
                    break;
                }
            }
        }
        SORT_LIST((sel_list*)&vec, 3);

        for (unsigned n = 0; n < vec.size(); n++) {
            int v = vec[n];
            if (selected < 0)
                selected = v;
            if (v == arg_)
                sprintf(buf2, "%s\n", WORLD->quests[v].text.strip_tags().c_str());
            else
                sprintf(buf2, "<qst=%d>%s</>\n", v, WORLD->quests[v].text.strip_tags().c_str());
            line.append(buf2);
        }
        if (line.len > 0)
            line.erase(line.len - 1, 1);
        y_cur = TEXTSET(this->report, &line).y;
        break;
    }

    default:
        break;
    }

    // ---- collect the back references -------------------------------------
    tstring other;              // +0x70   "other pages that link here"  (default-constructed after the switch, 0x50bc44)
    tstring link_text;          // +0xb0   scratch for the links widget  (0x50bc50)
    this->links->put(&link_text);
    if (acc.len > 0) {
        for (int i = 0; i < WORLD->ntroops; i++) {
            for (int k = 0; k < 8; k++) {
                if (WORLD->troops[i].notes[k].len > 0 && WORLD->troops[i].notes[k].find(acc, 0) >= 0) {
                    sprintf(buf2, "<trp=%d>%s</>", i, troop_name(WORLD->troops[i])->c_str());
                    if (link_text.find(buf2, 0) < 0) {
                        if (other.len > 0)
                            other.append(", ");
                        other.append(buf2);
                    }
                    break;
                }
            }
        }
        for (int i = 0; i < WORLD->n_factions; i++) {
            for (int k = 0; k < 8; k++) {
                if (WORLD->factions[i].notes[k].len > 0 && WORLD->factions[i].notes[k].find(acc, 0) >= 0) {
                    sprintf(buf2, "<fac=%d>%s</>", i, faction_name(WORLD->factions[i])->c_str());
                    if (link_text.find(buf2, 0) < 0) {
                        if (other.len > 0)
                            other.append(", ");
                        other.append(buf2);
                    }
                    break;
                }
            }
        }
        {
            int p = WORLDMAP->first_party_id();
            while ((unsigned)p < WORLD->parties.n_parties) {
                for (int k = 0; k < 8; k++) {
                    if (WORLDMAP->get_party(p)->notes[k].len > 0 && WORLDMAP->get_w(p)->notes[k].find(acc, 0) >= 0) {
                        sprintf(buf2, "<p=%d>%s</>", p, party_name((char*)WORLDMAP->get_party(p))->c_str());
                        if (link_text.find(buf2, 0) < 0) {
                            if (other.len > 0)
                                other.append(", ");
                            other.append(buf2);
                        }
                        break;
                    }
                }
                WORLDMAP->next_party_id(&p);
            }
        }
        for (int i = 0; i < WORLD->n_quests; i++) {
            for (int k = 0; k < 8; k++) {
                if (WORLD->quests[i].notes[k].len > 0 && WORLD->quests[i].notes[k].find(acc, 0) >= 0) {
                    sprintf(buf2, "<qst=%d>%s</>", i, WORLD->quests[i].text.strip_tags().c_str());
                    if (link_text.find(buf2, 0) < 0) {
                        if (other.len > 0)
                            other.append(", ");
                        other.append(buf2);
                    }
                    break;
                }
            }
        }
    }

    if (other.len > 0) {
        tstring t;
        this->links->put(&t);
        if (t.equals(*TR("ui_click_button_to_view_note"))) {
            TEXTSET(this->links, TR("ui_this_page_contains_no_information"));
            this->links->put(&t);
        }
        t.append("\n\n");
        t.append(*TR("ui_other_pages_that_link_here"));
        t.append(other);
        x_cur = TEXTSET(this->links, &t).y;
    }

    // ---- key / data scrolling panels -------------------------------------
    // x_cur holds the links text height, y_cur the report's (0x50c405)
    x_cur += 0.001f;
    y_cur += 0.001f;
    this->keys_panel = new text_box;
    label_refresh(this->keys_panel, tstring("note_bo_keys"));
    {
        float2 v(this->keys_panel->screen[0], y_cur + this->keys_panel->screen[1]);
        this->keys_panel->set_xy((float*)&v);
    }

    this->data_panel = new text_box;
    label_refresh(this->data_panel, tstring("note_bo_data"));

    if (this->picture->flag1() == 0) {
        rgl::vector4 a, b;                                // calc_bounds writes whole vectors
        this->picture->tracks[0]->calc_bounds(&a, &b);    // 0x438d80 (the font object)
        float h = b.y - a.y;
        float w = b.x - a.x;
        x_cur = h + cfg_float("note_image_border") + x_cur;     // the picture sits above the links (0x50c54f)
        {
            float v[2];
            v[0] = (this->data_panel->screen[0] - w) * 0.5f - a.x;       // centred across the panel (0x50c568)
            v[1] = x_cur - b.y;
            this->picture->move_a(v);
        }
    }

    {
        float2 v(this->data_panel->screen[0], x_cur + this->data_panel->screen[1]);
        this->data_panel->set_xy((float*)&v);
    }

    this->keys_panel->add_child(this->report);
    this->data_panel->add_child(this->picture);
    this->data_panel->add_child(this->links);

    UiD* keys_scroll_base = mk_scrollbar(this->panel, "note_s_keys");      // 0x4f6160
    this->keys_scroll = mk_editor_list(this->keys_panel, "note_sc_keys", (int)keys_scroll_base);   // 0x4f62f0

    {
        float h = this->keys_scroll->screen[1] - y_cur;
        h = h > 0.0f ? h : 0.0f;
        float2 v(this->keys_scroll->fE0, h + this->keys_scroll->fE4);
        this->keys_scroll->move_a((float*)&v);
    }
    {
        float2 v(this->keys_scroll->screen[0], fmin2(y_cur, this->keys_scroll->screen[1]));
        this->keys_scroll->set_xy((float*)&v);
    }
    ((Camera*)this->keys_scroll)->draw();       // 0x488d50
    this->panel->add_child(this->keys_scroll);

    if (this->scroll_pos >= -0.0001f) {
        ((UiD*)*(void**)((char*)this->keys_scroll + 0x484))->set_value(this->scroll_pos);
        this->scroll_pos = -1.0f;
    }

    UiD* data_scroll_base = mk_scrollbar(this->panel, "note_s_data");      // 0x4f6160
    this->data_scroll = mk_editor_list(this->data_panel, "note_sc_data", (int)data_scroll_base);   // 0x4f62f0

    {
        float h = this->data_scroll->screen[1] - x_cur;
        h = h > 0.0f ? h : 0.0f;
        float2 v(this->data_scroll->fE0, h + this->data_scroll->fE4);
        this->data_scroll->move_a((float*)&v);
    }
    {
        float2 v(this->data_scroll->screen[0], fmin2(x_cur, this->data_scroll->screen[1]));
        this->data_scroll->set_xy((float*)&v);
    }
    ((Camera*)this->data_scroll)->draw();       // 0x488d50
    this->panel->add_child(this->data_scroll);

    // ---- bottom row of page buttons --------------------------------------
    {
        float bx; cfg_get(bx, "note_button_x");
        float by; cfg_get(by, "note_button_y");
        float bh; cfg_get(bh, "note_button_h");
        float ba; cfg_get(ba, "note_button_x_a");

        this->btn_game_log = make_short_button(this->panel, TR("ui_game_log"), bx, by, bh);
        float x = bx + ba;
        *(int*)((char*)this->btn_game_log + 0x184) = 1;
        this->btn_recent = make_short_button(this->panel, TR("ui_recent_messages"), x, by, bh);
        x = x + ba;
        *(int*)((char*)this->btn_recent + 0x184) = 1;
        this->btn_quests = make_short_button(this->panel, TR("ui_quests"), x, by, bh);
        x = x + ba;
        *(int*)((char*)this->btn_quests + 0x184) = 1;
        this->btn_troops = make_short_button(this->panel, TR("ui_troops2"), x, by, bh);
        x = x + ba;
        *(int*)((char*)this->btn_troops + 0x184) = 1;
        this->btn_locations = make_short_button(this->panel, TR("ui_locations"), x, by, bh);
        x = x + ba;
        *(int*)((char*)this->btn_locations + 0x184) = 1;
        this->btn_factions = make_short_button(this->panel, TR("ui_factions"), x, by, bh);
        *(int*)((char*)this->btn_factions + 0x184) = 1;
        this->btn_return = make_short_button(this->panel, TR("ui_return"), x + ba, by, bh);
    }

    // ---- highlight the active page button and disable it (0x50cbb8: slot 0xb8) ----
    switch (page_) {
    case 1:
        this->btn_troops->set15c(1);
        this->btn_troops->flag2_set();
        break;
    case 2:
        this->btn_factions->set15c(1);
        this->btn_factions->flag2_set();
        break;
    case 3:
        this->btn_locations->set15c(1);
        this->btn_locations->flag2_set();
        break;
    case 4:
        this->btn_quests->set15c(1);
        this->btn_quests->flag2_set();
        break;
    default:
        break;
    }

    ((sel_list*)&g_gui)->add((int)this->panel);     // 0x50d0e0: pushes and sets g_gui.dirty (+0x1708)

    if (any && selected >= 0)
        this->build(page_, selected);
}
