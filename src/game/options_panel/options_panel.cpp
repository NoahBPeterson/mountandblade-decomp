// options_panel.cpp: the options / character panel (0x4d7370..0x4d92c0): control-key names, the faction report, the
// character import / export flow, the character file writer (0x4d86d0), the character import reader (0x4d8c20)
// and the export path builder (0x4d9260). Names provisional; see options_panel.h.
#include "./options_panel.h"
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include <string.h>
#include <stdlib.h>
#include "../main_menu.h"      // (listed directly: the resolution audit reads only the source's own includes)

#define UI_NO_KEY_ASSIGNED  "ui_no_key_assigned"                    // 0x59687c

// fields of Troop (../../include/troop.h) the header leaves in padding: read at their offsets
#define TROOP_INT(t, off)  (*(int*)((char*)(t) + (off)))
#define TROOP_I64(t, off)  (*(unsigned __int64*)((char*)(t) + (off)))

// ---- the skill table: 42 records of 0x44 bytes, name (tstring) first (0x8ee3110 .. 0x8ee3c38) ----
#include "../skill_entry.h"
extern int g_xp_table[];                                            // 0x5937c8
void mb_snprintf(char* buf, unsigned int n, const char* fmt, ...);  // 0x415100
// 0x432510: clamp *v into [*lo, *hi) and return it.
int __cdecl clamp_index(const int* v, const int* lo, const int* hi);

// @ 0x4d7370
void options_panel::key_name(int which, tstring* out)
{
    switch (which) {
    case 6:
        *out = g_keyboard.key_name(0x39);                           // Space
        return;
    case 4:
        *out = g_keyboard.key_name(0x4e);                           // Numpad Enter
        return;
    case 5:
        *out = g_keyboard.key_name(0x4a);                           // Numpad +
        return;
    case 0:
        if (key_menu < 0) {
            *out = *g_strtable.lookup(&tstring(UI_NO_KEY_ASSIGNED));
            return;
        }
        *out = g_keyboard.key_name(key_menu);
        return;
    case 1:
        if (key_prev < 0) {
            *out = *g_strtable.lookup(&tstring(UI_NO_KEY_ASSIGNED));
            return;
        }
        *out = g_keyboard.key_name(key_prev);
        return;
    case 2:
        if (key_select < 0) {
            *out = *g_strtable.lookup(&tstring(UI_NO_KEY_ASSIGNED));
            return;
        }
        *out = g_keyboard.key_name(key_select);
        return;
    case 3:
        if (key_next < 0) {
            *out = *g_strtable.lookup(&tstring(UI_NO_KEY_ASSIGNED));
            return;
        }
        *out = g_keyboard.key_name(key_next);
        return;
    }
}

