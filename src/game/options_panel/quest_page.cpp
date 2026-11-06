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
// State of the row: `fill` is byte-exact. The days line computes `days` from g_game_G directly and then
// names the same pointer in a local `g` (cl folds the two loads into one, which puts g_game in ebx and
// days in edi); "now" is the standalone __int64 at 0x5e2950, not g_ticks[1].
#include "../../include/image_addrs.h"
#include <windows.h>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"
#include "./quest_page.h"
#include "../window_mgr.h"      // g_windows slots 19 (0xa739cc), 20 (0xa739d0)
#include "../screen_records.h"       // Recd8: the options screen record (slot 3 is update_settings 0x4d5f20)
#include "../party/game.h"
#include "../../include/game_recs.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

using rgl::vector4;

// 0x43a1e0: localized text of a table key; the returned object's `p` is the text (0 -> the object itself).
// 0x43a1e0 (thiscall on the table at 0x5e2a18, arg pushed, ret 4): localized text of a table key.
#include "../str_table.h"                                                 // StrTable g_strtable (0x5e2a18)

extern "C" int __cdecl sprintf(char*, const char*, ...);         // 0x536f8f
#include "../../include/crt.h"   // canonical: floor (0x536b10)

struct Pos2 { float x, y; };    // 8-byte position slot; the original passes an unset 8-byte stack slot

// 0x5e2950: the current time stamp (the second entry of the clock table, read as one __int64).
// (g_ticks[0], declared in fx.h)

// 0x489f40 (thiscall on the game object): the game's difficulty rating (percent). GameData (include/game_data.h) cannot be
// included next to party/game.h, so the member is repeated under the class name.
struct GameData {
    int sl_value_factor();   // 0x489f40
    char emit_event(int id, int a, int b);   // 0x435790 (agent/agent.cpp, ret 0xc: the game object's event emitter)
};

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
#include "../../rgl/text_stream.h"

// The settings-tab view of the same controls that 0x4d5f20's polls go through (changed() is MissionUi's +0x44):
// the integer value at +0x30 (MissionUi::get_value) and the float at +0x38 (MissionUi::get_float).
struct SettingsControl {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a(); virtual void v0b();
    virtual int  value();                   // +0x30
    virtual void v0d();                     // +0x34 (MissionUi::set_value)
    virtual float float_value();            // +0x38 (returned in st0: 0x4d5f20 stores it with fstp)
};

// The integer a polled control returns (an int slot of the settings view).
static inline int control_value(MissionUi* w)
{
    int v = ((SettingsControl*)w)->value();
    return (int)(long)(void*)v;
}

// The float behind get_float (an x87 return: the caller stores st0, fstp dword [0x5aa010]).
static inline float control_float(MissionUi* w)
{
    return ((SettingsControl*)w)->float_value();
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

// 0x4d6dd0: KeysPage::set_enabled_text (keys_page.cpp); the page calls it on itself.
struct KeysPage { void set_enabled_text(MissionUi* w); };

// @ 0x4d5ba0
// The selected quest's record: the selection indexes the int table at this+0x4c.
#define QREC (&g_game_G->quests[((int*)((char*)this + 0x4c))[sel]])

void QuestPage::fill()
{
    char buf[0x200];

    {
        Pos2 at;                                 // passed unset to the first call
        box1()->set_text((float*)&at, &(QREC->title));
        at.x = 0.0f;
        at.y = 0.43125f;
        box1()->layout_text_to(&(QREC->text), &at.x);
    }

    // the date line: "ui_given_on_date" formatted with g_quest_text, which the game's text helper fills
    {
        tstring key("ui_given_on_date");
        int days = (int)(g_game_G->quests[((int*)((char*)this + 0x4c))[sel]].f11c);
        Game* g = g_game_G;
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
        Pos2 at;
        at.x = 0.0f;
        at.y = 0.38125f;
        box1()->layout_text_to(&(text), &at.x);
    }

    // "N days since given"
    {
        tstring key("ui_days_since_given");
        tstring* e = g_strtable.lookup(key);
        const char* fmt = e->buf ? e->buf : (const char*)&e->buf;
        int given = (int)(QREC->f11c * (1.0f / 24));
        int now = (int)(g_ticks[0] * 4.1666667e-07f);
        mb_snprintf(buf, 0x200, fmt, now - given);
    }
    {
        tstring text(buf);
        Pos2 at;
        at.x = 0.0f;
        at.y = 0.35f;
        box1()->layout_text_to(&(text), &at.x);
    }

    // the description
    {
        Pos2 at;
        at.x = 0.0f;
        at.y = 0.31875f;
        tstring desc(QREC->giver);
        box1()->layout_text_to(&(desc), &at.x);
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
            Pos2 at;
            box2()->set_text((float*)&at, &(text));
        }
        box2()->show();   // slot +0xb4 (the slice called it enable)
    } else {
        box2()->hide();   // slot +0xb0 (the slice called it show)
    }
}

