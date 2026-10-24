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
#include "../dialog_manager.h"

extern StrTable g_strtable;                              // 0x5e2a18

// the troop's translated id, falling back to its name (the original's inlined 0x4c6010 use)
static const char* troop_text(int id)
{
    TroopRec* t = &g_game->chars[id];
    tstring* r = g_strtable.find(&t->key);
    if (!r)
        r = &t->name;
    char* v = r->buf;
    return v ? v : (const char*)&r->buf;
}

// ---- dialog manager (layout verified in s0045ff30) --------------------------------------------------


extern DialogMgr g_n_dialogs;                            // 0x8ed50c8 (?g_n_dialogs@@3UDialogMgr@@A)

// ---- widget slots (see s004dfd50 / s004e05c0) -------------------------------------------------------
// used slots: 0x58/0x64/0x68/0x6c/0x70 pos & scale, 0x74 flag, 0x80 track, 0xa4 text, 0xb0 advance, 0xb4 clear
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// the conversation screen (`this` of 0x4f73c0)
#include "../conv_screen.h"                      // ConvScreen (full layout), base_mesh

struct base_mesh;
struct resmgr { base_mesh* get_object(const tstring& name); };   // 0x419540
extern resmgr g_alloc;                                   // 0x5d6fa0

float decode_utf8(const tstring* s, int* out_count, const void* font);   // 0x487b00 (pixel width)

// ---- conversation screen ----------------------------------------------------------------------------
#endif