// @ 0x4d7610
tstring ctr::key_name(int key)
{
    tstring s("unknown");                                           // 0x596b9c
    switch (key) {
    case 2: s.assign("1"); break;                                   // 0x596b98
    case 3: s.assign("2"); break;                                   // 0x596b94
    case 4: s.assign("3"); break;                                   // 0x596b90
    case 5: s.assign("4"); break;                                   // 0x596b8c
    case 6: s.assign("5"); break;                                   // 0x596b88
    case 7: s.assign("6"); break;                                   // 0x596b84
    case 8: s.assign("7"); break;                                   // 0x596b80
    case 9: s.assign("8"); break;                                   // 0x596b7c
    case 10: s.assign("9"); break;                                   // 0x596b78
    case 11: s.assign("0"); break;                                   // 0x590e24
    case 30: s.assign("A"); break;                                   // 0x596b74
    case 48: s.assign("B"); break;                                   // 0x596b70
    case 46: s.assign("C"); break;                                   // 0x596b6c
    case 32: s.assign("D"); break;                                   // 0x596b68
    case 18: s.assign("E"); break;                                   // 0x596b64
    case 33: s.assign("F"); break;                                   // 0x596b60
    case 34: s.assign("G"); break;                                   // 0x596b5c
    case 35: s.assign("H"); break;                                   // 0x596b58
    case 23: s.assign("I"); break;                                   // 0x596b54
    case 36: s.assign("J"); break;                                   // 0x596b50
    case 37: s.assign("K"); break;                                   // 0x596b4c
    case 38: s.assign("L"); break;                                   // 0x596b48
    case 50: s.assign("M"); break;                                   // 0x596b44
    case 49: s.assign("N"); break;                                   // 0x596b40
    case 24: s.assign("O"); break;                                   // 0x596b3c
    case 25: s.assign("P"); break;                                   // 0x596b38
    case 16: s.assign("Q"); break;                                   // 0x596b34
    case 19: s.assign("R"); break;                                   // 0x596b30
    case 31: s.assign("S"); break;                                   // 0x596b2c
    case 20: s.assign("T"); break;                                   // 0x596b28
    case 22: s.assign("U"); break;                                   // 0x596b24
    case 47: s.assign("V"); break;                                   // 0x596b20
    case 17: s.assign("W"); break;                                   // 0x596b1c
    case 45: s.assign("X"); break;                                   // 0x596b18
    case 21: s.assign("Y"); break;                                   // 0x596b14
    case 44: s.assign("Z"); break;                                   // 0x596b10
    case 82: s.assign("Numpad 0"); break;                                   // 0x596b04
    case 79: s.assign("Numpad 1"); break;                                   // 0x596af8
    case 80: s.assign("Numpad 2"); break;                                   // 0x596aec
    case 81: s.assign("Numpad 3"); break;                                   // 0x596ae0
    case 75: s.assign("Numpad 4"); break;                                   // 0x596ad4
    case 76: s.assign("Numpad 5"); break;                                   // 0x596ac8
    case 77: s.assign("Numpad 6"); break;                                   // 0x596abc
    case 71: s.assign("Numpad 7"); break;                                   // 0x596ab0
    case 72: s.assign("Numpad 8"); break;                                   // 0x596aa4
    case 73: s.assign("Numpad 9"); break;                                   // 0x596a98
    case 69: s.assign("Num Lock"); break;                                   // 0x596a8c
    case 181: s.assign("Numpad /"); break;                                   // 0x596a80
    case 55: s.assign("Numpad *"); break;                                   // 0x596a74
    case 74: s.assign("Numpad -"); break;                                   // 0x596a68
    case 78: s.assign("Numpad +"); break;                                   // 0x596a5c
    case 156: s.assign("Numpad Enter"); break;                                   // 0x596a4c
    case 83: s.assign("Numpad ."); break;                                   // 0x596a40
    case 210: s.assign("Insert"); break;                                   // 0x596a38
    case 211: s.assign("Delete"); break;                                   // 0x596a30
    case 199: s.assign("Home"); break;                                   // 0x596a28
    case 207: s.assign("End"); break;                                   // 0x596a24
    case 201: s.assign("Page Up"); break;                                   // 0x596a1c
    case 209: s.assign("Page Down"); break;                                   // 0x596a10
    case 200: s.assign("Up Arrow"); break;                                   // 0x596a04
    case 208: s.assign("Down Arrow"); break;                                   // 0x5969f8
    case 203: s.assign("Left Arrow"); break;                                   // 0x5969ec
    case 205: s.assign("Right Arrow"); break;                                   // 0x5969e0
    case 59: s.assign("F1"); break;                                   // 0x5969dc
    case 60: s.assign("F2"); break;                                   // 0x5969d8
    case 61: s.assign("F3"); break;                                   // 0x5969d4
    case 62: s.assign("F4"); break;                                   // 0x5969d0
    case 63: s.assign("F5"); break;                                   // 0x5969cc
    case 64: s.assign("F6"); break;                                   // 0x5969c8
    case 65: s.assign("F7"); break;                                   // 0x5969c4
    case 66: s.assign("F8"); break;                                   // 0x5969c0
    case 67: s.assign("F9"); break;                                   // 0x5969bc
    case 68: s.assign("F10"); break;                                   // 0x5969b8
    case 87: s.assign("F11"); break;                                   // 0x5969b4
    case 88: s.assign("F12"); break;                                   // 0x5969b0
    case 57: s.assign("Space"); break;                                   // 0x5969a8
    case 1: s.assign("Esc"); break;                                   // 0x5969a4
    case 28: s.assign("Enter"); break;                                   // 0x59699c
    case 15: s.assign("Tab"); break;                                   // 0x596998
    case 14: s.assign("Back Space"); break;                                   // 0x59698c
    case 26: s.assign("["); break;                                   // 0x596988
    case 27: s.assign("]"); break;                                   // 0x596984
    case 51: s.assign(","); break;                                   // 0x596980
    case 52: s.assign("."); break;                                   // 0x596828
    case 53: s.assign("/"); break;                                   // 0x59379c
    case 43: s.assign("\\"); break;                                   // 0x54a4b0
    case 13: s.assign("="); break;                                   // 0x54ba44
    case 12: s.assign("-"); break;                                   // 0x595ef4
    case 39: s.assign(";"); break;                                   // 0x59697c
    case 40: s.assign("'"); break;                                   // 0x54ba3c
    case 41: s.assign("~"); break;                                   // 0x596978
    case 58: s.assign("Caps Lock"); break;                                   // 0x59696c
    case 42: s.assign("Shift (left)"); break;                                   // 0x59695c
    case 54: s.assign("Shift (right)"); break;                                   // 0x59694c
    case 29: s.assign("Ctrl (left)"); break;                                   // 0x596940
    case 157: s.assign("Ctrl (right)"); break;                                   // 0x596930
    case 56: s.assign("Alt (left)"); break;                                   // 0x596924
    case 184: s.assign("Alt (right)"); break;                                   // 0x596918
    case 224: s.assign("Left Mouse Btn"); break;                                   // 0x596908
    case 225: s.assign("Right Mouse Btn"); break;                                   // 0x5968f8
    case 226: s.assign("Middle Mouse Btn"); break;                                   // 0x5968e4
    case 227: s.assign("Mouse Btn 4"); break;                                   // 0x5968d8
    case 228: s.assign("Mouse Btn 5"); break;                                   // 0x5968cc
    case 229: s.assign("Mouse Btn 6"); break;                                   // 0x5968c0
    case 230: s.assign("Mouse Btn 7"); break;                                   // 0x5968b4
    case 231: s.assign("Mouse Btn 8"); break;                                   // 0x5968a8
    case 238: s.assign("Scroll Up"); break;                                   // 0x59689c
    case 239: s.assign("Scroll Down"); break;                                   // 0x596890
    }
    return s;
}

