// Slice s004d5ba0: the quest-log page (the "quest" tab of the M&B character sheet) and the options-change
// handler shared by the settings tab it belongs to.
//   0x4d5ba0  fill the quest box: the quest name at 0.45 / 0.38, "given on <date>", "days since given",
//             the description, and (bit 0 of +0x50) the progression number in the second box.
//   0x4d5f20  options-change handler (takes the change reason, 4 bytes on the stack): poll every settings
//             control into the render-config globals (0x5aa0xx), re-captioning the difficulty page as it
//             goes; the toolbar buttons (+0x38/+0x3c/+0x40) pick a main-menu option and leave the page.
//   0x4d6270  the "days" caption: 25 + the fraction of *(float*)0x5aa07c * g_caption_scale.
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
#include "rgl.h"

using rgl::vector4;

// rgl::string (tstring): 16 bytes; layout verified in match/include/rgl.h (ctor 0x42bbe0).
struct tstring {
    char* p; int size, cap, len;     // 0x0 / 0x4 / 0x8 / 0xc
    tstring(const char* s);          // 0x42bbe0
    tstring(int n);                  // 0x42bbe0 (the integer format is the string's "p" slot)
    tstring(const tstring& o);       // 0x42c110
    tstring& operator=(const tstring& o);   // 0x42bcb0
    ~tstring();                      // 0x42bc40
};

// 0x43a1e0: localized text of a table key; the returned object's `p` is the text (0 -> the object itself).
// 0x43a1e0 (thiscall on the table at 0x5e2a18, arg pushed, ret 4): localized text of a table key.
struct StringTable {
    tstring* lookup(tstring& key);                  // 0x43a1e0
};
extern StringTable g_string_table;                  // 0x5e2a18

tstring* strtab_lookup(tstring& key);               // 0x43a1e0

void mb_snprintf(char* buf, unsigned n, const char* fmt, ...);   // 0x415100
extern "C" int __cdecl sprintf(char*, const char*, ...);         // 0x536f8f
int ftol(double v);                              // 0x535a64 (x87 argument, int in eax)
double floor_d(double v);                        // 0x536b10

// 0x5e2930: the running game object (module state); 0 == no game (main menu).
extern int g_game;                                  // 0x5e2930
struct Game {
    char    pad0[0x13a58];
    char*   quests;                                 // +0x13a58 (array of 0x138-byte quest records)
};

// 0x489f40 (thiscall on the game object): the game's difficulty rating (percent).
struct GameRating { int rating(); };                // 0x489f40

// 0x495700 (thiscall on 0xa73980, ret 4; see s00494e40): choose a main-menu option by index.
struct MainMenu { int pad[0x4c]; void choose(int option); };     // choose: 0x495700 (thiscall, ret 4)

// 0xa73980: the main menu.
extern MainMenu g_main_menu;                        // 0xa73980
// 0xa739d0 / 0xa739cc: menu entries whose +0x18 index the video/options buttons set.
extern int* g_menu_video_entry;                     // 0xa739d0 (index at +0x18)
extern int* g_menu_options_entry;                   // 0xa739cc (index at +0x18)

// 0x5e2950 and the per-day factors next to 0x596770.
extern double g_tick_f;                             // 0x5e2950 ((low, high) at 0x5e2948)
extern float  g_tick_per_day;                       // 0x596770
extern float  g_ticks_per_time;                     // 0x596774
extern float  g_caption_scale;                      // 0x5910fc

// quest record (0x138 bytes; see s0049b630 Rec138)
struct QuestRec {
    char    pad0[0x20];
    tstring text;                                   // +0x20  description
    char    pad30[0x40 - 0x30];
    tstring title;                                  // +0x40  name
    int     f50;                                    // +0x50  (bit 0 -> progression number shown)
    char    pad54[0x64 - 0x54];
    int     f64;                                    // +0x64  (low byte = progression)
    char    pad68[0x118 - 0x68];
    int     f118;                                   // +0x118 the date the quest was given
    float   f11c;                                   // +0x11c game time when given
};

