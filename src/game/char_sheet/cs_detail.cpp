#include "./cs_detail.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/skill_rec.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../char_sheet_panel.h"      // (listed directly: the resolution audit reads only the source's own includes)

// a skill's "name: description" title; the separator and the partial sum die before the caller goes on, as in the
// original (both destroyed straight after the second operator+)
static __forceinline tstring skill_row_title(SkillRec* sk)
{
    tstring sep(": ");
    tstring* name = g_strtable.find(&sk->key);
    if (!name)
        name = &sk->name;
    return (*name + sep) + *sk->desc_name();
}

// @ 0x4ef9f0 -------------------------------------------------------------------------------
void CharSheet::update_detail()
{
    {
        tstring empty("");
        { char r[8]; f_4b8->set_text(r, empty); }
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
        SkillRec* sk = &g_skill_tbl[id];
        tstring s = skill_row_title(sk);
        const char* anchor = 0;
        if (id == 2)
            anchor = g_skill_prisoner_management_bonus;
        else if (id == 1)
            anchor = g_skill_leadership_bonus;
        mb_snprintf(title, 0x400, s.c_str(), anchor);
        {
            tstring tt(title);
            { char r[8]; f_4b8->set_text(r, tt); }
        }
        mb_snprintf(body, 0x200, tr(tstring("ui_current_level_value")), g_game->skill_level(f_14, id));
        {
            tstring ky("char_t_current_level_y");
            float fy = g_config.get_float(ky);
            tstring kx("char_t_current_level_x");
            float fx = g_config.get_float(kx);
            tstring bt(body);
            vec2 xy = { fx, fy };
            f_4b8->set_text_at(bt, (const float*)&xy);
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
            f_4b8->set_text_at(bt, (const float*)&xy);
        }
        GameData* g = g_game;
        if (f_14 != g->player) {
            f_434->hide();
            return;
        }
        int maxlvl = g_skill_tbl[id].cap;
        TroopRec* t = &g->chars[f_14];
        if (t->nibble(id) < maxlvl && t->skill_points >= 1 &&
            (g_game->chars[f_14].nibble(id) + 1) * 3 <= g->chars[f_14].attrs[SKILL_ATTR(id)]) {
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
            { char r[8]; f_434->set_text(r, ht); }
        }
        f_434->show();
        return;
    }
    if (f_2d8 == 2) {                                       // proficiencies page
        weapon_class_caption(row, body);
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
            { char r[8]; f_4b8->set_text(r, tt); }
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
            { char r[8]; f_4b8->set_text(r, tt); }
        }
        f_434->hide();
        return;
    }
    if (f_2d8 == 3) {                                       // face page
        { char r[8]; f_4b8->set_text(r, *g_strtable.lookup(tstring("ui_edit_face"))); }
        f_434->hide();
    }
}