// @ 0x4d7d10
void options_panel::key_binding_name(int slot, tstring* out, int* translated)
{
    const int key = (&key_menu)[slot * 2];
    if (key < 0) {
        {
            tstring no_key("ui_no_key_assigned");                   // 0x59687c
            *out = *g_strtable.lookup(&no_key);
        }
        *(char*)translated = 1;
    } else {
        *out = g_keyboard.key_name(key);
        *(char*)translated = 0;
    }
}

// @ 0x4d7d90
void options_panel::on_key_event(int)
{
    if (cursor_control->is_key_down(0x10000)) {
        menu_open = 0;
        update_cursor();
        refresh_texts();
    }
    if (menu_keys->is_key_down(0x10000))
        g_main_menu.set_state(menu_choice);
}

// @ 0x4d7de0
void options_panel::update_cursor()
{
    if (menu_open == 0)
        cursor_control->set_cursor(1);
    else
        cursor_control->set_cursor(0);
}

// @ 0x4d7e00
void options_panel::refresh_texts()
{
    {
        tstring empty(EMPTY_TEXT);                                  // 0x54a480 ""
        texts->set_text(empty);
    }
    if (menu_open == 0)
        draw_faction_report();
}

// @ 0x4d7e50
void options_panel::draw_faction_report()
{
    float row = 0.6f;
    {
        tstring empty(EMPTY_TEXT);                                  // 0x54a480 ""
        texts->set_text(empty);
    }
    {
        tstring title("ui_factions_report");                        // 0x596bac
        float pos[2];
        pos[0] = 0.1f;
        pos[1] = 0.65f;
        texts->add_text(g_strtable.lookup(&title), pos);
    }
    for (int i = 0; i < g_game->nfactions; i++) {
        Faction* f = &g_game->factions[i];
        float renown = f->rel[0] * 100.0f;                          // 0x54ab28
        float whole = (float)floor(renown);
        int base = (int)whole;
        int n;
        if (renown - whole <= 0.5f)                                 // 0x549d70
            n = base;
        else
            n = base + 1;
        tstring* shown = &f->name;
        if (f->flag == 0) {
            tstring* found = g_strtable.find(&f->key);
            if (found)
                shown = found;
        }
        char text[512];
        sprintf(text, "%s:  %d", shown->c_str(), n);                // 0x596ba4
        {
            tstring line(text);
            float xy[2];
            xy[0] = 0.2f;
            xy[1] = row;
            texts->add_text(&line, xy);
        }
        row -= 0.06f;                                               // 0x590e5c
    }
}

