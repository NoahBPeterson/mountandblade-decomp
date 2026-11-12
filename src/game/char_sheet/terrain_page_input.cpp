#include "./terrain_page_input.h"
#include "../terrain_a/terrain.h"      // g_terrain 0xa76630 (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"        // MapWidget: the preview widget whose frustum (+0x1ac) takes the camera
#include "../key_names.h"
#include "../../include/render_flags.h"
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)

unsigned g_5e2464 = (unsigned)g_bit[2];                       // 0x5e2464 (no reader in this source; set at startup like its twin)
unsigned g_5e2468 = (unsigned)g_bit[4];                       // 0x5e2468

// @ 0x4f3570
void TerrainPage::read_fields()
{
    water_on = (g_use_shaders == 0) ? 1 : 0;                // "Enable water" holds the inverse of the shader flag
    river_on = g_terrain.get_bits(0xa2, 0xa3);                      // the raw two bits of the "Enable river" checkbox
    terrain_on = g_terrain.get_bits(0x7f, 0x7f);
    terraingen_on = g_terrain.get_bits(0x3f, 0x3f);
    camera_x = (float)g_terrain.get_bits(0x60, 0x69);
    camera_y = (float)g_terrain.get_bits(0x6a, 0x73);
    camera_z = (float)g_terrain.get_bits(0x87, 0x8d);
    fog = (float)g_terrain.get_bits(0x80, 0x86) * 0.01f;
    layer_bits[0] = g_terrain.get_bits(0x00, 0x1e);
    layer_bits[1] = g_terrain.get_bits(0x20, 0x3e);
    layer_bits[2] = g_terrain.get_bits(0x40, 0x5e);
    water_level = g_terrain.get_bits(0x8e, 0x94);
    water_height = (float)g_terrain.get_bits(0x95, 0x9b) * 0.01f;
    terrain_type = g_terrain.get_bits_inline(0x9c, 0x9f);   // the body of Terrain::get_bits expanded in place:
    map_size = g_terrain.get_bits_inline(0xa0, 0xa1);               // bits[4] >> 28 & (g_bit[4] - 1), bits[5] & (g_bit[2] - 1)
}

// @ 0x4f37f0
void TerrainPage::simulate(float dt)
{
    if (g_keyboard.down[0x4c])
        pitch = pitch - dt * 1.1f;
    if (g_keyboard.down[0x50])
        pitch = dt * 1.1f + pitch;
    if (g_keyboard.down[0x4e])
        zoom = (dt * 2.1f + 1.0f) * zoom;
    if (g_keyboard.down[0x4a])
        zoom = (1.0f - dt * 2.1f) * zoom;

    rot2 m;                                                 // the camera frame: r[0] = strafe axis, r[1] = forward
    identity_2x2(&m.r[0].x);                                // 0x4f3be0
    float a = -yaw;
    m.rotate_2x2(a);                                        // 0x4f3b40

    if (g_keyboard.down[0x1e]) {
        rot2_row d = m.r[0] * 50.0f * dt;
        view_pos.x = view_pos.x - d.x;
        view_pos.y = view_pos.y - d.y;
    }
    if (g_keyboard.down[0x20]) {
        rot2_row d = m.r[0] * 50.0f * dt;
        view_pos.x = d.x + view_pos.x;
        view_pos.y = d.y + view_pos.y;
    }
    if (g_keyboard.down[0x11]) {
        rot2_row d = m.r[1] * 50.0f * dt;
        view_pos.x = d.x + view_pos.x;
        view_pos.y = d.y + view_pos.y;
    }
    if (g_keyboard.down[0x1f]) {
        rot2_row d = m.r[1] * 50.0f * dt;
        view_pos.x = view_pos.x - d.x;
        view_pos.y = view_pos.y - d.y;
    }
    if (view_pos.x < 0.0f)
        view_pos.x = 0.0f;
    else if (view_pos.x > 250.0f)
        view_pos.x = 250.0f;
    if (view_pos.y < 0.0f)
        view_pos.y = 0.0f;
    else if (view_pos.y > 250.0f)
        view_pos.y = 250.0f;

    if (g_gui.mouse[0] < 0.5f) {
        if (g_keyboard.down[0xe1]) {
            yaw = (g_keyboard.axis[0] * dt) * 1.7f + yaw;
            pitch = pitch - (g_keyboard.axis[1] * dt) * 1.7f;
        }
        if (g_keyboard.down[0xe0])
            zoom = ((g_keyboard.axis[1] * dt) * 0.5f + 1.0f) * zoom;
    }

    if (pitch < 0.1f)
        pitch = 0.1f;
    else if (pitch > 1.5f)
        pitch = 1.5f;
    if (zoom < 1.5f)
        zoom = 1.5f;
    else if (zoom > 400.5f)
        zoom = 400.5f;
}

