#ifndef CONV_REFRESH_H
#define CONV_REFRESH_H
// Slice s004f73c0: the conversation (dialogue) screen body 0x4f73c0 -- it picks or advances the current
// dialog line, writes the speaker's name and the (tag-stripped) line text into the two label widgets, advances
// all 0x400 choice buttons, then, when the dialog has more than one line, lays out one button per line
// (position/size from the "conv_sentence_*" settings, mesh "dlg_button"/"dlg_button_down" for each) and finally
// repositions the choice box -- plus the agent appearance refresh 0x4f7e00 (calls the appearance builders
// 0x482240 / 0x47cfe0 on the body, caches the equipment value, re-wields slots 0 and 1) and its caller
// 0x4f7d70 (pick equipment, refresh the appearance, reset the wielded items).
// MissionUi slots, tstring (0x42bbe0 ctor, 0x42bc40 dtor, 0x42c110 copy), the setting table (0xa44240, entries of
// 0x2c from 0xa4425c), the dialog manager accessors (s0045ff30) and the agent layout (s0045ff30) follow the
// neighbouring slices.  Names are provisional.  Neither function is byte-exact yet: the notes in the comments
// record what the original does (stack frame, call order, value flow) so the sources can be finished.
// clang-format off
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <new>
#include <vector>
#include "../../rgl/rgl_math.h"

using rgl::vector4;

// ---- strings / setting table ------------------------------------------------------------------------
#include "../tstring.h"
#include "../ini_file.h"                               // ini_file, g_game_vars (0xa44240; its count is 0xa70240)


// lookup of one setting, 0.0f when the key is missing (the original inlines this seven times)
static float setting_float(const char* name)
{
    tstring key(name);
    int idx = -1;
    int i = 0;
    do {
        if (i >= g_game_vars.count)
            break;
        if (key.equals(g_game_vars.entries[i].value))
            idx = i;
        i++;
    } while (idx == -1);
    float r = 0.0f;
    if (idx >= 0) {
        ini_entry* e = &g_game_vars.entries[idx];
        r = (float)atof(e->value.c_str());
    }
    return r;
}

// ---- game data --------------------------------------------------------------------------------------
#include "../dialog_manager.h"

#include "../str_table.h"      // StrTable g_strtable (0x5e2a18): the one definition

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


#include "../game_globals.h"

// ---- widget slots (see s004dfd50 / s004e05c0) -------------------------------------------------------
// used slots: 0x58/0x64/0x68/0x6c/0x70 pos & scale, 0x74 flag, 0x80 track, 0xa4 text, 0xb0 advance, 0xb4 clear
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)

// the conversation screen (`this` of 0x4f73c0)
#include "../conv_screen.h"                      // ConvScreen (full layout)

#include "./cs_common.h"
#include "../res_mgr.h"                          // res_manager()
#include "../../rgl/mesh.h"                    // rgl::mesh::clone 0x41c6d0

// 0x487b00 (UTF-8 decoder) is a static helper with a register convention: conv_refresh.cpp has its local copy

// ---- conversation screen ----------------------------------------------------------------------------
#endif
