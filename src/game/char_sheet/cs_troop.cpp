#include "./cs_troop.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../char_sheet_panel.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ef420 -------------------------------------------------------------------------------
void CharSheet::apply_to_troop()
{
    { TroopRec* t = &g_game->troops[f_14]; t->skill_points = f_508; }
    { TroopRec* t = &g_game->troops[f_14]; t->attr_points = f_50c; }
    { TroopRec* t = &g_game->troops[f_14]; t->prof_points = f_510; }
    { TroopRec* t = &g_game->troops[f_14]; t->fa0 = f_504; }
    { TroopRec* t = &g_game->troops[f_14]; t->attrs[0] = f_4d8; }
    { TroopRec* t = &g_game->troops[f_14]; t->attrs[1] = f_4dc; }
    { TroopRec* t = &g_game->troops[f_14]; t->attrs[2] = f_4e0; }
    { TroopRec* t = &g_game->troops[f_14]; t->attrs[3] = f_4e4; }
    { TroopRec* t = &g_game->troops[f_14]; t->nib[0] = f_514; }
    { TroopRec* t = &g_game->troops[f_14]; t->nib[1] = f_518; }
    { TroopRec* t = &g_game->troops[f_14]; t->nib[2] = f_51c; }
    { TroopRec* t = &g_game->troops[f_14]; t->nib[3] = f_520; }
    { TroopRec* t = &g_game->troops[f_14]; t->nib[4] = f_524; }
    { TroopRec* t = &g_game->troops[f_14]; t->nib[5] = f_528; }
    { TroopRec* t = &g_game->troops[f_14]; t->istat[0] = f_4e8; }
    { TroopRec* t = &g_game->troops[f_14]; t->istat[1] = f_4ec; }
    { TroopRec* t = &g_game->troops[f_14]; t->istat[2] = f_4f0; }
    { TroopRec* t = &g_game->troops[f_14]; t->istat[3] = f_4f4; }
    { TroopRec* t = &g_game->troops[f_14]; t->istat[4] = f_4f8; }
    { TroopRec* t = &g_game->troops[f_14]; t->istat[5] = f_4fc; }
    { TroopRec* t = &g_game->troops[f_14]; t->istat[6] = f_500; }
}

// @ 0x4ef7d0 -------------------------------------------------------------------------------
// One pass over the skill rows: the rank as text, coloured green when it is below the skill's cap,
// red when at the cap; then show the upgrade button only for a spendable row of the player's hero.
void CharSheet::update_skill_rows()
{
    if (f_4c8 != 0) {
        char num[512];
        for (int i = 0; i < f_2d0; i++) {
            int rank;
            int cap;
            int id;
            {
                int troop = f_14;
                id = skill_id[i];
                cap = (int)((unsigned)((int*)g_game->troops)[troop * 0x142 + id / 8 + 0x1a] >> (id % 8 * 4)) & 0xf;
                if (cap < 0)
                    cap = 0;
                else if (cap > SKILL_MAX(id))
                    cap = SKILL_MAX(id);
                rank = g_game->skill_rank(troop, id);
            }
            sprintf(num, "%d", rank);
            {
                unsigned color;
                if (rank <= cap) {
                    color = 0xff990000;
                    if (rank >= cap)
                        color = 0xff000000;
                } else {
                    color = 0xff009900;
                }
                label[i]->set_color(&color);
            }
            {
                tstring str(num);
                { char r[8]; label[i]->set_text(r, str); }
            }
            btn[i]->hide();
            {
                int troop = f_14;
                TroopRec* t = &g_game->troops[troop];
                id = skill_id[i];
                if (t->skill_points > 0) {
                    int cur = (int)((unsigned)t->nib[id / 8] >> (id % 8 * 4)) & 0xf;
                    if (cur < 0)
                        cur = 0;
                    else if (cur > SKILL_MAX(id))
                        cur = SKILL_MAX(id);
                    if (cur < SKILL_MAX(id) &&
                        t->nibble(id) < t->attrs[SKILL_ATTR(id) & 0xf] / 3 &&
                        (id != 1 && id != 2 || troop == g_game->player))
                        btn[i]->show();
                }
            }
        }
    }
}

// @ 0x4ef6d0 -------------------------------------------------------------------------------
// Number of falloff steps needed before the proficiency value rounds up one point (max 1000).
int Troop::steps_to_point(int idx)
{
    float v = stat[idx];
    int r = (int)v;
    int i = 0;
    do {
        v = falloff(v, 1.0f);
        i++;
        int cur = (int)v;
        if (cur > r)
            return i;
    } while (i < 1000);
    return i;
}

// @ 0x4ef730 -------------------------------------------------------------------------------
// Item type -> the (activity, skill) event pair used to learn it; *a = -1 for "not used".
void item_events(int item, int* a, int* b, int* c)
{
    *a = -1;
    *b = -1;
    *c = -1;
    if (item < 0)
        return;
    unsigned t = g_items[item].type_flags & 0xff;
    if (t == 2 || t == 3 || t == 4 || t == 9) {
        *a = 0;
        return;
    }
    if (t == 1) {
        *b = 0x18;
        return;
    }
    if (t == 8) {
        *b = 0x21;
        return;
    }
    if (t == 0xa) {
        *b = 0x22;
        return;
    }
    if (t == 7) {
        *b = 0x1a;
        return;
    }
    if (t == 0xc || t == 0xd || t == 0xe || t == 0xf)
        *a = 0;
}
