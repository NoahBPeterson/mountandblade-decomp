// Slice s00504ef0: the party-window setup (0x504ef0): builds the two party pages, the two member
// lists per page, the 0x40 member buttons of each, the information bars and the party captions.
// Names and layouts are provisional; every offset comes from this slice's disassembly.
#include <windows.h>
#include <stdlib.h>
#include <vector>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---------------------------------------------------------------------------------------------------------
// tstring (layout from slice s004bd170).  The copy ctor is only ever *declared*: with it, cl 13.10
// constructs a `tstring("literal")` argument by value directly in the callee's argument slot
// (sub esp,0x10 / mov ecx,esp / push lit / call ctor) instead of building a frame temporary and
// copying it (which is what an implicit copy ctor produces).
struct tstring {
    int   pos;                                          // +0
    char* buf;                                          // +4
    int   cap;                                          // +8
    int   len;                                          // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // declared only (never emitted)
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    bool equals(const tstring& o) const;                // 0x42bdd0
};

struct ini_entry {
    tstring key;                                        // +0
    char    pad10[0xc];                                 // +0x10
    tstring value;                                      // +0x1c
};

// 0xa44240.  get_float is an inline header function: the original inlines the whole entry scan
// (tstring::equals loop + atof of the value buffer) at every call site.
struct ini_file {
    ini_entry entries[0x1000];                          // +0
    int       count;                                    // +0x2c000 (0xa70240)
    float get_float(const tstring& key) {
        float r   = 0.0f;
        int   idx = -1;
        for (int i = 0; i < count; i++) {
            if (entries[i].key.equals(key)) { idx = i; break; }
        }
        if (idx >= 0) {
            char* p = entries[idx].value.buf;
            if (p == 0) p = (char*)&entries[idx].value.buf;
            r = (float)atof(p);
        }
        return r;
    }
};
ini_file g_ini;                                         // 0xa44240

struct str_table {                                      // 0x5e2a18
    tstring* lookup(const tstring& key);                // 0x43a1e0 (never null)
    tstring* find(const tstring& key);                  // 0x4c6010 (null when missing)
};
str_table g_strtable;                                   // 0x5e2a18

// ---------------------------------------------------------------------------------------------------------
// the rgl widget base (ctor 0x48ae20, size 0x178): position at +0xe0.
struct panel {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(float* p);                                 // +0x58 (slot 22)
    virtual void v23(); virtual void v24(); virtual void v25(); virtual void v26();
    virtual void v27(); virtual void v28(); virtual void v29(); virtual void v30();
    virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38();
    virtual void v39(); virtual void v40();
    virtual void set_text(const float* pos, const tstring* s);      // +0xa4 (slot 41)
    virtual void v42(); virtual void v43(); virtual void v44(); virtual void v45();
    virtual void v46(); virtual void v47(); virtual void v48(); virtual void v49();
    virtual void add_child(panel* c);                               // +0xc8 (slot 50)

    char  pad04[0xe0 - 4];
    float x;                                            // +0xe0
    float y;                                            // +0xe4
    char  pad_e8[0x178 - 0xe8];
    panel() MB_NOTHROW;                                 // 0x48ae20
};

// 0x188 bytes, vptr 0x5924a0, three zeroed fields: its ctor is inlined at the `new` below
// (the base ctor 0x48ae20 is called out of line, then the vptr and +0x17c/+0x180/+0x184 are stored).
struct list_widget : panel {
    int f178;                                           // +0x178 (left uninitialised by the ctor)
    int f17c;                                           // +0x17c
    int f180;                                           // +0x180
    unsigned f184;                                      // +0x184
    list_widget() MB_NOTHROW { f17c = 0; f180 = 0; f184 = 0; }
};

// the same layout, but its ctor lives outside this TU (0x4931a0): `new party_bar` therefore emits
// `call 0x4931a0` and then stores the derived vptr 0x597050 in the caller.
struct window : panel {
    int f178;                                           // +0x178
    int f17c;                                           // +0x17c
    int f180;                                           // +0x180
    unsigned f184;                                      // +0x184
    window() MB_NOTHROW;                                // 0x4931a0
};
struct party_bar : window {                             // vptr 0x597050 (implicit ctor)
};

