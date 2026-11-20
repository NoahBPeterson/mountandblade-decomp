// Slice s005093d0: character-sheet / party-window helpers.
//   0x5093d0  ini_file::get_int        - find the key, atoi() its value
//   0x509410  C4c3050::init - clear the strings and install the eight arguments (the message window C4c3050)
//   0x5094a0  CharSheet::load_troop    - copy the troop-record display fields
//   0x509730  CharSheet::fill_skills   - gather the visible skill ids
//   0x509770  make_button              - build the numbered "buttonN_*" image button
// Names and layouts are provisional; every offset comes from this slice's disassembly and the
// verified neighbours (ini_file: s004bd170, widget slots: s00502300 / s004dfd50).
#include <windows.h>
#include "../../rgl/rgl_math.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---------------------------------------------------------------------------------------------------------
// tstring (16 bytes): read cursor / heap buffer / capacity / length.
#include "../tstring.h"

// ---------------------------------------------------------------------------------------------------------
// ini file: a fixed 0x1000-entry table, each entry 0x2c bytes (key, 12 unknown, value): ../ini_file.h
#include "../ini_file.h"
#include "../screen_records.h"          // C4c3050: init 0x509410 on the message window (slot 1)

extern "C" int atoi(const char*);                       // 0x53734d (atoi: jmp atol)

// @ 0x5093d0
int ini_file::get_int(const tstring& key)
{
    int ret = 0;
    int idx = find(key);
    if (idx >= 0)
        ret = atoi(entries[idx].value.c_str());
    return ret;
}

// ---------------------------------------------------------------------------------------------------------
#include "../../include/game_data.h"

#include "../char_sheet_panel.h"            // CharSheet: load_troop 0x5094a0 / fill_skills 0x509730 act on the character sheet

// @ 0x509410
void C4c3050::init(const tstring& msg, int n, const char* b0, const char* b1,
                              const char* b2, const char* b3, const char* b4, const char* b5)
{
    t5c.clear();
    name = msg;
    count = n;
    buttons[0].assign(b0);
    buttons[1].assign(b1);
    buttons[2].assign(b2);
    buttons[3].assign(b3);
    buttons[4].assign(b4);
    buttons[5].assign(b5);
    sel = -1;
}

// @ 0x5094a0
void CharSheet::load_troop()
{
    int a = w14;
    f_508 = g_game->chars[a].skill_points;
    f_50c = g_game->chars[a].attr_points;
    f_510 = g_game->chars[a].prof_points;
    f_504 = g_game->chars[a].fa0;
    int& t = w14;
    f_4d8 = g_game->chars[w14].attributes[0];
    f_4dc = g_game->chars[t].attributes[1];
    f_4e0 = g_game->chars[t].attributes[2];
    f_4e4 = g_game->chars[t].attributes[3];
    f_514 = g_game->chars[t].nib[0];
    f_518 = g_game->chars[t].nib[1];
    f_51c = g_game->chars[t].nib[2];
    f_520 = g_game->chars[t].nib[3];
    f_524 = g_game->chars[t].nib[4];
    f_528 = g_game->chars[t].nib[5];
    f_4e8 = g_game->chars[t].istat[0];
    f_4ec = g_game->chars[t].istat[1];
    f_4f0 = g_game->chars[t].istat[2];
    f_4f4 = g_game->chars[t].istat[3];
    f_4f8 = g_game->chars[t].istat[4];
    f_4fc = g_game->chars[t].istat[5];
    f_500 = g_game->chars[t].istat[6];
}

// ---------------------------------------------------------------------------------------------------------
// the skill attribute table at 0x8ee3140, 0x44-byte records.
#include "../skill_cap.h"

// @ 0x509730
void CharSheet::fill_skills()
{
    int n = 0;
    int i = 0;
    unsigned* se = &g_skill_tbl[0].attr;           // the attribute word of record 0 (g_skill_tbl[0].attr)
    int* out = skill_id;
    do {
        unsigned char c = (unsigned char)(*se >> 8);
        c = ~c;
        if (c & 1) {
            *out = i;
            n++;
            out++;
        }
        se = (unsigned*)((char*)se + sizeof(SkillRec));
        i++;
    } while ((intptr_t)se < (intptr_t)&g_skill_tbl[42].attr);
    num_skills = n;
}

// ---------------------------------------------------------------------------------------------------------
// widgets (slot numbers from the call sites; see s00502300 / s004dfd50).
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)

#include "../ui/ui_widgets.h"                  // UiA: the 0x188-byte image button (base ctor 0x48ae20, vptr 0x591d58)
#include "../ui/container.h"                    // ContainerBase

// engine mesh + resource manager

using rgl::mesh;
#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules

#include <new>
extern float g_590d94;                                  // 0x590d94 (0.6f)

// @ 0x509770
UiA* make_button(ContainerBase* parent, const tstring* text, float x, float y, int n, float s)
{
    UiA* b = new UiA;

    {
        V2 v(x, y);
        b->move_a(v);

        v.x = n * s;
        v.y = s;
        b->set_xy(v);
    }

    tstring up("button");
    up.push_back((char)('0' + n));
    {
        tstring sfx("_up");
        up.append(sfx);
    }

    tstring down("button");
    down.push_back((char)('0' + n));
    {
        tstring sfx("_down");
        down.append(sfx);
    }

    tstring hl("button");
    hl.push_back((char)('0' + n));
    {
        tstring sfx("_hl");
        hl.append(sfx);
    }

    b->set_mesh(2, g_resmgr.get_object(&up)->clone());
    b->set_mesh(1, g_resmgr.get_object(&down)->clone());
    b->set_mesh(3, g_resmgr.get_object(&hl)->clone());

    {
        float sz = s * 0.6f;
        b->set_size(V2(sz, sz));

        b->set_text(*text);
    }

    parent->add_child(b);
    return b;
}