// ---- control stub -----------------------------------------------------------------------------------
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a(); virtual void v0b();
    virtual int  get_value();               // +0x30
    virtual void v0d(); virtual void v0e();
    virtual int  float_value();             // +0x38 get_float
    virtual void v10();
    virtual char changed(int flag);         // +0x44
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v1a(); virtual void v1b(); virtual void v1c(); virtual void v1d();
    virtual void v1e(); virtual void v1f(); virtual void v20(); virtual void v21();
    virtual void v22(); virtual void v23();
    virtual void set_default();             // +0x90
    virtual void v25(); virtual void v26(); virtual void v27(); virtual void v28();
    virtual void set_text(const tstring& s);                        // +0xa4
    virtual void v2a();
    virtual void set_text(const tstring& s, const vector4& pos);    // +0xac
    virtual void show();                    // +0xb0
    virtual void enable();                  // +0xb4
};

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

// 0x4d6dd0: caption the control's number as enabled/disabled (s004d6b80).
void set_enabled_text(Widget* w);                   // 0x4d6dd0

struct QuestPage {
    char    pad00[0x18];
    int     menu_option;                            // +0x18 (the option the "back" button picks)
    char    pad1c[0x34 - 0x1c];
    Widget* w34;                                    // +0x34  day caption
    Widget* w38;                                    // +0x38  toolbar: video
    Widget* w3c;                                    // +0x3c  toolbar: options
    Widget* w40;                                    // +0x40  toolbar: back
    Widget* w44;                                    // +0x44
    Widget* w48;                                    // +0x48
    Widget* w4c;                                    // +0x4c
    Widget* w50;                                    // +0x50
    Widget* w54;                                    // +0x54
    Widget* w58;                                    // +0x58
    Widget* w5c;                                    // +0x5c
    Widget* w60;                                    // +0x60
    Widget* w64;                                    // +0x64
    Widget* w68;                                    // +0x68
    Widget* w6c;                                    // +0x6c
    Widget* w70;                                    // +0x70
    Widget* w74;                                    // +0x74
    Widget* w78;                                    // +0x78  a slider (its float is the day scale)
    Widget* w7c;                                    // +0x7c
    Widget* w80;                                    // +0x80
    Widget* w84;                                    // +0x84
    Widget* w88;                                    // +0x88
    char    pad8c[0xd4 - 0x8c];
    Widget* wd4;                                    // +0xd4  difficulty caption
    char    pad_d8[0x250 - 0xd8];
    int     sel;                                    // +0x250 selected quest
    int     idx[0x3c];                              // +0x254 (0x4d5ba0 reads the table at +0x4c)

    // the two text boxes of fill() (+0x44 name/date/description, +0x48 progression)
    Widget* box1() { return w44; }
    Widget* box2() { return w48; }

    void fill();
    void update_days();
    void update_difficulty();
    void update_settings(int reason);
};

// The text handed to set_text: the key object built from the caption's literal, looked up in the string
// table, and the entry copied into the caller's slot (tstring::operator=).
struct localized {
    tstring key;
    tstring* e;

    explicit localized(const char* k) : key(k) { e = g_string_table.lookup(key); }
    ~localized() { key.~tstring(); }
    tstring text() const
    {
        tstring t = *e;
        return t;
    }
    const char* c_str() const { return e->p ? e->p : (const char*)&e->p; }
};

// 0xa70464: the caption the game object's text helper writes into before it is shown.
extern tstring g_quest_text;                        // 0xa70464

// 0x435790 (thiscall on the game, ret 0xc).
struct QuestTextHook {
    char    pad0[0x1a068];
    int     on;                                     // +0x1a068
    void    format(int event, int a, int b);        // 0x435790
};