// a widget whose children are an MSVC 7.1 std::vector<panel*> at +0x178 (proxy, first, last, end):
// the party bar body at party_window::link.
struct widget_bar : panel {
    std::vector<panel*> children;                       // +0x178
};

// a 0x5ac8-byte party record from the party manager's block table.
struct party_record {
    char    pad00[0x0c];
    tstring name2;                                      // +0x0c
    char    f1c;                                        // +0x1c
    char    pad1d[0x20 - 0x1d];
    tstring name;                                       // +0x20
};

// g_game + 0xf8: the party manager (party_id -> record).
struct party_manager {
    char   pad0[0x138d0];
    void** pages;                                       // +0x138d0
    int    index_of(int id);                            // 0x48ad00
};

struct World {
    char          pad0[0x8c];
    int           f8c;                                  // +0x8c
    char          pad90[0xf8 - 0x90];
    party_manager party;                                // +0xf8
    int           query(int a, int b);                  // 0x437b20
};
extern World* g_game;                                   // 0x5e2930
extern char g_use_alt_name;                             // 0xa181b8

// the party window (this): its mode, the party id and every widget it owns.
struct party_window {
    char    pad00[4];
    int     mode;                                       // +0x04
    char    pad08[0x10 - 8];
    int     party_id;                                   // +0x10
    char    pad14[0x2c - 0x14];
    widget_bar* link;                                   // +0x2c
    char    pad30[0x40 - 0x30];
    panel*  prisoner_management;                        // +0x40
    char    pad44[0x5c - 0x44];
    panel*  gold;                                       // +0x5c
    char    pad60[0x64 - 0x60];
    panel*  weekly_cost;                                // +0x64
    panel*  max_companions;                             // +0x68
    panel*  max_prisoners;                              // +0x6c
    panel*  choose_companions;                          // +0x70
    panel*  choose_prisoners;                           // +0x74
    char    pad78[0x80 - 0x78];
    panel*  morale;                                     // +0x80
    char    pad84[0x88 - 0x84];
    panel*  members_base[2];                            // +0x88
    char    pad90[0x98 - 0x90];
    panel*  member_list[2][2];                          // +0x98  [side][page]
    panel*  members_page[2][2];                         // +0xa8  [side][page]
    panel*  buttons[4][0x40];                           // +0xb8  [2*side + page][i]

    void setup_party(int side);                         // 0x504ef0
};

// engine callees
void   party_bar_setup(panel* w, tstring s);                        // 0x4e4c70
panel* make_scrollbar(panel* parent, const tstring* name);          // 0x4f6160
panel* make_member_list(panel* parent, const tstring* name, panel* sc);  // 0x4f62f0
panel* new_member_button(panel* parent, const tstring& name, float x, int zero, float h,
                         const tstring& text, const tstring& pressed, float width_ratio,
                         float height, float ratio);                // 0x4dcc50

