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
// +0xb4 enable. 0x4d5f20 polls changed() and reads the control's value/float through the settings view
// below (changed at +0x30), whose changed/get_value slots are the +0x10 shift its calls show.
//
// Text values. The original builds the texts it hands to set_text from 16-byte globals (0xa70464 and the
// string-table entries), copying with 0x42bcb0 (tstring::operator=); localized::text() is that copy.
//
// State of the row: every path and call of `fill` is reconstructed, but its frame (0x220 + ebx/esi/edi with
// the prologue emitted right after the game test and no epilogue on the early-out) could not be reproduced
// from any source shape tried here, so it is recorded in nonmatching.txt rather than manifest.txt.
#include <windows.h>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"
#include "./quest_page.h"
#include "../party/game.h"
#include "../../include/game_recs.h"

using rgl::vector4;

// 0x43a1e0: localized text of a table key; the returned object's `p` is the text (0 -> the object itself).
// 0x43a1e0 (thiscall on the table at 0x5e2a18, arg pushed, ret 4): localized text of a table key.
#include "../str_table.h"                                                 // StrTable g_strtable (0x5e2a18)

void mb_snprintf(char* buf, unsigned n, const char* fmt, ...);   // 0x415100
extern "C" int __cdecl sprintf(char*, const char*, ...);         // 0x536f8f
#include "../../include/crt.h"   // canonical: floor (0x536b10)

struct Pos2 { float x, y; };    // 8-byte position slot; the original passes an unset 8-byte stack slot

// 0x489f40 (thiscall on the game object): the game's difficulty rating (percent). GameData (include/game_data.h) cannot be
// included next to party/game.h, so the member is repeated under the class name.
struct GameData {
    int sl_value_factor();   // 0x489f40
};

// 0x495700 (thiscall on 0xa73980, ret 4; see s00494e40): choose a main-menu option by index.
#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition

// 0xa73980: the main menu.
// 0xa739d0 / 0xa739cc: menu entries whose +0x18 index the video/options buttons set.
extern int* g_menu_video_entry;                     // 0xa739d0 (index at +0x18)
extern int* g_menu_options_entry;                   // 0xa739cc (index at +0x18)

// 0x5e2950 and the per-day factors next to 0x596770.
#include "../../include/fx.h"   // canonical: g_ticks (0x5e2950)
extern float  g_tick_per_day;                       // 0x596770
extern float  g_ticks_per_time;                     // 0x596774
#include "../../rgl/resource_manager.h"   // canonical: g_options.f74 (0x5aa07c)

// ---- control stub -----------------------------------------------------------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// The settings-tab view of the same controls that 0x4d5f20's polls go through: changed() at +0x30, the
// integer value at +0x34 and the float at +0x38.
struct SettingsControl {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a(); virtual void v0b();
    virtual char changed(int flag);         // +0x30
    virtual int  value();                   // +0x34
    virtual int  float_value();             // +0x38
};

// The integer a polled control returns (an int slot of the settings view).
static inline int control_value(Widget* w)
{
    int v = ((SettingsControl*)w)->value();
    return (int)(long)(void*)v;
}

