// quest_page.cpp (was slice s004d5ba0): the quest-log page (the "quest" tab of the M&B character sheet) and the options-change
// handler shared by the settings tab it belongs to.
//   0x4d5ba0  fill the quest box: the quest name at 0.45 / 0.38, "given on <date>", "days since given",
//             the description, and (bit 0 of +0x50) the progression number in the second box.
//   0x4d5f20  options-change handler (takes the change reason, 4 bytes on the stack): poll every settings
//             control into the render-config globals (0x5aa0xx), re-captioning the difficulty page as it
//             goes; the toolbar buttons (+0x38/+0x3c/+0x40) pick a main-menu option and leave the page.
//   0x4d6270  the "days" caption: 25 + the fraction of g_difficulty_f * *(float*)0x5910fc.
//   0x4d6300  the difficulty-rating caption ("<difficulty>%", hidden in the main menu).
// All names provisional.
//
// Control layouts. The captions use the verified order (s004cf7b0): +0x30 get_value, +0x38 get_float,
// +0x44 changed(flag), +0x94 set_default, +0xa4 set_text(text), +0xac set_text(text, position), +0xb0 show,
// +0xb4 enable. 0x4d5f20 polls changed() (+0x44) and reads the control's value (+0x30) / float (+0x38)
// through the settings view below.
//
// Text values. The original builds the texts it hands to set_text from 16-byte globals (0xa70464 and the
// string-table entries), copying with 0x42bcb0 (tstring::operator=); localized::text() is that copy.
//
// State of the row: `fill` is byte-exact. The days line computes `days` from g_game directly and then
// names the same pointer in a local `g` (cl folds the two loads into one, which puts g_game in ebx and
// days in edi); "now" is the standalone __int64 at 0x5e2950, not g_ticks[1].
#include "../../include/image_addrs.h"
#include <windows.h>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"
#include "../window_mgr.h"      // g_windows slots 19 (0xa739cc), 20 (0xa739d0)
#include "../screen_records.h"       // Recd8: the options screen record (slot 3 is update_settings 0x4d5f20); Rec454 (the quest log, fill 0x4d5ba0)
#include "../party/game.h"
#include "../../include/game_recs.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

using rgl::vector4;

// 0x43a1e0: localized text of a table key; the returned object's `p` is the text (0 -> the object itself).
// 0x43a1e0 (thiscall on the table at 0x5e2a18, arg pushed, ret 4): localized text of a table key.
#include "../str_table.h"                                                 // StrTable g_strtable (0x5e2a18)

extern "C" int __cdecl sprintf(char*, const char*, ...);         // 0x536f8f
#include "../../include/crt.h"   // canonical: floor (0x536b10)


// 0x5e2950: the current time stamp (the second entry of the clock table, read as one __int64).
// (g_ticks[1], 0x5e2950: g_ticks starts at 0x5e2948; declared in fx.h)

// 0x489f40 (thiscall on the game object): the game's difficulty rating (percent). GameData (include/game_data.h) cannot be
// included next to party/game.h, so the member is repeated under the class name.

// 0x495700 (thiscall on 0xa73980, ret 4; see s00494e40): choose a main-menu option by index.
#include "../window_mgr.h"                         // g_windows (0xa73980)

// 0xa73980: the main menu.
// 0xa739d0 / 0xa739cc: menu entries whose +0x18 index the video/options buttons set.
// the option / video menu entries are slots of g_windows (window_mgr.h): 19 (0xa739cc) and 20 (0xa739d0), the index at +0x18

// 0x5e2950 and the per-day factors next to 0x596770.
#include "../../include/fx.h"   // canonical: g_ticks (0x5e2950)
#include "../../rgl/resource_manager.h"   // canonical: g_options.f74 (0x5aa07c)

// ---- control stub -----------------------------------------------------------------------------------
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)
#include "../config/widget_mgr.h"         // UiA / UiB / DropDown: the page's controls (all MissionUi)
#include "../../rgl/text_stream.h"

// The integer a polled control returns (MissionUi::get15c, +0x30).
static inline int control_value(MissionUi* w)
{
    return w->get15c();
}

// The float behind get_float (MissionUi::get168, +0x38; an x87 return: the caller stores st0, fstp dword [0x5aa010]).
static inline float control_float(MissionUi* w)
{
    return w->get168();
}



// The text handed to set_text: the key object built from the caption's literal, looked up in the string
// table, and the entry copied into the caller's slot (tstring::operator=).
struct localized {
    tstring key;
    tstring* e;

    explicit localized(const char* k) : key(k) { e = g_strtable.lookup(key); }
    tstring text() const
    {
        tstring t = *e;
        return t;
    }
    const char* c_str() const { return e->buf ? e->buf : (const char*)&e->buf; }
};


// 0xa70464: the caption the game object's text helper writes into before it is shown.

// 0x4d6dd0: head_rec::set_enabled_text (keys_page.cpp); the page calls it on itself.

// @ 0x4d5ba0
// The selected quest's record: the selected row indexes the quest numbers of the rows.
#define QREC (&g_game->quests[indices[f250]])

