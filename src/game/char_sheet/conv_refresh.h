#ifndef CONV_REFRESH_H
#define CONV_REFRESH_H
// Slice s004f73c0: the conversation (dialogue) screen body 0x4f73c0 -- it picks or advances the current
// dialog line, writes the speaker's name and the (tag-stripped) line text into the two label widgets, advances
// all 0x400 choice buttons, then, when the dialog has more than one line, lays out one button per line
// (position/size from the "conv_sentence_*" settings, mesh "dlg_button"/"dlg_button_down" for each) and finally
// repositions the choice box -- plus the agent appearance refresh 0x4f7e00 (calls the appearance builders
// 0x482240 / 0x47cfe0 on the body, caches the equipment value, re-wields slots 0 and 1) and its caller
// 0x4f7d70 (pick equipment, refresh the appearance, reset the wielded items).
// Widget slots, tstring (0x42bbe0 ctor, 0x42bc40 dtor, 0x42c110 copy), the setting table (0xa44240, entries of
// 0x2c from 0xa4425c), the dialog manager accessors (s0045ff30) and the agent layout (s0045ff30) follow the
// neighbouring slices.  Names are provisional.  Neither function is byte-exact yet: the notes in the comments
// record what the original does (stack frame, call order, value flow) so the sources can be finished.
// clang-format off
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- strings / setting table ------------------------------------------------------------------------
#include "../tstring.h"
struct setting { int f0; char* value; char pad[0x2c - 8]; };   // string pointer +4 (null: field is the text)
extern int g_n_settings;                                 // 0xa70240
extern setting g_settings;                               // 0xa44240 (entry i at 0xa4425c + 0x2c*i)
extern tstring g_empty_string;                           // 0x8ee30f0 (?g_empty_string@@3Utstring@@A)

// lookup of one setting, 0.0f when the key is missing (the original inlines this seven times)
static float setting_float(const char* name)
{
    tstring key(name);
    int idx = -1;
    int i = 0;
    do {
        if (i >= g_n_settings)
            break;
        if (key.equals(((tstring*)((char*)&g_settings + 0x1c))[i]))
            idx = i;
        i++;
    } while (idx == -1);
    float r = 0.0f;
    if (idx >= 0) {
        setting* e = (setting*)((char*)&g_settings + 0x1c) + idx;
        r = (float)atof(e->value ? e->value : (char*)&e->value);
    }
    return r;
}

// ---- game data --------------------------------------------------------------------------------------
struct TroopRec {
    tstring name;                                        // +0
    char pad10[0x10];
    tstring id;                                          // +0x20
    char pad30[0x508 - 0x30];
};
struct GameData {
    char pad0[0x13a00];
    TroopRec* chars;                                     // +0x13a00
};
#include "../str_table.h"
extern GameData* g_game;                                 // 0x5e2930
extern StrTable g_strtable;                              // 0x5e2a18

// the troop's translated id, falling back to its name (the original's inlined 0x4c6010 use)
static const char* troop_text(int id)
{
    TroopRec* t = &g_game->chars[id];
    tstring* r = g_strtable.find(&t->id);
    if (!r)
        r = &t->name;
    char* v = r->buf;
    return v ? v : (const char*)&r->buf;
}

// ---- dialog manager (layout verified in s0045ff30) --------------------------------------------------
struct DialogRec { char pad0[0x20]; unsigned flags; char pad24[0x40]; };
struct rawline {                                         // one dialog line, 0x1c bytes
    int dialog;
    int idx;
    unsigned flags;                                      // +8
    tstring text;                                        // +0xc
};
struct DialogMgr {
    int count;                                           // +0
    DialogRec* recs;                                     // +4
    int f8;                                              // +8
    int fc;                                              // +0xc
    char pad10[8];
    int cur;                                             // +0x18
    char pad1c[8];
    rawline list[0x400];                                 // +0x24
    void select(rawline* l);                             // 0x460d80
    int get_c8();                                        // 0x460620
    int speaker();                                       // 0x460640
    void advance();                                      // 0x460ff0
    void advance_all(int* n);                            // 0x461140
    tstring line_text(int i);                            // 0x4606a0 (ret 8)
    int line_flags(int i);                               // 0x4606c0
};
extern DialogMgr g_n_dialogs;                            // 0x8ed50c8 (?g_n_dialogs@@3UDialogMgr@@A)
extern rawline* g_dlg_lines;                             // 0x8ed50ec (?g_dlg_lines@@3PAUrawline@@A)

