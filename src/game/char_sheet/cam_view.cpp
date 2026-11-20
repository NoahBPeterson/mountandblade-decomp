#include "../../include/image_addrs.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "./cam_view.h"
#include "../window_mgr.h"      // g_windows slots (slot_as<T>(i))
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/skin.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../window_mgr.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../key_names.h"
#include "../ui_globals.h"      // UI / core globals
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)
#include "../../include/game_data.h"                        // GameData g_game (0x5e2930)
#include "../party2/camera_class.h"      // Camera: the deform list (follow[1] its scroll bar)

// @ 0x4eb050
void FaceGen::update(float dt)
{
    if (g_keyboard.down[0x4c])
        pitch = pitch - dt * 1.1f;
    if (g_keyboard.down[0x50])
        pitch = dt * 1.1f + pitch;
    if (g_keyboard.down[0x4e])
        zoom = zoom - dt * 2.1f;
    if (g_keyboard.down[0x4a])
        zoom = dt * 2.1f + zoom;
    if (g_keyboard.down[0xe0]) {
        if (g_keyboard.prev[0xe0] == 0 &&
            g_gui.mouse[0] < body->rect_max.x &&
            g_gui.mouse[1] > body->position.y)
            follow = 1;
        if (g_keyboard.down[0xe0])
            goto check_follow;
    }
    if (g_keyboard.prev[0xe0])
        follow = 0;
check_follow:
    if (g_keyboard.down[0xe1] || follow) {
        yaw = (g_keyboard.axis[0] * dt) * 1.7f + yaw;
        pitch = pitch - (g_keyboard.axis[1] * dt) * 1.7f;
    }
    if (pitch < -0.6519978f)
        pitch = -0.6519978f;
    else if (pitch > 0.6519978f)
        pitch = 0.6519978f;
    if (zoom < 0.3f)
        zoom = 0.3f;
    else if (zoom > 2.0f)
        zoom = 2.0f;
    mat4 m;
    identity_pos((pos*)&m);
    float angle = 1.5707964f;
    ((rgl::matrix*)&m)->rotate_about_s(&angle);
    ((rgl::matrix*)&m)->rotate_about_f(&yaw);
    angle = -pitch;
    ((rgl::matrix*)&m)->rotate_about_s(&angle);
    m.o.z = -0.1f;
    static_cast<mat4&>(ent->local) = m;
}

// The previous index of a cyclic list of count entries.
static inline int wrap_prev(int cur, int count) { return (cur + count - 1) % count; }

