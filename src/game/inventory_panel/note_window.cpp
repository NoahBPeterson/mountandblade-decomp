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

#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <stdlib.h>
#include <math.h>
#include "rgl.h"

// ---------------------------------------------------------------------------
// string object (0x42bbd0 family)
// ---------------------------------------------------------------------------

#include "../tstring.h"

extern "C" int sprintf(char* dst, const char* fmt, ...);     // 0x536f8f
extern "C" double atof(const char* s);                       // 0x537352
// ---------------------------------------------------------------------------
// widgets -- only the vtable slots touched by this function are named
// ---------------------------------------------------------------------------

#include "../ui_widget.h"                 // Widget (the UI widget interface)

// (each constructor on its own line inside its class: the resolution tools read a '// 0x' annotation in the class's scope)
struct NotePanel : Widget {
    char pad[0x190 - 4];
    NotePanel();                        // 0x4931a0
};
struct UiText2 : Widget {
    char pad[0x178 - 4];
    UiText2();                          // 0x48ae20
};
struct UiList : Widget {
    char pad[0x1d0 - 4];
    UiList();                           // 0x4fd150 (MWidget::MWidget, src/game/agent5/agent_hud.cpp)
};
struct UiPanel2 : Widget {
    char pad[0x188 - 4];
    UiPanel2();                         // 0x50d0b0
};

// widget helper functions
extern "C" void  label_refresh(Widget* w, tstring name);                       // 0x4e4c70
extern "C" Widget* make_short_button(Widget* parent, tstring* text,
                                     float x, float y, float h);            // 0x4e5570
extern "C" Widget* make_map_button(Widget* parent, tstring* text,
                                   float x, float y, float h);              // 0x4dcbd0
extern "C" Widget* make_scrollbar(Widget* parent, const tstring* name);        // 0x4f6160
extern "C" Widget* make_scroll2(Widget* parent, const tstring* name,
                                Widget* other);                            // 0x4f62f0

#include "../party/widget_ops.h"            // WidgetOps: the one definition (relayout 0x488d50, visible 0x50cc60)
struct MissionPanel {
    void draw();   // 0x488d50 (game/ui/mission_ui.h): WidgetOps::draw is the same code
};
struct mnode {                                       // game/agent2/agent_types.h: the font object's bounds are mnode::calc_bounds
    void calc_bounds(rgl::vector4* a, rgl::vector4* b);                // 0x438d80 (this = font object)
    __forceinline void calc_bounds(float* a, float* b) { calc_bounds((rgl::vector4*)a, (rgl::vector4*)b); }
};
struct UiD {                                         // game/ui (ui_widgets_c.cpp): the scroll bar's value setter
    void set_value(float pos);                       // 0x48baa0 (this = scroll bar)
};
namespace rgl { struct mesh; }
struct resmgr {
    rgl::mesh* get_object(const rgl::string* name);   // 0x419540 (rgl/core/resmgr.h)
};
struct ThemeDb {
    __forceinline void* get_object(const tstring* key) { return ((resmgr*)this)->get_object((const rgl::string*)key); }   // 0x419540
};
struct mesh_x {
    mesh_x* clone();   // 0x41c6d0 (rgl/mesh_x.h)
};
#define THEME ((ThemeDb*)0x5d6fa0)

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------

#include "../gui_mgr.h"
#define GUI ((GuiManager*)0x5d7080)

struct StrTable {
    tstring* lookup(const tstring& key);   // 0x43a1e0 (game/str_table.h)
};
struct LangTables {
    tstring* find(const tstring* key);   // 0x4c6010 (rgl/mesh2/gui_edit.cpp)
};
struct UiTextDb {                                            // the UI string table at 0x5e2a18, as both classes call it
    __forceinline tstring* lookup(const tstring* key) { return ((StrTable*)this)->lookup(*key); }
    __forceinline tstring* find(const tstring* key) { return ((LangTables*)this)->find(key); }
};
#define UITEXT ((UiTextDb*)0x5e2a18)

#include "../ini_file.h"
#define CONFIG (&g_game_vars)

struct TableauTbl {                                          // world_data/obj_tbl.h: the image table at 0xa75f40
    rgl::mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0
    __forceinline void* make_mesh_from(void* rec, int* id) { return make_mesh_from((const int*)rec, (const void*)id); }
};
#define IMAGES ((TableauTbl*)0xa75f40)

