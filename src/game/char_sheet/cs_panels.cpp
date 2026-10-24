#include "./cs_panels.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f0820
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
        W->set_position(t, pos);                         \
    } while (0)

void CharSheet::update_texts()
{
    int i;
    CharRec* c = (CharRec*)((char*)g_game->chars + player * 0x508);
    char line[512];
    int* stats = (int*)((char*)c + 0x58);    // the four stat dwords at +0x58..+0x64 (str, agi, int, cha)

    PostSlot out;
    {
        tstring t("");
        level_at->post_text(&out, t);
    }
    {
        tstring t("");
        xp_bar->post_text(&out, t);
    }

    if (name->has_label == 0) {
        tstring* n = g_strtable.find((const tstring*)c->name);
        if (n == 0) n = (tstring*)c->name;
        tstring t(n->c_str());
        name->post_text(&out, t);
    }

    {
        tstring k("ui_level");
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), c->xp);
    }
    POS(xp_bar, line, "char_t_level_y", "char_t_level_x");

    {
        tstring k("ui_xp");
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), c->xp_val);
    }
    POS(xp_bar, line, "char_t_xp_y", "char_t_xp_x");

    {
        tstring k("ui_next_level_at");
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), g_level_table[c->xp]);
    }
    POS(xp_bar, line, "char_t_next_level_y", "char_t_next_level_x");

    if ((c->flags & 0x10) != 0) {
        tstring k("ui_health_pyer");
        i = c->attrib[1] & 0xf;
        if (i < 0) i = 0;
        else if (i > g_skill_clamp) i = g_skill_clamp;
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), (int)c->health, c->level_at + i * 2 + 0x23);
    } else {
        tstring k("ui_health");
        i = c->attrib[1] & 0xf;
        if (i < 0) i = 0;
        else if (i > g_skill_clamp) i = g_skill_clamp;
        mb_vsprintf(line, g_strtable.lookup(k)->c_str(), c->level_at + i * 2 + 0x23);
    }
    POS(xp_bar, line, "char_t_health_y", "char_t_health_x");

    {
        tstring k("ui_attribute_points");
        mb_sprintf(line, 0x200, g_strtable.lookup(k)->c_str(), c->health_max);
    }
    POS(level_at, line, "char_t_attribut_points_y", "char_t_attribut_points_x");

    {
        tstring k("ui_skillts");
        mb_sprintf(line, 0x200, g_strtable.lookup(k)->c_str(), c->hp);
    }
    POS(level_at, line, "char_t_skillts_y", "char_t_skillts_x");

    {
        tstring k("ui_weapon_points");
        mb_sprintf(line, 0x200, g_strtable.lookup(k)->c_str(), c->next_level_xp);
    }
    POS(level_at, line, "char_t_weapon_points_y", "char_t_weapon_points_x");

    for (i = 0; i < 4; i++) {
        mb_sprintf(line, 0x200, "%s:  %d",
                   g_strtable.lookup(i == 0 ? tstring("ui_str") : i == 1 ? tstring("ui_agi")
                                     : i == 2 ? tstring("ui_int") : tstring("ui_cha"))->c_str(),
                   stats[i]);
        {
            tstring t(line);
            skill_pos[i]->post_text(&out, t);
        }
        if (c->health_max > 0)
            skill_val[i]->show_selected();
        else
            skill_val[i]->show();
    }
}
