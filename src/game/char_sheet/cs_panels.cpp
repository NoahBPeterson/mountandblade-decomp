#include "./cs_panels.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../config_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../char_sheet_panel.h"      // (listed directly: the resolution audit reads only the source's own includes)

// Each looked-up string / posted line is a named tstring local (the original takes their address with lea).
#define POS(W, LINE, KEY_Y, KEY_X)                       \
    do {                                                 \
        tstring ky(KEY_Y);                               \
        float y = config_float(ky);                      \
        tstring kx(KEY_X);                               \
        float x = config_float(kx);                      \
        tstring t(LINE);                                 \
        float pos[2];                                    \
        pos[0] = x;                                      \
        pos[1] = y;                                      \
        W->set_text_at(t, pos);                         \
    } while (0)

// @ 0x4f0820
void CharSheet::refresh_panels()
{
    int i;
    Troop* c = (Troop*)((char*)g_game->chars + player * 0x508);
    char line[512];
    int* stats = (int*)((char*)c + 0x58);    // the four stat dwords at +0x58..+0x64 (str, agi, int, cha)

    PostSlot out;
    {
        tstring t("");
        level_at->set_text(&out, t);
    }
    {
        tstring t("");
        xp_bar->set_text(&out, t);
    }

    if (((SheetWidget*)f_4c4)->has_label == 0) {
        tstring* n = g_strtable.find(&c->key);
        if (n == 0) n = (tstring*)c;     // 0x4f08d6: mov eax, esi (the Troop itself, not its key at +0x20)
        tstring t(n->c_str());
        f_4c4->set_text(&out, t);
    }

    {
        tstring k("ui_level");
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), c->level);
    }
    POS(xp_bar, line, "char_t_level_y", "char_t_level_x");

    {
        tstring k("ui_xp");
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), c->exp);
    }
    POS(xp_bar, line, "char_t_xp_y", "char_t_xp_x");

    {
        tstring k("ui_next_level_at");
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), g_level_table[c->level]);
    }
    POS(xp_bar, line, "char_t_next_level_at_y", "char_t_next_level_at_x");

    if ((c->flags32 & 0x10) != 0) {
        tstring k("ui_health_player");
        i = c->nib[3] & 0xf;
        if (i < 0) i = 0;
        else if (i > g_skill_clamp) i = g_skill_clamp;
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), (int)c->hp, c->lvl58 + i * 2 + 0x23);
    } else {
        tstring k("ui_health");
        i = c->nib[3] & 0xf;
        if (i < 0) i = 0;
        else if (i > g_skill_clamp) i = g_skill_clamp;
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), c->lvl58 + i * 2 + 0x23);
    }
    POS(xp_bar, line, "char_t_health_y", "char_t_health_x");

    {
        tstring k("ui_attribute_points");
        mb_snprintf(line, 0x200, g_strtable.lookup(k)->c_str(), c->attr_points);
    }
    POS(level_at, line, "char_t_attrib_points_y", "char_t_attrib_points_x");

    {
        tstring k("ui_skill_points");
        mb_snprintf(line, 0x200, g_strtable.lookup(k)->c_str(), c->skill_points);
    }
    POS(level_at, line, "char_t_skill_points_y", "char_t_skill_points_x");

    {
        tstring k("ui_weapon_points");
        mb_snprintf(line, 0x200, g_strtable.lookup(k)->c_str(), c->prof_points);
    }
    POS(level_at, line, "char_t_weapon_points_y", "char_t_weapon_points_x");

    for (i = 0; i < 4; i++) {
        mb_snprintf(line, 0x200, "%s: %d",
                   g_strtable.lookup(i == 0 ? tstring("ui_str") : i == 1 ? tstring("ui_agi")
                                     : i == 2 ? tstring("ui_int") : tstring("ui_cha"))->c_str(),
                   stats[i]);
        {
            tstring t(line);
            skill_pos[i]->set_text(&out, t);
        }
        if (c->attr_points > 0)
            skill_val[i]->show();
        else
            skill_val[i]->hide();
    }
}