// @ 0x4ea8e0
void FaceGen::frame_update(float dt)
{
    int n = g_anim.skin_vec[skin].n_face_keys - 1;

    if (g_cfg.enable_edit_mode && (g_keyboard.down[0x1d] || g_keyboard.down[0x9d]) && g_keyboard.down[0x12] && !g_keyboard.prev[0x12]) {
        if (g_game->editor.active == 0) {
            g_game->editor.active = 1;
            cam_btn->show();
        } else {
            g_game->editor.active = 0;
            cam_btn->hide();
        }
    }

    if (g_gui.wheel != 0) {
        UiD* u = slider->follow[1];
        float a = u->v168 - u->f16c;
        float b = u->f170 - u->f16c;
        float* arr = &u->rect_max[u->f178];
        float v = (a / b) * (arr[0] - u->f17c)
                + ((float)g_gui.wheel / arr[0]) * u->f17c * 0.0005f;
        u->set_value(v);
        ((MissionUi*)slider->follow[1])->set_key_flags(0x10000);
    }

    if (cam_btn && cam_btn->clear_mask(0x10000)) {
        if (OpenClipboard(0)) {
            char text[0x200];
            mb_snprintf(text, 0x200, "0x%s%s%s%s", keys[0], keys[1], keys[2], keys[3]);
            int len = strlen(text) + 1;
            HGLOBAL h = GlobalAlloc(0x2002, len);
            if (h) {
                EmptyClipboard();
                char* src = text;
                char* dst = (char*)GlobalLock(h);
                char c;
                do {
                    c = *src++;
                    *dst++ = c;
                } while (c);
                GlobalUnlock(h);
                SetClipboardData(1, h);
            }
            CloseClipboard();
        }
    }

    char flag = 0;
    for (int i = 0; i < n; i++) {
        if (deform_rows[i]->clear_mask(0x10000)) {
            g_dummy_mat_arr[i] = deform_rows[i]->get168();
            g_anim.skin_vec[g_anim.cur].relax(i, g_dummy_mat_arr);
            for (int j = 0; j < n; j++)
                deform_rows[j]->set168(g_dummy_mat_arr[j]);
            flag = 1;
        }
    }

    if (b_next_beard && b_next_beard->clear_mask(0x10000)) {
        f350 = f350 + 1;
        flag = 1;
    }
    if (b_next_hair->clear_mask(0x10000)) {
        flag = 1;
        f354 = f354 + 1;
    }
    if (b_next_face->clear_mask(0x10000)) {
        flag = 1;
        f35c = f35c + 1;
    }
    if (b_prev_beard && b_prev_beard->clear_mask(0x10000)) {
        int m = SKIN_INT(n_beard);
        f350 = (f350 + m) % (m + 1);
        flag = 1;
    }
    if (b_prev_hair->clear_mask(0x10000)) {
        int m = SKIN_INT(n_hair);
        f354 = (f354 + m) % (m + 1);
        flag = 1;
    }
    if (b_prev_face->clear_mask(0x10000)) {
        f35c = wrap_prev(f35c, SKIN_INT(n_face_tex));
        flag = 1;
    }
    if (sl_age->clear_mask(0x10000))
        flag = 1;
    if (sl_hair_color->clear_mask(0x10000))
        flag = 1;
    if (b_randomize->clear_mask(0x10000)) {
        flag = 1;
        g_anim.randomise();
        for (int i = 0; i < n; i++)
            deform_rows[i]->set168(g_dummy_mat_arr[i]);
        int m = SKIN_INT(n_hair);
        f354 = m <= 1 ? 0 : rand() % m;
        f354 = f354 + 1;
        f350 = rand();
        f35c = rand();
        f358 = rand();
        sl_hair_color->set168(g_anim.time);
    }
    if (b_reset->clear_mask(0x10000)) {
        float* p = g_dummy_mat_arr;
        for (int i = 0x3f; i != 0; i--)
            *p++ = 0.5f;
        flag = 1;
        g_anim.skin_vec[g_anim.cur].relax(-1, g_dummy_mat_arr);
        for (int i = 0; i < n; i++)
            deform_rows[i]->set168(g_dummy_mat_arr[i]);
    }

    f350 = f350 % (SKIN_INT(n_beard) + 1);
    f354 = f354 % (SKIN_INT(n_hair) + 1);
    f35c = f35c % SKIN_INT(n_face_tex);

    if (flag)
        refresh();

    update(dt);
    world->step(dt);
    update_view(*(int*)&dt);

    if (w08 == 0) {
        w08 = 1;
        {
            tstring t1("ui_tutorial_face_generator");
            tstring t2("ui_tutorial");
            tstring* r1 = g_strtable.lookup(t1);
            tstring* r2 = g_strtable.lookup(t2);
            g_windows.box()->set_inline(r2, r1);
        }
        g_windows.open(0);
        return;
    }

    if (b_done->clear_mask(0x10000)) {
        if (w14 >= 0) {
            g_game->troops[w14].face_keys[0] = state.a;
            g_game->troops[w14].face_keys[1] = state.b;
            g_game->troops[w14].face_keys[2] = state.c;
            g_game->troops[w14].face_keys[3] = state.d;
        }
        if (w08 == 1) {
            w08 = -1;
            g_game->current_menu = 1;
            slot_as<head_rec>(23)->w18 = 7;
            g_windows.set_state(0x17);
        } else {
            g_windows.set_state(w18);
        }
    }
}