// @ 0x504ef0
void party_window::setup_party(int side)
{
    float caption_pos[2];           // passed uninitialized by the original (a game bug)

    members_base[side] = new party_bar;
    if (side == 0) {
        party_bar_setup(members_base[0], tstring("party_bo_party1"));
        if (g_game->query(g_game->f8c, 2) == 0) {
            prisoner_management = new panel;
            party_bar_setup(prisoner_management, tstring("party_t_prisoner_management"));
            prisoner_management->set_text(
                caption_pos, g_strtable.lookup(tstring("ui_need_to_learn_prisoner_management")));
            members_base[0]->add_child(prisoner_management);
        }
    } else {
        party_bar_setup(members_base[side], tstring("party_bo_party2"));
    }

    for (int page = 0; page < 2; page++) {
        members_page[side][page] = new list_widget;
        party_bar_setup(members_page[side][page], tstring("party_bo_members_base"));
        if (page == 0) {
            tstring s1("party_s_members1");
            panel* sc = make_scrollbar(members_base[side], &s1);
            tstring s2("party_sc_members1");
            member_list[side][page] = make_member_list(members_base[side], &s2, sc);
        } else {
            tstring s1("party_s_members2");
            panel* sc = make_scrollbar(members_base[side], &s1);
            tstring s2("party_sc_members2");
            member_list[side][page] = make_member_list(members_base[side], &s2, sc);
        }
        members_base[side]->add_child(member_list[side][page]);

        for (int i = 0; i < 0x40; i++) {
            tstring k_button_h("party_b_members_button_h");
            tstring k_width_ratio("party_b_members_width_ratio");
            tstring k_h("party_b_members_h");
            tstring k_x("party_b_members_x");
            tstring k_empty("");                    // built and destroyed, never read (original)
            float ev    = g_ini.get_float(k_button_h);
            float ratio = g_ini.get_float(k_width_ratio);
            float h     = g_ini.get_float(k_h);
            float x     = g_ini.get_float(k_x);
            buttons[side * 2 + page][i] =
                new_member_button(members_page[side][page], k_x, x, 0, h,
                                  tstring("party_member_button"),
                                  tstring("party_member_button_pressed"),
                                  ratio, ev, 0.085f);
            ((list_widget*)buttons[side * 2 + page][i])->f184 |= 3;
        }
    }

    // the member base is a child of the party body
    link->children.push_back(members_base[side]);

    if (side == 0) {
        gold = new panel;
        party_bar_setup(gold, tstring("party_t_gold"));
        float d[2];
        d[1] = gold->y - members_base[0]->y;
        d[0] = gold->x - members_base[0]->x;
        gold->set_pos(d);
        members_base[0]->add_child(gold);

        weekly_cost = new panel;
        party_bar_setup(weekly_cost, tstring("party_t_weekly_cost"));
        d[1] = weekly_cost->y - members_base[0]->y;
        d[0] = weekly_cost->x - members_base[0]->x;
        weekly_cost->set_pos(d);
        members_base[0]->add_child(weekly_cost);

        max_companions = new panel;
        party_bar_setup(max_companions, tstring("party_t_max_companions"));
        d[1] = max_companions->y - members_base[0]->y;
        d[0] = max_companions->x - members_base[0]->x;
        max_companions->set_pos(d);
        members_base[0]->add_child(max_companions);

        max_prisoners = new panel;
        party_bar_setup(max_prisoners, tstring("party_t_max_prisoners"));
        d[1] = max_prisoners->y - members_base[0]->y;
        d[0] = max_prisoners->x - members_base[0]->x;
        max_prisoners->set_pos(d);
        members_base[0]->add_child(max_prisoners);

        morale = new panel;
        party_bar_setup(morale, tstring("party_t_morale"));
        d[1] = morale->y - members_base[0]->y;
        d[0] = morale->x - members_base[0]->x;
        morale->set_pos(d);
        members_base[0]->add_child(morale);
    } else {
        choose_companions = new panel;
        party_bar_setup(choose_companions, tstring("party_t_choose_companions"));
        float d[2];
        d[1] = choose_companions->y - members_base[side]->y;
        d[0] = choose_companions->x - members_base[side]->x;
        choose_companions->set_pos(d);
        members_base[side]->add_child(choose_companions);

        choose_prisoners = new panel;
        party_bar_setup(choose_prisoners, tstring("party_t_choose_prisoners"));
        d[1] = choose_prisoners->y - members_base[side]->y;
        d[0] = choose_prisoners->x - members_base[side]->x;
        choose_prisoners->set_pos(d);
        members_base[side]->add_child(choose_prisoners);

        // the two captions depend on what kind of party is shown
        if (mode == 1 || mode == 6 || mode == 7) {
            int           idx = g_game->party.index_of(party_id);
            party_record* rec = (party_record*)((char*)g_game->party.pages[idx >> 4]
                                                + (idx & 0xf) * 0x5ac8);
            tstring* name;
            if (g_use_alt_name)              name = &rec->name;
            else if (rec->f1c != 0)          name = &rec->name2;
            else if (g_strtable.find(rec->name) != 0) name = &rec->name;
            else                             name = &rec->name2;
            choose_companions->set_text(caption_pos, name);
            tstring empty("");
            choose_prisoners->set_text(caption_pos, &empty);
        } else if (mode == 2 || mode == 3) {
            tstring empty("");
            choose_companions->set_text(caption_pos, &empty);
            tstring empty2("");
            choose_prisoners->set_text(caption_pos, &empty2);
        } else {
            tstring s1("ui_rescued_prisoners");
            choose_companions->set_text(caption_pos, g_strtable.lookup(s1));
            tstring s2("ui_captured_enemies");
            choose_prisoners->set_text(caption_pos, g_strtable.lookup(s2));
        }
    }
}
