#include "./cs_update.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4eeb10 -------------------------------------------------------------------------------
// The click handling of the rows is `selected |= press on either button` / `released |= release on either`;
// a click on the right button of a row spends the corresponding points.
void CharSheet::update(float dt)
{
    int zero = 0;
    bool selected = false;
    bool released = false;

    if (f_2e4 != 0)
        ((World*)f_2e4)->step(dt);
    g_msglog.flush();
    if (f_4d4 != 0)
        f_430->show();
    else
        f_430->hide();

    if (f_4c8 != 0) {
        if (g_world_time != 0) {
            ScrollHost* host = (ScrollHost*)f_4bc;
            UiDsv* bar = host->bar;
            bar->set_value(((float)g_world_time / bar->stops[bar->f_178]) * bar->f_17c * 0.0005f +
                           (bar->stops[bar->f_178] - bar->f_17c) *
                               ((bar->f_168 - bar->f_16c) / (bar->f_170 - bar->f_16c)));
            ((UiVt*)((ScrollHost*)f_4bc)->bar)->set_key_flags(0x10000);
        }
        // the skill rows: select on press, then learn a point on click
        for (int i = 0; i < f_2d0; i++) {
            if (f_30[i]->clicked_on(2) || f_180[i]->clicked_on(2)) {
                selected = true;
                f_2d4 = i;
                f_2d8 = 1;
                update_detail();
            }
            if (f_30[i]->clicked_on(4) || f_180[i]->clicked_on(4))
                released = true;
            if (f_180[i]->clicked_on(0x10000)) {
                int troop = f_14;
                int id = f_228[i];
                TroopRec* tb = g_game->troops;
                int rank = (int)((unsigned)((int*)tb)[troop * 0x142 + id / 8 + 0x1a] >> (id % 8 * 4)) & 0xf;
                if (rank < 0)
                    rank = 0;
                else if (rank > g_skill_tbl[id].cap)
                    rank = g_skill_tbl[id].cap;
                tb[troop].set_skill(id, rank + 1);
                g_game->troops[f_14].skill_points--;
                f_4d4 = 1;
                update_detail();
                update_skill_rows();
                refresh_panels();
                refresh_lists();
            }
        }
        // the seven proficiency rows: train one point on click, paid with proficiency points
        for (int i = 0; i < 7; i++) {
            if (f_478[i]->clicked_on(2) || f_494[i]->clicked_on(2)) {
                selected = true;
                f_2d4 = i;
                f_2d8 = 2;
                update_detail();
            }
            if (f_478[i]->clicked_on(4) || f_494[i]->clicked_on(4))
                released = true;
            if (f_494[i]->clicked_on(0x10000)) {
                int cost = g_game->troops[f_14].steps_to_point(i);
                g_game->troops[f_14].bump_prof(i, (float)cost);
                g_game->troops[f_14].prof_points -= cost;
                f_4d4 = 1;
                refresh_lists();
                refresh_panels();
            }
        }
    }

    // the four attribute rows
    for (int i = 0; i < 4; i++) {
        if (f_44c[i]->clicked_on(2) || f_45c[i]->clicked_on(2)) {
            selected = true;
            f_2d4 = i;
            f_2d8 = 0;
            update_detail();
        }
        if (f_44c[i]->clicked_on(4) || f_45c[i]->clicked_on(4))
            released = true;
        if (f_45c[i]->clicked_on(0x10000)) {
            g_game->troops[f_14].bump_attr(i, 1);
            g_game->troops[f_14].attr_points--;
            if (i == 1)
                g_game->troops[f_14].prof_points += 5;
            f_4d4 = 1;
            refresh_panels();
            update_detail();
            update_skill_rows();
            refresh_lists();
        }
    }

    // the face page
    if (f_52c != 0 && f_4cc != 0 && f_4cc->clicked_on(2)) {
        selected = true;
        f_2d4 = 0;
        f_2d8 = 3;
        update_detail();
    }
    if ((f_52c != 0 && f_4cc != 0 && f_4cc->clicked_on(4) || released) && !selected) {
        f_2d4 = -1;
        update_detail();
    }
    if (f_14 == 0 && f_4c4->clicked_on(0x10000)) {
        g_game->troops[0].name = tstring(((HeroEntry*)f_4c4)->name);
        g_game->sl_query(0x18, 0, 0);
    }

    // previous / next troop
    int step = 0;
    if (f_444 != 0 && f_444->clicked_on(0x10000))
        step = 1;
    else if (f_448 != 0 && f_448->clicked_on(0x10000))
        step = -1;

    if (f_2dc == 0) {
        if (g_profile->troop != -1) {
            g_game->f_b4 = g_profile->troop;
            f_2dc++;
            menu_choose(0xf);
            return;
        }
    } else if (f_2dc == 1) {
        TroopRec* hero = g_game->troops;
        if (hero->attr_points > 0 || hero->skill_points > 0 || hero->prof_points > 5 ||
            ((HeroEntry*)f_4c4)->f181 != 0)
            f_42c->enable();
        else
            f_42c->disable();
        if (f_42c->clicked_on(0x10000)) {
            f_2dc = -1;
            g_game->troops[g_game->player].auto_equip(-1, 0, 0, 0);
            g_menu_state[5] = f_14;
            menu_choose(0x1d);
            return;
        }
        if (f_430->clicked_on(0x10000)) {
            apply_to_troop();
            f_4d4 = 0;
            update_detail();
            update_skill_rows();
            refresh_panels();
            refresh_lists();
            return;
        }
    } else {
        if (step != 0) {
            f_14 += step;
            f_14 = (f_14 + g_game->troop_count) % g_game->troop_count;
            f_4 = 1;
            menu_choose(0xf);
            return;
        }
        if (f_52c != 0 && f_4cc != 0 && f_4cc->clicked_on(0x10000)) {
            g_menu_state[5] = f_14;
            g_menu_state[6] = 0xf;
            menu_choose(0x1d);
            return;
        }
        if (f_440 != 0 && f_440->clicked_on(0x10000)) {
            g_army_state[5] = f_14;
            g_army_state[6] = 0xf;
            menu_choose(0x18);
            return;
        }
        if (f_430->clicked_on(0x10000)) {
            apply_to_troop();
            f_4d4 = 0;
            update_detail();
            update_skill_rows();
            refresh_panels();
            refresh_lists();
            return;
        }
        if (f_42c->clicked_on(0x10000)) {
            menu_choose(f_18);
            return;
        }
        // game keys: rest / export / learning
        if (g_a170d8 == 0 && g_a174d8 != 0) {
            menu_choose(f_18);
        } else {
            if (g_a17148 == 0 && g_a17348 == 0)
                return;
            if (g_a17188 != 0 && g_a17588 == 0 && g_a5a9fbc != 0) {
                if (g_trial_mode != 0) {
                    g_game->equip_hero(f_14, 1000);
                    menu_choose(0xf);
                }
                return;
            }
            if ((g_a17148 != 0 || g_a17348 != 0) && g_a17118 != 0 && g_a17518 == 0 && g_a5a9fbc != 0) {
                for (int i = 0; i < 7; i++)
                    g_game->troops->bump_prof(i, 50.0f);
                menu_choose(0xf);
                return;
            }
        }
    }
}
