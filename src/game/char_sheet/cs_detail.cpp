#include "./cs_detail.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/skill_rec.h"      // (listed directly: the resolution audit scans only the source's own includes)

// @ 0x4ef9f0 -------------------------------------------------------------------------------
void CharSheet::update_detail()
{
    {
        tstring empty("");
        f_4b8->set_text(empty);
    }
    f_434->hide();
    int row = f_2d4;
    if (row < 0)
        return;
    char title[0x800];
    char body[0x200];
    char attr[0x200];
    if (f_2d8 == 1) {
        // skills page: one skill row
        int id = f_228[row];
        SkillRec* sk = &g_skills[id];
        tstring sep(": ");
        tstring* name = g_strtable.find(&sk->key);
        if (!name)
            name = &sk->name;
        tstring s = (*name + sep) + *sk->desc_name();
        const char* anchor = 0;
        if (id == 2)
            anchor = g_char_t_level_y;
        else if (id == 1)
            anchor = g_char_t_level_x;
        mb_snprintf(title, 0x400, s.c_str(), anchor);
        {
            tstring tt(title);
            f_4b8->set_text(tt);
        }
        mb_snprintf(body, 0x200, tr(tstring("ui_current_level_value")), g_game->get_prof(f_14, id));
        {
            tstring ky("char_t_current_level_y");
            float fy = g_config.get_float(ky);
            tstring kx("char_t_current_level_x");
            float fx = g_config.get_float(kx);
            tstring bt(body);
            vec2 xy = { fx, fy };
            f_4b8->set_text_at(bt, xy);
        }
        mb_snprintf(body, 0x200, tr(tstring("ui_base_attribute_value")),
                    SKILL_ATTR(id) == 0 ? tr(tstring("ui_str"))
                    : SKILL_ATTR(id) == 1 ? tr(tstring("ui_agi"))
                    : SKILL_ATTR(id) == 2 ? tr(tstring("ui_int")) : tr(tstring("ui_cha")));
        {
            tstring ky("char_t_base_attrib_y");
            float fy = g_config.get_float(ky);
            tstring kx("char_t_base_attrib_x");
            float fx = g_config.get_float(kx);
            tstring bt(body);
            vec2 xy = { fx, fy };
            f_4b8->set_text_at(bt, xy);
        }
        Game* g = g_game;
        if (f_14 != g->player) {
            f_434->hide();
            return;
        }
        int maxlvl = g_skills[id].cap;
        TroopRec* t = &g->troops[f_14];
        if (t->nibble(id) < maxlvl && t->skill_points >= 1 &&
            (g_game->troops[f_14].nibble(id) + 1) * 3 <= g->troops[f_14].attrs[SKILL_ATTR(id)]) {
            f_434->hide();
            return;
        }
        if (t->nibble(id) >= maxlvl) {
            mb_snprintf(body, 0x200, tr(tstring("ui_at_learning_limit")));
        } else if (t->skill_points < 1) {
            mb_snprintf(body, 0x200, tr(tstring("ui_not_enough_skill_points_to_learn")));
        } else {
            if (SKILL_ATTR(id) == 0)
                mb_snprintf(attr, 0x200, tr(tstring("ui_strength")));
            else if (SKILL_ATTR(id) == 1)
                mb_snprintf(attr, 0x200, tr(tstring("ui_agility")));
            else if (SKILL_ATTR(id) == 2)
                mb_snprintf(attr, 0x200, tr(tstring("ui_intelligence")));
            else
                mb_snprintf(attr, 0x200, tr(tstring("ui_charisma")));
            mb_snprintf(body, 0x200, tr(tstring("ui_not_enough_attributetype_to_learn_this_skill")), attr);
        }
        {
            tstring ht(body);
            f_434->set_text(ht);
        }
        f_434->show();
        return;
    }
    if (f_2d8 == 2) {                                       // proficiencies page
        prof_group_name(row, body);
        row = f_2d4;
        if (row == 0)
            strcpy(attr, tr(tstring("ui_explanation_one_handed_weapon")));
        else if (row == 1)
            strcpy(attr, tr(tstring("ui_explanation_two_handed_weapon")));
        else if (row == 2)
            strcpy(attr, tr(tstring("ui_explanation_polearm")));
        else if (row == 3)
            strcpy(attr, tr(tstring("ui_explanation_archery")));
        else if (row == 4)
            strcpy(attr, tr(tstring("ui_explanation_crossbow")));
        else if (row == 5)
            strcpy(attr, tr(tstring("ui_explanation_throwing")));
        else
            strcpy(attr, tr(tstring("ui_explanation_firearms")));
        mb_snprintf(title, 0x400, "%s: %s", body, attr);
        {
            tstring tt(title);
            f_4b8->set_text(tt);
        }
        f_434->hide();
        return;
    }
    if (f_2d8 == 0) {                                       // attributes page
        if (row == 0)
            strcpy(title, tr(tstring("ui_explanation_strength")));
        else if (row == 1)
            strcpy(title, tr(tstring("ui_explanation_agility")));
        else if (row == 2)
            strcpy(title, tr(tstring("ui_explanation_intelligence")));
        else if (row == 3)
            strcpy(title, tr(tstring("ui_explanation_charisma")));
        {
            tstring tt(title);
            f_4b8->set_text(tt);
        }
        f_434->hide();
        return;
    }
    if (f_2d8 == 3) {                                       // face page
        f_4b8->set_text(*g_strtable.lookup(tstring("ui_edit_face")));
        f_434->hide();
    }
}