// @ 0x4f3b40
// Rotate the 2x2 matrix by `angle`: row0 -> row0*c + row1*s, row1 -> row1*c - row0*s, through the engine's inline
// row operators (rot2.h: rot2_row). Byte-exact only in this spelling: the sine a double local (bound to the operators'
// `const float&`, so cl makes a float temporary of it at each use: the original's `fst [esp]` after fsin and the
// second copy in the angle's slot) and the cosine written out at each use (it stays on the x87 stack).
// Found 2026-11-10 by a grid over the shapes an outside analysis suggested (work/match/rotgrid/).
void rot2::rotate_2x2(float angle)
{
    double s = sin(angle);
    rot2_row r0 = r[0] * cos(angle) + r[1] * s;
    rot2_row r1 = r[1] * cos(angle) - r[0] * s;
    r[0] = r0;
    r[1] = r1;
}

// @ 0x4f3be0
__declspec(noinline) void identity_2x2(float* m)
{
    struct V2 { float x, y; };
    V2 t, u;
    t.x = 1.0f; t.y = 0.0f; u.x = 0.0f; u.y = 1.0f;
    *(V2*)&m[0] = t;
    *(V2*)&m[2] = u;
}

// @ 0x4f3c30
void TerrainPage::paste_clipboard()
{
    // Write the hex words and the hex field from the clipboard's text. Only a successful read re-reads the fields and
    // refreshes the page (0x4f3cc7); a closed clipboard returns without CloseClipboard (0x4f3c40).
    if (!OpenClipboard(0))
        return;
    if (IsClipboardFormatAvailable(CF_TEXT)) {                  // 0x4f3c48
        HANDLE h = GetClipboardData(CF_TEXT);
        if (h) {
            const char* p = (const char*)GlobalLock(h);
            if (p) {
                { tstring s0(p); g_terrain.parse(s0); }   // 0x4658c0
                { tstring s(p); float r[2]; hexbox->set_text(r, &s); }
                GlobalUnlock(h);
                read_fields();
                update();
            }
        }
    }
    CloseClipboard();
}

// @ 0x4f3cf0
void TerrainPage::path(float dt)
{
    vector4 pos;                                            // the eye (w stays uninitialised, copied into m.o.w)
    mat_self m;
    identity_pos((::pos*)&m);                                 // 0x40cd00 (cdecl, returns its argument)
    pos.x = view_pos.x;
    pos.y = view_pos.y;
    pos.z = view_pos.z;
    m.o = pos;
    float angle = 1.5707964f;                               // +90 degrees
    ((rgl::matrix*)&m)->rotate_about_s(&angle);             // 0x40e570
    angle = -yaw;
    ((rgl::matrix*)&m)->rotate_about_f(&angle);             // 0x40e670
    angle = -pitch;
    ((rgl::matrix*)&m)->rotate_about_s(&angle);
    m.o += m.u * zoom;                                      // inline operator* temporary shares the eye's frame slot

    rgl::frustum* f = &cam;                  // page +0x70
    f->m = m;
    f->build_planes();                    // 0x41e4f0: thiscall on this+0x70
    *(FrustumBlock*)&view->scene3d.fr = *(FrustumBlock*)f;             // 0x120 bytes: page +0x70 -> world +0x1ac
    g_terrain.update_particles(&m);        // 0x4f3e10
}