// @ 0x4d8000
void options_panel::import_export(int)
{
    char buf_a[512];
    char buf_b[512];
    if (export_pending == 0) {
        export_path(buf_a);
        export_pending = -1;
        outfile file(buf_a);
        file.fp = fopen(buf_a, MODE_W);
        if (file.fp) {
            file.failed = 0;
            file.open = 1;
        } else {
            file.failed = 1;
        }
        ((GameIo*)g_game)->export_character(&file);
        file.shut();
        {
            tstring msg("ui_character_playername_exported_successfully");   // 0x596d20
            Troop* t = &g_game->troops[g_game->cur_troop];
            tstring* nm = g_strtable.find(&t->key);
            if (!nm)
                nm = &t->name;
            sprintf(buf_b, g_strtable.lookup(&msg)->c_str(), nm->c_str());
        }
        {
            tstring text(buf_b);
            tstring header("");
            g_main_menu.rec->setup(&header, &text);
        }
        g_main_menu.reset(0);
        return;
    }
    if (import_pending == 0) {
        export_path(buf_b);
        import_pending = -1;
        ini_file table;
        FILE* f = fopen(buf_b, MODE_R);
        if (f) {
            table.load(f);
            fclose(f);
            ((GameIo*)g_game)->import_apply(&table);
            {
                tstring msg("ui_character_playername_imported_successfully");   // 0x596cf0
                Troop* t = &g_game->troops[g_game->cur_troop];
                tstring* nm = g_strtable.find(&t->key);
                if (!nm)
                    nm = &t->name;
                sprintf(buf_a, g_strtable.lookup(&msg)->c_str(), nm->c_str());
            }
            {
                tstring text(buf_a);
                tstring header("");
                g_main_menu.rec->setup(&header, &text);
            }
            g_main_menu.reset(0);
        } else {
            {
                tstring msg("ui_unable_to_open_import_file");       // 0x596cd0
                tstring header("");
                g_main_menu.rec->setup(&header, g_strtable.lookup(&msg));
            }
            g_main_menu.reset(0);
        }
        return;
    }
    if (import_key->is_key_down(0x10000)) {
        if (g_trial_mode) {
            export_path(buf_b);
            if (mb_file_exists(buf_b)) {
                import_pending = -1;
                {
                    tstring msg("ui_are_you_sure_you_want_to_import_the_character");   // 0x596c9c
                    sprintf(buf_a, g_strtable.lookup(&msg)->c_str());
                }
                {
                    tstring no("ui_no");                            // 0x596c94
                    tstring yes("ui_yes");                          // 0x596c8c
                    tstring text(buf_a);
                    g_main_menu.rec->ask_yes_no(&text, g_strtable.lookup(&yes), g_strtable.lookup(&no), &import_pending);
                }
                g_main_menu.set_state(0);
            } else {
                {
                    tstring msg("ui_unable_to_find_character_import_file"); // 0x596c64
                    tstring header("");
                    g_main_menu.rec->setup(&header, g_strtable.lookup(&msg));
                }
                g_main_menu.reset(0);
            }
        } else {
            {
                tstring msg("ui_mount_and_blade_is_running_in_trial_mode_please_buy_the_game_for_importing_a_character");
                tstring header("");
                g_main_menu.rec->setup(&header, g_strtable.lookup(&msg));
            }
            g_main_menu.reset(0);
        }
        return;
    }
    if (cursor_control->is_key_down(0x10000)) {
        export_path(buf_b);
        if (mb_file_exists(buf_b)) {
            export_pending = -1;
            {
                tstring msg("ui_export_file_for_character_playername_already_exists_overwrite_it"); // 0x596bc0
                Troop* t = &g_game->troops[g_game->cur_troop];
                sprintf(buf_a, g_strtable.lookup(&msg)->c_str(), ((TroopRec*)t)->display_name()->c_str());
            }
            {
                tstring no("ui_no");
                tstring yes("ui_yes");
                tstring text(buf_a);
                g_main_menu.rec->ask_yes_no(&text, g_strtable.lookup(&yes), g_strtable.lookup(&no), &export_pending);
            }
            g_main_menu.set_state(0);
        } else {
            export_pending = 0;
        }
        return;
    }
    if (menu_keys->is_key_down(0x10000))
        g_main_menu.set_state(menu_choice);
}

