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
    f290 = dt * C_549D74 * f2bc + f290;
    f294 = dt * C_549D74 * f2c0 + f294;
    f2b0 = dt * f2c4 * C_549D74 + f2b0;

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
    if (f290 < *(float*)&g_5aa120)
        f290 = *(float*)&g_5aa120;
    else if (f290 > *(float*)&g_5aa118)
        f290 = *(float*)&g_5aa118;
    if (f294 < *(float*)&g_5aa124)
        f294 = *(float*)&g_5aa124;
    else if (f294 > *(float*)&g_5aa11c)
        f294 = *(float*)&g_5aa11c;

    // --- the pitch follows the zoom ----------------------------------------------------------------
    f2b4 = f2b8 * C_590F7C + C_590E68;
    if (f2b4 < C_549D7C) {
        f2b4 = 0.1f;
    } else if (f2b4 > C_54A534) {
        f2b4 = 1.5707964f;
    }

    // --- follow the selected party while the map is live -------------------------------------------
    if (g_game->f88) {
        if (g_game->parties.is_live(g_game->selected)) {
            Party* p = g_game->parties.get_party(g_game->selected);   // the slice cast the dense index to a pointer; 0x48ad00 returns the index, the page arithmetic is get_party
            vector4 rect = *(vector4*)&p->pos;
            rect.z = rect.z + C_549CDC;
            float dx = rect.x - f290;
            float dy = rect.y - f294;
            float dz = rect.z - f298;
            if (f2b8 * 0.05f >= sqrtf(dz * dz + dy * dy + dx * dx)) {
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
    identity(&frame.s);
    float ang = 1.5707964f;
    frame.rotate_fu(&ang);
    ang = -f2b0;
    frame.rotate_su(&ang);
    ang = -f2b4;
    frame.rotate_fu(&ang);
    f29c = 0.0f;
    f290 = frame.f.x * f2b8 + f290;
    f294 = frame.f.y * f2b8 + f294;
    f298 = frame.f.z * f2b8 + f298;
    float hz = frame.u.z * C_549D70;
    float hx = f290 - frame.u.x * C_549D70;
    float hy = f294 - frame.u.y * C_549D70;
    float q = 0.0f;
    ((World*)g_game)->scene_query(g_scene, (vector4*)&hx, (vector4*)&q);
    q = q + C_549D70;
    if (f298 < q)
        f298 = q;

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
    ((Envelope*)g_scene)->set_fade(&fogdist, &fog);

    // --- frustum planes, the frame into this+0x238, then the widget's label matrix and the scene ---
    memcpy((char*)this + 0x238, &frame, 0x40);
    ((frustum*)((char*)this + 0x170))->build_planes();
    MapWidget* w = f30;
    float nx = -frame.f.x;
    memcpy((char*)w + 0x1ac, (char*)this + 0x170, 0x120);
    float ny = -frame.f.y;
    float nz = -frame.f.z;
    WorldCam* sc = g_scene;
    sc->f30c.x = nx;
    sc->f30c.y = ny;
    sc->f30c.z = nz;
    sc->f30c.w = frame.s.w;
    sc->f31c.x = f290;
    sc->f31c.y = f294;
    sc->f31c.z = f298;
    sc->f31c.w = f29c;
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
    widget_ctor(w);
    *(void**)w = (void*)0x598698;
    scene_ctor((scene*)w->scene_bytes());
    *(char*)((char*)w + 0x2e6) = 1;
    *(vector2*)((char*)w + 0xe0) = vector2(0.0f, 0.0f);
    *(vector2*)((char*)w + 0xf0) = vector2(1.0f, 0.75f);
    *(int*)((char*)w + 0x308) = 0x200;
    return w;
}
// @ 0x4ed6b0
void* __fastcall dtor_4ed6b0(MapWidget* w, int, unsigned flags)
{
    *(void**)w = (void*)0x598698;
    scene_dtor((scene*)w->scene_bytes());
    widget_dtor(w);
    if (flags & 1)
        operator delete(w);
    return w;
}
