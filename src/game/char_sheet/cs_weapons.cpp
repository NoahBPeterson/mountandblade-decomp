#include "./cs_weapons.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f04f0
void weapon_class_caption(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_one_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_two_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_polearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 3) {
        tstring k("ui_archery");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 4) {
        tstring k("ui_crossbows");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 5) {
        tstring k("ui_throwing");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 6) {
        tstring k("ui_firearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}

// ---- widgets: only what these two functions call ---------------------------------------------------------
// font text object: set_text(out8, text) is vtable slot 41 (+0xa4; out8 = address of 8 uninitialised stack bytes in
// the caller), show / show_selected are slots 44 / 45 (+0xb0 / +0xb4)
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// ---- the character-sheet panel that posts the weapon rows ------------------------------------------------
#include "../char_sheet_panel.h"             // CharSheet (the character sheet panel)

// @ 0x4f06f0
void CharSheet::update_weapons()
{
    Troop* c = &g_game->chars[player];
    int i = 0;
    do {
        char line[512];
        char caption[512];
        weapon_class_caption(i, caption);
        sprintf(line, "%s:  %d", caption, (int)g_game->chars[player].f80[i]);
        {
            tstring text(line);
            char unset[8];                          // the original passes the address of 8 uninitialised bytes
            labels[i]->set_text(unset, text);
        }
        if (c->prof_points > 0
            && (float)g_weapon_xp[g_game->skill_level(player, 0x1b)] >= c->f80[i]
            && g_display_wp_one_handed[i] != 0
            && c->steps_to_point(i) <= c->prof_points)
            rows[i]->show();   // slot +0xb4 (the slice called it show_selected)
        else
            rows[i]->hide();   // slot +0xb0 (the slice called it show)
        i++;
    } while (i < 7);
}