// @ 0x4d86d0
void GameIo::export_character(outfile* stream)
{
    char line[0x400];
    Troop* troop = &troops[cur_troop];

    sprintf(line, "%s = %ld\n", " charfile_version", 1);
    stream->write(line);

    tstring name(troop->name);
    name.spaces_to_underscores();
    mb_snprintf(line, 0x400, "%s = %s\n", "name", name.c_str());
    stream->write(line);

    sprintf(line, "%s = %ld\n", "xp", troop->exp);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "money", troop->f3b4);
    stream->write(line);
    stream->write("\n");

    sprintf(line, "%s = %ld\n", "attribute_points", troop->fa8);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "skill_points", troop->fa4);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "weapon_points", troop->fac);
    stream->write(line);
    stream->write("\n");

    sprintf(line, "%s = %ld\n", "strength", troop->f58[0]);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "agility", troop->f58[1]);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "intelligence", TROOP_INT(troop, 0x60));
    stream->write(line);
    sprintf(line, "%s = %ld\n", "charisma", troop->f64);
    stream->write(line);
    stream->write("\n");

    tstring skill;
    for (int i = 0; i < 42; i++) {
        skill = g_skills[i].name;
        skill.spaces_to_underscores();
        skill.to_lower();
        sprintf(line, "%s = %ld\n", skill.c_str(), troop->nibble(i));
        stream->write(line);
    }
    stream->write("\n");

    char weapon_text[0x100];
    tstring weapon_name;
    for (int i = 0; i < 7; i++) {
        switch (i) {
        case 0: strcpy(weapon_text, "one_handed_weapons"); break;
        case 1: strcpy(weapon_text, "two_handed_weapons"); break;
        case 2: strcpy(weapon_text, "polearms"); break;
        case 3: strcpy(weapon_text, "archery"); break;
        case 4: strcpy(weapon_text, "crossbows"); break;
        case 5: strcpy(weapon_text, "throwing"); break;
        case 6: strcpy(weapon_text, "firearms"); break;
        }
        weapon_name.assign(weapon_text);
        weapon_name.spaces_to_underscores();
        sprintf(line, "%s = %ld\n", weapon_name.c_str(), (int)troop->f80[i]);
        stream->write(line);
    }
    stream->write("\n");

    sprintf(line, "%s = %I64x\n", "face_key_1", TROOP_I64(troop, 0x3c0));
    stream->write(line);
    sprintf(line, "%s = %I64x\n", "face_key_2", TROOP_I64(troop, 0x3c8));
    stream->write(line);
}

