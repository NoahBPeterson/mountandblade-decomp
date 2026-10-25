//-----------------------------------------------------------------------------
// File: mission_5.cpp  (0x525160..0x52cf60)
//
// Desc: the game-loop event handler and the dialogue / conversation update (0x525160, 0x5257f0, speaker and listener
//       finders), the mission screen's per-frame tick (0x5261e0) with the scene save path (.sco writer, 0x527c90..0x528470),
//       the view controller (0x5284a0), the mission HUD (markers, party heroes, 0x529130), the agent bookkeeping helpers
//       (0x529840..0x529d80), the interaction probe (0x529ec0), the order markers and the order menu (0x52ae70..0x52b3c0),
//       Mission::order_agent / update_ui / update_cursor / find_position (0x52b5c0..0x52bdb0), the editor-side terrain
//       update and the scene brush (0x52bf40, 0x52c2b0) and the small grid element helpers (0x52ce00..0x52cef0).
//       Names provisional. "T regime" region (docs/tu_map.md): the file keeps the binary order; every function that
//       follows its caller is a separate (noinline) function in the original. Flags: /O2 /MT /Gy /TP.
//
// Boundary: starts at 0x525160 (Game::handle_event, a class change after the record readers of game_2.cpp); ends where the
//       map editor starts (0x52cf60, class Mesh | Elem). The map's boundary 0x5249f0 is not kept (see game_2.cpp).
//-----------------------------------------------------------------------------
#include "./mission_5.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../../rgl/mesh.h"

// ======================================================= slice s00525160: functions
#undef g_mgr
#define g_mgr       (*(Mgr_s25160*)0x5d7080)
#undef g_dlg
#define g_dlg       (*(Dlg_s25160*)0xa73980)
#undef g_513710
#define g_513710    (*(Mgr513710_s25160*)0x5e291c)
#undef g_big
#define g_big       (*(Big5e292c_s25160**)0x5e292c)
// @ 0x525160
void Game_s25160::handle_event(int code)
{
    g_mgr.update();

    if (*(int*)0x5d8790 != 0)
    {
        // camera/sky interpolation over the current site's passage table
        char* o = *(char**)(f188 + 0x484);
        float* pa = (float*)(o + *(int*)(o + 0x178) * 4 + 0xf0);
        float v = (*(float*)(o + 0x168) - *(float*)(o + 0x16c))
                / (*(float*)(o + 0x170) - *(float*)(o + 0x16c))
                * (*pa - *(float*)(o + 0x17c))
                + (float)*(int*)0x5d8790 / *pa * *(float*)(o + 0x17c)
                  * *(float*)0x590d7c;
        set_value(v);
        ((VObj_s25160*)o)->v18(0x10000);
    }

    char* b = *(char**)0x5e2930;
    if (f1204 == *(int*)(b + 0x8c) || *(int*)(b + 0x1a23c) == 0)
        goto hide;
    if (*(float*)0x5d7094 >= *(float*)(f30 + 0xf0))
        goto hide;
    if (*(float*)(f30 + 0xe4) + *(float*)0x549d7c >= *(float*)0x5d7098)
        goto hide;
    {
        f11e0->v45();
        int s = *(int*)(b + 0x1a238);
        if (s < -0x65 || s > 0x65)
        {
            f11e8->v44();
            f11e4->v44();
        }
        else
        {
            float pair[2];
            pair[0] = (float)(s + 0x64) * *(float*)0x59c268 + *(float*)0x590cd0;
            pair[1] = 0.02f;
            f11e4->v22(pair);
            f11e8->v45();
            f11e4->v45();
        }
    }
    goto shown;
hide:
    f11e0->v44();
shown:

    for (int i = 0; i < 5; ++i)
    {
        b = *(char**)0x5e2930;
        if (*(char*)(b + 0x1a290 + i) == 0)
            continue;
        int l1;
        int l2;
        f11ec[i]->v41((void*)(b + 0x1a240 + i * 0x10), &l1);
        f11ec[i]->v37(&l2);
        *(char*)(b + 0x1a290 + i) = 0;
    }

    *(char*)(g_big + 0x6554) = 1;

    int res = -1;
    if (*(int*)0x8ed50dc >= 0)
    {
        *(int*)(*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)0x8ed50dc >> 4] + 0x31c)
            = *(int*)((char*)g_big + 0x5c);
        *(int*)(*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)0x8ed50dc >> 4] + 0x533c) = 2;
        *(int*)(*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)0x8ed50dc >> 4] + 0x5338) = 0;
        ((Chunk_s25160*)*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)0x8ed50dc >> 4])
            ->m_4464d0(0);
    }

    g_big->m_483f60(code, 0);

    {
        if (*(int*)((char*)g_big + 0x5c) >= 0)
        {
            ((Chunk_s25160*)*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)((char*)g_big + 0x5c) >> 4])
                ->m_4f8bc0();
            *(int*)(*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)((char*)g_big + 0x5c) >> 4] + 0x55d0) = 0;
        }
    }

    if (f1190 > 1)
    {
        int i;
        for (i = 0; i < 0x400; ++i)
        {
            if (!arr190[i]->v17(0x10000))
                continue;
            if (i >= f1190)
                continue;
            refresh(i, &res);
        }
    }
    else if (*(int*)0xa17454 == 0 && *(int*)0xa17854 != 0)
    {
        refresh(-2, &res);
    }

    update();

    int v = res;

    if (v == 1)
        goto case1;
    if (*(int*)0x5a9fb0 != 0 && *(int*)0xa17110 != 0 && *(int*)0xa17510 == 0)
        goto case1;

    if (v == 0x10)
    {
        char* t = *(char**)0xa739e8;
        *(int*)(t + 0x18) = 0x10;
        g_dlg.open(0x1a);
        return;
    }

    if (v == 4)
    {
        char* b2 = *(char**)0x5e2930;
        char* rec = *(char**)(b2 + 0x13a00) + *(int*)0x8ed50d4 * 0x508;
        int save = *(int*)(rec + 0x34);
        int flags = *(int*)(rec + 0x30);
        (void)save;
        if (!(flags & 0x10) || (flags & 0x1000))
        {
            for (int i = 0; i < 0xa; ++i)
                ((Troop_s25160*)rec)->m_4630d0(i);
        }
        char* t = *(char**)0xa739a0;
        *(int*)(t + 0x18) = 0x10;
        *(int*)(t + 0x14) = *(int*)0x8ed50d4;
        *(int*)(t + 4) = 2;
        g_dlg.open(8);
        return;
    }

    if (v == 3)
        f1200 = *(int*)0x8ed50d4;
    if (v == 2 || v == 3)
    {
        char* t = *(char**)0xa739a0;
        *(int*)(t + 0x18) = 0x10;
        int a = *(int*)(*(char**)0x5e2930 + 0x13af0);
        if (a == 0)
            a = *(int*)0x8ed50d4;
        *(int*)(t + 0x14) = a;
        *(int*)(t + 4) = ((v != 3) ? 0 : 3) + 1;
        g_dlg.open(8);
        return;
    }

    if (v == 5 || v == 7 || v == 6 || v == 9 || v == 0xa)
    {
        char* t = *(char**)0xa739b0;
        *(int*)(t + 0x18) = 0x10;
        if (v == 5 || v == 6 || v == 7)
        {
            *(int*)(t + 0x10) = *(int*)0x8ed50d8;
        }
        else
        {
            *(int*)(t + 0x10) = -1;
            *(int*)(t + 0x14) = *(int*)0x8ed50d4;
        }
        if (v == 5)
        {
            *(int*)(t + 4) = 1;
            g_dlg.open(0xc);
            return;
        }
        if (v == 7)
        {
            *(int*)(t + 4) = 7;
            g_dlg.open(0xc);
            return;
        }
        if (v == 6)
        {
            *(int*)(t + 4) = 6;
            g_dlg.open(0xc);
            return;
        }
        if (v == 9)
        {
            *(int*)(t + 4) = 2;
            g_dlg.open(0xc);
            return;
        }
        *(int*)(t + 4) = 3;
        g_dlg.open(0xc);
        return;
    }

    if (v == 0xb || v == 0xc)
    {
        char* t = *(char**)0xa739bc;
        *(int*)(t + 0x18) = 0x10;
        *(int*)(t + 0x14) = *(int*)0x8ed50d4;
        *(int*)(t + 4) = (v != 0xb) + 1;
        ((VObj_s25160*)t)->v4();
        g_dlg.open(0xf);
        return;
    }

    return;

case1:
    {
        int x = f18;
        if (x == 0x64)
            g_513710.m(&code);
        else
            g_dlg.open(x);
    }
}

// ======================================================= slice s005257f0: functions
#undef g_mission_s257f0
#define g_mission_s257f0 (*(Mission_s257f0**)&g_mission)
#undef g_game_s257f0
#define g_game_s257f0 (*(Game_s257f0**)&g_game)
#undef g_n_dialogs_s257f0
#define g_n_dialogs_s257f0 (*(DialogMgr_s257f0*)g_dialogs)
#undef AGENT
#define AGENT(id) (g_mission_s257f0->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))
// @ 0x526070 -------------------------------------------------------------------------------------------
int find_speaker_agent()
{
    unsigned found = (unsigned)-1;
    int speaker = g_n_dialogs_s257f0.speaker();
    if ((g_game_s257f0->chars[speaker].flags & 0x10) || (*(int*)((char*)&g_dialogs + 0x14)) == -1) {
        AgentTable_s257f0* t = &g_mission_s257f0->agents;
        Agent_s257f0** pages = t->pages;
        int any = 0;
        for (unsigned i = t->first_used(); i < (unsigned)t->count; t->next_used(&i)) {
            if ((pages[i >> 4] + (i & 0xf))->troop == speaker) {
                found = i;
                any = 1;
            }
        }
        if (any)
            return found;
    }
    return (*(int*)((char*)&g_dialogs + 0x14));
}

// @ 0x5257f0 -------------------------------------------------------------------------------------------
void ViewObj_s257f0::update()
{
    unsigned target = find_speaker_agent();
    int listener = find_listener_agent();
    if (target == (unsigned)-1)
        return;

    Agent_s257f0* a = AGENT(target);
    vector4 G = a->body->m.o;
    vector4 F;
    G.z = a->speed_scale() + G.z;

    if (target == (unsigned)(*(int*)((char*)&g_dialogs + 0x14))) {
        Agent_s257f0* sel = AGENT(g_mission_s257f0->selected);
        vector4 dir;
        dir.x = sel->pos.x - a->pos.x;
        dir.y = sel->pos.y - a->pos.y;
        dir.z = sel->pos.z - a->pos.z;
        float len = dir.normalize();
        if (len > 1.0f)
            len = 1.0f;
        G.x = dir.x * len + G.x;
        G.y = dir.y * len + G.y;
        G.z = dir.z * len + G.z;
    } else {
        F = a->body->m.f;
        G.x = F.x + G.x;
        G.y = F.y + G.y;
        G.z = F.z + G.z;
        if (listener != -1) {
            Agent_s257f0* la = AGENT(listener);
            vector4 A = la->body->m.o;
            A.z = (la->speed_scale() - 0.1f) + A.z;
            A.x = A.x + 0.01f;
            A.y = A.y + 0.01f;
            A.x = A.x - G.x;
            A.y = A.y - G.y;
            A.z = A.z - G.z;
            A.normalize();
            float t = -(F.x * A.x + F.y * A.y + F.z * A.z);
            if (t < 1.4f) {
                vector4 B;
                B.x = A.x + F.x * t;
                B.y = A.y + F.y * t;
                B.z = A.z + F.z * t;
                B.normalize();
                vector4 Q;
                Q.x = G.x - F.x;
                Q.y = G.y - F.y;
                Q.z = G.z - F.z;
                vector4 half;
                half.x = B.x * 0.5f;
                half.y = B.y * 0.5f;
                half.z = B.z * 0.5f;
                ((vec3_s257f0*)&G)->sub_assign(*(const vec3_s257f0*)&half);
                F.x = G.x - Q.x;
                F.y = G.y - Q.y;
                F.z = G.z - Q.z;
                F.normalize();
            }
        }
    }

    // orthonormal frame with up = (0,0,1); the original inlined cross(), so spell it out
    vector4 up(0.0f, 0.0f, 1.0f);
    vector4 D;
    D.x = up.y * F.z - up.z * F.y;
    D.y = up.z * F.x - up.x * F.z;
    D.z = up.x * F.y - up.y * F.x;
    D.normalize();
    vector4 E;
    E.x = F.y * D.z - F.z * D.y;
    E.y = F.z * D.x - F.x * D.z;
    E.z = F.x * D.y - F.y * D.x;

    matrix* m = &this->m;
    if (sqrtf((m->o.x - G.x) * (m->o.x - G.x) + (m->o.y - G.y) * (m->o.y - G.y) +
              (m->o.z - G.z) * (m->o.z - G.z)) > 0.3f ||
        sqrtf((m->u.x - F.x) * (m->u.x - F.x) + (m->u.y - F.y) * (m->u.y - F.y) +
              (m->u.z - F.z) * (m->u.z - F.z)) > 0.1f) {
        blend = blend - g_look_rate * g_step_tbl[2];
    }

    float t = clamp01_s257f0(1.0f - blend);
    if (!started) {
        started = 1;
        t = t * g_step_tbl[2] * 1.5f;
    }
    t = clamp01_s257f0(t);
    float inv = 1.0f - t;

    float d0 = D.x * t, d1 = D.y * t, d2 = D.z * t;
    m->s.x = inv * m->s.x + d0;
    m->s.y = inv * m->s.y + d1;
    m->s.z = inv * m->s.z + d2;
    float e0 = E.x * t, e1 = E.y * t, e2 = E.z * t;
    m->f.x = inv * m->f.x + e0;
    m->f.y = inv * m->f.y + e1;
    m->f.z = inv * m->f.z + e2;
    float f0 = F.x * t, f1 = F.y * t, f2 = F.z * t;
    m->u.x = inv * m->u.x + f0;
    m->u.y = inv * m->u.y + f1;
    m->u.z = inv * m->u.z + f2;
    m->o.x = inv * m->o.x + G.x * t;
    m->o.y = inv * m->o.y + G.y * t;
    m->o.z = inv * m->o.z + G.z * t;

    m->orthonormalize();
    m_fc = *m;
    float far_plane = 2.0f;
    g_mission_s257f0->camera_to(m, far_plane);

    struct Pose { char b[0x120]; };
    *(Pose*)((char*)f30 + 0x1ac) = *(Pose*)((char*)this + 0x34);

    blend = blend + (g_step_tbl[2] + g_step_tbl[2]);
    blend = clamp01_s257f0(blend);
}

// ======================================================= slice s00526130: functions
#undef g_mission_s26130
#define g_mission_s26130 (*(Mission_s26130**)&g_mission)
#undef g_game_s26130
#define g_game_s26130 (*(Game_s26130**)&g_game)
#undef g_n_dialogs_s26130
#define g_n_dialogs_s26130 (*(DialogMgr_s26130*)g_dialogs)
// @ 0x526130 -------------------------------------------------------------------------------------------
int find_listener_agent()
{
    int found = -1;
    int listener = g_n_dialogs_s26130.listener();
    if ((g_game_s26130->chars[listener].flags & 0x10) | ((*(int*)((char*)&g_dialogs + 0x14)) == -1)) {
        AgentTable_s26130* t = &g_mission_s26130->agents;
        unsigned i = t->first_used();
        Agent_s26130** pages = t->pages;
        while (i < (unsigned)t->count) {
            if ((pages[i >> 4] + (i & 0xf))->troop == listener)
                found = i;
            t->next_used(&i);
        }
    }
    return found;
}