// The float behind get_float (the original returns it as an integer the caller stores as a float).
static inline float control_float(Widget* w)
{
    int v = ((SettingsControl*)w)->float_value();
    return *(float*)&v;
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

extern char g_empty_text[];                         // 0x54a480 (an empty C string)

// 0xa70464: the caption the game object's text helper writes into before it is shown.
extern tstring g_quest_text;                        // 0xa70464

// 0x435790 (thiscall on the game, ret 0xc): SoundCtl of agent/agent.h, the game object's event emitter (the header cannot be
// included here).
struct SoundCtl {
    char emit_event(int id, int a, int b);   // 0x435790
};
// 0x4d6dd0: KeysPage::set_enabled_text (keys_page.cpp); the page calls it on itself.
struct KeysPage { void set_enabled_text(Widget* w); };

// @ 0x4d5ba0
// The selected quest's record: the selection indexes the int table at this+0x4c.
#define QREC (&g_game->quests[((int*)((char*)this + 0x4c))[sel]])

void QuestPage::fill()
{
    char buf[0x200];

    {
        Pos2 at;                                 // passed unset to the first call
        box1()->set_text(&at, QREC->title);
        at.x = 0.0f;
        at.y = 0.43125f;
        box1()->set_text_at(QREC->text, &at.x);
    }

    // the date line: "ui_given_on_date" formatted with g_quest_text, which the game's text helper fills
    {
        tstring key("ui_given_on_date");
        Game* g = g_game;
        int days = (int)(g->quests[((int*)((char*)this + 0x4c))[sel]].f11c);
        g_quest_text.assign(g_empty_text);
        ((SoundCtl*)g)->emit_event(0xf, days / 24, days);
        {
            tstring formatted(g_quest_text);
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
        box1()->set_text_at(text, &at.x);
    }

    // "N days since given"
    {
        tstring key("ui_days_since_given");
        tstring* e = g_strtable.lookup(key);
        const char* fmt = e->buf ? e->buf : (const char*)&e->buf;
        int given = (int)(QREC->f11c * g_ticks_per_time);
        int now = (int)(g_ticks[1] * g_tick_per_day);
        mb_snprintf(buf, 0x200, fmt, now - given);
    }
    {
        tstring text(buf);
        Pos2 at;
        at.x = 0.0f;
        at.y = 0.35f;
        box1()->set_text_at(text, &at.x);
    }

    // the description
    {
        Pos2 at;
        at.x = 0.0f;
        at.y = 0.31875f;
        tstring desc(QREC->giver);
        box1()->set_text_at(desc, &at.x);
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
            box2()->set_text(&at, text);
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
    v = g_options.f74 * *(float*)0x5910fc;
    float f = (float)floor(v);
    int n = (int)f;
    int r;
    if (v - f <= 0.5f) r = n; else r = n + 1;     // jnp form: test ah,0x41 over fcomp [0x549d70]
    n = r;
    char buf[0x40];
    sprintf(buf, "%d", n + 0x19);
    tstring text(buf);
    w34->set_text(&fr, text);
}

// @ 0x4d6300
void QuestPage::update_difficulty()
{
    if (g_game == 0) {
        wd4->hide();   // slot +0xb0 (the slice called it show)
        return;
    }
    char buf[0x80];
    {
        // the localized caption is the format; the rating is the argument
        tstring key((const char*)0x5967a0);
        tstring* e = g_strtable.lookup(key);
        const char* fmt = e->buf ? e->buf : (const char*)&e->buf;
        sprintf(buf, fmt, ((GameData*)g_game)->sl_value_factor());
    }
    {
        tstring text((const char*)&buf[0]);
        Pos2 slot;
        wd4->set_text(&slot, text);
    }
}

// @ 0x4d5f20
void QuestPage::update_settings(int reason)
{
    if (w7c->changed(0x10000))
        *(float*)0x5aa010 = control_float(w7c);
    if (w80->changed(0x10000))
        *(float*)0x5aa014 = control_float(w80);
    if (w84->changed(0x10000))
        *(float*)0x5aa018 = control_float(w84);
    if (w78->changed(0x10000)) {
        g_options.f74 = control_float(w78);
        update_difficulty();
        update_days();
    }
    if (w44->changed(0x10000)) {
        *(int*)0x5aa054 = control_value(w44);
        update_difficulty();
    }
    if (w48->changed(0x10000)) {
        *(int*)0x5aa058 = control_value(w48);
        update_difficulty();
    }
    if (w4c->changed(0x10000)) {
        *(int*)0x5aa04c = control_value(w4c);
        update_difficulty();
    }
    if (w50->changed(0x10000)) {
        *(int*)0x5aa050 = control_value(w50);
        update_difficulty();
    }
    if (w54->changed(0x10000)) {
        *(int*)0x5aa078 = control_value(w54);
        update_difficulty();
    }
    if (w58->changed(0x10000)) {
        *(int*)0x5aa074 = control_value(w58);
        update_difficulty();
    }
    if (w5c->changed(0x10000)) {
        *(int*)0x5aa06c = control_value(w5c);
        ((KeysPage*)(void*)this)->set_enabled_text(w5c);
    }
    if (w60->changed(0x10000)) {
        *(int*)0x5aa070 = control_value(w60);
        ((KeysPage*)(void*)this)->set_enabled_text(w60);
    }
    if (w64->changed(0x10000)) {
        *(int*)0x5aa05c = control_value(w64);
        ((KeysPage*)(void*)this)->set_enabled_text(w64);
    }
    if (w68->changed(0x10000)) {
        *(int*)0x5aa060 = control_value(w68);
        ((KeysPage*)(void*)this)->set_enabled_text(w68);
    }
    if (w6c->changed(0x10000)) {
        *(int*)0x5aa064 = control_value(w6c);
        ((KeysPage*)(void*)this)->set_enabled_text(w6c);
    }
    if (w70->changed(0x10000)) {
        *(int*)0x5aa068 = control_value(w70);
        ((KeysPage*)(void*)this)->set_enabled_text(w70);
    }
    if (w74->changed(0x10000)) {
        *(int*)0x5aa080 = control_value(w74);
        ((KeysPage*)(void*)this)->set_enabled_text(w74);
    }
    if (w88->changed(0x10000)) {
        *(int*)0x5aa01c = control_value(w88);
        ((KeysPage*)(void*)this)->set_enabled_text(w88);
    }
    // the toolbar: leave the page (the last branch ends in the epilogue)
    if (w40->changed(0x10000)) {
        g_main_menu.set_state(menu_option);
        return;
    }
    if (w38->changed(0x10000)) {
        g_menu_video_entry[6] = 0x12;
        g_main_menu.set_state(0x14);
        return;
    }
    if (w3c->changed(0x10000)) {
        g_menu_options_entry[6] = 0x12;
        g_main_menu.set_state(0x13);
    }
}