// @ 0x4d5ba0
void QuestPage::fill()
{
    // the index arithmetic is hoisted to the top in the original (it computes the record before anything else)
    int    k = idx[sel];
    char*  recs = ((Game*)g_game)->quests;
    QuestRec* rec = (QuestRec*)(recs + k * 0x138);
    tstring* title = &rec->title;
    vector4 at;

    // the name of the selected quest, twice: at the default height, then just under it
    {
        box1()->set_text(*title, at);
        at.x = 0.0f;
        at.y = 0.40f;
        box1()->set_text(*title, at);
    }

    char buf[0x200];

    // the day number: g_quest_text gets the date as text, then "(<days> fifteen)"-style formatting
    tstring date(rec->f118);
    g_quest_text = date;
    {
        int days = ftol(((QuestRec*)(((Game*)g_game)->quests + idx[sel] * 0x138))->f11c);
        ((QuestTextHook*)g_game)->format(0xf, days / 0x19, days);
    }
    {
        tstring formatted(g_quest_text);
        localized key("ui_given_on_date");
        mb_snprintf(buf, 0x200, key.c_str(), formatted.p ? formatted.p : (const char*)&formatted.p);
        {
            tstring text(buf);
            vector4 at;
            at.x = 0.0f;
            at.y = 0.40f;
            box1()->set_text(text, at);
        }
        formatted.~tstring();
    }
    date.~tstring();

    // "N days since given"
    {
        localized key("ui_days_since_given");
        int now = ftol(g_tick_f * g_tick_per_day);
        int given = ftol(((QuestRec*)(((Game*)g_game)->quests + idx[sel] * 0x138))->f11c * g_ticks_per_time);
        mb_snprintf(buf, 0x200, key.c_str(), now - given);
        tstring text(buf);
        vector4 at;
        at.x = 0.0f;
        at.y = 0.35f;
        box1()->set_text(text, at);
    }

    // the description
    {
        tstring desc(((QuestRec*)(((Game*)g_game)->quests + idx[sel] * 0x138))->text);
        vector4 at;
        at.x = 0.0f;
        at.y = 0.32f;
        box1()->set_text(desc, at);
    }

    // the progression number (second box) when the quest has one
    {
        QuestRec* q = (QuestRec*)(((Game*)g_game)->quests + idx[sel] * 0x138);
        if (q->f50 & 1) {
            localized key("ui_quest_progression_number");
            mb_snprintf(buf, 0x200, key.c_str(), q->f64 & 0xff);
            {
                tstring text(buf);
                box2()->set_text(text);
            }
            box2()->enable();
        } else {
            box2()->show();
        }
    }
}

// @ 0x4d6270
void QuestPage::update_days()
{
    float v = *(float*)0x5aa07c * g_caption_scale;
    double f = floor_d(v);
    int n = ftol(v);
    if (v - f >= 0.5f)          // (the original compares the fraction with 0.5f at 0x549d70)
        n++;
    char buf[0x40];
    sprintf(buf, "%d", n + 0x19);
    tstring text(buf);
    tstring caption((const char*)0x54a0f4);
    w34->set_text(text, *(const vector4*)&caption);            // the second argument is the 16-byte slot itself
}

// @ 0x4d6300
void QuestPage::update_difficulty()
{
    if (g_game == 0) {
        wd4->show();
        return;
    }
    char buf[0x80];
    {
        // the localized caption is the format; the rating is the argument
        tstring key((const char*)0x5967a0);
        tstring* e = g_string_table.lookup(key);
        const char* fmt = e->p ? e->p : (const char*)&e->p;
        sprintf(buf, fmt, ((GameRating*)g_game)->rating());
        key.~tstring();
    }
    {
        tstring text((const char*)&buf[0]);
        wd4->set_text(text, *(const vector4*)(const char*)0x54a480);
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
        *(float*)0x5aa07c = control_float(w78);
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
        set_enabled_text(w5c);
    }
    if (w60->changed(0x10000)) {
        *(int*)0x5aa070 = control_value(w60);
        set_enabled_text(w60);
    }
    if (w64->changed(0x10000)) {
        *(int*)0x5aa05c = control_value(w64);
        set_enabled_text(w64);
    }
    if (w68->changed(0x10000)) {
        *(int*)0x5aa060 = control_value(w68);
        set_enabled_text(w68);
    }
    if (w6c->changed(0x10000)) {
        *(int*)0x5aa064 = control_value(w6c);
        set_enabled_text(w6c);
    }
    if (w70->changed(0x10000)) {
        *(int*)0x5aa068 = control_value(w70);
        set_enabled_text(w70);
    }
    if (w74->changed(0x10000)) {
        *(int*)0x5aa080 = control_value(w74);
        set_enabled_text(w74);
    }
    if (w88->changed(0x10000)) {
        *(int*)0x5aa01c = control_value(w88);
        set_enabled_text(w88);
    }
    // the toolbar: leave the page (the last branch ends in the epilogue)
    if (w40->changed(0x10000)) {
        g_main_menu.choose(menu_option);
        return;
    }
    if (w38->changed(0x10000)) {
        g_menu_video_entry[6] = 0x12;
        g_main_menu.choose(0x14);
        return;
    }
    if (w3c->changed(0x10000)) {
        g_menu_options_entry[6] = 0x12;
        g_main_menu.choose(0x13);
    }
}
