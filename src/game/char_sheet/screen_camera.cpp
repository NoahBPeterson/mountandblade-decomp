#include "../../include/image_addrs.h"
#include "./screen_camera.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../overlay.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ece20
void Screen::camera_update(float dt)
{
    // --- pan: the stick and yaw rates are integrated at 10x ---------------------------------------
    // the original shares dt*10 and spills the y increment as a float before adding it (x87 register pressure);
    // the volatile temporary keeps that rounding, which an in-register sum does not
    float r = dt * C_549D74;
    volatile float ry = r * f2c0;
    f290 = r * f2bc + f290;
    f294 = ry + f294;
    f2b0 = dt * f2c4 * 10.0f + f2b0;        // a literal: with the 0x549d74 global cl evaluates (10 * f2c4) * dt

    float max_zoom = C_598690;                  // 105
    if (g_a17148 != 0 && g_a1717c != 0 && g_5a9fbc != 0) {
        f2b0 = 0.0f;
        f2c4 = 0.0f;
        max_zoom = C_59868C;                    // 235
    }

    // --- zoom -------------------------------------------------------------------------------------
    if (f2c8 > C_54A6D0 || f2c8 < C_598688) {
        f2b8 = dt * f2c8 * C_549D74 + f2b8;
        if (f2b8 < C_590D8C) {
            f2b8 = 3.5f;
        } else if (f2b8 > max_zoom) {
            f2b8 = max_zoom;
        }
        camera_mark_entities();
    }

    // --- ease the four rates back to zero (each through its own 0-floor clamp) ---------------------
    float d = 1.0f - dt * C_590AF0;
    if (d < C_549CA8)
        d = 0.0f;
    f2bc *= d;
    d = 1.0f - dt * C_590AF0;
    if (d < C_549CA8)
        d = 0.0f;
    f2c0 *= d;
    d = 1.0f - dt * C_590AF0;
    if (d < C_549CA8)
        d = 0.0f;
    f2c4 *= d;
    d = 1.0f - dt * C_590AF0;
    if (d < C_549CA8)
        d = 0.0f;
    f2c8 *= d;

    // --- keep the camera over the map --------------------------------------------------------------
    if (f290 < g_map_min_x)
        f290 = g_map_min_x;
    else if (f290 > g_map_max_x)
        f290 = g_map_max_x;
    if (f294 < g_map_min_y)
        f294 = g_map_min_y;
    else if (f294 > g_map_max_y)
        f294 = g_map_max_y;

    // --- the pitch follows the zoom ----------------------------------------------------------------
    f2b4 = f2b8 * C_590F7C + C_590E68;
    if (f2b4 < C_549D7C) {
        f2b4 = 0.1f;
    } else if (f2b4 > C_54A534) {
        f2b4 = 1.5707964f;
    }

    // --- follow the selected party while the map is live -------------------------------------------
    if (g_game->f88) {
        if (((PartyMgrW*)(void*)&g_game->parties)->is_live(g_game->selected)) {
            // get_party (include/party.h) expanded: the id's dense index is Map10007::lookup (0x48ad00), then the page arithmetic
            int pidx = ((Map10007*)(void*)&g_game->parties)->lookup(g_game->selected);
            Party* p = (Party*)((char*)g_game->parties.pages._Myfirst[pidx >> 4] + (pidx & 0xf) * 0x5ac8);
            vector4 rect = *(vector4*)&p->pos;
            rect.z = rect.z + C_549CDC;
            float dx = rect.x - f290;
            float dy = rect.y - f294;
            float dz = rect.z - f298;
            // pan toward the party only while it is farther than 5% of the zoom (0x4ed165: fcompp; test ah, 5; jp skip)
            if (f2b8 * 0.05f < sqrtf(dz * dz + dy * dy + dx * dx)) {
                if (dx < C_5915A0)
                    dx = -10.0f;
                else if (dx > C_549D74)
                    dx = 10.0f;
                if (dy < C_5915A0)
                    dy = -10.0f;
                else if (dy > C_549D74)
                    dy = 10.0f;
                f2a0 += (dx * C_5935C8) * dt;
                f2a4 += (dy * C_5935C8) * dt;
                f2a8 += (dz * C_5935C8) * dt;
            }
        } else {
            f2a0 *= C_549CA8;
            f2a4 *= C_549CA8;
            f2a8 *= C_549CA8;
        }
    }

    // --- integrate the velocity and decay it (1 - 2*dt) -------------------------------------------
    f290 = dt * f2a0 + f290;
    f294 = dt * f2a4 + f294;
    f298 = dt * f2a8 + f298;
    float d2 = 1.0f - (dt + dt);
    f2a0 *= d2;
    f2a4 *= d2;
    f2a8 *= d2;

    // --- the frame: identity, +90deg, -yaw, then -pitch; the eye becomes frame.o --------------------
    mat4 frame;
    identity_pos((pos*)&frame.s);
    frame.o.x = f290;
    frame.o.y = f294;
    frame.o.z = f298;
    frame.o.w = f29c;
    float ang = 1.5707964f;
    ((rgl::matrix*)&frame)->rotate_about_s(&ang);
    ang = -f2b0;
    ((rgl::matrix*)&frame)->rotate_about_f(&ang);
    ang = -f2b4;
    ((rgl::matrix*)&frame)->rotate_about_s(&ang);
    float q = 0.0f;
    frame.o.x = frame.u.x * f2b8 + frame.o.x;
    frame.o.y = frame.u.y * f2b8 + frame.o.y;
    frame.o.z = frame.u.z * f2b8 + frame.o.z;
    // the ground probe point under the eye (3-component constructor: w is never written)
    vector4 h(frame.o.x - frame.u.x * C_549D70, frame.o.y - frame.u.y * C_549D70, frame.o.z - frame.u.z * C_549D70);
    ground_height((CellGrid*)g_level, (const float*)&h, &q);
    q = q + C_549D70;
    if (frame.o.z < q)
        frame.o.z = q;

    // the fog distance parameter (the 0.9 / 0.94 / 0.98 range steps shift it progressively)
    float g1 = g_game->f13b40 + 0.4f;
    float fogdist = (((g_game->f13b3c + 1.0f) * g1) * g1) * 0.08f / (f2b8 + 10.0f);
    if (g_game->f13b40 > C_549D6C) {
        fogdist = fogdist * C_590FA4;
        if (g_game->f13b40 > C_591AF4) {
            fogdist = fogdist * C_590C48;
            if (g_game->f13b40 > C_5911D8)
                fogdist = fogdist * C_54B8B0;
        }
    }
    unsigned fog = pack_color((const vector4*)g_game->fog_color);
    ((envelope*)g_level)->set_fade(&fogdist, (const int*)&fog);

    // --- frustum planes, the frame into this+0x238, then the widget's label matrix and the scene ---
    memcpy((char*)this + 0x238, &frame, 0x40);
    ((frustum*)((char*)this + 0x170))->build_planes();
    MapWidget* w = f30;
    memcpy((char*)w + 0x1ac, (char*)this + 0x170, 0x120);
    WorldCam* sc = g_level;
    sc->f30c = vector4(-frame.u.x, -frame.u.y, -frame.u.z);   // w never written (3-component constructor)
    sc->f31c = frame.o;
    if (!g_5a9fd8) {
        *(mat4*)&g_listener.pos = frame;
        // FMOD wants y/z swapped (the engine is z-up)
        listener_set(&g_listener.up.x, 0, g_listener.vel.x, g_listener.vel.z, g_listener.vel.y,
                     g_listener.dir.x, g_listener.dir.z, g_listener.dir.y);
    }
}

MapWidget* __fastcall ctor_4ed630(MapWidget* w);   // 0x4ed630 (ecx = object)

// @ 0x4ed630
MapWidget* __fastcall ctor_4ed630(MapWidget* w)
{
    ((MissionUi*)w)->MissionUi::MissionUi();   // 0x48ae20
    *(void**)w = (void*)vtbl_598698;
    ((scene*)w->scene_bytes())->scene::scene();   // 0x429740
    *(char*)((char*)w + 0x2e6) = 1;
    *(vector2*)((char*)w + 0xe0) = vector2(0.0f, 0.0f);
    *(vector2*)((char*)w + 0xf0) = vector2(1.0f, 0.75f);
    *(int*)((char*)w + 0x308) = 0x200;
    return w;
}
// @ 0x4ed6b0
void* __fastcall dtor_4ed6b0(MapWidget* w, int, unsigned flags)
{
    *(void**)w = (void*)vtbl_598698;
    ((scene*)w->scene_bytes())->scene::~scene();   // 0x41f4e0
    ((Buffer*)w)->Buffer::~Buffer();   // 0x487c00
    if (flags & 1)
        operator delete(w);
    return w;
}