static const char* const weapon_names[7] = {
    "one_handed_weapons", "two_handed_weapons", "polearms", "archery",
    "crossbows", "throwing", "firearms"
};

// @ 0x4d8c20
void GameIo::import_apply(ini_file* imp)
{
    Troop* troop = &troops[cur_troop];

    for (int i = 0; i < imp->count; i++) {
        ini_entry* e = &imp->entries[i];
        tstring key = e->key;
        tstring value = e->value;

        if (key.equals("name")) {
            value.underscores_to_spaces();
            troop->name = value;
            continue;
        }
        if (key.equals("xp")) {
            int xp = atoi(value.c_str());
            if (xp < 0) {
                xp = 0;
            } else if (xp >= 0x26635cb0) {
                xp = 0x26635caf;
            }
            troop->exp = xp;
            troop->level = 1;
            int j = 1;
            while (j < 0x40) {
                if (xp <= g_xp_table[j]) break;
                troop->level = j;
                if (xp <= g_xp_table[j + 1]) break;
                troop->level = j + 1;
                if (xp <= g_xp_table[j + 2]) break;
                j += 3;
                troop->level = j - 1;
            }
            continue;
        }
        if (key.equals("money")) {
            troop->f3b4 = atoi(value.c_str());
            continue;
        }
        if (key.equals("attribute_points")) {
            troop->fa8 = atoi(value.c_str());
            continue;
        }
        if (key.equals("skill_points")) {
            troop->fa4 = atoi(value.c_str());
            continue;
        }
        if (key.equals("weapon_points")) {
            troop->fac = atoi(value.c_str());
            continue;
        }
        if (key.equals("strength")) {
            troop->f58[0] = atoi(value.c_str());
            continue;
        }
        if (key.equals("agility")) {
            troop->f58[1] = atoi(value.c_str());
            continue;
        }
        if (key.equals("intelligence")) {
            TROOP_INT(troop, 0x60) = atoi(value.c_str());
            continue;
        }
        if (key.equals("charisma")) {
            troop->f64 = atoi(value.c_str());
            continue;
        }
        if (key.equals("face_key_1")) {
            sscanf(value.c_str(), "%I64x", &TROOP_I64(troop, 0x3c8));
            continue;
        }
        if (key.equals("face_key_2")) {
            sscanf(value.c_str(), "%I64x", &TROOP_I64(troop, 0x3c8));
            continue;
        }

        // Anything else is a skill / weapon proficiency name.
        key.to_lower();
        bool found = false;
        tstring tmp;
        for (int j = 0; j < 42 && !found; j++) {
            tmp = g_skills[j].name;
            tmp.spaces_to_underscores();
            tmp.to_lower();
            if (tmp.equals(key)) {
                int v = atoi(value.c_str());
                int lo = 0, hi = 0xb;
                int r = clamp_index(&v, &lo, &hi);
                troop->set_skill(j, r);
                found = true;
            }
        }
        if (!found) {
            tstring wname;
            for (int j = 0; j < 7 && !found; j++) {
                wname.assign(weapon_names[j]);
                wname.spaces_to_underscores();
                if (wname.equals(key)) {
                    int v = atoi(value.c_str());
                    int lo = 0, hi = 0x2bc;
                    int r = clamp_index(&v, &lo, &hi);
                    troop->f80[j] = (float)r;
                    found = true;
                }
            }
        }
    }
}

// @ 0x4d9260
// sprintf(out, "CharExport\\%s.txt", <active character's name>);
void options_panel::export_path(char* out)
{
    Troop* c = &g_game->troops[g_game->cur_troop];
    void* e = g_strtable.find(&c->key);
    if (e == 0) e = (void*)c;
    char* name = (char*)e + 4;
    if (*(char**)name != 0) name = *(char**)name;
    sprintf(out, "CharExport\\%s.txt", name);
}