// ---- widget slots (see s004dfd50 / s004e05c0) -------------------------------------------------------
// used slots: 0x58/0x64/0x68/0x6c/0x70 pos & scale, 0x74 flag, 0x80 track, 0xa4 text, 0xb0 advance, 0xb4 clear
struct Widget {
    virtual void s00(void* p); virtual void s01(void* p); virtual void s02(void* p);
    virtual void s03(void* p); virtual void s04(void* p); virtual void s05(void* p);
    virtual void s06(void* p); virtual void s07(void* p); virtual void s08(void* p);
    virtual void s09(void* p); virtual void s10(void* p); virtual void s11(void* p);
    virtual void s12(void* p); virtual void s13(void* p); virtual void s14(void* p);
    virtual void s15(void* p); virtual void s16(void* p); virtual void s17(void* p);
    virtual void s18(void* p); virtual void s19(void* p); virtual void s20(void* p);
    virtual void s21(void* p);
    virtual void set_pos(const float* p);                // +0x58
    virtual void s23(void* p); virtual void s24(void* p);
    virtual void set_scale(const float* p);              // +0x64
    virtual void s26(void* p);
    virtual void set_size(const float* p);               // +0x6c
    virtual void set_pos2(const float* p);               // +0x70
    virtual void set_flag(const int* f);                 // +0x74
    virtual void s31(void* p); virtual void s32(void* p); virtual void s33(void* p);
    virtual void s34(void* p); virtual void s35(void* p); virtual void s36(void* p);
    virtual void s37(void* p); virtual void s38(void* p);
    virtual void set_track(int i, void* m);              // +0x80
    virtual void s40(void* p); virtual void s41(void* p); virtual void s42(void* p);
    virtual void s43(void* p); virtual void s44(void* p); virtual void s45(void* p);
    virtual void s46(void* p); virtual void s47(void* p); virtual void s48(void* p);
    virtual void s49(void* p);
    virtual void set_text(const tstring* in, tstring* out);   // +0xa4
    virtual void s51(void* p); virtual void s52(void* p); virtual void s53(void* p);
    virtual void s54(void* p); virtual void s55(void* p); virtual void s56(void* p);
    virtual void s57(void* p);
    virtual void advance();                              // +0xb0
    virtual void clear();                                // +0xb4
};

// the conversation screen (`this` of 0x4f73c0)
struct ConvScreen {
    char pad0[0x174];
    Widget* label_speaker;                               // +0x174
    Widget* label_text;                                  // +0x178
    Widget* body;                                        // +0x17c
    char pad180[8];
    Widget* choice;                                      // +0x188
    char pad18c[4];
    Widget* choice_buttons[0x400];                       // +0x190
    int n_lines;                                         // +0x1190
    char pad1194[0x1204 - 0x1194];
    int speaker_id;                                      // +0x1204
    void refresh(int sel, int* out);                     // 0x4f73c0
};

struct base_mesh;
struct resmgr { base_mesh* get_object(const tstring& name); };   // 0x419540
extern resmgr g_alloc;                                   // 0x5d6fa0
struct base_mesh {
    base_mesh* clone();                                  // 0x41c6d0
    void set_color(const float* c);                      // 0x47af30 (four floats)
};

float decode_utf8(const tstring* s, int* out_count, const void* font);   // 0x487b00 (pixel width)

// ---- conversation screen ----------------------------------------------------------------------------
#endif