// @ 0x4d6270
void QuestPage::update_days()
{
    Pos2 fr;                                 // x = the float spill slot; y is never written (it is passed on to the widget)
    float& v = fr.x;
    v = g_options.f74 * 75.0f;
    float f = (float)floor(v);
    int n = (int)f;
    int r;
    if (v - f <= 0.5f) r = n; else r = n + 1;     // jnp form: test ah,0x41 over fcomp [0x549d70]
    n = r;
    char buf[0x40];
    sprintf(buf, "%d", n + 0x19);
    tstring text(buf);
    w34->set_text((float*)&fr, &(text));
}

// @ 0x4d6300
void QuestPage::update_difficulty()
{
    if (g_game_G == 0) {
        wd4->hide();   // slot +0xb0 (the slice called it show)
        return;
    }
    char buf[0x80];
    {
        // the localized caption is the format; the rating is the argument
        tstring key("ui_difficulty_rating_percentage");
        tstring* e = g_strtable.lookup(key);
        const char* fmt = e->buf ? e->buf : (const char*)&e->buf;
        sprintf(buf, fmt, ((GameData*)g_game_G)->sl_value_factor());
    }
    {
        tstring text((const char*)&buf[0]);
        Pos2 slot;
        wd4->set_text((float*)&slot, &(text));
    }
}

#define W(x) ((MissionUi*)(void*)(x))
// @ 0x4d5f20
void Recd8::frame_update(float)
{
    if (W(s7c)->clear_mask(0x10000))
        g_options.music_volume = control_float(W(s7c));
    if (W(s80)->clear_mask(0x10000))
        g_options.master_volume = control_float(W(s80));
    if (W(s84)->clear_mask(0x10000))
        g_options.exp_arg = control_float(W(s84));
    if (W(sl78)->clear_mask(0x10000)) {
        g_options.f74 = control_float(W(sl78));
        update_difficulty();
        ((QuestPage*)(void*)this)->update_days();
    }
    if (W(dd44)->clear_mask(0x10000)) {
        g_options.dmg_to_player = control_value(W(dd44));
        update_difficulty();
    }
    if (W(dd48)->clear_mask(0x10000)) {
        g_options.dmg_to_friends = control_value(W(dd48));
        update_difficulty();
    }
    if (W(dd4c)->clear_mask(0x10000)) {
        g_options.difficulty = control_value(W(dd4c));
        update_difficulty();
    }
    if (W(dd50)->clear_mask(0x10000)) {
        g_options.agent_count = control_value(W(dd50));
        update_difficulty();
    }
    if (W(dd54)->clear_mask(0x10000)) {
        g_options.stick_mode = control_value(W(dd54));
        update_difficulty();
    }
    if (W(dd58)->clear_mask(0x10000)) {
        g_options.ctl_mode = control_value(W(dd58));
        update_difficulty();
    }
    if (W(b5c)->clear_mask(0x10000)) {
        g_options.i64 = control_value(W(b5c));
        ((KeysPage*)(void*)this)->set_enabled_text(W(b5c));
    }
    if (W(b60)->clear_mask(0x10000)) {
        g_options.i68 = control_value(W(b60));
        ((KeysPage*)(void*)this)->set_enabled_text(W(b60));
    }
    if (W(b64)->clear_mask(0x10000)) {
        g_options.fx_on = control_value(W(b64));
        ((KeysPage*)(void*)this)->set_enabled_text(W(b64));
    }
    if (W(b68)->clear_mask(0x10000)) {
        g_options.damage_msgs = control_value(W(b68));
        ((KeysPage*)(void*)this)->set_enabled_text(W(b68));
    }
    if (W(b6c)->clear_mask(0x10000)) {
        g_options.shot_difficulty = control_value(W(b6c));
        ((KeysPage*)(void*)this)->set_enabled_text(W(b6c));
    }
    if (W(b70)->clear_mask(0x10000)) {
        g_options.kill_msgs = control_value(W(b70));
        ((KeysPage*)(void*)this)->set_enabled_text(W(b70));
    }
    if (W(b74)->clear_mask(0x10000)) {
        g_options.show_messages = control_value(W(b74));
        ((KeysPage*)(void*)this)->set_enabled_text(W(b74));
    }
    if (W(b88)->clear_mask(0x10000)) {
        g_options.invert = control_value(W(b88));
        ((KeysPage*)(void*)this)->set_enabled_text(W(b88));
    }
    // the toolbar: leave the page (the last branch ends in the epilogue)
    if (W(b40)->clear_mask(0x10000)) {
        g_windows.set_state(w18);
        return;
    }
    if (W(b38)->clear_mask(0x10000)) {
        slot_as<int>(20)[6] = 0x12;
        g_windows.set_state(0x14);
        return;
    }
    if (W(b3c)->clear_mask(0x10000)) {
        slot_as<int>(19)[6] = 0x12;
        g_windows.set_state(0x13);
    }
}
#undef W