void Rec454::fill()
{
    char buf[0x200];

    {
        panel1->set_text(QREC->title);
        panel1->layout_text_to(QREC->text, V2(0.0f, 0.43125f));
    }

    // the date line: "ui_given_on_date" formatted with g_quest_text, which the game's text helper fills
    {
        tstring key("ui_given_on_date");
        int days = (int)(g_game->quests[indices[f250]].f11c);
        GameData* g = g_game;
        g_script.result_string.assign("");
        ((GameData*)g)->emit_event(0xf, days / 24, days);
        {
            tstring formatted(g_script.result_string);
            const char* arg = formatted.buf ? formatted.buf : (const char*)&formatted.buf;
            tstring* e = g_strtable.lookup(key);
            mb_snprintf(buf, 0x200, e->buf ? e->buf : (const char*)&e->buf, arg);
        }
    }
    {
        tstring text(buf);
        panel1->layout_text_to(text, V2(0.0f, 0.38125f));
    }

    // "N days since given"
    {
        tstring key("ui_days_since_given");
        tstring* e = g_strtable.lookup(key);
        const char* fmt = e->buf ? e->buf : (const char*)&e->buf;
        int given = (int)(QREC->f11c * (1.0f / 24));
        int now = (int)(g_ticks[1] * 4.1666667e-07f);
        mb_snprintf(buf, 0x200, fmt, now - given);
    }
    {
        tstring text(buf);
        panel1->layout_text_to(text, V2(0.0f, 0.35f));
    }

    // the description
    {
        V2 at(0.0f, 0.31875f);
        tstring desc(QREC->giver);
        panel1->layout_text_to(desc, at);
    }

    // the progression number (second box) when the quest has one
    if (QREC->f50_0) {
        {
            tstring key("ui_quest_progression_number");
            tstring* e = g_strtable.lookup(key);
            mb_snprintf(buf, 0x200, e->buf ? e->buf : (const char*)&e->buf, QREC->f64_lo);
        }
        {
            tstring text(buf);
            panel2->set_text(text);
        }
        panel2->show();   // slot +0xb4 (the slice called it enable)
    } else {
        panel2->hide();   // slot +0xb0 (the slice called it show)
    }
}

// @ 0x4d6270
void Recd8::update_days()
{
    float v = g_options.f74 * 75.0f;
    float f = (float)floor(v);
    int n = (int)f;
    int r;
    if (v - f <= 0.5f) r = n; else r = n + 1;     // jnp form: test ah,0x41 over fcomp [0x549d70]
    n = r;
    char buf[0x40];
    sprintf(buf, "%d", n + 0x19);
    tstring text(buf);
    h34->set_text(text);
}

// @ 0x4d6300
void Recd8::update_difficulty()
{
    if (g_game == 0) {
        hd4->hide();   // slot +0xb0 (the slice called it show)
        return;
    }
    char buf[0x80];
    {
        // the localized caption is the format; the rating is the argument
        tstring key("ui_difficulty_rating_percentage");
        tstring* e = g_strtable.lookup(key);
        const char* fmt = e->buf ? e->buf : (const char*)&e->buf;
        sprintf(buf, fmt, g_game->sl_value_factor());
    }
    {
        tstring text((const char*)&buf[0]);
        hd4->set_text(text);
    }
}

// @ 0x4d5f20
void Recd8::frame_update(float)
{
    if (s7c->clear_mask(0x10000))
        g_options.music_volume = control_float(s7c);
    if (s80->clear_mask(0x10000))
        g_options.master_volume = control_float(s80);
    if (s84->clear_mask(0x10000))
        g_options.exp_arg = control_float(s84);
    if (sl78->clear_mask(0x10000)) {
        g_options.f74 = control_float(sl78);
        update_difficulty();
        update_days();
    }
    if (dd44->clear_mask(0x10000)) {
        g_options.dmg_to_player = control_value(dd44);
        update_difficulty();
    }
    if (dd48->clear_mask(0x10000)) {
        g_options.dmg_to_friends = control_value(dd48);
        update_difficulty();
    }
    if (dd4c->clear_mask(0x10000)) {
        g_options.difficulty = control_value(dd4c);
        update_difficulty();
    }
    if (dd50->clear_mask(0x10000)) {
        g_options.agent_count = control_value(dd50);
        update_difficulty();
    }
    if (dd54->clear_mask(0x10000)) {
        g_options.stick_mode = control_value(dd54);
        update_difficulty();
    }
    if (dd58->clear_mask(0x10000)) {
        g_options.ctl_mode = control_value(dd58);
        update_difficulty();
    }
    if (b5c->clear_mask(0x10000)) {
        g_options.i64 = control_value(b5c);
        set_enabled_text(b5c);
    }
    if (b60->clear_mask(0x10000)) {
        g_options.i68 = control_value(b60);
        set_enabled_text(b60);
    }
    if (b64->clear_mask(0x10000)) {
        g_options.fx_on = control_value(b64);
        set_enabled_text(b64);
    }
    if (b68->clear_mask(0x10000)) {
        g_options.damage_msgs = control_value(b68);
        set_enabled_text(b68);
    }
    if (b6c->clear_mask(0x10000)) {
        g_options.shot_difficulty = control_value(b6c);
        set_enabled_text(b6c);
    }
    if (b70->clear_mask(0x10000)) {
        g_options.kill_msgs = control_value(b70);
        set_enabled_text(b70);
    }
    if (b74->clear_mask(0x10000)) {
        g_options.show_messages = control_value(b74);
        set_enabled_text(b74);
    }
    if (b88->clear_mask(0x10000)) {
        g_options.invert = control_value(b88);
        set_enabled_text(b88);
    }
    // the toolbar: leave the page (the last branch ends in the epilogue)
    if (b40->clear_mask(0x10000)) {
        g_windows.set_state(w18);
        return;
    }
    if (b38->clear_mask(0x10000)) {
        slot_as<head_rec>(20)->w18 = 0x12;
        g_windows.set_state(0x14);
        return;
    }
    if (b3c->clear_mask(0x10000)) {
        slot_as<head_rec>(19)->w18 = 0x12;
        g_windows.set_state(0x13);
    }
}