// ======================================================= slice s005261e0: functions
#undef g_strtable_s261e0
#define g_strtable_s261e0 (*(StrTable_s261e0*)g_strtable)
#undef g_gui_s261e0
#define g_gui_s261e0 (*(MsgLog_s261e0*)g_gui)
#undef g_keytable_s261e0
#define g_keytable_s261e0 (*(KeyTable_s261e0*)g_game_keys)
#undef g_keyready_s261e0
#define g_keyready_s261e0 (*(KeyReady_s261e0*)g_keyboard)
#undef g_mission_s261e0
#define g_mission_s261e0 (*(Mission_s261e0**)&g_mission)
#undef g_game_s261e0
#define g_game_s261e0 (*(GameData_s261e0**)&g_game)
#undef g_main_menu_s261e0
#define g_main_menu_s261e0 (*(MainMenu_s261e0*)g_windows)
#undef g_w_a739a0_s261e0
#define g_w_a739a0_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x20)))
#undef g_w_a739bc_s261e0
#define g_w_a739bc_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x3c)))
#undef g_w_a739b0_s261e0
#define g_w_a739b0_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x30)))
#undef g_w_a739b4_s261e0
#define g_w_a739b4_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x34)))
#undef g_w_a739e8_s261e0
#define g_w_a739e8_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x68)))
#undef g_w_a739e4_s261e0
#define g_w_a739e4_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x64)))
#undef g_w_a73994_s261e0
#define g_w_a73994_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x14)))
#undef g_w_a739c8_s261e0
#define g_w_a739c8_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x48)))
#undef g_w_a739c0_s261e0
#define g_w_a739c0_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x40)))
#undef g_flags_s261e0
#define g_flags_s261e0 (*(Flags5e291c_s261e0*)g_map_flags)
#undef g_a73300_s261e0
#define g_a73300_s261e0 (*(Obj73300_s261e0*)g_mission_ctx)
#undef g_deque_s261e0
#define g_deque_s261e0 (*(IntDeque_s261e0*)((char*)((char*)&g_windows + 0x78)))
#undef g_dialogmgr
#define g_dialogmgr (*(DialogMgr_s261e0*)0x8ed50c8)
#undef g_clamp_max
#define g_clamp_max (*(int*)0x8ee3ac4)                  // 0x8ee3ac4
// @ 0x5261e0
void Screen_s261e0::tick(float dt)
{
    float sdt;              // local_438
    bool  b6;               // local at esp+0x12
    unsigned char flag13;   // esp+0x13 ("value valid")
    int   val14;            // esp+0x14 (dword; compared as int in the original)

    sdt = dt;

    if (g_mt_inited)
        EnterCriticalSection(&g_mt_cs);

    // ---- key toggles (slow-motion / pause) ----------------------------------------------------------
    if ((*(int*)((char*)&g_cfg + 0x24))) {
        if ((((int*)((char*)&g_keyboard + 0xc))[29] || ((int*)((char*)&g_keyboard + 0xc))[157]) && ((int*)((char*)&g_keyboard + 0xc))[67] && !((int*)((char*)&g_keyboard + 0x40c))[67])
            f2e1 = (unsigned char)!f2e1;
        if ((*(int*)((char*)&g_cfg + 0x24)) && (((int*)((char*)&g_keyboard + 0xc))[29] || ((int*)((char*)&g_keyboard + 0xc))[157]) && ((int*)((char*)&g_keyboard + 0xc))[87] && !((int*)((char*)&g_keyboard + 0x40c))[87])
            f2e0 = (unsigned char)!f2e0;
    }
    if (f2e0)
        sdt = 0.0f;
    else if (f2e1)
        sdt = dt * g_f54a6e4;

    // ---- scene time-scale blend --------------------------------------------------------------------
    if ((*(int*)((char*)&g_cfg + 0x24))) {
        Mission_s261e0* m = g_mission_s261e0;
        if (m->f6810 > 0.0f) {
            __int64 d = g_ticks[m->f6800] - *(__int64*)((char*)m + 0x6808);
            float r = (float)d * g_f54a6d0 / m->f6810;
            if (r >= 1.0f) {
                *(int*)&m->f67f4 = *(int*)&m->f67f8;
                sdt = sdt * m->f67f4;
                m->f6810 = 0.0f;
            } else {
                sdt = (r * m->f67f8 + (1.0f - r) * m->f67f4) * sdt;
            }
        } else {
            sdt = sdt * m->f67f4;
        }
    }

    // ---- renderer "ready" rate keys ----------------------------------------------------------------
    {
        char c7 = g_keyready_s261e0.is_ready(0xd0);
        unsigned char c8 = g_a17908;
        if (c7 || ((*(int*)((char*)&g_keyboard + 0x34c)) != 0 && c8 != 0))
            g_vert_budget -= 5;
        c7 = g_keyready_s261e0.is_ready(0xc8);
        if (c7 || ((*(int*)((char*)&g_keyboard + 0x32c)) != 0 && c8 != 0))
            g_vert_budget += 5;
    }

    update(sdt);
    g_mission_s261e0->b6554 = 0;
    g_mission_s261e0->frame(sdt, 1);
    if (g_mission_s261e0->local_id >= 0) {
        update_status();
        update_marks();
    }
    update_camera(sdt);
    g_gui_s261e0.flush();

    // ---- no-camera position save -------------------------------------------------------------------
    if ((*(int*)((char*)&g_cfg + 0x18)) && (((int*)((char*)&g_keyboard + 0xc))[29] || ((int*)((char*)&g_keyboard + 0xc))[157]) && ((int*)((char*)&g_keyboard + 0xc))[18] && !((int*)((char*)&g_keyboard + 0x40c))[18]) {
        if (g_game_s261e0->f1a090 != 0) {
            restore_defaults();
            if (g_mt_inited) {
                LeaveCriticalSection(&g_mt_cs);
                SetEvent(g_mt_event);
            }
            return;
        }
        if (g_mission_s261e0->local_id < 0) {
            *(int*)((char*)g_game_s261e0 + 0x1a1a4) = *(int*)((char*)this + 0x218);
            *(int*)((char*)g_game_s261e0 + 0x1a1a8) = *(int*)((char*)this + 0x21c);
            *(int*)((char*)g_game_s261e0 + 0x1a1ac) = *(int*)((char*)this + 0x220);
            *(int*)((char*)g_game_s261e0 + 0x1a1b0) = *(int*)((char*)this + 0x224);
        } else {
            char* p = (char*)page_agent_s261e0(g_mission_s261e0->local_id) + 0x50;
            *(int*)((char*)g_game_s261e0 + 0x1a1a4) = *(int*)p;
            *(int*)((char*)g_game_s261e0 + 0x1a1a8) = *(int*)(p + 4);
            *(int*)((char*)g_game_s261e0 + 0x1a1ac) = *(int*)(p + 8);
            *(int*)((char*)g_game_s261e0 + 0x1a1b0) = *(int*)(p + 12);
            *(float*)((char*)g_game_s261e0 + 0x1a1ac) += g_f54b8b0;
        }
        g_game_s261e0->f139e4 = 0;
        g_game_s261e0->f1a090 = 1;
        *(int*)((char*)g_game_s261e0 + 0x1a20c) = 0;
        g_game_s261e0->f1a1b4 = -1;
        ((VectorInt_s261e0*)((char*)g_game_s261e0 + 0x1a1b8))->resize(0, 0);
        *(int*)((char*)g_game_s261e0 + 0x1a1cc) = 0;
        init_order_markers();
    }
    if (g_game_s261e0->f1a090 != 0) {
        update_ui();
        if (g_mt_inited) {
            LeaveCriticalSection(&g_mt_cs);
            SetEvent(g_mt_event);
        }
        return;
    }

    // ---- "mission started" pending item ------------------------------------------------------------
    if (g_mission_s261e0->b6338 == 0) {
        b6 = true;
    } else {
        float e = ((MissionSub_s261e0*)((char*)g_mission_s261e0 + 0x6328))->elapsed();
        if (!(0.0f <= e)) {
            b6 = true;
        } else {
            int u14 = g_deque_s261e0.off + g_deque_s261e0.size - 1;
            unsigned page = (unsigned)u14 >> 2;
            unsigned sub = (unsigned)u14 & 3;
            if ((unsigned)g_deque_s261e0.mapsize <= page)
                page -= (unsigned)g_deque_s261e0.mapsize;
            int* blk = (int*)g_deque_s261e0.map[page];
            b6 = (blk[sub] == 0);
        }
    }
    if (g_pending_action != -1) {
        if (g_pending_action == 0) {
            ((MissionSub_s261e0*)((char*)g_mission_s261e0 + 0x6698))->build_geometry();
            g_mission_s261e0->save_scene_data();
            g_mission_s261e0->fn_529d80();
            g_game_s261e0->save_scene(g_mission_s261e0->f5df4);
        } else {
            b6 = false;
        }
        g_pending_action = -1;
    }

    // ---- mission-editor key block ------------------------------------------------------------------
    if (g_mission_s261e0->local_id >= 0) {
        if (((int*)((char*)&g_keyboard + 0xc))[29] == 0 && ((int*)((char*)&g_keyboard + 0xc))[157] == 0) {
            if (g_keytable_s261e0.clicked(0x15)) {
                f2a0 = 0;
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x16)) {
                f2a0 = 1;
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x17)) {
                f2a0 = 2;
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x1a)) {
                f2a0 = 5;
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x1b)) {
                f2a0 = 6;
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x1c)) {
                f2a0 = 7;
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x1d)) {
                f2a0 = 8;
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x18)) {
                if (g_mission_s261e0->f6674 == 0) {
                    f2a0 = 4;
                    g_mission_s261e0->f6674 = 1;
                } else {
                    f2a0 = 3;
                    g_mission_s261e0->f6674 = 0;
                }
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x1e)) {
                if (g_mission_s261e0->f666c == 0) {
                    f2a0 = 9;
                    g_mission_s261e0->f666c = 1;
                } else {
                    f2a0 = 10;
                    g_mission_s261e0->f666c = 0;
                }
                order_tick();
            } else if (g_keytable_s261e0.clicked(0x19)) {
                if (g_mission_s261e0->f6670 == 0) {
                    f2a0 = 0xc;
                    g_mission_s261e0->f6670 = 1;
                } else {
                    f2a0 = 0xd;
                    g_mission_s261e0->f6670 = 0;
                }
                order_tick();
            }
        }
        if (g_keytable_s261e0.clicked(0x1f) || g_keytable_s261e0.clicked(0x20) || g_keytable_s261e0.clicked(0x21) ||
            g_keytable_s261e0.clicked(0x22) || g_keytable_s261e0.clicked(0x23)) {
            bool b3 = (((int*)((char*)&g_keyboard + 0xc))[29] || ((int*)((char*)&g_keyboard + 0xc))[157] || ((int*)((char*)&g_keyboard + 0xc))[42] || ((int*)((char*)&g_keyboard + 0xc))[54]);
            if (b3)
                *(int*)((char*)g_game_s261e0 + 0x1a1d4) = 1;
            else
                *(int*)((char*)g_game_s261e0 + 0x1a1d4) = 0;

            int kind = page_agent_s261e0(g_mission_s261e0->local_id)->f1c4;
            if (kind < 0 || 8 <= kind)
                kind = 7;
            char* base = (char*)g_mission_s261e0 + 0x527c + kind * 0x16c;

            if (g_keytable_s261e0.clicked(0x23)) {
                bool changed = false;
                for (int i = 0; i < 3; i++) {
                    int* p = (int*)(base + 0x4c + i * 0x6c);
                    if (*p == 0) {
                        *p = 1;
                        changed = true;
                    } else {
                        *p = 0;
                    }
                }
                if (!changed) {
                    for (int i = 0; i < 3; i++)
                        *(int*)(base + 0x4c + i * 0x6c) = 1;
                }
            } else if (g_keytable_s261e0.clicked(0x1f)) {
                for (int i = 0; i < 3; i++)
                    *(int*)(base + 0x4c + i * 0x6c) = 1;
            } else {
                if (!b3) {
                    for (int i = 0; i < 3; i++)
                        *(int*)(base + 0x4c + i * 0x6c) = 0;
                }
                int idx = 0;
                if (g_keytable_s261e0.clicked(0x20))
                    idx = 0;
                else if (g_keytable_s261e0.clicked(0x21))
                    idx = 1;
                else if (g_keytable_s261e0.clicked(0x22))
                    idx = 2;
                *(int*)(base + 0x4c + idx * 0x6c) = 1;
            }

            bool all = true;
            for (int i = 0; i < 3; i++) {
                if (*(int*)(base + 0x4c + i * 0x6c) == 0)
                    all = false;
            }
            if (all) {
                g_gui_s261e0.add(*g_strtable_s261e0.lookup(&tstring("ui_everyone_hear_me")), 0xffddee88);
            } else {
                char label[512];
                ((LabelSub_s261e0*)base)->make_label(label);
                const char* fmt = g_strtable_s261e0.lookup(&tstring("ui_str_hear_me"))->c_str();
                char buf[512];
                sprintf(buf, fmt, label, buf);
                g_gui_s261e0.add(tstring(buf), 0xffddee88);
            }
        }
    }

    // ---- region-change / texture timers ------------------------------------------------------------
    {
        __int64 d = g_ticks[f2a8] - f2b0;
        if ((float)d * g_f54a6d0 > g_f590b38 && g_mission_s261e0->local_id >= 0) {
            f29c = 0;
            __int64 d2 = g_ticks[f2b8] - f2c0;
            if ((float)d2 * g_f54a6d0 > g_f549d7c) {
                f_529ec0();
                f2c0 = g_ticks[f2b8];
            }
            if (f2c8 == -1) {
                g_gui_s261e0.set_o568(0, 1);
                float v[2];
                v[0] = 0.5f;
                v[1] = 0.5f;
                g_gui_s261e0.set_view(v);
            }
        }
    }

    // ---- item-use keys + sound slots ---------------------------------------------------------------
    flag13 = 0;
    val14 = -1;
    if (g_mission_s261e0->local_id >= 0) {
        if (any_pressed_s261e0(g_keys_7350, 2) && f2c8 == 4 && f2d4 != 0) {
            int v = *(int*)((char*)g_game_s261e0->f13a50 + 0x7c +
                            (g_mission_s261e0->f5df4 * 0x125 + f2cc) * 4);
            if (v == 100000)
                b6 = false;
            else {
                g_game_s261e0->f13af4 = v;
                g_game_s261e0->f13af8 = 0;
            }
        } else if (any_pressed_s261e0(g_keys_7350, 2) && f2c8 == 1 && f2d4 != 0) {
            g_mission_s261e0->f6620 = f2cc;
        } else if (any_pressed_s261e0(g_keys_7350, 2) && f2c8 == 7 && f2d4 != 0) {
            int off = f2cc * 0x1c8;
            int pair[2];
            pair[0] = *(int*)((char*)g_mission_s261e0->f668c + off + 0x18c);
            pair[1] = *(int*)((char*)g_mission_s261e0->f668c + off + 0x1a0);
            if (order_agent(g_mission_s261e0->local_id, pair)) {
                g_mission_s261e0->release_item(f2cc);
                update_equipment();
            }
        } else if (any_pressed_s261e0(g_keys_7350, 2) && f2c8 == 5 && f2d4 != 0) {
            int v = *(int*)((char*)g_mission_s261e0->f668c + f2cc * 0x1c8 + 0x1a0);
            if (v < *(int*)((char*)g_game_s261e0->f13a50 + g_mission_s261e0->f5df4 * 0x494 + 0x100) && 0 <= v)
                val14 = *(int*)((char*)g_game_s261e0->f13a50 + 0xc0 +
                                (g_mission_s261e0->f5df4 * 0x125 + v) * 4);
            else
                val14 = -1;
            flag13 = 1;
        } else if (any_pressed_s261e0(g_keys_7350, 2) && f2c8 == 6 && f2d4 != 0) {
            int idx = *(int*)((char*)g_mission_s261e0->f668c + f2cc * 0x1c8 + 0x1a0);
            g_game_s261e0->f13af8 = idx;
            char* obj = *(char**)((char*)g_mission_templates + (*(int*)((char*)&g_mission_ctx + 0x2c4)) * 0x60 + 0x58);
            ((EmitSub_s261e0*)(obj + *(int*)((char*)g_mission_s261e0->f668c + f2cc * 0x1c8 + 0x1a4) * 0x68 + 0x4c))
                ->play(0x10);
        }

        if (any_pressed_s261e0(g_keys_73b0, 2)) {
            (*(int*)((char*)&g_script + 0x18)) = 0;
            (*(int*)((char*)&g_script + 0x1c)) = 0;
            int* slots = rec_base_s261e0(g_mission_s261e0->cur_rec);
            for (int i = 0; i < *slots; i++)
                ((SoundSlot_s261e0*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-0x16);
            if ((*(int*)((char*)&g_script + 0x18)) != 0 || (*(int*)((char*)&g_script + 0x1c)) != 0)
                flag13 = 1;
        }
    }

    if ((*(int*)((char*)&g_cfg + 0x18)) && ((int*)((char*)&g_keyboard + 0xc))[56] && ((int*)((char*)&g_keyboard + 0xc))[38] == 0 && ((int*)((char*)&g_keyboard + 0x40c))[38] != 0)
        light_entities((char*)g_mission_s261e0 + 0x6340, g_mission_s261e0->f633c);

    if (f2d8 == 0) {
        b6 = false;
    } else if (f2dc == 0) {
        int* slots = rec_base_s261e0(g_mission_s261e0->cur_rec);
        for (int i = 0; i < *slots; i++)
            ((SoundSlot_s261e0*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-0x1e);
        b6 = false;
    }

    if (any_pressed_s261e0(g_keys_7388, 2) && ((int*)((char*)&g_keyboard + 0xc))[56] == 0 && ((int*)((char*)&g_keyboard + 0xc))[184] == 0) {
        (*(int*)((char*)&g_script + 0x18)) = 0;
        (*(int*)((char*)&g_script + 0x1c)) = 0;
        ((SoundBank_s261e0*)rec_base_s261e0(g_mission_s261e0->cur_rec))->update_all(-0x15);
        if ((*(int*)((char*)&g_script + 0x18)) != 0 || (*(int*)((char*)&g_script + 0x1c)) != 0)
            b6 = false;
    }

    if (g_game_s261e0->f1a208 >= 0) {
        int v = g_game_s261e0->f1a208;
        (*(__int64*)((char*)&g_script + 0x8)) = v;
        int* slots = rec_base_s261e0(g_mission_s261e0->cur_rec);
        for (int i = 0; i < *slots; i++)
            ((SoundSlot_s261e0*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-8);
        g_game_s261e0->f1a208 = -1;
    }

    // ---- "leave area" / agent selection ------------------------------------------------------------
    if ((g_mission_s261e0->f6528 & 1) == 0) {
        int flags20 = *(int*)((char*)g_mission_templates + g_mission_s261e0->cur_rec * 0x58 + 0x20);
        if (any_pressed_s261e0(g_keys_7350, 2) && f2c8 == 8 &&
            (((flags20 >> 1) & 1) == 0 || g_mission_s261e0->f6524 > 0)) {
            f2dc = 0;
        } else if (((flags20 >> 1) & 1) != 0 && g_mission_s261e0->local_id >= 0 &&
                   g_keytable_s261e0.clicked(4) && f2c8 == 8) {
            AgentTable_s261e0* table = (AgentTable_s261e0*)((char*)g_mission_s261e0 + 4);
            float best = 10000.0f;
            unsigned id = (unsigned)table->first_used();
            Agent_s261e0* player = page_agent_s261e0(g_mission_s261e0->local_id);
            while (id < (unsigned)table->count) {
                Agent_s261e0* a = page_agent_s261e0((int)id);
                if (a->f14 == 1 && a->f10 == 0 && player->is_enemy(a)) {
                    float dx = a->px - player->px;
                    float dy = a->py - player->py;
                    float dz = a->pz - player->pz;
                    float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
                    if (dist < best)
                        best = dist;
                }
                table->next_used(&id);
            }
            if (g_keytable_s261e0.clicked(4) && f2c8 == 8) {
                if (best > 8.0f) {
                    f2dc = -1;
                    g_main_menu_s261e0.setup2(
                        g_strtable_s261e0.lookup(&tstring("ui_question_leave_area")),
                        g_strtable_s261e0.lookup(&tstring("ui_yes")),
                        g_strtable_s261e0.lookup(&tstring("ui_no")),
                        &f2dc);
                    g_main_menu_s261e0.open(0);
                } else {
                    g_gui_s261e0.add(*g_strtable_s261e0.lookup(&tstring("ui_cant_retreat_there_are_enemies_nearby")),
                              0xffff3344);
                }
            }
        }
    } else {
        if (any_pressed_s261e0(g_keys_7350, 2) && f2c8 == 8)
            f2dc = 0;
    }

    // ---- conversation / mission setup --------------------------------------------------------------
    if (g_game_s261e0->fc8 == 0xf) {
        g_game_s261e0->fc8 = 0;
        f324 = g_game_s261e0->f13b04;
    }
    if (f324 >= 0 && g_presentation == 0) {
        ((Src_s261e0*)((char*)this + 0x30))->init(f2c, f324);
        if ((flags44 & 1) == 0) {
            if (g_render.dev != 0)
                ((void (__cdecl*)(void*, int))*(void**)(*(int*)g_render.dev + 0x30))(g_render.dev, 1);
            (*(unsigned char*)((char*)&g_gui + 0x560)) = 1;
        }
    }
    if (g_presentation != 0) {
        ((Src_s261e0*)((char*)this + 0x30))->pick((int*)&val14);
        __int64 d = g_ticks[f48] - f50;
        if ((float)d * g_f54a6d0 > f58) {
            g_presentation = 0;
            if (f34 != 0)
                operator delete(f34);
            f34 = 0;
            f38 = 0;
            f3c = 0;
            ((SrcVtbl_s261e0*)f5c)->v49();
            if (g_render.dev != 0)
                ((void (__cdecl*)(void*, int))*(void**)(*(int*)g_render.dev + 0x30))(g_render.dev, 0);
            (*(unsigned char*)((char*)&g_gui + 0x560)) = 0;
            f324 = -1;
        }
    }

    if (b6) {
        // ---- boolean-true branch -------------------------------------------------------------------
        if (flag13 != 0 && g_mission_s261e0->local_id >= 0 && g_game_s261e0->b18 == 0) {
            f2f1 = 1;
            g_w_a739a0_s261e0->f18 = 0xe;
            if (val14 < 0) {
                g_w_a739a0_s261e0->f4 = 0;
                g_main_menu_s261e0.set_state(8);
            } else {
                g_w_a739a0_s261e0->f4 = 3;
                g_w_a739a0_s261e0->f14 = val14;
                if (g_w_a739a0_s261e0->f14 < 0) {
                    g_w_a739a0_s261e0->f14 = 0;
                    g_main_menu_s261e0.set_state(8);
                } else {
                    int lim = g_game_s261e0->f13a04 - 1;
                    if (g_w_a739a0_s261e0->f14 > lim)
                        g_w_a739a0_s261e0->f14 = lim;
                    g_main_menu_s261e0.set_state(8);
                }
            }
            goto after;
        }

        if (any_pressed_s261e0(g_keys_73a8, 2) && g_mission_s261e0->local_id >= 0 && g_game_s261e0->b18 == 0) {
            f2f2 = 1;
            char* rec = (char*)g_game_s261e0->chars;
            int v = (unsigned char)rec[0x7a] & 0xf;
            if (v > g_clamp_max)
                v = g_clamp_max;
            f2f4 = *(int*)(rec + 0x58) + 0x23 + v * 2;
            g_w_a739bc_s261e0->f18 = 0xe;
            g_w_a739bc_s261e0->f4 = 0;
            g_w_a739bc_s261e0->f14 = g_game_s261e0->f8c;
            g_main_menu_s261e0.set_state(0xf);
            goto after;
        }

        {
            int flags20 = *(int*)((char*)g_mission_templates + g_mission_s261e0->cur_rec * 0x58 + 0x20);
            if (any_pressed_s261e0(g_keys_73b8, 2) && g_game_s261e0->b18 == 0 &&
                (((flags20 >> 1) & 1) == 0)) {
                g_w_a739b0_s261e0->f18 = 0xe;
                g_w_a739b0_s261e0->f4 = 5;
                g_main_menu_s261e0.set_state(0xc);
                goto after;
            }
        }

        if (any_pressed_s261e0(g_keys_73c0, 2) && g_game_s261e0->b18 == 0) {
            if (g_show_quest_notes == 0) {
                g_w_a739b4_s261e0->f18 = 0xe;
                g_w_a739b4_s261e0->f4 = 0;
                g_main_menu_s261e0.set_state(0xd);
            } else {
                g_w_a739e8_s261e0->f18 = 0xe;
                *(int*)((char*)g_game_s261e0 + 0x13b08) = 4;
                *(int*)((char*)g_game_s261e0 + 0x13b0c) = 0;
                g_main_menu_s261e0.set_state(0x1a);
            }
            goto after;
        }

        if (any_pressed_s261e0(g_keys_73c8, 2) && g_game_s261e0->b18 == 0) {
            g_w_a739e4_s261e0->f18 = 0xe;
            g_w_a739e4_s261e0->f4 = 0;
            g_main_menu_s261e0.set_state(0x19);
            goto after;
        }

        if (g_a170d8 != 0 && g_a174d8 == 0) {
            char c = g_mission_s261e0->mode_flag1();
            if (c == 0) {
                g_w_a73994_s261e0->f18 = 0xe;
                g_main_menu_s261e0.set_state(5);
            } else {
                g_w_a739c8_s261e0->f18 = 0xe;
                g_main_menu_s261e0.set_state(0x12);
            }
            goto after;
        }

        if (g_game_s261e0->b18 == 0 && g_keytable_s261e0.clicked(0x24)) {
            if (!g_mission_s261e0->mode_flag1() && g_game_s261e0->b18 == 0) {
                quick_save_msg();
            } else {
                g_gui_s261e0.add(*g_strtable_s261e0.lookup(&tstring("ui_cant_quick_save")), 0xffffffff);
            }
            goto after;
        }

        if (g_mission_s261e0->f6620 != -1) {
            if (g_mission_s261e0->f6620 < 0)
                goto after;
            f2f1 = 1;
            unsigned id = (unsigned)g_mission_s261e0->f6620;
            g_mission_s261e0->f6620 = -1;
            Agent_s261e0* a = page_agent_s261e0((int)id);
            int dialog_id = a->f88;
            g_w_a739c0_s261e0->f18 = 0xe;
            g_dialogmgr.start(dialog_id, (int)id, -1, 0, 0);
            g_mission_s261e0->f6558 = a->f88;
            g_game_s261e0->f13a08 = a->f88;
            g_main_menu_s261e0.set_state(0x10);
            goto after;
        }

        if (g_game_s261e0->f13af4 < 0)
            goto after;
        g_a73300_s261e0.f_5295a0();
        g_a7333c |= 8;
        g_flags_s261e0.commit((char*)&flag13);
        goto unlock;
    }

    // ---- boolean-false branch ----------------------------------------------------------------------
    g_a73300_s261e0.f_5295a0();
    if (f18 != 100) {
        if (g_game_s261e0->f13ad0 > 0.0f && g_game_s261e0->f13ad4 != 0)
            g_main_menu_s261e0.set_state(7);
        else
            g_main_menu_s261e0.set_state(f18);
        goto unlock;
    }
    if (0 < (*(int*)((char*)&g_mission_ctx + 0x2c4))) {
        int f = *(int*)((char*)g_menus + (*(int*)((char*)&g_mission_ctx + 0x2c4)) * 0x60 + 0x20);
        if ((f & 0x10) != 0)
            g_game_s261e0->fa8 |= 1;
    }
    g_flags_s261e0.commit((char*)&flag13);

after:
unlock:
    if (g_mt_inited) {
        LeaveCriticalSection(&g_mt_cs);
        SetEvent(g_mt_event);
    }
}

// ======================================================= slice s00527c90: functions
#undef g_game_s27c90
#define g_game_s27c90 (*(GameData_s27c90**)&g_game)
#undef g_mission_s27c90
#define g_mission_s27c90 (*(Mission_s27c90**)&g_mission)
// @ 0x527c90
void GameData_s27c90::save_scene(int idx)
{
    tstring path = scene_path(idx);
    bin_writer_i w;
    const char* p = module_path(path.c_str());
    w.name.assign(p);
    w.is_open = 0;
    w.fp = fopen(p, "wb");
    if (w.fp) {
        w.failed = 0;
        w.is_open = 1;
    } else {
        w.failed = 1;
    }
    scenes[idx].save(&w, 0);
    if (w.is_open) {
        stream_base& s = w;
        if (s.flush() == 0)
            fclose(w.fp);
        w.failed = 1;
        w.is_open = 0;
    }
}

// @ 0x527da0
void SceneObjRec_s27c90::save(stream_base* s, int arg)
{
    int version = -0x2cd;
    s->w_int(&version);
    int four = 4;
    s->w_int(&four);
    int count = 0;
    for (unsigned i = 0;
         objs_first && i < (unsigned)((objs_last - objs_first) / 0x1c8);
         i++) {
        if (((SceneObj_s27c90*)objs_first)[i].f10 == arg)
            count++;
    }
    s->w_int(&count);
    for (unsigned i = 0;
         objs_first && i < (unsigned)((objs_last - objs_first) / 0x1c8);
         i++) {
        if (((SceneObj_s27c90*)objs_first)[i].f10 == arg)
            ((SceneObj_s27c90*)objs_first)[i].save(s);
    }
    stream_b.rewind();
    s->w_stream(&stream_b);
    if (!stream_a.at_end()) {
        int marker = 0xff4ad1a6;
        s->w_int(&marker);
        stream_a.rewind();
        s->w_stream(&stream_a);
    }
}

// @ 0x527ef0
void SceneObj_s27c90::save(stream_base* s)
{
    s->w_int(&f188);
    s->w_int(&f18c);
    int tmp;
    s->w_int(&tmp);
    w_matrix(s, &mat[0]);
    s->w_tstr((const rgl::string*)&name);
    s->w_int(&f1a0);
    s->w_int(&f1a4);
    float* p = &f158;
    s->w_float(p).w_float(p + 1).w_float(p + 2);
}

// @ 0x527f90
void Mission_s27c90::save_scene_data()
{
    SceneObjRec_s27c90* rec = g_game_s27c90->scenes + cur_scene;
    if (rec->flags30 & 0x100) {
        rec->stream_a.reset();
        grids.save(&rec->stream_a);
    }
    rec->stream_b.reset();
    other.read(&rec->stream_b);
}

// @ 0x528000
void GridSet_s27c90::save(stream_base* s)
{
    s->w_int(&count);
    s->w_int(&width);
    s->w_int(&height);
    int i = 0;
    if (count > 0) {
        Grid_s27c90* g = grids;
        do {
            g->save(s, &width);
            i++;
            g++;
        } while (i < count);
    }
}

// @ 0x528060
void Grid_s27c90::save(stream_base* s, int* dims)
{
    s->w_int(&type);
    s->w_tstr((const rgl::string*)&name);
    if (data == 0) {
        int zero = 0;
        s->w_int(&zero);
        return;
    }
    int one = 1;
    s->w_int(&one);

    bool run = false;
    int count = 0;
    int gap = 0;
    int saved_row = 0;
    int saved_col = 0;
    int row = 0;
    if (dims[1] <= 0) {
        s->w_int(&gap);
        return;
    }
    for (row = 0; row < dims[1]; row++) {
        int col;
        for (col = 0; col < dims[0]; col++) {
            float v = data[col * 0x1a4 + row];
            bool transparent;
            if (type == -0x3115)
                transparent = ((*(int*)&v & 0xffffff) == 0xffffff);
            else
                transparent = (-0.001f < v && v < 0.001f);
            if (transparent) {
                if (run) {
                    s->w_int(&count);
                    int n = count;
                    int r = saved_row;
                    int c = saved_col;
                    for (int i = 0; i < n; i++) {
                        if (type < 0)
                            s->w_float(&data[c * 0x1a4 + r]);
                        else {
                            float y = data[c * 0x1a4 + r] * 255.0f;
                            double f = floor((double)y);
                            int ci = (int)f;
                            if (y - f > 0.5f)
                                ci++;
                            unsigned char b;
                            if (ci < 0)
                                b = 0;
                            else {
                                b = 0xff;
                                if (ci < 0x100)
                                    b = (unsigned char)ci;
                            }
                            s->w_byte(&b);
                        }
                        c++;
                        if (c >= dims[0]) {
                            c = 0;
                            r++;
                        }
                    }
                    count = 0;
                    run = false;
                    gap = 1;
                } else {
                    gap++;
                }
            } else {
                if (run) {
                    count++;
                } else {
                    run = true;
                    s->w_int(&gap);
                    gap = 0;
                    count = 1;
                    saved_row = row;
                    saved_col = col;
                }
            }
        }
    }
    if (run) {
        s->w_int(&count);
        int n = count;
        int r = saved_row;
        int c = saved_col;
        for (int i = 0; i < n; i++) {
            if (type < 0)
                s->w_float(&data[c * 0x1a4 + r]);
            else {
                float y = data[c * 0x1a4 + r] * 255.0f;
                double f = floor((double)y);
                int ci = (int)f;
                if (y - f > 0.5f)
                    ci++;
                unsigned char b;
                if (ci < 0)
                    b = 0;
                else {
                    b = 0xff;
                    if (ci < 0x100)
                        b = (unsigned char)ci;
                }
                s->w_byte(&b);
            }
            c++;
            if (c >= dims[0]) {
                c = 0;
                r++;
            }
        }
    } else {
        s->w_int(&gap);
    }
}

// @ 0x528410
int Mission_s27c90::release_item(int idx)
{
    int off = idx * 0x1c8;
    char* p = (char*)item_array + off;
    if (*(void**)(p + 0x1b4)) {
        (*(ResourceOwner_s27c90**)(p + 0x1b4))->release();
        int id = *(int*)((char*)*(void**)(item_array + off + 0x1b4) + 8);
        int r = ((MissionHdr_s27c90*)this)->world->remove_entity(id);
        *(void**)(item_array + off + 0x1b4) = 0;
        return r;
    }
    return (int)p;
}

// @ 0x528470
void ResourceOwner_s27c90::release()
{
    Resource_s27c90** p = first;
    if (p < last) {
        do {
            (*p)->flags |= 0x8000;
            p++;
        } while (p < last);
    }
}

// ======================================================= slice s005284a0: functions
#undef g_items_s284a0
#define g_items_s284a0 (*(ItemKind_s284a0**)&g_items)
#undef g_mission_s284a0
#define g_mission_s284a0 (*(Mission_s284a0**)&g_mission)
#undef g_game_s284a0
#define g_game_s284a0 (*(Game_s284a0**)&g_game)
#undef g_keys_s284a0
#define g_keys_s284a0 (*(KeyTable_s284a0*)g_game_keys)
#undef g_key_names_s284a0
#define g_key_names_s284a0 (*(KeyNames_s284a0*)g_keyboard)
#undef AGENT
#define AGENT(id) (g_mission_s284a0->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))
// @ 0x5284a0 ------------------------------------------------------------------------------------------
void ViewCtrl_s284a0::update(float dt)
{
    if (g_game_s284a0->f1a090) {
        if (g_a17188) return;
        if (g_a17128) return;
        if (g_a17184) return;
        if (g_a1712c) return;
        if (g_a17190) return;
        if (g_a1715c) return;
        if ((*(int*)((char*)&g_keyboard + 0x5c))) return;
        if (g_game_s284a0->f1a090)
            dt = g_step_tbl[0];
    }
    float k = expf(g_options.f10 * 4.0f) * 0.04f;

    // ---- aiming at an agent while a melee weapon (or the fists) is out is slower ---------------------
    if (!g_game_s284a0->f1a090) {
        if (g_presentation == 0 || (flags44 & 1)) {
            if (g_mission_s284a0->selected >= 0) {
                Agent_s284a0* a = AGENT(g_mission_s284a0->selected);
                ItemRef_s284a0 w0 = a->slot_pair(0);
                ItemRef_s284a0 w1 = a->slot_pair(1);
                bool hand0;
                if (w0.item < 0) {
                    hand0 = true;
                } else {
                    int t = g_items_s284a0[w0.item].type_flags & 0xff;
                    hand0 = (t == 2 || t == 3 || t == 4);
                }
                if (((hand0 && a->f560c == 1 && g_options.i6c > 0 && a->f100 == a->f55ec) ||
                     (w1.item < 0 && a->f5610 > 0 && g_stick_mode > 0 && a->f100 == a->f5614)) &&
                    a->f118 < 0.6f)
                    k *= 0.1f;
            }
        }
    }

    // ---- look input ---------------------------------------------------------------------------------
    float look_x, look_y;
    if (g_presentation == 0 || (flags44 & 1)) {
        look_x = (*(float*)((char*)&g_keyboard + 0x80c));
        look_y = (*(float*)((char*)&g_keyboard + 0x810));
    } else {
        look_x = 0.0f;
        look_y = 0.0f;
        if ((*(float*)((char*)&g_gui + 0x14)) <= 0.01f)
            look_x = dt * -400.0f;
        else if ((*(float*)((char*)&g_gui + 0x14)) >= 0.99f)
            look_x = dt * 400.0f;
        if ((*(float*)((char*)&g_gui + 0x18)) <= 0.01f)
            look_y = dt * 400.0f;
        if ((*(float*)((char*)&g_gui + 0x18)) >= 0.74f)
            look_y = dt * -400.0f;
    }

    // ---- damp the view rates and integrate them into yaw / pitch ------------------------------------
    if (!g_game_s284a0->f1a090 || (*(int*)((char*)&g_keyboard + 0x38c)) != 0) {
        float r = 1.0f - dt * 20.0f;
        if (r < 0.0f)
            r = 0.0f;
        yaw_rate = yaw_rate * r;
        pitch_rate = pitch_rate * r;
        yaw_rate = yaw_rate + look_x * fov * 0.0018f;
        if (g_invert)
            look_y = -look_y;
        pitch_rate = pitch_rate + look_y * fov * 0.0018f;
        yaw = yaw - yaw_rate * k * dt;
        pitch = pitch - pitch_rate * k * dt;
    }

    // ---- clamps -------------------------------------------------------------------------------------
    if (pitch < -0.9519978165626526f)
        pitch = -0.9519978165626526f;
    else if (pitch > 0.9519978165626526f)
        pitch = 0.9519978165626526f;
    if (look_speed < 0.7f)
        look_speed = 0.7f;
    else if (look_speed > 10.0f)
        look_speed = 10.0f;

    if (g_mission_s284a0->selected < 0)
        return;

    // ---- build the view frame and hand its forward row to the local agent ----------------------------
    Agent_s284a0* a;
    {
        matrix m;
        identity3(&m);
        ((MatrixExt_s284a0*)&m)->rotate_sf(&yaw);
        ((MatrixExt_s284a0*)&m)->rotate_fu(&pitch);

        a = AGENT(g_mission_s284a0->selected);
        bool looking = ((*(int*)((char*)&g_game_keys + 0x68)) >= 0 && ((int*)((char*)&g_keyboard + 0xc))[(*(int*)((char*)&g_game_keys + 0x68))] != 0);
        if (((*(int*)((char*)&g_game_keys + 0x6c)) < 0 || ((int*)((char*)&g_keyboard + 0xc))[(*(int*)((char*)&g_game_keys + 0x6c))] == 0) && !looking)
            a->dir55d8 = m.f;
    }

    if (snap < 5) {
        a->dir55d8 = a->dir;
        yaw = (float)atan2(-a->dir.x, a->dir.y);
        pitch = 0.0f;
        snap++;
    }

    // ---- movement / action bits ---------------------------------------------------------------------
    a->in_flags = 0;
    if (!g_game_s284a0->f1a090) {
        if (g_a1717c == 0)
            a->in_flags = 0x200000;
        if (g_keys_s284a0.clicked(5))
            a->in_flags |= 0x100000;

        if (a->mount_id < 0) {
            if (g_keys_s284a0.is_down(0))
                a->in_flags |= 1;
            else if (g_keys_s284a0.is_down(1))
                a->in_flags |= 2;
            if (g_keys_s284a0.is_down(3))
                a->in_flags |= 4;
            else if (g_keys_s284a0.is_down(2))
                a->in_flags |= 8;
            if (g_keys_s284a0.clicked(4) && mode == 2 && aim_valid != 0) {
                a->in_flags |= 0x20000;
                a->target_id = target;
            }
        } else {
            if (g_keys_s284a0.clicked(0))
                a->in_flags |= 0x80000;
            if (g_keys_s284a0.is_down(0))
                a->in_flags |= 0x10;
            else if (g_keys_s284a0.is_down(1))
                a->in_flags |= 0x20;
            if (g_keys_s284a0.is_down(3))
                a->in_flags |= 0x40;
            else if (g_keys_s284a0.is_down(2))
                a->in_flags |= 0x80;
            if (g_keys_s284a0.clicked(4) && mode == 3)
                a->in_flags |= 0x10000;
            if (g_key_names_s284a0.is_ready(0x24) && ((*(int*)((char*)&g_keyboard + 0x80)) != 0 || (*(int*)((char*)&g_keyboard + 0x280)) != 0))
                a->in_flags |= 0x40000;
        }

        if (g_presentation == 0 || (flags44 & 1)) {
            if (g_keys_s284a0.is_down(6))
                a->in_flags |= 0x100;
            if (g_keys_s284a0.clicked(6))
                a->in_flags |= 0x400;
            if (g_keys_s284a0.is_down(7))
                a->in_flags |= 0x200;
        }

        if (g_keys_s284a0.clicked(8)) {
            a->cycle_slot(0);
        } else if (g_keys_s284a0.clicked(9)) {
            a->cycle_slot(1);
        } else if (g_keys_s284a0.clicked(10)) {
            a->wield_a = -1;
            a->on_wield_reset();
        }
    }

    if (g_keys_s284a0.clicked(0xe)) {
        g_game_s284a0->f139e4 = (g_game_s284a0->f139e4 == 0);
        update_equipment();
    }
    AGENT(g_mission_s284a0->selected)->f55d4 = (g_game_s284a0->f139e4 != 0);

    // ---- field of view ------------------------------------------------------------------------------
    bool zoom_key = (g_a73790 >= 0 && ((int*)((char*)&g_keyboard + 0xc))[g_a73790] != 0);
    if ((g_a73794 >= 0 && ((int*)((char*)&g_keyboard + 0xc))[g_a73794] != 0) || zoom_key)
        fov = fov - dt * 350.0f;
    else
        fov = fov + dt * 350.0f;

    float limit = g_game_s284a0->f139e4 ? 90.0f : 97.0f;
    if (fov < 37.0f)
        fov = 37.0f;
    else if (fov > limit)
        fov = limit;

    // ---- the "use / attack" keys walk the agent table and hit the agent under the crosshair ---------
    if ((*(int*)((char*)&g_cfg + 0x24)) != 0 && ((*(int*)((char*)&g_keyboard + 0x80)) != 0 || (*(int*)((char*)&g_keyboard + 0x280)) != 0) && g_a171d0 != 0 && g_a175d0 == 0) {
        Agent_s284a0* t = AGENT(g_mission_s284a0->selected);
        int old = t->f7c;
        t->f7c = (old == 2) ? 1 : 2;
    }

    if ((*(int*)((char*)&g_cfg + 0x24)) != 0 && (*(int*)((char*)&g_keyboard + 0x80)) != 0 && g_key_names_s284a0.is_ready(0x3e)) {
        AgentTable_s284a0* t = &g_mission_s284a0->agents;
        bool done = false;
        unsigned i = t->first_used();
        while (i < (unsigned)t->count && !done) {
            Agent_s284a0* o = AGENT(i);
            if (o->f14 == 1 && o->f10 == 0 && AGENT(g_mission_s284a0->selected)->is_enemy(o)) {
                Blow_s284a0 b;
                b.f48 = 2;
                b.f38 = 0;
                b.attacker = g_mission_s284a0->selected;
                b.pos = AGENT(i)->pos;
                b.damage = 100.0f;
                AGENT(i)->note_blow(&b);
                if (g_a1717c == 0)
                    done = true;
            }
            t->next_used(&i);
        }
    }

    if ((*(int*)((char*)&g_cfg + 0x24)) != 0 && (*(int*)((char*)&g_keyboard + 0x80)) != 0 && g_a171d4 != 0 && g_a175d4 == 0) {
        AgentTable_s284a0* t = &g_mission_s284a0->agents;
        bool done = false;
        unsigned i = t->first_used();
        while (i < (unsigned)t->count && !done) {
            Agent_s284a0* o = AGENT(i);
            if (i != (unsigned)g_mission_s284a0->selected && o->f14 == 1 && o->f10 == 0 &&
                AGENT(g_mission_s284a0->selected)->is_friend(o)) {
                Blow_s284a0 b;
                b.f48 = 2;
                b.f38 = 0;
                b.attacker = g_mission_s284a0->selected;
                b.pos = AGENT(i)->pos;
                b.damage = 100.0f;
                AGENT(i)->note_blow(&b);
                if (g_a1717c == 0)
                    done = true;
            }
            t->next_used(&i);
        }
    }

    if ((*(int*)((char*)&g_cfg + 0x24)) != 0 && (*(int*)((char*)&g_keyboard + 0x80)) != 0 && g_a171c8 != 0 && g_a175c8 == 0) {
        Blow_s284a0 b;
        b.f48 = 2;
        b.f38 = 0;
        b.attacker = g_mission_s284a0->selected;
        b.pos = AGENT(g_mission_s284a0->selected)->pos;
        b.damage = 200.0f;
        AGENT(g_mission_s284a0->selected)->note_blow(&b);
    }

    if ((*(int*)((char*)&g_keyboard + 0x80)) != 0 || (*(int*)((char*)&g_keyboard + 0x280)) != 0) {
        if (g_a17160 != 0 && g_a17560 == 0 && (*(int*)((char*)&g_cfg + 0x24)) != 0) {
            Agent_s284a0* t = AGENT(g_mission_s284a0->selected);
            t->f5724 = t->f5720;
        }
    }
}

// ======================================================= slice s00529130: functions
#undef g_scene_s29130
#define g_scene_s29130 (*(Mission_s29130**)&(*(void**)((char*)&g_mission + 0x0)))
#undef g_game_s29130
#define g_game_s29130 (*(GameData_s29130**)&g_game)
#undef g_items_s29130
#define g_items_s29130 (*(ItemKind_s29130**)&g_items)
// @ 0x529130
void MissionHud_s29130::update_marks()
{
    Agent_s29130* a = &g_scene_s29130->agents.pages[g_scene_s29130->cur_agent_s29130 >> 4][g_scene_s29130->cur_agent_s29130 & 0xf];

    float y = (g_half / tanf(f288 * g_deg2rad * g_half)) * tanf(a->aim_error);

    float p[2];
    p[0] = 0.5f;
    p[1] = y + g_0375;
    w[0]->set_pos(p);

    float x = 0.0f;
    p[0] = x;
    p[1] = y;
    float c = (float)cos(2.268928f);
    float s = (float)sin(2.268928f);
    float nx = p[0] * c - p[1] * s;
    float ny = p[0] * s + p[1] * c;
    p[0] = nx + g_half;
    p[1] = ny + g_0375;
    w[1]->set_pos(p);

    p[0] = x;
    p[1] = y;
    c = (float)cos(-2.268928f);
    s = (float)sin(-2.268928f);
    nx = p[0] * c - p[1] * s;
    ny = p[0] * s + p[1] * c;
    p[0] = nx + g_half;
    p[1] = ny + g_0375;
    w[2]->set_pos(p);

    w[0]->hide();
    w[1]->hide();
    w[2]->hide();
    w[3]->hide();
    w[4]->hide();
    w[5]->hide();
    w[6]->hide();

    float target[7] = {0, 0, 0, 0, 0, 0, 0};

    ItemRef_s29130 pair = a->slot_pair(0);
    float aim = signed_angle(a->blk60, &a->facing);

    if (pair.item >= 0) {
        bool other = false;
        if ((*(int*)((char*)&g_game_keys + 0x68)) >= 0 && ((void**)((char*)&g_keyboard + 0xc))[(*(int*)((char*)&g_game_keys + 0x68))]) other = true;
        if (((*(int*)((char*)&g_game_keys + 0x6c)) < 0 || ((void**)((char*)&g_keyboard + 0xc))[(*(int*)((char*)&g_game_keys + 0x6c))] == 0) && !other) {
            int type = g_items_s29130[pair.item].type & 0xff;
            if ((type == 8 || type == 9 || type == 10 || type == 16 || type == 17) && g_flag5aa06c) {
                if (!cur_agent_s29130()->f100_is_0x48_and_charged()) {
                    Agent_s29130* a2 = cur_agent_s29130();
                    bool in_range = true;
                    if (a2->mount != -1) {
                        int idx = a2->f56c4 * 2;
                        in_range = (g_range_tbl[idx] <= aim && aim < g_range_tbl[idx + 1]);
                    }
                    if (in_range) {
                        target[0] = 0.9f;
                        target[1] = 0.9f;
                        target[2] = 0.9f;
                    }
                }
            } else if (type == 2 || type == 3 || type == 4) {
                if (g_flag5aa070 || !g_options.i6c) {
                    int t = a->pick_front();
                    if (t >= 0) {
                        int dir = a->pick_attack_dir(t);
                        if (dir >= 0 && dir < 4) {
                            if (dir == 3)
                                target[3] = 0.7f;
                            else if (dir == 1)
                                target[4] = 0.7f;
                            else if (dir == 0)
                                target[5] = 0.7f;
                            else if (dir == 2)
                                target[6] = 0.7f;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < 7; i++) {
        float step = g_step_tbl[0] * (i < 3 ? 0.5f : 2.5f);
        if (target[i] > color[i]) {
            color[i] += step * g_1p2;
            if (color[i] > target[i]) color[i] = target[i];
        } else if (target[i] < color[i]) {
            color[i] -= step;
            if (color[i] < target[i]) color[i] = target[i];
        }
        if (i < 3)
            marks[i]->color = 0xffffff;
        else
            marks[i]->color = 0xffbb11;
        int alpha = (int)(color[i] * g_255);
        if (alpha) {
            if (i < 3)
                w[i]->show();
            else
                w[i]->show();
        }
        marks[i]->color |= (unsigned)alpha << 24;
    }
}

// @ 0x5295a0
void MissionHud_s29130::tick()
{
    g_game_s29130->more_start();
    if (f2c8 != 0 || (f154 >= 0 && (((char*)g_mission_templates)[f154 * 0x58 + 0x20] & 0x10))) {
        refresh_markers();
    }
}

// @ 0x529600
void MissionHud_s29130::refresh_markers()
{
    int n = 1;
    if (g_scene_s29130->f6524 > 0)
        n = 0x701;
    else if (g_scene_s29130->f6524 < 0)
        n = 0x7031;
    g_scene_s29130->fn_529840(n);
    if (f14 == 3 || f14 == 4 || f14 == 1) {
        reset_party_heroes(g_game_s29130->cur_party);
        if (f0 >= 0) reset_party_heroes(f0);
        if (f4 >= 0) reset_party_heroes(f4);
    }
}

// @ 0x529670
void MissionHud_s29130::reset_party_heroes(int party_id)
{
    PartyMgr_s29130* mgr = party_mgr_s29130();
    int idx = mgr->index_of(party_id);
    Party_s29130* party = &mgr->pages[idx >> 4][idx & 0xf];

    int n = 0;
    int i = 0;
    while (i < party->nstacks) {
        if (party->stacks[i].flags & 1) break;
        n++;
        i++;
    }

    for (int j = 0; j < n; j++) {
        Troop_s29130* t = (Troop_s29130*)(*(int*)((char*)g_game_s29130 + 0x13a00) + party->stacks[j].troop * 0x508);
        if (t->flags & 0x10) {
            if (t->f_a0 < 0.0f) t->f_a0 = 0.0f;
        }
    }
}

// ======================================================= slice s00529840: functions
#undef g_scene_s29840
#define g_scene_s29840 (*(Mission_s29840**)&(*(void**)((char*)&g_mission + 0x0)))
#undef g_game_s29840
#define g_game_s29840 (*(GameData_s29840**)&g_game)
// @ 0x529840
void Mission_s29840::fn_529840(int)
{
    unsigned i = agents.first_used();
    while (i < (unsigned)agents.count) {
        Agent_s29840* a = &agents.pages[i >> 4][i & 0xf];
        if (a->f10 != 1 && a->f10 == 0) {
            if (a->f1bc >= 0 && a->f1c0 >= 0) {
                Troop_s29840* t = &troop_tbl_s29840()[a->troop];
                if (!(t->flags & 0x10) && a->f1bc == g_game_s29840->cur_party && t->f50 > 0) {
                    float xp = (float)a->f5730;
                    int cur = troop_tbl_s29840()[g_game_s29840->cur_troop].b0;
                    if (t->b0 > cur) {
                        int diff = t->b0 - cur;
                        xp = xp / (diff + g_exp_rate);
                        xp = xp * g_exp_rate;
                    }
                    int v = (int)xp;
                    party_mgr_s29840()->get(a->f1bc)->add_xp_to_stack(a->f1c0, v);
                }
                t = &troop_tbl_s29840()[a->troop];
                if (t->flags & 0x10) {
                    t->f_a0 = a->f5724;
                    Party_s29840* p = party_mgr_s29840()->at(party_mgr_s29840()->index_of(a->f1bc));
                    int skill = p->skill_level(9);
                    float v = a->f571c - a->f5724;
                    if (v < 0.0f) v = 0.0f;
                    troop_tbl_s29840()[a->troop].f_a0 += (skill * 0.05f + 0.1f) * v;
                }
                if (a->f14 != 1) {
                    if (a->f14 == 5)
                        fn_529b70(a);
                    else if (a->f14 == 4)
                        fn_529d00(a);
                }
            } else {
                Troop_s29840* t = &troop_tbl_s29840()[a->troop];
                if (t->flags & 0x10) {
                    t->f_a0 = a->f5724;
                    if (a->troop == g_game_s29840->cur_troop) {
                        Party_s29840* p = party_mgr_s29840()->at(party_mgr_s29840()->index_of(g_game_s29840->cur_party));
                        int skill = p->skill_level(9);
                        float v = a->f571c - a->f5724;
                        if (v < 0.0f) v = 0.0f;
                        troop_tbl_s29840()[a->troop].f_a0 += (skill * 0.05f + 0.1f) * v;
                    }
                }
            }
        }
        agents.next_used(&i);
    }
}

// @ 0x529b70
void Mission_s29840::fn_529b70(Agent_s29840* a)
{
    fn_529bc0(a, 1);
    if (a->f1bc == g_game_s29840->cur_party)
        fn_529c30((int*)((char*)g_game_s29840 + 0x1405c), a->f1bc, a->f1c0, a->f94, 0);
}

// @ 0x529bc0
__declspec(noinline) void __stdcall fn_529bc0(Agent_s29840* a, int add)
{
    if (a->f1bc < 0) return;
    int slot = a->f1c0;
    if (slot < 0) return;
    if (add)
        party_mgr_s29840()->get(a->f1bc)->stacks[slot].count--;
    else
        party_mgr_s29840()->get(a->f1bc)->stacks[slot].f10++;
}

// @ 0x529c30
void Mission_s29840::fn_529c30(int* hist, int party_id, int slot, int value, int flag)
{
    Party_s29840* p = party_mgr_s29840()->at(party_mgr_s29840()->index_of(party_id));
    int count = p->stacks[slot].count;
    int* h = hist + slot * 32;
    int found = -1;
    int i = 0;
    while (found == -1 && i < count) {
        if (h[i] == value) found = i;
        i++;
    }
    if (found >= 0) {
        if (found < 0x1f)
            memcpy(h + found, h + found + 1, (0x1f - found) * 4);
        h[31] = rand();
        if (flag && p->stacks[slot].count <= 0x20)
            h[p->stacks[slot].count - 1] = value;
    }
}

// @ 0x529d00
void Mission_s29840::fn_529d00(Agent_s29840* a)
{
    if (a->f1bc >= 0) {
        int slot = a->f1c0;
        if (slot >= 0) {
            Party_s29840* p = party_mgr_s29840()->get(a->f1bc);
            int* v = (int*)p->stacks + slot * 8 + 4;
            (*v)++;
        }
    }
    if (a->f1bc == g_game_s29840->cur_party)
        fn_529c30((int*)((char*)g_game_s29840 + 0x1405c), a->f1bc, a->f1c0, a->f94, 1);
}

// @ 0x529d80
void Mission_s29840::fn_529d80()
{
    Scene_s29840* sc = (Scene_s29840*)(*(char**)((char*)g_game_s29840 + 0x13a50) + f5df4 * 0x494);
    for (unsigned i = 0; i < records.size(); i++) {
        Rec1c8_s29840* r = &records[i];
        if (r->f10 == 0 && r->f14 >= 0) {
            r->f18c = sc->recs[r->f14].f18c;
            r->name = sc->recs[r->f14].name;
        }
    }
    sc->recs.resize(0);
    for (unsigned i = 0; i < records.size(); i++) {
        Rec1c8_s29840* r = &records[i];
        if (r->f10 == 0) {
            sc->recs.push_back(*r);
            r->f14 = (int)sc->recs.size() - 1;
        }
    }
}

// ======================================================= slice s00529ec0: functions
#undef g_gui_s29ec0
#define g_gui_s29ec0 (*(gui_manager_s29ec0*)g_gui)
#undef g_strtable_s29ec0
#define g_strtable_s29ec0 (*(StrTable_s29ec0*)g_strtable)
#undef g_items_s29ec0
#define g_items_s29ec0 (*(ItemKind_s29ec0**)&g_items)
#undef g_game_s29ec0
#define g_game_s29ec0 (*(GameData_s29ec0**)&g_game)
#undef g_prop_kinds_s29ec0
#define g_prop_kinds_s29ec0 (*(PropKind_s29ec0**)&g_propkinds)
#undef g_mission_s29ec0
#define g_mission_s29ec0 (*(Mission_s29ec0**)&g_mission)
#undef g_mapobjs_s29ec0
#define g_mapobjs_s29ec0 (*(MapObjList_s29ec0**)&g_menus)
#undef g_areas_s29ec0
#define g_areas_s29ec0 (*(Area_s29ec0**)&g_mission_templates)
#undef g_panel_s29ec0
#define g_panel_s29ec0 (*(hud_panel_s29ec0**)&(*(void**)((char*)&g_gui + 0x568)))
// @ 0x529ec0
void Screen_s29ec0::f_529ec0()
{
    Mission_s29ec0* m = g_mission_s29ec0;
    Agent_s29ec0* me = agent_at_s29ec0(m->local_id);

    vector4 pos = me->pos;
    int mode = -1;
    int target = -1;
    float best = 1e9f;
    float second = 1e9f;

    // ---- phase A: cast the thin probe 10.5 m ahead and find the nearest face hit ----------------------
    float eye_z = me->speed_scale() + pos.z;
    vector4 front(pos.x + me->facing.x * 10.5f,
                  pos.y + me->facing.y * 10.5f,
                  eye_z + me->facing.z * 10.5f);
    vector4 back(pos.x + me->facing.x * 0.05f,
                 pos.y + me->facing.y * 0.05f,
                 eye_z + me->facing.z * 0.05f);
    front.w = pos.w;
    back.w = pos.w;

    capsule_s29ec0 cap(1e-4f, &back, &front);
    matrix ident;
    identity_pos(&ident);
    cap.transform(&ident);

    shape_base_s29ec0* found[0x2000];
    int count = 0;

    m->scene->query_segment((const float*)&back, (const float*)&front, 0x110, found, &count);
    for (int i = 0; i < count; i++) {
        shape_base_s29ec0* s = found[i];
        if (s->shape_type() != 2)
            continue;
        contact_s29ec0 c;
        if (!shape_contact_count(s, &cap, 1, &c))
            continue;
        vector4 hit = c.point;
        vector4 d = hit - back;
        float len = d.normalize();
        if (len < best) {
            best = len;
            mode = -1;
            second = len;
        }
    }

    // ---- phase B: the same probe as a 0.1 m capsule_s29ec0, 0.1 m further out, against the entities ---------
    vector4 axis = cap.p1 - cap.p0;
    axis.normalize();
    cap.p0.x += axis.x * 0.1f;
    cap.p0.y += axis.y * 0.1f;
    cap.p0.z += axis.z * 0.1f;
    cap.radius = 0.1f;
    best += 1.0f;

    m->scene->collect(9, (SceneEntity_s29ec0**)found, &count);
    for (int i = 0; i < count; i++) {
        SceneEntity_s29ec0* e = (SceneEntity_s29ec0*)found[i];
        if (!e->shape)
            continue;
        contact_s29ec0 c;
        if (!shape_contact_count(e->shape, &cap, 1, &c))
            continue;

        vector4 d = c.point - back;
        float len = d.length();
        if (len > best)
            continue;

        bool keep = false;
        if (e->type == 0x0b) {
            Agent_s29ec0* a = agent_at_s29ec0(e->id);
            if (a->f10 == 0) {
                // a human: talk target, unless it is the player or an enemy
                if (e->id == m->local_id)
                    continue;
                if (me->is_enemy(a))
                    continue;
                mode = 1;
                target = e->id;
                best = len;
                vector4 r = e->pos - back;
                second = r.length();
            } else if (a->f10 == 1) {
                // a ridden mount
                if (g_items_s29ec0[a->item.item].type_flags != 1)
                    continue;
                if (a->f30 != -1 || a->f14 != 1 || a->f5830 != 2)
                    continue;
                mode = 2;
                target = e->id;
                best = d.out_length();
                vector4 r = e->pos - back;
                second = r.length();
            }
        } else if (e->type == 0x0c) {
            SceneProp_s29ec0* p = &m->props[e->id];
            if (p->destroyed == 0) {
                int kind = p->kind;
                int flags = g_prop_kinds_s29ec0[kind].flags & 0xff;
                if (flags == 5) {
                    mode = 5;
                    target = e->id;
                    best = len;
                    second = len;
                } else if (flags == 10 && len < 4.0f) {
                    mode = 8;
                    target = e->id;
                    best = len;
                    second = len;
                } else {
                    keep = true;
                }
            } else if (p->destroyed == 5) {
                if ((*(int*)((char*)&g_mission_ctx + 0x2c4)) <= 0 || p->obj < 0 || p->obj >= g_mapobjs_s29ec0[(*(int*)((char*)&g_mission_ctx + 0x2c4))].count) {
                    keep = true;
                } else {
                    mode = 6;
                    target = e->id;
                    best = len;
                    second = len;
                }
            } else if (p->destroyed == 6) {
                mode = 7;
                target = e->id;
                best = len;
                second = len;
            } else {
                keep = true;
            }
        } else {
            keep = true;
        }

        if (keep && mode >= 0) {
            best = len;
            second = len;
        }
    }

    // ---- phase C: nothing picked -> mount/dismount, then the four mission borders ---------------------
    if (mode == -1) {
        if (me->mount != -1 && f26c < -0.5519978f && me->f9c < 1.0f) {
            mode = 3;
        } else if ((m->f6544 - 0.5f < me->pos.x && me->facing.x > 0.5f) ||
                   (m->f6548 - 0.5f < me->pos.y && me->facing.y > 0.5f) ||
                   (me->pos.x < m->f653c + 0.5f && me->facing.x < -0.5f) ||
                   (me->pos.y < m->f6540 + 0.5f && me->facing.y < -0.5f)) {
            mode = 8;
        }
    }

    // ---- phase D: validate --------------------------------------------------------------------------
    int ok = 0;
    if (mode == 1) {
        int flagged = (int)((g_areas_s29ec0[m->f5df0].flags >> 1) & 1);
        if (flagged || (m->f6528 & 1)) {
            if (!g_game_s29ec0->f13b10)
                goto no_selection;
        }
        if (g_game_s29ec0->f13b11)
            goto no_selection;
        if (second < 3.5f)
            ok = 1;
    } else if (mode == 2) {
        if (me->mount != -1)
            goto no_selection;
        {
            Agent_s29ec0* a = agent_at_s29ec0(target);
            int stat = 0;
            if (a->item.item >= 0) {
                ItemKind_s29ec0* k = &g_items_s29ec0[a->item.item];
                stat = k->f1ec;
                if (stat != 0) {
                    int mod = (signed char)(a->item.m4 >> 24);
                    if (k->type_flags == 1) {
                        if (mod == 0x21) stat -= 1;
                        else if (mod == 0x20) stat += 1;
                        else if (mod == 0x24) stat += 2;
                    } else {
                        if (mod == 0x12) stat += 1;
                        else if (mod == 0x13) stat += 2;
                        else if (mod == 0x11) stat += 4;
                    }
                }
            }
            if (stat > g_game_s29ec0->skill_level(g_game_s29ec0->f8c, 0x18))
                goto no_selection;
            vector4 r = a->pos - me->pos;
            if (r.length() < 1.3f)
                ok = 1;
        }
    } else if (mode == 4) {
        if (second < 2.5f)
            ok = 1;
    } else if (mode == 5) {
        if (second < 5.0f)
            ok = 1;
    } else if (mode == 6 || mode == 7) {
        if (second < 2.7f)
            ok = 1;
    }
no_selection:

    // ---- phase E: reset the HUD cache ----------------------------------------------------------------
    tstring name;
    int color = -1;
    tstring action;

    if (g_panel_s29ec0)
        g_panel_s29ec0->reset(1);
    (*(int*)((char*)&g_gui + 0x16e0)) = -1;
    (*(int*)((char*)&g_gui + 0x16e4)) = 0xff101010;
    for (int i = 0; i < 80; i++)
        ((tstring*)((char*)&g_gui + 0x7d0))[i].assign("");
    (*(int*)((char*)&g_gui + 0xfa4)) = 0;
    (*(float*)((char*)&g_gui + 0xfa8)) = -0.03f;
    (*(int*)((char*)&g_gui + 0xfa0)) = 0;
    g_panel_s29ec0 = 0;
    (*(int*)((char*)&g_gui + 0x1700)) = 0;
    float view[2];
    view[0] = 0.5f;
    view[1] = 0.23f;
    g_gui_s29ec0.set_view(view);

    // ---- phase F: name the selection -----------------------------------------------------------------
    if (mode == 1) {
        Agent_s29ec0* a = agent_at_s29ec0(target);
        char* rec = g_game_s29ec0->chars + a->troop * 0x508;
        tstring* tr = g_strtable_s29ec0.find((tstring*)(rec + 0x20));
        name = tr ? *tr : *(tstring*)rec;
        if (ok) {
            tstring k("ui_talk");
            action = *g_strtable_s29ec0.lookup(k);
        }
        if (me->is_enemy(a))
            color = (int)0xffd06050;
        else if (me->is_friend(a))
            color = (int)0xff60d050;
    } else if (mode == 2) {
        Agent_s29ec0* a = agent_at_s29ec0(target);
        char buf[0x40];
        a->item.name(buf);
        name.assign(buf);
        if (ok) {
            tstring k("ui_mount");
            action = *g_strtable_s29ec0.lookup(k);
        } else if (me->mount == -1) {
            int skill = g_game_s29ec0->skill_level(g_game_s29ec0->f8c, 0x18);
            int stat = agent_at_s29ec0(target)->item.stat_1ec();
            if (stat > skill) {
                tstring k("ui_riding_skill_not_adequate_to_mount");
                action = *g_strtable_s29ec0.lookup(k);
            }
        }
    } else if (mode == 3) {
        tstring k("ui_dismount");
        action = *g_strtable_s29ec0.lookup(k);
    } else if (mode == 4) {
        char* rec = g_game_s29ec0->map_records + m->f5df4 * 0x494;
        int dest = *(int*)(rec + 0x7c + target * 4);
        if (dest > g_game_s29ec0->f13a54) {
            tstring k("ui_exit");
            action = *g_strtable_s29ec0.lookup(k);
        } else if (dest >= 0) {
            tstring k("ui_door_to");
            name = *g_strtable_s29ec0.lookup(k);
            name.append(*(tstring*)(g_game_s29ec0->map_records + dest * 0x494 + 0x20));
            name.underscores_to_spaces();
            if (ok) {
                tstring k2("ui_open");
                action = *g_strtable_s29ec0.lookup(k2);
            }
        }
    } else if (mode == 5) {
        SceneProp_s29ec0* p = &m->props[target];
        char* rec = g_game_s29ec0->map_records + m->f5df4 * 0x494;
        int inv = p->inv;
        if (inv < *(int*)(rec + 0x100) && inv >= 0) {
            tstring k("ui_chest");
            name = *g_strtable_s29ec0.lookup(k);
            if (ok) {
                tstring k2("ui_open");
                action = *g_strtable_s29ec0.lookup(k2);
            }
        } else {
            tstring k("ui_baggage");
            name = *g_strtable_s29ec0.lookup(k);
            if (ok) {
                tstring k2("ui_access_inventory");
                action = *g_strtable_s29ec0.lookup(k2);
            }
        }
    } else if (mode == 6) {
        SceneProp_s29ec0* p = &m->props[target];
        MapObject_s29ec0* o = (MapObject_s29ec0*)((char*)g_mapobjs_s29ec0[(*(int*)((char*)&g_mission_ctx + 0x2c4))].objs + p->obj * 0x68);
        name = *o->mesh_ptr();
        if (ok) {
            tstring k("ui_go");
            action = *g_strtable_s29ec0.lookup(k);
        }
    } else if (mode == 7) {
        SceneProp_s29ec0* p = &m->props[target];
        ItemRef_s29ec0 ref;
        ref.item = p->kind;
        ref.m4 = p->inv;
        char buf[0x40];
        ref.name(buf);
        name.assign(buf);
        if (ok) {
            tstring k("ui_equip");
            action = *g_strtable_s29ec0.lookup(k);
        }
    } else if (mode == 8) {
        name.assign("");
        tstring k(((g_areas_s29ec0[m->f5df0].flags >> 1) & 1) ? "ui_retreat_battle" : "ui_leave_area");
        action = *g_strtable_s29ec0.lookup(k);
    }

    // ---- phase G: draw -------------------------------------------------------------------------------
    if (name.len)
        g_gui_s29ec0.draw_string(&name, 0.027f, (unsigned)color, 1);
    if (action.len) {
        tstring sep(" ");
        g_gui_s29ec0.draw_string(&sep, 0.01f, 0xffff9d10, 1);
        g_gui_s29ec0.draw_string(&action, 0.035f, 0xffff9d10, 1);
    }

    // ---- phase H: publish ----------------------------------------------------------------------------
    f2c8 = mode;
    f2cc = target;
    f2d4 = ok;
}

// ======================================================= slice s0052ae70: functions
#undef g_resmgr_s2ae70
#define g_resmgr_s2ae70 (*(resmgr_s2ae70*)&g_resmgr)
#undef g_gui_s2ae70
#define g_gui_s2ae70 (*(gui_manager_s2ae70*)g_gui)
#undef g_strtable_s2ae70
#define g_strtable_s2ae70 (*(StrTable_s2ae70*)g_strtable)
#undef g_mission_s2ae70
#define g_mission_s2ae70 (*(Mission_s2ae70**)&g_mission)
// @ 0x52ae70
void Screen_s2ae70::init_order_markers()
{
    if (f6738 == 0) {
        f6738 = world->create_entity(9);
        float scale = 100000.0f;
        f6738->set_scale(&scale);

        f6730 = new rgl::mesh_s2ae70;
        f6734 = new rgl::mesh_s2ae70;
        f672c = new rgl::mesh_s2ae70;

        {
            tstring name("vertex_color_blend_mat");
            f6730->res = g_resmgr_s2ae70.get_material(&name);
        }
        f6734->res = f6730->res;
        f672c->res = f6730->res;

        f6738->add_extra(f6730);
        f6738->add_extra(f6734);
        f6738->add_extra(f672c);

        Shape_s2ae70* s = &shape;
        s->polygon_to_mesh(f672c);
        s->build_ribbons(f6734);
        s->build_spikes(f6730);

        scale = 100000.0f;
        f6738->set_scale(&scale);
        f6738->f140 = 0;
    }

    if (f6740 == 0) {
        f6740 = world->create_entity(9);

        f673c = new rgl::mesh_s2ae70;
        {
            tstring name("vertex_color_blend_mat");
            f673c->res = g_resmgr_s2ae70.get_material(&name);
        }
        f6740->add_extra(f673c);

        rgl::vector4 p;
        p.x = -1000.0f; p.y = -1000.0f; p.z = f654c;
        int p0 = f673c->add_point(&p);
        p.x = 1000.0f; p.y = -1000.0f; p.z = f654c;
        int p1 = f673c->add_point(&p);
        p.x = 1000.0f; p.y = 1000.0f; p.z = f654c;
        int p2 = f673c->add_point(&p);
        p.x = -1000.0f; p.y = 1000.0f; p.z = f654c;
        int p3 = f673c->add_point(&p);

        int v0, v1, v2, v3;
        rgl::vec2_s2ae70 uv;
        unsigned color;
        int zero;
        uv.x = 0.0f; uv.y = 0.0f; color = 0x663333ff; p.x = 0.0f; p.y = 0.0f; p.z = 1.0f; zero = 0;
        v0 = f673c->add_vertex(p0, &uv, &color, &p, &zero);
        uv.x = 0.0f; uv.y = 0.0f; color = 0x663333ff; p.x = 0.0f; p.y = 0.0f; p.z = 1.0f; zero = 0;
        v1 = f673c->add_vertex(p1, &uv, &color, &p, &zero);
        uv.x = 0.0f; uv.y = 0.0f; color = 0x663333ff; p.x = 0.0f; p.y = 0.0f; p.z = 1.0f; zero = 0;
        v2 = f673c->add_vertex(p2, &uv, &color, &p, &zero);
        uv.x = 0.0f; uv.y = 0.0f; color = 0x663333ff; p.x = 0.0f; p.y = 0.0f; p.z = 1.0f; zero = 0;
        v3 = f673c->add_vertex(p3, &uv, &color, &p, &zero);

        f673c->add_face(v0, v1, v2);
        f673c->add_face(v0, v2, v3);

        f673c->f164 = 1;
        f673c->f138 = 1;

        float scale = 100000.0f;
        f6740->set_scale(&scale);
        f6740->f140 = 1;
    }
}

// @ 0x52b300
void Screen_s2ae70::order_tick()
{
    unsigned id = g_mission_s2ae70->local_id;
    if ((int)id < 0)
        return;

    Mission_s2ae70* m = g_mission_s2ae70;
    Agent_s2ae70* a = m->pages[id >> 4] + (id & 0xf);
    int kind = a->kind;
    if (kind < 0 || kind >= 8)
        kind = 7;
    if ((*(int*)((char*)&g_cfg + 0x24)) && g_a1717c && (*(int*)((char*)&g_keyboard + 0x80)))
        kind = 1;

    m->order(kind, f2a0);

    if (f29c == 1) {
        f2b0 = g_ticks[f2a8] - 750000;
        f29c = 2;
        draw_order_menu();
    }
}

// @ 0x52b3c0
void Screen_s2ae70::draw_order_menu()
{
    {
        float view[2];
        view[0] = 0.5f;
        view[1] = 0.4f;
        g_gui_s2ae70.set_o568(0, 1);
        g_gui_s2ae70.set_view(view);
    }

    {
        tstring s("ui_order_group");
        g_gui_s2ae70.draw_string(g_strtable_s2ae70.lookup(s), 0.035f, 0xfff0e0d0, 1);
    }

    unsigned base = (f29c == 1) ? 0xffffffff : 0xff50ff60;

    unsigned c;
    c = (f2a0 == 0) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_hold_position");
        g_gui_s2ae70.draw_string(g_strtable_s2ae70.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 1) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_follow");
        g_gui_s2ae70.draw_string(g_strtable_s2ae70.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 2) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_charge");
        g_gui_s2ae70.draw_string(g_strtable_s2ae70.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 3) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_mount");
        g_gui_s2ae70.draw_string(g_strtable_s2ae70.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 4) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_dismount");
        g_gui_s2ae70.draw_string(g_strtable_s2ae70.lookup(s), 0.03f, c, 1);
    }
}

// ======================================================= slice s0052b5c0: functions
#undef g_strtable_s2b5c0
#define g_strtable_s2b5c0 (*(StrTable_s2b5c0*)g_strtable)
#undef g_gui_s2b5c0
#define g_gui_s2b5c0 (*(Gui_s2b5c0*)g_gui)
#undef g_items_s2b5c0
#define g_items_s2b5c0 (*(ItemKind_s2b5c0**)&g_items)
#undef g_terrain_s2b5c0
#define g_terrain_s2b5c0 (*(Terrain_s2b5c0*)g_terrain)
#undef g_game_s2b5c0
#define g_game_s2b5c0 (*(CampaignGame_s2b5c0**)&g_game)
#undef g_mission_s2b5c0
#define g_mission_s2b5c0 (*(Mission_s2b5c0**)&g_mission)
#undef g_editor_s2b5c0
#define g_editor_s2b5c0 (*(Editor_s2b5c0*)g_app_obj)
// @ 0x52b5c0
bool Mission_s2b5c0::order_agent(unsigned agent_id, ItemRef_s2b5c0* order)
{
    Agent_s2b5c0* agent = pages[agent_id >> 4] + (agent_id & 0xf);
    ItemKind_s2b5c0* ok = &g_items_s2b5c0[order->id];
    int chosen = -1;

    for (int i = 0; i < 4 && chosen == -1; i++) {
        ItemRef_s2b5c0 slot = agent->orders[i];
        if (slot.id < 0) {
            chosen = i;
            continue;
        }
        unsigned k = g_items_s2b5c0[slot.id].kind & 0xff;
        if (k == 5 || k == 6 || k == 0x12 || k == 0xa) {
            if (slot.max_amount() - (slot.count & 0xff) <= 0)
                chosen = i;
        } else if (k == 7) {
            if ((slot.count & 0xffff) == 0)
                chosen = i;
        }
    }

    if (chosen == -1) {
        unsigned k = ok->kind & 0xff;
        int cur;
        if (k == 2 || k == 3 || k == 4 || k == 8 || k == 9 || k == 0xa ||
            k == 0x10 || k == 0x11)
            cur = agent->f22c;
        else if (k == 7)
            cur = agent->f230;
        else
            goto L_rot;
        if (cur >= 0 && cur != -1) {
            chosen = cur;
            goto L_done;
        }
    L_rot:
        {
            int d = agent->f22c;
            int e = (d + 1) % 4;
            int f = agent->f230;
            if (e == f)
                e = (f + 1) % 4;
            if (e == d)
                e = (f + 2) % 4;
            chosen = e;
        }
    L_done: ;
    }

    if (chosen < 0) {
        tstring key("ui_unable_to_take");
        g_gui_s2b5c0.add(*g_strtable_s2b5c0.lookup(key), -1);
        return false;
    }

    ItemRef_s2b5c0* slot = &agent->orders[chosen];
    char newname[128];
    char oldname[128];
    char buf[512];

    if (slot->id < 0) {
        order->name(newname);
        {
            tstring key("ui_picking_up");
            sprintf(buf, g_strtable_s2b5c0.lookup(key)->c_str(), newname);
        }
        g_gui_s2b5c0.add(tstring(buf), -1);
    } else {
        slot->name(oldname);
        order->name(newname);
        {
            tstring key("ui_dropping_picking_up");
            sprintf(buf, g_strtable_s2b5c0.lookup(key)->c_str(), oldname, newname);
        }
        g_gui_s2b5c0.add(tstring(buf), -1);

        unsigned k = g_items_s2b5c0[slot->id].kind & 0xff;
        bool place = false;
        if (k == 7)
            place = ((slot->count & 0xffff) > 1);
        else if (k == 2 || k == 3 || k == 4 || k == 8 || k == 9 ||
                 k == 0x10 || k == 0x11)
            place = true;
        else
            place = (slot->max_amount() - (slot->count & 0xff) > 0);

        if (place) {
            matrix m = agent->frame->m;
            m.o.y += 0.1f;
            int mode = 6;
            add_placed(&mode, &slot->id, &slot->count, &m,
                       (int*)((char*)agent + 8));
        }
    }

    slot->id = order->id;
    slot->count = order->count;
    agent->active[chosen] = 0;

    unsigned k = g_items_s2b5c0[order->id].kind & 0xff;
    if (k == 2 || k == 3 || k == 4 || k == 8 || k == 9 || k == 0xa ||
        k == 0x10 || k == 0x11)
        agent->f22c = chosen;
    else if (k == 7)
        agent->f230 = chosen;

    refresh_agent(agent);
    agent->on_wield_reset();
    return true;
}

// =====================================================================================================
//  Mission_s2b5c0::update_ui
// =====================================================================================================
// @ 0x52ba10
void Mission_s2b5c0::update_ui()
{
    if (g_a17160 != 0 && g_a17560 == 0)
        g_game_s2b5c0->f1a20c = (g_game_s2b5c0->f1a20c == 0);

    g_mission_s2b5c0->f65d0->f140 = 0;
    g_mission_s2b5c0->f6738->f140 = 0;

    int m = g_game_s2b5c0->game_mode;
    if (m != 0) {
        if (m == 1 || m == 3 || m == 2) {
            g_mission_s2b5c0->f65dc->f140 = 0;
            g_mission_s2b5c0->f65e4->f140 = 0;
            this->update_cursor();
        } else if (m == 4) {
            g_mission_s2b5c0->f6738->f140 = 1;
            this->update_mode4();
        }
        goto L_bbc5;
    }

    if (g_game_s2b5c0->f1a20c != 0)
        return;

    if (g_a171b8 == 0 || g_a175b8 != 0)
        goto L_bb21;

    g_game_s2b5c0->f1a098 = 1 - g_game_s2b5c0->f1a098;
    {
        vector4 v;
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        g_game_s2b5c0->color = v;
    }
    g_game_s2b5c0->f1a0ac = 0;
    g_editor_s2b5c0.update_checks();
    if (g_game_s2b5c0->f1a098 == 0) {
        g_editor_s2b5c0.refresh();
        goto L_bb21;
    }

L_bb4b:
    g_mission_s2b5c0->f65dc->f140 = 0;
    this->update_map();
    goto L_bbc5;

L_bb21:
    if (g_game_s2b5c0->f1a098 == 0 && g_game_s2b5c0->f1a0c8 == 0) {
        g_mission_s2b5c0->f65e4->f140 = 0;
        this->update_dialog();
        goto L_bbc5;
    }
    goto L_bb4b;

L_bbc5:
    if (g_game_s2b5c0->game_mode == 0 && g_game_s2b5c0->f1a098 == 0) {
        long long now = g_ticks[this->f2b8];
        long long prev = this->f2c0;
        if ((float)(now - prev) * 1e-05f > 0.05f) {
            this->pick_entity();
            this->f2c0 = g_ticks[this->f2b8];
        }
    }
    if (this->f2cc == -1) {
        g_gui_s2b5c0.set_o568(0, 1);
        float v[2] = { 0.5f, 0.5f };
        g_gui_s2b5c0.set_view(v);
    }
}

// @ 0x52ba1d
// The original 0x52ba1d is the 601-byte interior continuation of update_ui: it begins
// at `push esi` with EBX=0 and the ZF from the `cmp eax,ebx` in the 13-byte prologue at
// 0x52ba10, and returns through the caller's epilogue -- it is not a callable function
// on its own.  A standalone C definition cannot reproduce an interior entry point, so
// this is the continuation body translated in full as its own function (own prologue /
// epilogue); the whole state machine is emitted once at 0x52ba10 in Mission_s2b5c0::update_ui.
void Mission_s2b5c0::update_ui_continue()
{
    if (g_a17160 != 0 && g_a17560 == 0)
        g_game_s2b5c0->f1a20c = (g_game_s2b5c0->f1a20c == 0);

    g_mission_s2b5c0->f65d0->f140 = 0;
    g_mission_s2b5c0->f6738->f140 = 0;

    int m = g_game_s2b5c0->game_mode;
    if (m != 0) {
        if (m == 1 || m == 3 || m == 2) {
            g_mission_s2b5c0->f65dc->f140 = 0;
            g_mission_s2b5c0->f65e4->f140 = 0;
            this->update_cursor();
        } else if (m == 4) {
            g_mission_s2b5c0->f6738->f140 = 1;
            this->update_mode4();
        }
        goto L_bbc5;
    }

    if (g_game_s2b5c0->f1a20c != 0)
        return;

    if (g_a171b8 == 0 || g_a175b8 != 0)
        goto L_bb21;

    g_game_s2b5c0->f1a098 = 1 - g_game_s2b5c0->f1a098;
    {
        vector4 v;
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        g_game_s2b5c0->color = v;
    }
    g_game_s2b5c0->f1a0ac = 0;
    g_editor_s2b5c0.update_checks();
    if (g_game_s2b5c0->f1a098 == 0) {
        g_editor_s2b5c0.refresh();
        goto L_bb21;
    }

L_bb4b:
    g_mission_s2b5c0->f65dc->f140 = 0;
    this->update_map();
    goto L_bbc5;

L_bb21:
    if (g_game_s2b5c0->f1a098 == 0 && g_game_s2b5c0->f1a0c8 == 0) {
        g_mission_s2b5c0->f65e4->f140 = 0;
        this->update_dialog();
        goto L_bbc5;
    }
    goto L_bb4b;

L_bbc5:
    if (g_game_s2b5c0->game_mode == 0 && g_game_s2b5c0->f1a098 == 0) {
        long long now = g_ticks[this->f2b8];
        long long prev = this->f2c0;
        if ((float)(now - prev) * 1e-05f > 0.05f) {
            this->pick_entity();
            this->f2c0 = g_ticks[this->f2b8];
        }
    }
    if (this->f2cc == -1) {
        g_gui_s2b5c0.set_o568(0, 1);
        float v[2] = { 0.5f, 0.5f };
        g_gui_s2b5c0.set_view(v);
    }
}

// =====================================================================================================
//  Mission_s2b5c0::update_cursor
// =====================================================================================================
// @ 0x52bc80
void Mission_s2b5c0::update_cursor()
{
    if (g_mission_s2b5c0->f0->f64 == 0)
        return;

    vector4 pos;
    char ok;
    // the original stores this flag as a byte (setne / mov [esp+8],cl) but passes
    // the whole 4-byte stack slot to terrain.update (mov ecx,[esp+8]); a bool/int
    // union reproduces both the byte store and the dword load.
    union { bool b; int i; } f;

    if ((*(int*)((char*)&g_keyboard + 0x390)) == 0 && g_a1745c == 0)
        goto L_bd6f;

    if ((*(float*)((char*)&g_keyboard + 0x810)) != 0.0f || (*(float*)((char*)&g_keyboard + 0x80c)) != 0.0f) {
        this->find_position(&ok, &pos);
        f.b = (g_a1745c != 0);
        if (ok) {
            int mode = 0;
            if (g_game_s2b5c0->game_mode == 2)
                mode = 1;
            else if (g_game_s2b5c0->game_mode == 3)
                mode = 2;
            if (mode != 0)
                this->f2f0 = 1;
            g_terrain_s2b5c0.update((int*)((char*)g_mission_s2b5c0 + 0x63d0), &pos,
                             g_mission_s2b5c0->f651c, mode, f.i);
            g_game_s2b5c0->f1a1cc = 1;
        }
    }

    if ((*(int*)((char*)&g_keyboard + 0x390)) != 0)
        goto L_bd78;

L_bd6f:
    if ((*(int*)((char*)&g_keyboard + 0x790)) != 0)
        goto L_bd8a;
L_bd78:
    if (g_a1745c != 0)
        return;
    if (g_a1785c == 0)
        return;
L_bd8a:
    if (this->f2f0 != 0) {
        g_terrain_s2b5c0.conv_build_scene();
        this->f2f0 = 0;
    }
}

// =====================================================================================================
//  Mission_s2b5c0::find_position
// =====================================================================================================
// @ 0x52bdb0
void Mission_s2b5c0::find_position(char* ok, vector4* pos)
{
    *ok = 0;

    vector4 near_pt, far_pt;
    ((Camera_s2b5c0*)this->view)->screen_to_world(&near_pt, &far_pt);

    capsule_s2b5c0 cap(0.0f, &near_pt, &far_pt);
    matrix m;
    cap.transform(identity(&m));

    vector4 p;
    p.x = near_pt.x;
    p.y = near_pt.y;
    float b[3];
    b[0] = far_pt.x;
    b[1] = far_pt.y;

    float best = 1000000.0f;
    shape_base_s2b5c0* shapes[8191];
    int count;
    g_mission_s2b5c0->f0->query_segment((const float*)&p, b, 0x100, shapes, &count);

    for (int i = 0; i < count; i++) {
        shape_base_s2b5c0* s = shapes[i];
        if (s->shape_type() == 2 && shape_contact_count(s, &cap, 0, 0)) {
            s->intersect(&p, &near_pt, &far_pt);
            float dx = p.x - near_pt.x;
            float dy = p.y - near_pt.y;
            float dz = p.z - near_pt.z;
            float d = (float)sqrt(dx * dx + dy * dy + dz * dz);
            if (d < best) {
                best = d;
                *pos = p;
                *ok = 1;
            }
        }
    }
}



// ======================================================= slice s0052bf40: functions
#undef g_game_s2bf40
#define g_game_s2bf40 (*(CampaignGame_s2bf40**)&g_game)
#undef g_terrain_s2bf40
#define g_terrain_s2bf40 (*(Terrain_s2bf40*)g_terrain)
// @ 0x52bf40
void Terrain_s2bf40::update(int* objs, vector4* pos, int sel, int mode, int flag)
{
    clear();

    int m = mode;
    if (m == 1)
        sel = objs[1];
    else if (m == 2)
        sel = objs[2];

    float speed;
    if (g_game_s2bf40->game_mode == 1)
        speed = g_game_s2bf40->f1a0d4;
    else if (g_game_s2bf40->game_mode == 3)
        speed = g_game_s2bf40->f1a0dc;
    else
        speed = g_game_s2bf40->f1a0d8;

    float r1 = speed * speed;
    float r2 = this->f_c3b0e0 * 10.0f + speed;
    r2 = r2 * r2;

    int row = 0;
    if (this->f27c >= 0) {
        do {
            int col = 0;
            if (this->f278 >= 0) {
                do {
                    int off = (col * 0xfb + row) * 0x94;
                    float dx = *(float*)((char*)this + off + 0x288) - pos->x;
                    float dy = *(float*)((char*)this + off + 0x28c) - pos->y;
                    float dz = *(float*)((char*)this + off + 0x290) - pos->z;
                    float d2 = dx * dx + (dz * dz + dy * dy);
                    if ((m == 0 && d2 <= r2) || d2 <= r1) {
                        char place = 1;
                        float d = (float)sqrt(d2);
                        if (!(d <= r1))
                            place = 0;
                        if (sel < 0)
                            return;
                        if (sel >= objs[0])
                            return;
                        brush(objs, &sel, &col, &d, &m, &flag, &place);
                    }
                    col++;
                } while (col <= this->f278);
            }
            row++;
        } while (row <= this->f27c);
    }

    if (m == 0) {
        if (objs[0] - 2 > 0) {
            int i = 0;
            int rowbase = 0;
            do {
                char found = 0;
                if (this->f845d984 >= 0) {
                    int j = 0;
                    do {
                        if (this->f845d988 >= 0) {
                            int k = 0;
                            if (i + 4 < 0xf) {
                                do {
                                    if (this->f845e094[k + (rowbase + j) * 5] != 0)
                                        found = 1;
                                    k++;
                                } while (k <= this->f845d988);
                            }
                        }
                        j++;
                    } while (j <= this->f845d984);
                }
                if (found)
                    this->load_layer(objs, i);
                i++;
                rowbase += 5;
            } while (i < objs[0] - 2);
        }
    } else if (m == 1) {
        this->load_layer(objs, objs[1]);
    } else if (m == 2) {
        this->load_layer(objs, objs[2]);
    }

    char* rec = (char*)this->f_c3b0dc;
    float a = *(float*)(rec + 0x68);
    float b = *(float*)(rec + 0x6c);
    float c = *(float*)(rec + 0x70);
    float h = (c + b + a) * 0.3f;

    if (this->f845d984 > 0) {
        for (int bx = 0; bx < this->f845d984; bx++) {
            if (this->f845d988 > 0) {
                for (int by = 0; by < this->f845d988; by++) {
                    for (int layer = 0; layer < 0xf; layer++) {
                        if (this->f845e030[by + (bx + layer * 5) * 5] != 0) {
                            int blend = -1;
                            if (g_use_shaders != 0 && this->f60 != 0) {
                                if (layer == 1)
                                    blend = 2;
                                else if (layer == 2)
                                    continue;
                            }
                            this->emit_particle(layer, blend, bx, by, h);
                        }
                    }
                }
            }
        }
    }
}

// =====================================================================================================
//  Terrain_s2bf40::clear
// =====================================================================================================
// @ 0x52c290
__declspec(noinline) void Terrain_s2bf40::clear()
{
    // written as a byte loop (not memset) so cl expands it exactly like the
    // original: lea edi,[ecx+0x845e030]; xor eax,eax; mov ecx,0x5d;
    // rep stosd; stosw; stosb.
    char* p = (char*)this + 0x845e030;
    for (int i = 0; i < 0x177; i++)
        p[i] = 0;
}

// ======================================================= slice s0052c2b0: functions
#undef g_game_c
#define g_game_c ((char*)g_game)
// @ 0x52c2b0
void SceneBrush_s2c2b0::brush(int* elems, int* pidx, int* cell, float* amt, int* op, int* fA, char* fB)
{
    char* me = (char*)this;
    BrushElem_s2c2b0* const e = (BrushElem_s2c2b0*)((char*)elems + 0xc);
    const int ci = cell[0] * 0x1a4 + cell[1];

    int mode = *(int*)(g_game_c + 0x1a094);

    float sel;
    if (mode == 1)      sel = *(float*)(g_game_c + 0x1a0d4);
    else if (mode == 3) sel = *(float*)(g_game_c + 0x1a0dc);
    else                sel = *(float*)(g_game_c + 0x1a0d8);

    float lo;
    if (mode == 1)      lo = *(float*)(g_game_c + 0x1a0ec);
    else if (mode == 3) lo = *(float*)(g_game_c + 0x1a0f4);
    else                lo = *(float*)(g_game_c + 0x1a0f0);

    float t = *amt / sel;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;

    float hi;
    if (mode == 1)      hi = *(float*)(g_game_c + 0x1a0e0);
    else if (mode == 3) hi = *(float*)(g_game_c + 0x1a0e8);
    else                hi = *(float*)(g_game_c + 0x1a0e4);

    const float amount = hi * (1.0f - (1.0f - lo) * t);

    int stampbase = 0;

    if (*(char*)fA == 0 || *fB == 0) {
        // ---- one of the two flags is clear: normal brush -------------------------------------------
        BrushElem_s2c2b0* E = &e[*pidx];
        float v = 0.0f;
        if (E->v) v = E->v[ci];

        if (*op == 0) {
            v = v + amount;
            if (v > 1.0f) v = 1.0f;

            int any = 0;
            for (int i = *pidx + 1; i < *elems - 2; i++) {
                float x = 0.0f;
                if (e[i].v) x = e[i].v[ci];
                if (x > 0.98f) any = 1;
            }

            if (any) {
                if (E->v == 0) {
                    E->v = (float*)operator new[](0xac440);
                    E->clear();
                }
                E->v[ci] = 1.0f;
            } else {
                if (*fB == 0) goto done;
                if (E->v == 0) {
                    if (!(v > 0.0f)) goto decayed;
                    E->v = (float*)operator new[](0xac440);
                    E->clear();
                }
                E->v[ci] = v;
            }

        decayed:
            if (*fB != 0) {
                for (int i = *pidx + 1; i < *elems - 2; i++) {
                    if (e[i].v) {
                        float old = e[i].v[ci];
                        if (old > 0.0f) {
                            float nv = old - amount;
                            if (nv < 0.0f) nv = 0.0f;
                            else if (nv > old) nv = old;
                            if (e[i].v == 0) {
                                if (!(nv > 0.0f)) continue;
                                e[i].v = (float*)operator new[](0xac440);
                                e[i].clear();
                            }
                            e[i].v[ci] = nv;
                        }
                    }
                }
            }
        } else if (*op == 1) {
            if (*(int*)(g_game_c + 0x1a108) == 1) {
                v = v - amount;
            } else if (*(int*)(g_game_c + 0x1a108) == 0) {
                v = v + amount;
            } else {
                // response = average of the nine surrounding terrain tiles, blended by `amount`
                int x = cell[0];
                int yp1 = cell[1] + 1;
                int cnt = 0;
                float sum = 0.0f;
                if (x >= 0 && x < w && yp1 >= 0 && yp1 < h) {
                    sum = *(float*)(me + (x * 0xfb + yp1) * 0x94 + 0x310)
                        + *(float*)(me + (x * 0xfb + yp1) * 0x94 + 0x30c);
                    cnt = 1;
                }
                int xp1 = x + 1;
                if (xp1 >= 0) {
                    if (xp1 < w && yp1 >= 0 && yp1 < h) {
                        sum += *(float*)(me + (xp1 * 0xfb + yp1) * 0x94 + 0x310)
                             + *(float*)(me + (xp1 * 0xfb + yp1) * 0x94 + 0x30c);
                        cnt++;
                    }
                    if (xp1 >= 0 && xp1 < w && cell[1] >= 0 && cell[1] < h) {
                        cnt++;
                        sum += *(float*)(me + (xp1 * 0xfb + cell[1]) * 0x94 + 0x310)
                             + *(float*)(me + (xp1 * 0xfb + cell[1]) * 0x94 + 0x30c);
                    }
                }
                int ym1 = cell[1] - 1;
                if (xp1 >= 0 && xp1 < w && ym1 >= 0 && ym1 < h) {
                    sum += *(float*)(me + (xp1 * 0xfb + ym1) * 0x94 + 0x310)
                         + *(float*)(me + (xp1 * 0xfb + ym1) * 0x94 + 0x30c);
                    cnt++;
                }
                if (x >= 0 && x < w && ym1 >= 0 && ym1 < h) {
                    sum += *(float*)(me + (x * 0xfb + ym1) * 0x94 + 0x310)
                         + *(float*)(me + (x * 0xfb + ym1) * 0x94 + 0x30c);
                    cnt++;
                }
                int xm1 = x - 1;
                if (xm1 >= 0) {
                    if (xm1 < w && ym1 >= 0 && ym1 < h) {
                        cnt++;
                        sum += *(float*)(me + (xm1 * 0xfb + ym1) * 0x94 + 0x310)
                             + *(float*)(me + (xm1 * 0xfb + ym1) * 0x94 + 0x30c);
                    }
                    if (xm1 >= 0 && xm1 < w && cell[1] >= 0 && cell[1] < h) {
                        cnt++;
                        sum += *(float*)(me + (xm1 * 0xfb + cell[1]) * 0x94 + 0x310)
                             + *(float*)(me + (xm1 * 0xfb + cell[1]) * 0x94 + 0x30c);
                    }
                    if (xm1 >= 0 && xm1 < w && yp1 >= 0 && yp1 < h) {
                        sum += *(float*)(me + (xm1 * 0xfb + yp1) * 0x94 + 0x310)
                             + *(float*)(me + (xm1 * 0xfb + yp1) * 0x94 + 0x30c);
                        cnt++;
                    }
                }
                if (x >= 0 && x < w && cell[1] >= 0 && cell[1] < h) {
                    cnt++;
                    sum += *(float*)(me + (x * 0xfb + cell[1]) * 0x94 + 0x310)
                         + *(float*)(me + (x * 0xfb + cell[1]) * 0x94 + 0x30c);
                }
                float centre = *(float*)(me + (x * 0xfb + cell[1]) * 0x94 + 0x310)
                             + *(float*)(me + (x * 0xfb + cell[1]) * 0x94 + 0x30c);
                v = ((1.0f - amount) * centre + ((centre + sum) * amount) / (float)(cnt + 1)) + v - centre;
            }
            if (E->v == 0) {
                if (!(v > 0.0f)) goto done;
                E->v = (float*)operator new[](0xac440);
                E->clear();
            }
            E->v[ci] = v;
        } else if (*op == 2) {
            // blend the tile colour toward the brush colour
            unsigned argb = 0xffffffff;
            if (E->v) argb = *(unsigned*)&E->v[ci];
            float col[4];
            unpack_color(col, &argb);
            float k = 1.0f - amount;
            float out[4];
            out[0] = col[0] * k + amount * *(float*)(g_game_c + 0x1a0f8);
            out[1] = col[1] * k + amount * *(float*)(g_game_c + 0x1a0fc);
            out[2] = col[2] * k + amount * *(float*)(g_game_c + 0x1a100);
            out[3] = 1.0f;   // original copies an uninitialised slot here; it saturates alpha to 255
            unsigned c = pack_color(out);
            E->put(cell, cell + 1, (int*)&c);
        }
    } else {
        // ---- both flags set ------------------------------------------------------------------------
        if (*op == 0) {
            for (int i = 0; i < *elems - 2; i++) {
                float v = 0.0f;
                if (e[i].v) {
                    v = e[i].v[ci] - amount;
                    if (v < 0.0f) v = 0.0f;
                }
                if (e[i].v == 0) {
                    if (!(v > 0.0f)) continue;
                    e[i].v = (float*)operator new[](0xac440);
                    e[i].clear();
                }
                e[i].v[ci] = v;
            }
            stampbase = 4;
        } else if (*op == 1) {
            BrushElem_s2c2b0* E = &e[*pidx];
            float v = 0.0f;
            if (E->v) v = E->v[ci];
            float nv = (1.0f - amount) * v;
            if (nv < 0.0f) nv = 0.0f;
            if (E->v == 0) {
                if (!(nv > 0.0f)) goto done;
                E->v = (float*)operator new[](0xac440);
                E->clear();
            }
            E->v[ci] = nv;
        } else if (*op == 2) {
            BrushElem_s2c2b0* E = &e[*pidx];
            unsigned argb = 0xffffffff;
            if (E->v) argb = *(unsigned*)&E->v[ci];
            float col[4];
            unpack_color(col, &argb);
            float k = 1.0f - amount;
            float out[4];
            out[0] = col[0] * k + amount;
            out[1] = col[1] * k + amount;
            out[2] = col[2] * k + amount;
            out[3] = 1.0f;   // original copies an uninitialised slot here; it saturates alpha to 255
            unsigned c = pack_color(out);
            E->put(cell, cell + 1, (int*)&c);
        }
    }

done:
    int P1[2], P2[2], P3[2];
    this->clamp(P1, cell);
    int up[2]   = { cell[0] + 1, cell[1] + 1 };
    int down[2] = { cell[0] - 1, cell[1] - 1 };
    this->clamp(P2, up);
    this->clamp(P3, down);

    if (stampbase < 0xf) {
        int k = stampbase * 5;
        do {
            me[0x845e030 + (P1[0] + k) * 5 + P1[1]] = 1;
            me[0x845e030 + (P2[0] + k) * 5 + P2[1]] = 1;
            me[0x845e030 + (P3[0] + k) * 5 + P3[1]] = 1;
            k += 5;
        } while (k < 0x4b);
    }
}

// ======================================================= slice s0052ce00: functions
// @ 0x52ce00
void Elem_s2ce00::clear()
{
    if (v != 0) {
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                v[x * 0x1a4 + y] = 0.0f;
    }
}

// @ 0x52ce70
void Elem_s2ce00::put(int* ix, int* iy, int* value)
{
    if (v == 0) {
        if (*value == -1)
            return;
        v = (float*)operator new[](0xac440);
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                ((int*)v)[x * 0x1a4 + y] = -1;
    }
    ((int*)v)[*ix * 0x1a4 + *iy] = *value;
}

// @ 0x52cef0
int* Scene_s2ce00::clamp(int* out, int* in)
{
    int ox;
    int ix = in[0] / *(int*)((char*)this + 0x843bd70);
    if (ix < 0) {
        ox = 0;
    } else {
        int m = *(int*)((char*)this + 0x845d984);
        ox = m - 1;
        if (ix < m)
            ox = ix;
    }

    int iy = in[1] / *(int*)((char*)this + 0x843bd70);
    if (iy < 0) {
        out[0] = ox;
        out[1] = 0;
        return out;
    }
    int mx = *(int*)((char*)this + 0x845d988);
    if (iy >= mx) {
        out[0] = ox;
        out[1] = mx - 1;
        return out;
    }
    out[0] = ox;
    out[1] = iy;
    return out;
}
