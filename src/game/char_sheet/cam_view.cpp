#include "./cam_view.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/skin_set.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../main_menu.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4eb050
void CamView::update(float dt)
{
    if (g_a17204)
        pitch = pitch - dt * 1.1f;
    if (g_a17214)
        pitch = dt * 1.1f + pitch;
    if (g_a1720c)
        zoom = zoom - dt * 2.1f;
    if (g_a171fc)
        zoom = dt * 2.1f + zoom;
    if (g_a17454) {
        if (g_a17854 == 0 &&
            g_5d7094 < *(float*)(target + 0xf0) &&
            g_5d7098 > *(float*)(target + 0xe4))
            follow = 1;
        if (g_a17454)
            goto check_follow;
    }
    if (g_a17854)
        follow = 0;
check_follow:
    if (g_a17458 || follow) {
        yaw = (g_a178d4 * dt) * 1.7f + yaw;
        pitch = pitch - (g_a178d8 * dt) * 1.7f;
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
void CamView::update_model(float dt)
{
    int n = *(int*)((char*)g_skins_begin + skin * 0x1f8 + 0x3c) - 1;

    if (g_no_cam && (g_a17148 || g_a17348) && g_a1711c && !g_a1751c) {
        if (*(int*)(g_game + 0x1a090) == 0) {
            *(int*)(g_game + 0x1a090) = 1;
            cam_btn->vb4();
        } else {
            *(int*)(g_game + 0x1a090) = 0;
            cam_btn->vb0();
        }
    }

    if (g_world_time != 0) {
        Ctrl* sl = *(Ctrl**)(*(char**)((char*)this + 0x17c) + 0x484);
        float a = *(float*)((char*)sl + 0x168) - *(float*)((char*)sl + 0x16c);
        float b = *(float*)((char*)sl + 0x170) - *(float*)((char*)sl + 0x16c);
        float* arr = (float*)((char*)sl + *(int*)((char*)sl + 0x178) * 4 + 0xf0);
        float v = (a / b) * (arr[0] - *(float*)((char*)sl + 0x17c))
                + ((float)g_world_time / arr[0]) * *(float*)((char*)sl + 0x17c) * g_scroll;
        ((UiD*)(void*)sl)->set_value(v);
        sl->v48(0x10000);
    }

    if (cam_btn && cam_btn->v44(0x10000)) {
        if (OpenClipboard(0)) {
            char text[0x200];
            mb_snprintf(text, 0x200, "0x%s%s%s%s", buf388, buf3b0, buf3d8, buf400);
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
        Ctrl* row = rows[i];
        if (row->v44(0x10000)) {
            g_dummy_mat_arr[i] = row->v38();
            SkinSet* set = (SkinSet*)((char*)g_skins_begin + g_cur * 0x1f8);
            ((FaceRace*)set)->relax(i, g_dummy_mat_arr);
            for (int j = 0; j < n; j++)
                rows[j]->v34(g_dummy_mat_arr[j]);
            flag = 1;
        }
    }

    if (b_next_beard && b_next_beard->v44(0x10000)) {
        f350 = f350 + 1;
        flag = 1;
    }
    if (b_next_hair->v44(0x10000)) {
        f354 = f354 + 1;
        flag = 1;
    }
    if (b_next_face->v44(0x10000)) {
        f35c = f35c + 1;
        flag = 1;
    }
    if (b_prev_beard && b_prev_beard->v44(0x10000)) {
        int m = SKIN_INT(0x88);
        f350 = (f350 + m) % (m + 1);
        flag = 1;
    }
    if (b_prev_hair->v44(0x10000)) {
        int m = SKIN_INT(0x18c);
        f354 = (f354 + m) % (m + 1);
        flag = 1;
    }
    if (b_prev_face->v44(0x10000)) {
        int m = SKIN_INT(0x190);
        f35c = (m - 1 + f35c) % m;
        flag = 1;
    }
    if (sl_age->v44(0x10000))
        flag = 1;
    if (sl_hair_color->v44(0x10000))
        flag = 1;
    if (b_randomize->v44(0x10000)) {
        g_anim.randomise();
        for (int i = 0; i < n; i++)
            rows[i]->v34(g_dummy_mat_arr[i]);
        int m = SKIN_INT(0x18c);
        f354 = (m < 2 ? 0 : rand() % m) + 1;
        f350 = rand();
        f35c = rand();
        f358 = rand();
        sl_hair_color->v34(g_dummy1);
        flag = 1;
    }
    if (b_reset->v44(0x10000)) {
        float* p = g_dummy_mat_arr;
        for (int i = 0x3f; i != 0; i--)
            *p++ = 0.5f;
        SkinSet* set = (SkinSet*)((char*)g_skins_begin + g_cur * 0x1f8);
        ((FaceRace*)set)->relax(-1, g_dummy_mat_arr);
        flag = 1;
        for (int i = 0; i < n; i++)
            rows[i]->v34(g_dummy_mat_arr[i]);
    }

    f350 = f350 % (SKIN_INT(0x88) + 1);
    f354 = f354 % (SKIN_INT(0x18c) + 1);
    f35c = f35c % SKIN_INT(0x190);

    if (flag)
        ((FaceGen*)(void*)this)->refresh();

    update(dt);
    world->step(dt);
    ((FaceGen*)(void*)this)->update_view(*(int*)&dt);

    if (state == 0) {
        state = 1;
        tstring t1("ui_tutorial_face_generator");
        tstring t2("ui_tutorial");
        const tstring* r1 = g_strtable.lookup(t1);
        const tstring* r2 = g_strtable.lookup(t2);
        g_main_menu.header = *r2;
        g_main_menu.title = *r1;
        g_main_menu.choice = 0;
        g_main_menu.v50 = 0;
        g_main_menu.open = 0;
        ((WindowMgrW*)(void*)&g_main_menu)->open(0);
        return;
    }

    if (b_done->v44(0x10000)) {
        if (troop >= 0) {
            char* rec = *(char**)(g_game + 0x13a00);
            int idx = troop;
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
        if (state == 1) {
            state = -1;
            *(int*)(g_game + 0x13aec) = 1;
            *(int*)(0xa739dc + 0x18) = 7;
            ch = 0x17;
        } else {
            ch = choice;
        }
        g_main_menu.set_state(ch);
    }
}
