#include "../../include/image_addrs.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "./cam_view.h"
#include "../window_mgr.h"      // g_windows slots (slot_as<T>(i))
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/skin_set.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../main_menu.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../key_names.h"
#include "../ui_globals.h"      // UI / core globals
struct GameData;
extern GameData* g_game;                                    // 0x5e2930

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
            g_gui.mouse[0] < *(float*)((char*)body + 0xf0) &&
            g_gui.mouse[1] > *(float*)((char*)body + 0xe4))
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
    *(mat4*)((char*)ent + 0x58) = m;
}

// @ 0x4ea8e0
void FaceGen::frame_update(float dt)
{
    int n = *(int*)((char*)g_anim.skins + skin * 0x1f8 + 0x3c) - 1;

    if (g_cfg.enable_edit_mode && (g_keyboard.down[0x1d] || g_keyboard.down[0x9d]) && g_keyboard.down[0x12] && !g_keyboard.prev[0x12]) {
        if (*(int*)((char*)g_game + 0x1a090) == 0) {
            *(int*)((char*)g_game + 0x1a090) = 1;
            cam_btn->show();
        } else {
            *(int*)((char*)g_game + 0x1a090) = 0;
            cam_btn->hide();
        }
    }

    MissionUi* sl = *(MissionUi**)((char*)slider + 0x484);
    if (g_mission_clock != 0) {
        float a = *(float*)((char*)sl + 0x168) - *(float*)((char*)sl + 0x16c);
        float b = *(float*)((char*)sl + 0x170) - *(float*)((char*)sl + 0x16c);
        float* arr = (float*)((char*)sl + *(int*)((char*)sl + 0x178) * 4 + 0xf0);
        float v = (a / b) * (arr[0] - *(float*)((char*)sl + 0x17c))
                + ((float)g_mission_clock / arr[0]) * *(float*)((char*)sl + 0x17c) * 0.0005f;
        ((UiD*)(void*)sl)->set_value(v);
        sl->set_key_flags(0x10000);
    }

    if (cam_btn && cam_btn->clear_mask(0x10000)) {
        if (OpenClipboard(0)) {
            char text[0x200];
            mb_snprintf(text, 0x200, "0x%s%s%s%s", keys[0], keys[1], keys[2], keys[3]);
            int len = 0;
            while (text[len])
                len++;
            len++;
            HGLOBAL h = GlobalAlloc(0x2002, len);
            if (h) {
                EmptyClipboard();
                char* dst = (char*)GlobalLock(h);
                char* src = text;
                while (*src) {
                    *dst++ = *src++;
                }
                *dst = 0;
                GlobalUnlock(h);
                SetClipboardData(1, h);
            }
            CloseClipboard();
        }
    }

    char flag = 0;
    for (int i = 0; i < n; i++) {
        ContainerBase* row = deform_rows[i];
        if (row->clear_mask(0x10000)) {
            g_dummy_mat_arr[i] = row->get168();
            SkinSet* set = (SkinSet*)((char*)g_anim.skins + g_anim.cur * 0x1f8);
            ((FaceRace*)set)->relax(i, g_dummy_mat_arr);
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
        f354 = f354 + 1;
        flag = 1;
    }
    if (b_next_face->clear_mask(0x10000)) {
        f35c = f35c + 1;
        flag = 1;
    }
    if (b_prev_beard && b_prev_beard->clear_mask(0x10000)) {
        int m = SKIN_INT(0x88);
        f350 = (f350 + m) % (m + 1);
        flag = 1;
    }
    if (b_prev_hair->clear_mask(0x10000)) {
        int m = SKIN_INT(0x18c);
        f354 = (f354 + m) % (m + 1);
        flag = 1;
    }
    if (b_prev_face->clear_mask(0x10000)) {
        int m = SKIN_INT(0x190);
        f35c = (m - 1 + f35c) % m;
        flag = 1;
    }
    if (sl_age->clear_mask(0x10000))
        flag = 1;
    if (sl_hair_color->clear_mask(0x10000))
        flag = 1;
    if (b_randomize->clear_mask(0x10000)) {
        g_anim.randomise();
        for (int i = 0; i < n; i++)
            deform_rows[i]->set168(g_dummy_mat_arr[i]);
        int m = SKIN_INT(0x18c);
        f354 = (m < 2 ? 0 : rand() % m) + 1;
        f350 = rand();
        f35c = rand();
        f358 = rand();
        sl_hair_color->set168(g_anim.time);
        flag = 1;
    }
    if (b_reset->clear_mask(0x10000)) {
        float* p = g_dummy_mat_arr;
        for (int i = 0x3f; i != 0; i--)
            *p++ = 0.5f;
        SkinSet* set = (SkinSet*)((char*)g_anim.skins + g_anim.cur * 0x1f8);
        ((FaceRace*)set)->relax(-1, g_dummy_mat_arr);
        flag = 1;
        for (int i = 0; i < n; i++)
            deform_rows[i]->set168(g_dummy_mat_arr[i]);
    }

    f350 = f350 % (SKIN_INT(0x88) + 1);
    f354 = f354 % (SKIN_INT(0x18c) + 1);
    f35c = f35c % SKIN_INT(0x190);

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
            const tstring* r1 = g_strtable.lookup(t1);
            const tstring* r2 = g_strtable.lookup(t2);
            // the dialog record's fields (0xa73980+0 is the record pointer: the manager's +0x78 is the screen deque,
            // so writing these through the manager destroys the deque)
            g_main_menu.rec->header = *r2;
            g_main_menu.rec->title = *r1;
            g_main_menu.rec->choice = 0;
            g_main_menu.rec->v50 = 0;
            g_main_menu.rec->open = 0;
        }
        ((WindowMgrW*)(void*)&g_main_menu)->open(0);
        return;
    }

    if (b_done->clear_mask(0x10000)) {
        if (w14 >= 0) {
            char* rec = *(char**)((char*)g_game + 0x13a00);
            int idx = w14;
            *(float*)(rec + idx * 0x508 + 0x3c0) = pose[0];
            *(float*)(rec + idx * 0x508 + 0x3c4) = pose[1];
            *(float*)(rec + idx * 0x508 + 0x3c8) = pose[2];
            *(float*)(rec + idx * 0x508 + 0x3cc) = pose[3];
            *(float*)(rec + idx * 0x508 + 0x3d0) = pose[4];
            *(float*)(rec + idx * 0x508 + 0x3d4) = pose[5];
            *(float*)(rec + idx * 0x508 + 0x3d8) = pose[6];
            *(float*)(rec + idx * 0x508 + 0x3dc) = pose[7];
        }
        int ch;
        if (w08 == 1) {
            w08 = -1;
            *(int*)((char*)g_game + 0x13aec) = 1;
            *(int*)((char*)slot_as<char>(23) + 0x18) = 7;
            ch = 0x17;
        } else {
            ch = w18;
        }
        g_main_menu.set_state(ch);
    }
}