#include "../../include/game_data.h"             // GameData: the game object at 0x5e2930 (troops +0x13a00, factions +0x13a48, quests +0x13a58, parties +0xf8)
#define WORLD g_game

// map-party accessors (this = world + 0xf8: the party manager, include/party.h; the slice called it WorldMap)
#include "../../include/party.h"
#define WORLDMAP ((PartyMgr*)((char*)WORLD + 0xf8))
struct Map10007 {
    int lookup(unsigned id);   // 0x48ad00 (game/ui/mission.h): the party-id hash at the head of the manager (PartyMgr::index_of)
};


// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------

// localised string for a key
static const char* tr(const char* key)
{
    tstring k(key);
    tstring* s = UITEXT->lookup(&k);
    return s->buf ? s->buf : (char*)&s->buf;
}

// translated string object (kept as a pointer: the database owns the object). The key is a temporary of the full
// expression that uses the result: it is destroyed after the call the translation is passed to (0x50a476..0x50a4ae)
static __forceinline tstring* tr_key(const tstring& k)
{
    return UITEXT->lookup(const_cast<tstring*>(&k));
}
#define tr_obj(key) tr_key(tstring(key))

// config entry -> text value
static char* cfg_text(int idx)
{
    char* e = (char*)&g_game_vars + 0x1c + idx * 0x2c;
    char* s = *(char**)(e + 4);
    return s ? s : (e + 4);
}

static float cfg_float(const char* key)
{
    tstring k(key);
    int i = CONFIG->find(k);
    float v = 0.0f;
    if (i >= 0)
        v = (float)atof(cfg_text(i));
    return v;
}

// name of a game record (troop/party/faction), key stored in a tstring
// a quest's title without its markup: the tstring at +0x40 of the 0x138-byte quest record, strip_tags 0x42c5d0
// (thiscall, returns by value)
static __forceinline tstring quest_title(int q)
{
    return ((const tstring*)(((char*)WORLD->quests) + q * 0x138 + 0x40))->strip_tags();
}

static const char* record_name(void* key)
{
    tstring* s = UITEXT->find((tstring*)key);
    return s->buf ? s->buf : (char*)&s->buf;
}

static const char* ts_chars(const tstring* s)
{
    return s->buf ? s->buf : (const char*)&s->buf;
}

// name tstring of a map-party record (record base from the party table)
static tstring* party_name(char* rec)
{
    if (*(char*)0xa181b8 != 0)
        return (tstring*)(rec + 0x20);
    if (*(char*)(rec + 0x1c) == 0) {
        tstring* s = UITEXT->find((tstring*)(rec + 0x20));
        if (s)
            return s;
    }
    return (tstring*)(rec + 0x0c);
}

struct int_vector {
    int* first;
    int* last;
    int* cap;
    int_vector() { first = 0; last = 0; cap = 0; }
    ~int_vector() { delete[] first; }
    int size() const { return (int)(last - first); }
    int operator[](int i) const { return first[i]; }
    void push_back(int v)
    {
        if (last == cap) {
            int n = (int)(last - first);
            int m = n ? n * 2 : 8;
            int* p = new int[m];
            for (int i = 0; i < n; i++)
                p[i] = first[i];
            delete[] first;
            first = p;
            last = p + n;
            cap = p + m;
        }
        *last++ = v;
    }
};

// ---------------------------------------------------------------------------
// the window
// ---------------------------------------------------------------------------

struct note_window {
    char pad00[0x18];
    int  menu_id;              // +0x18
    char pad1c[0x2c - 0x1c];
    Widget* panel;             // +0x2c
    Widget* picture;           // +0x30
    Widget* report;            // +0x34
    Widget* title;             // +0x38
    Widget* links;             // +0x3c
    Widget* keys_panel;        // +0x40
    Widget* data_panel;        // +0x44
    Widget* keys_scroll;       // +0x48
    Widget* data_scroll;       // +0x4c
    Widget* btn_game_log;      // +0x50
    Widget* btn_recent;        // +0x54
    Widget* btn_troops;        // +0x58
    Widget* btn_quests;        // +0x5c
    Widget* btn_factions;      // +0x60
    Widget* btn_locations;     // +0x64
    Widget* btn_return;        // +0x68
    Widget* btn_back;          // +0x6c
    Widget* btn_forward;       // +0x70
    Widget* btn_map;           // +0x74
    char   pad78[4];
    int*   back_beg;           // +0x7c
    int*   back_end;           // +0x80
    int*   back_cap;           // +0x84
    int    unk_88;             // +0x88
    int*   fwd_beg;            // +0x8c
    int*   fwd_end;            // +0x90
    int*   fwd_cap;            // +0x94
    int    back_sel_a;         // +0x98
    int    back_sel_b;         // +0x9c
    int    fwd_sel_a;          // +0xa0
    int    fwd_sel_b;          // +0xa4
    int    page;               // +0xa8
    int    arg;                // +0xac
    float  scroll_pos;         // +0xb0

