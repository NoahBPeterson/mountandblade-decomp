#include "../../include/image_addrs.h"
#include "./screen_camera.h"
#include "../config/config_block.h"      // g_cfg (0x5a9f98); cheat_mode is +0x24 = 0x5a9fbc
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui/container.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128

#include "../key_names.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// @ 0x4ece20
void CampaignScreen::camera_update(float dt)
{
    // --- pan: the stick and yaw rates are integrated at 10x ---------------------------------------
    // the original shares dt*10 and spills the y increment as a float before adding it (x87 register pressure);
    float r = dt * 10.0f;
    float ry = r * f2c0;
    target.x = r * f2bc + target.x;
    target.y = ry + target.y;
    f2b0 = dt * f2c4 * 10.0f + f2b0;        // a literal: with the 0x549d74 global cl evaluates (10 * f2c4) * dt

    float max_zoom = 105.0f;                  // 105
    if (g_keyboard.down[0x1d] != 0 && g_keyboard.down[0x2a] != 0 && g_cfg.cheat_mode != 0) {
        f2b0 = 0.0f;
        f2c4 = 0.0f;
        max_zoom = 235.0f;                    // 235
    }

    // --- zoom -------------------------------------------------------------------------------------
    if (f2c8 > 1e-5f || f2c8 < -1e-05f) {
        f2b8 = dt * f2c8 * 10.0f + f2b8;
        if (f2b8 < 3.5f) {
            f2b8 = 3.5f;
        } else if (f2b8 > max_zoom) {
            f2b8 = max_zoom;
        }
        camera_mark_entities();
    }

    // --- ease the four rates back to zero (each through its own 0-floor clamp) ---------------------
    float d = 1.0f - dt * 9.8f;
    if (d < 0.0f)
        d = 0.0f;
    f2bc *= d;
    d = 1.0f - dt * 9.8f;
    if (d < 0.0f)
        d = 0.0f;
    f2c0 *= d;
    d = 1.0f - dt * 9.8f;
    if (d < 0.0f)
        d = 0.0f;
    f2c4 *= d;
    d = 1.0f - dt * 9.8f;
    if (d < 0.0f)
        d = 0.0f;
    f2c8 *= d;

    // --- keep the camera over the map --------------------------------------------------------------
    if (target.x < g_map_min_x)
        target.x = g_map_min_x;
    else if (target.x > g_map_max_x)
        target.x = g_map_max_x;
    if (target.y < g_map_min_y)
        target.y = g_map_min_y;
    else if (target.y > g_map_max_y)
        target.y = g_map_max_y;

    // --- the pitch follows the zoom ----------------------------------------------------------------
    f2b4 = f2b8 * 0.014f + C_590E68;
    if (f2b4 < 0.1f) {
        f2b4 = 0.1f;
    } else if (f2b4 > 1.5707964f) {
        f2b4 = 1.5707964f;
    }

    // --- follow the selected party while the map is live -------------------------------------------
    if (g_game->f88) {
        if (g_game->parties.is_live(g_game->selected)) {
            // get_party (include/party.h) expanded: the id's dense index is Map10007::lookup (0x48ad00), then the page arithmetic
            int pidx = g_game->parties.lookup(g_game->selected);
            Party* p = (Party*)((char*)g_game->parties.pages._Myfirst[pidx >> 4] + (pidx & 0xf) * 0x5ac8);
            vector4 rect = *(vector4*)&p->pos;
            rect.z = rect.z + C_549CDC;
            float dx = rect.x - target.x;
            float dy = rect.y - target.y;
            float dz = rect.z - target.z;
            // pan toward the party only while it is farther than 5% of the zoom (0x4ed165: fcompp; test ah, 5; jp skip)
            if (f2b8 * 0.05f < sqrtf(dz * dz + dy * dy + dx * dx)) {
                if (dx < -10.0f)
                    dx = -10.0f;
                else if (dx > 10.0f)
                    dx = 10.0f;
                if (dy < -10.0f)
                    dy = -10.0f;
                else if (dy > 10.0f)
                    dy = 10.0f;
                velocity.x += (dx * 4.0f) * dt;
                velocity.y += (dy * 4.0f) * dt;
                velocity.z += (dz * 4.0f) * dt;
            }
        } else {
            velocity.x *= 0.0f;
            velocity.y *= 0.0f;
            velocity.z *= 0.0f;
        }
    }

    // --- integrate the velocity and decay it (1 - 2*dt) -------------------------------------------
    target.x = dt * velocity.x + target.x;
    target.y = dt * velocity.y + target.y;
    target.z = dt * velocity.z + target.z;
    float d2 = 1.0f - (dt + dt);
    velocity.x *= d2;
    velocity.y *= d2;
    velocity.z *= d2;

    // --- the frame: identity, +90deg, -yaw, then -pitch; the eye becomes frame.o --------------------
    mat4 frame;
    identity_pos((pos*)&frame.s);
    frame.o.x = target.x;
    frame.o.y = target.y;
    frame.o.z = target.z;
    frame.o.w = target.w;
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
    vector4 h(frame.o.x - frame.u.x * 0.5f, frame.o.y - frame.u.y * 0.5f, frame.o.z - frame.u.z * 0.5f);
    g_game->ground_height((rgl::world*)g_script.level, (const float*)&h, &q);
    q = q + 0.5f;
    if (frame.o.z < q)
        frame.o.z = q;

    // the fog distance parameter (the 0.9 / 0.94 / 0.98 range steps shift it progressively)
    float g1 = g_game->f13b40 + 0.4f;
    float fogdist = (((g_game->f13b3c + 1.0f) * g1) * g1) * 0.08f / (f2b8 + 10.0f);
    if (g_game->f13b40 > 0.9f) {
        fogdist = fogdist * C_590FA4;
        if (g_game->f13b40 > 0.94f) {
            fogdist = fogdist * 1.6f;
            if (g_game->f13b40 > 0.98f)
                fogdist = fogdist * C_54B8B0;
        }
    }
    unsigned fog = pack_color((const vector4*)g_game->fog_color);
    g_script.level->set_fade(&fogdist, (const int*)&fog);   // 0x4624c0

    // --- frustum planes, the frame into this+0x238, then the widget's label matrix and the scene ---
    memcpy((char*)this + 0x238, &frame, 0x40);
    ((frustum*)((char*)this + 0x170))->build_planes();
    MapWidget* w = f30;
    memcpy(&w->scene3d.fr, (char*)this + 0x170, 0x120);
    WorldCam* sc = (WorldCam*)g_script.level;
    sc->f30c = vector4(-frame.u.x, -frame.u.y, -frame.u.z);   // w never written (3-component constructor)
    sc->f31c = frame.o;
    if (!g_cfg.disable_sound) {
        *(mat4*)&g_sound_mgr.frame = frame;
        // FMOD wants y/z swapped (the engine is z-up)
        FSOUND_3D_Listener_SetAttributes(&g_sound_mgr.frame.o.x, 0, g_sound_mgr.frame.f.x, g_sound_mgr.frame.f.z, g_sound_mgr.frame.f.y,
                     g_sound_mgr.frame.u.x, g_sound_mgr.frame.u.z, g_sound_mgr.frame.u.y);
    }
}

// @ 0x4ed630
MapWidget::MapWidget() throw()
{
    scene3d.f16a = 1;
    *(vector2*)&fE0 = vector2(0.0f, 0.0f);
    *(vector2*)&screen[0] = vector2(1.0f, 0.75f);
    scene3d.flags = 0x200;
}