    void build(int page, int arg);              // 0x50a1c0
};

struct sel_list;
void __stdcall sort_list(sel_list* l, int mode);       // 0x50cc70 (inventory_panel/widget_text.cpp)
static float fld(void* p, int off) { return *(float*)((char*)p + off); }
static void  fst(void* p, int off, float v) { *(float*)((char*)p + off) = v; }

// @ 0x50a1c0
void note_window::build(int page_, int arg_)
{
    char buf[0x400];
    char buf2[0x400];
    char link_buf[0x400];

    this->page = page_;
    this->arg = arg_;
    GUI->reset();

    // ---- main panel ------------------------------------------------------
    NotePanel* panel = new NotePanel;                // base constructor 0x4931a0, then the container's vtable and f188
    *(void**)panel = (void*)0x592848;                  // (0x50a20e; the same window class as agent_hud.h's FrameWidget)
    *(char*)((char*)panel + 0x188) = 0;
    this->panel = panel;
    {
        float v[2];
        v[0] = 0.0f;
        v[1] = 0.0f;
        panel->set_pos(v);
        v[0] = 1.0f;
        v[1] = 0.75f;
        panel->set_scale(v);
    }
    {
        tstring key("note_window");
        void* item = THEME->get_object(&key);       // 0x419540
        panel->set_track(0, ((mesh_x*)item)->clone());   // 0x41c6d0
    }

    // ---- title picture ---------------------------------------------------
    this->picture = new UiText2;
    label_refresh(this->picture, tstring("note_i_picture"));
    this->picture->hide();

    // ---- report text -----------------------------------------------------
    this->report = new UiList;
    {
        tstring empty("");
        float sz[2];
        this->report->set_text(sz, empty);
    }
    label_refresh(this->report, tstring("note_t_keys"));

    // ---- links list (made before the title: 0x50a34b / 0x50a3b3) ------------------------------------------------
    this->links = new UiList;
    {
        tstring empty("");
        float sz[2];
        this->links->set_text(sz, empty);
    }
    label_refresh(this->links, tstring("note_t_data"));          // (it joins the data panel later, 0x50c5eb)

    // ---- report title: the one widget the main panel holds directly (0x50a3e8..0x50a41d) ----
    this->title = new UiText2;
    label_refresh(this->title, tstring("note_t_title"));
    {
        tstring empty("");
        float sz[2];
        this->title->set_text(sz, empty);
    }
    panel->add_child(this->title);

    // ---- back / forward / map buttons ------------------------------------
    {
        float small_h = cfg_float("note_button_sm_h");      // the config's small-button height (0x50a423..0x50a466)
        this->btn_back = make_short_button(panel, tr_obj("ui_back"), 0.0f, 0.0f, small_h);
        label_refresh(this->btn_back, tstring("note_b_back"));
        this->btn_forward = make_short_button(panel, tr_obj("ui_forward"), 0.0f, 0.0f, small_h);
        label_refresh(this->btn_forward, tstring("note_b_forward"));
        this->btn_map = make_map_button(panel, tr_obj("ui_display_on_map"), 0.0f, 0.0f, small_h);
        label_refresh(this->btn_map, tstring("note_b_map_follow"));
        this->btn_map->hide();
    }

    // a button is enabled only when its history has an entry to go to (0x50a5db: slot 0xbc enable, 0x50a5e8: 0xb8
    // disable; these calls predate the ui_widget.h enable/disable slot fix and were left inverted)
    if (this->back_end == 0) {
        this->btn_back->disable();
    } else {
        this->back_sel_a = this->back_end[-2];
        this->back_sel_b = this->back_end[-1];
        this->btn_back->enable();
    }
    if (this->fwd_end == 0) {
        this->btn_forward->disable();
    } else {
        this->fwd_sel_a = this->fwd_end[-2];
        this->fwd_sel_b = this->fwd_end[-1];
        this->btn_forward->enable();
    }

    // ---- page link key + candidate list ----------------------------------
    tstring acc("");            // +0xa0   "<trp=%d>" for the current page
    int_vector vec;             // +0x60
    tstring other("");          // +0x70   "other pages that link here"
    tstring link_text("");      // +0xb0   scratch for the links widget

    float x_cur;                // [esp+0x14]  never initialized: the original leaves both unset (0x50a66d)
    float y_cur;                // [esp+0x24]  (case 0 and an empty troop page reach the panels with garbage)
    bool  any = true;           // [esp+0x1b]
    int   selected = -1;        // [esp+0x58]
    this->picture->hide();

    switch (page_) {
    case 0: {
        tstring line("");
        float sz[2];
        tstring* t = tr_obj("ui_click_button_to_view_note");
        x_cur = ((float*)this->links->set_text(sz, *t))[1];    // the returned size (0x50a6d8), not sz
        break;
    }

    case 1: {
        // ---- troop report -------------------------------------------------
        int ti = page_;
        char* troop = ((char*)WORLD->troops) + ti * 0x508;
        if (*(int*)(troop + 0x4a0) >= 0) {
            int id = ti;
            this->picture->set_track(0, IMAGES->make_mesh_from(troop + 0x4a0, &id));
            this->picture->show();
        }
        sprintf(buf, "<trp=%d>", ti);
        acc.assign(buf);

        tstring line("");
        for (int k = 0; k < 8; k++) {
            char* slot = troop + k * 0x10;
            if (*(int*)(slot + 0x42c) <= 0)
                continue;
            if (line.len > 0) {
                line.append(tstring("\n\n"));
            }
            line.append(*(tstring*)(slot + 0x420));
            int sid = *(int*)(((char*)WORLD->troops) + (ti * 0x142 + k) * 4 + 0x400);
            if (sid < 0)
                continue;
            if (sid < WORLD->string_count) {
                sprintf(buf2, tr("ui_report_is_value_days_old"), WORLD->string_count - sid);
                line.append(tstring(buf2));
            } else {
                line.append(tstring(tr("ui_report_is_current")));
            }
        }

        if (line.len > 0) {
            float sz[2];
            this->title->set_text(sz, tstring(record_name(troop + 0x20)));
            float* r = (float*)this->links->set_text(sz, line);
            x_cur = r[1];
            line.clear();
            any = false;
        } else {
            float sz[2];
            float* r = (float*)this->links->set_text(sz, *tr_obj("ui_click_button_to_view_note"));
            x_cur = r[1];
        }

        // all troops that carry at least one report item
        for (int i = 0; i < WORLD->ntroops; i++) {
            char* rec = ((char*)WORLD->troops) + i * 0x508;
            for (int k = 0; k < 8; k++) {
                if (*(int*)(rec + 0x42c + k * 0x10) > 0) {
                    vec.push_back(i);
                    break;
                }
            }
        }
        ::sort_list((sel_list*)&vec, 1);

        for (int i = 0; i < vec.size(); i++) {
            int v = vec[i];
            if (selected < 0)
                selected = v;
            const char* nm = record_name(((char*)WORLD->troops) + v * 0x508 + 0x20);
            if (v == page_)
                sprintf(link_buf, "%s\n", nm);
            else
                sprintf(link_buf, "<trp=%d>%s</>\n", v, nm);
            line.append(tstring(link_buf));
        }
        if (line.len > 0)
            line.erase(line.len - 1, 1);
        {
            float sz[2];
            this->report->set_text(sz, line);
        }
        break;
    }

    case 2: {
        // ---- faction report -----------------------------------------------
        int fi = page_;
        char* fac = ((char*)WORLD->factions) + fi * 0x300;
        if (*(int*)(fac + 0x2fc) >= 0) {
            int id = fi;
            this->picture->set_track(0, IMAGES->make_mesh_from(fac + 0x2fc, &id));
            this->picture->show();
        }
        sprintf(buf, "<fac=%d>", fi);
        acc.assign(buf);

        tstring line("");
        for (int k = 0; k < 8; k++) {
            char* slot = fac + k * 0x10;
            if (*(int*)(slot + 0x288) <= 0)
                continue;
            if (line.len > 0)
                line.append(tstring("\n\n"));
            line.append(*(tstring*)(slot + 0x27c));
            int sid = *(int*)(((char*)WORLD->factions) + (fi * 0xc0 + k) * 4 + 0x25c);
            if (sid < 0)
                continue;
            if (sid < WORLD->string_count) {
                sprintf(buf2, tr("ui_report_is_value_days_old"), WORLD->string_count - sid);
                line.append(tstring(buf2));
            } else {
                line.append(tstring(tr("ui_report_is_current")));
            }
        }

        if (line.len > 0) {
            float sz[2];
            tstring* nm = (tstring*)fac;
            if (*(char*)(fac + 0x10) == 0) {
                tstring* s = UITEXT->find((tstring*)(fac + 0x14));
                if (s)
                    nm = s;
            }
            this->title->set_text(sz, *nm);
            float* r = (float*)this->links->set_text(sz, line);
            x_cur = r[1];
            line.clear();
            any = false;
        } else {
            float sz[2];
            float* r = (float*)this->links->set_text(sz, *tr_obj("ui_click_button_to_view_note"));
            x_cur = r[1];
        }

        for (int i = 0; i < WORLD->n_factions; i++) {
            char* rec = ((char*)WORLD->factions) + i * 0x300;
            for (int k = 0; k < 8; k++) {
                if (*(int*)(rec + 0x288 + k * 0x10) > 0) {
                    vec.push_back(i);
                    break;
                }
            }
        }
        ::sort_list((sel_list*)&vec, 2);

        for (int i = 0; i < vec.size(); i++) {
            int v = vec[i];
            if (selected < 0)
                selected = v;
            const char* nm = record_name(((char*)WORLD->factions) + v * 0x300 + 0x14);
            if (v == page_)
                sprintf(link_buf, "%s\n", nm);
            else
                sprintf(link_buf, "<fac=%d>%s</>\n", v, nm);
            line.append(tstring(link_buf));
        }
        if (line.len > 0)
            line.erase(line.len - 1, 1);
        {
            float sz[2];
            this->report->set_text(sz, line);
        }
        break;
    }

    case 3: {
        // ---- map party (location) report ----------------------------------
        int pi = page_;
        void* party = WORLDMAP->get(pi);
        if (*(int*)((char*)party + 0x5aa8) >= 0) {
            int id = pi;
            this->picture->set_track(0, IMAGES->make_mesh_from((char*)party + 0x5aa8, &id));
            this->picture->show();
        }
        sprintf(buf, "<p=%d>", pi);
        acc.assign(buf);

        tstring line("");
        int idx = ((Map10007*)WORLDMAP)->lookup(pi);
        char* base = ((char**)WORLD->parties.pages._Myfirst)[idx >> 4] + (idx & 0xf) * 0x5ac8;
        char* ids  = ((char**)WORLD->parties.pages._Myfirst)[idx >> 4] + (idx & 0xf) * 0x16b2 * 4;
        for (int k = 0; k < 8; k++) {
            if (*(int*)(base + 0x5a34 + k * 0x10) <= 0)
                continue;
            if (line.len > 0)
                line.append(tstring("\n\n"));
            line.append(*(tstring*)(base + 0x5a28 + k * 0x10));
            int sid = *(int*)(ids + 0x5a08 + k * 4);
            if (sid < 0)
                continue;
            if (sid < WORLD->string_count) {
                sprintf(buf2, tr("ui_report_is_value_days_old"), WORLD->string_count - sid);
                line.append(tstring(buf2));
            } else {
                line.append(tstring(tr("ui_report_is_current")));
            }
        }

        if (line.len > 0) {
            float sz[2];
            if (this->menu_id == 7)
                this->btn_map->show();
            this->title->set_text(sz, *party_name(base));
            float* r = (float*)this->links->set_text(sz, line);
            x_cur = r[1];
            line.clear();
            any = false;
        } else {
            float sz[2];
            float* r = (float*)this->links->set_text(sz, *tr_obj("ui_click_button_to_view_note"));
            x_cur = r[1];
        }

        // live map parties
        {
            int v = WORLDMAP->first_party_id();
            while (v < (int)WORLD->parties.n_parties) {
                int vi = ((Map10007*)WORLDMAP)->lookup(v);
                char* prec = ((char**)WORLD->parties.pages._Myfirst)[vi >> 4] + (vi & 0xf) * 0x5ac8;
                for (int k = 0; k < 8; k++) {
                    if (*(int*)(prec + 0x5a34 + k * 0x10) > 0) {
                        vec.push_back(v);
                        break;
                    }
                }
                WORLDMAP->next_party_id(&v);
            }
        }
        ::sort_list((sel_list*)&vec, 3);

        for (int i = 0; i < vec.size(); i++) {
            int v = vec[i];
            if (selected < 0)
                selected = v;
            int vi = ((Map10007*)WORLDMAP)->lookup(v);
            char* rec = ((char**)WORLD->parties.pages._Myfirst)[vi >> 4] + (vi & 0xf) * 0x5ac8;
            const char* nm = ts_chars(party_name(rec));
            if (v == page_)
                sprintf(link_buf, "%s\n", nm);
            else
                sprintf(link_buf, "<p=%d>%s</>\n", v, nm);
            line.append(tstring(link_buf));
        }
        if (line.len > 0)
            line.erase(line.len - 1, 1);
        {
            float sz[2];
            y_cur = ((float*)this->report->set_text(sz, line))[1];
        }
        break;
    }

    case 4: {
        // ---- quest report -------------------------------------------------
        int qi = page_;
        char* quest = ((char*)WORLD->quests) + qi * 0x138;
        if (*(int*)(quest + 0x108) >= 0) {
            int id = qi;
            this->picture->set_track(0, IMAGES->make_mesh_from(quest + 0x108, &id));
            this->picture->show();
        }
        sprintf(buf, "<qst=%d>", qi);
        acc.assign(buf);

        tstring line("");
        for (int k = 0; k < 8; k++) {
            char* slot = quest + k * 0x10;
            if (*(int*)(slot + 0x94) <= 0)
                continue;
            if (line.len > 0)
                line.append(tstring("\n\n"));
            line.append(*(tstring*)(slot + 0x88));
            int sid = *(int*)(quest + 0x68);
            if (sid < 0)
                continue;
            if (sid < WORLD->string_count) {
                sprintf(buf2, tr("ui_report_is_value_days_old"), WORLD->string_count - sid);
                line.append(tstring(buf2));
            } else {
                line.append(tstring(tr("ui_report_is_current")));
            }
        }

        if (line.len > 0) {
            float sz[2];
            tstring qt = quest_title(qi);
            this->title->set_text(sz, qt);
            float* r = (float*)this->links->set_text(sz, line);
            x_cur = r[1];
            line.clear();
            any = false;
        } else {
            float sz[2];
            float* r = (float*)this->links->set_text(sz, *tr_obj("ui_click_button_to_view_note"));
            x_cur = r[1];
        }

        for (int i = 0; i < WORLD->n_quests; i++) {
            if (*(int*)(((char*)WORLD->quests) + i * 0x138 + 0x94) > 0)
                vec.push_back(i);
        }
        ::sort_list((sel_list*)&vec, 4);

        for (int i = 0; i < vec.size(); i++) {
            int v = vec[i];
            if (selected < 0)
                selected = v;
            sprintf(link_buf, "<qst=%d>%s</>\n", v, quest_title(v).c_str());
            line.append(tstring(link_buf));
        }
        if (line.len > 0)
            line.erase(line.len - 1, 1);
        {
            float sz[2];
            y_cur = ((float*)this->report->set_text(sz, line))[1];
        }
        break;
    }

    default:
        break;
    }

    // ---- collect the back references -------------------------------------
    this->links->get_text(&link_text);
    if (acc.len > 0) {
        for (int i = 0; i < WORLD->ntroops; i++) {
            char* rec = ((char*)WORLD->troops) + i * 0x508;
            for (int k = 0; k < 8; k++) {
                char* slot = rec + k * 0x10;
                if (*(int*)(slot + 0x42c) <= 0)
                    continue;
                if (((tstring*)(slot + 0x420))->find(acc, 0) < 0)
                    continue;
                sprintf(buf, "<trp=%d>%s</>", i, record_name(rec + 0x20));
                tstring entry(buf);
                if (other.find(entry, 0) < 0) {
                    if (other.len > 0)
                        other.append(tstring(", "));
                    other.append(entry);
                }
                break;
            }
        }
        for (int i = 0; i < WORLD->n_factions; i++) {
            char* rec = ((char*)WORLD->factions) + i * 0x300;
            for (int k = 0; k < 8; k++) {
                char* slot = rec + k * 0x10;
                if (*(int*)(slot + 0x288) <= 0)
                    continue;
                if (((tstring*)(slot + 0x27c))->find(acc, 0) < 0)
                    continue;
                tstring* fnm = (tstring*)rec;
                if (*(char*)(rec + 0x10) == 0) {
                    tstring* s = UITEXT->find((tstring*)(rec + 0x14));
                    if (s)
                        fnm = s;
                }
                sprintf(buf, "<fac=%d>%s</>", i, ts_chars(fnm));
                tstring entry(buf);
                if (other.find(entry, 0) < 0) {
                    if (other.len > 0)
                        other.append(tstring(", "));
                    other.append(entry);
                }
                break;
            }
        }
        {
            int v = WORLDMAP->first_party_id();
            while (v < (int)WORLD->parties.n_parties) {
                int vi = ((Map10007*)WORLDMAP)->lookup(v);
                char* rec = ((char**)WORLD->parties.pages._Myfirst)[vi >> 4] + (vi & 0xf) * 0x5ac8;
                for (int k = 0; k < 8; k++) {
                    if (*(int*)(rec + 0x5a34 + k * 0x10) <= 0)
                        continue;
                    if (((tstring*)(rec + 0x5a28 + k * 0x10))->find(acc, 0) < 0)
                        continue;
                    sprintf(buf, "<p=%d>%s</>", v, ts_chars(party_name(rec)));
                    tstring entry(buf);
                    if (other.find(entry, 0) < 0) {
                        if (other.len > 0)
                            other.append(tstring(", "));
                        other.append(entry);
                    }
                    break;
                }
                WORLDMAP->next_party_id(&v);
            }
        }
        for (int i = 0; i < WORLD->n_quests; i++) {
            tstring qt = quest_title(i);
            if (qt.find(acc, 0) < 0)
                continue;
            sprintf(buf, "<qst=%d>%s</>", i, qt.c_str());
            tstring entry(buf);
            if (other.find(entry, 0) < 0) {
                if (other.len > 0)
                    other.append(tstring(", "));
                other.append(entry);
            }
        }
    }

    if (other.len > 0) {
        tstring t("");
        this->links->get_text(&t);
        if (t.equals(*tr_obj("ui_click_button_to_view_note"))) {
            float sz[2];
            this->links->set_text(sz, *tr_obj("ui_this_page_contains_no_information"));
            this->links->get_text(&t);
        }
        t.append(tstring("\n\n"));
        t.append(*tr_obj("ui_other_pages_that_link_here"));
        t.append(other);
        float sz[2];
        float* r = (float*)this->links->set_text(sz, t);
        x_cur = r[1];
    }

    // ---- key / data scrolling panels -------------------------------------
    // x_cur holds the links text height, y_cur the report's (0x50c405)
    x_cur += 0.001f;
    y_cur += 0.001f;
    UiPanel2* keys = new UiPanel2;
    this->keys_panel = keys;
    label_refresh(keys, tstring("note_bo_keys"));
    {
        float v[2];
        v[0] = fld(keys, 0xf0);
        v[1] = y_cur + fld(keys, 0xf4);
        keys->set_scale(v);
    }

    UiPanel2* data = new UiPanel2;
    this->data_panel = data;
    label_refresh(data, tstring("note_bo_data"));

    if (((WidgetOps*)this->picture)->flag1() == 0) {
        float a[2], b[2];
        ((mnode*)(*(void**)((char*)this->picture + 0x108)))->calc_bounds(a, b);
        float h = b[1] - a[1];
        float w = b[0] - a[0];
        float border = cfg_float("note_image_border");
        x_cur = (h + border) + x_cur;                     // the picture sits above the links (0x50c54f)
        float v[2];
        v[0] = (fld(data, 0xf0) - w) * 0.5f - a[0];       // centred across the panel (0x50c568)
        v[1] = x_cur - b[1];
        this->picture->set_pos(v);
    }

    {
        float v[2];
        v[0] = fld(data, 0xf0);
        v[1] = x_cur + fld(data, 0xf4);
        data->set_scale(v);
    }

    this->keys_panel->add_child(this->report);
    this->data_panel->add_child(this->picture);
    this->data_panel->add_child(this->links);

    tstring s_keys("note_s_keys");
    tstring sc_keys("note_sc_keys");
    Widget* keys_scroll_base = make_scrollbar(this->panel, &s_keys);      // 0x4f6160
    this->keys_scroll = make_scroll2(this->keys_panel, &sc_keys,
                                     keys_scroll_base);                 // 0x4f62f0

    {
        float v[2];
        float h = fld(this->keys_scroll, 0xf4) - y_cur;
        if (h < 0.0f)
            h = 0.0f;
        v[0] = fld(this->keys_scroll, 0xe0);
        v[1] = h + fld(this->keys_scroll, 0xe4);
        this->keys_scroll->set_pos(v);

        v[1] = (y_cur <= fld(this->keys_scroll, 0xf4)) ? y_cur
                                                       : fld(this->keys_scroll, 0xf4);
        v[0] = fld(this->keys_scroll, 0xf0);
        this->keys_scroll->set_scale(v);
        ((MissionPanel*)this->keys_scroll)->draw();       // 0x488d50
        this->panel->add_child(this->keys_scroll);
    }

    if (this->scroll_pos >= -0.0001f) {
        ((UiD*)*(void**)((char*)this->keys_scroll + 0x484))->set_value(this->scroll_pos);
        this->scroll_pos = -1.0f;
    }

    tstring s_data("note_s_data");
    tstring sc_data("note_sc_data");
    Widget* data_scroll_base = make_scrollbar(this->panel, &s_data);      // 0x4f6160
    this->data_scroll = make_scroll2(this->data_panel, &sc_data,
                                     data_scroll_base);                 // 0x4f62f0

    {
        float v[2];
        float h = fld(this->data_scroll, 0xf4) - x_cur;
        if (h < 0.0f)
            h = 0.0f;
        v[0] = fld(this->data_scroll, 0xe0);
        v[1] = h + fld(this->data_scroll, 0xe4);
        this->data_scroll->set_pos(v);

        v[1] = (x_cur <= fld(this->data_scroll, 0xf4)) ? x_cur
                                                       : fld(this->data_scroll, 0xf4);
        v[0] = fld(this->data_scroll, 0xf0);
        this->data_scroll->set_scale(v);
        ((MissionPanel*)this->data_scroll)->draw();       // 0x488d50
        this->panel->add_child(this->data_scroll);
    }

    // ---- bottom row of page buttons --------------------------------------
    {
        float bx = cfg_float("note_button_x");
        float by = cfg_float("note_button_y");
        float bh = cfg_float("note_button_h");
        float ba = cfg_float("note_button_x_a");

        this->btn_game_log = make_short_button(this->panel, tr_obj("ui_game_log"), bx, by, bh);
        x_cur = bx + ba;
        this->btn_recent = make_short_button(this->panel, tr_obj("ui_recent_messages"), x_cur, by, bh);
        x_cur = x_cur + ba;
        this->btn_quests = make_short_button(this->panel, tr_obj("ui_quests"), x_cur, by, bh);
        x_cur = x_cur + ba;
        this->btn_troops = make_short_button(this->panel, tr_obj("ui_troops2"), x_cur, by, bh);
        x_cur = x_cur + ba;
        this->btn_locations = make_short_button(this->panel, tr_obj("ui_locations"), x_cur, by, bh);
        x_cur = x_cur + ba;
        this->btn_factions = make_short_button(this->panel, tr_obj("ui_factions"), x_cur, by, bh);
        x_cur = x_cur + ba;
        this->btn_return = make_short_button(this->panel, tr_obj("ui_return"), x_cur, by, bh);
        *(int*)((char*)this->btn_return + 0x184) = 1;
    }

    // ---- highlight the active page button and disable it (0x50cbb8: slot 0xb8) ----
    switch (page_) {
    case 1:
        this->btn_troops->activate(1);
        this->btn_troops->disable();
        break;
    case 2:
        this->btn_factions->activate(1);
        this->btn_factions->disable();
        break;
    case 3:
        this->btn_locations->activate(1);
        this->btn_locations->disable();
        break;
    case 4:
        this->btn_quests->activate(1);
        this->btn_quests->disable();
        break;
    default:
        break;
    }

    GUI->add(this->panel);

    if (any && selected >= 0)
        this->build(page_, selected);
}
