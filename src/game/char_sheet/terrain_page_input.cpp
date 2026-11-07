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
    terrain_type = (g_terrain.bits[4] >> 0x1c) & ((unsigned)g_bit[4] - 1);
    map_size = g_terrain.bits[5] & ((unsigned)g_bit[2] - 1);
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

    vector2 d;
    vector2 m[2];                                           // the camera frame: m[0] = strafe axis, m[1] = forward
    identity_2x2(&m[0].x);                                  // 0x4f3be0
    float a = -yaw;
    ((rot2*)&m[0].x)->rotate_2x2(a);                        // 0x4f3b40 turns m[0].xy / m[1].xy

    if (g_keyboard.down[0x1e]) {
        d.x = (m[0].x * 50.0f) * dt;
        d.y = (m[0].y * 50.0f) * dt;
        view_pos.x = view_pos.x - d.x;
        view_pos.y = view_pos.y - d.y;
    }
    if (g_keyboard.down[0x20]) {
        d.x = (m[0].x * 50.0f) * dt;
        d.y = (m[0].y * 50.0f) * dt;
        view_pos.x = d.x + view_pos.x;
        view_pos.y = d.y + view_pos.y;
    }
    if (g_keyboard.down[0x11]) {
        d.x = (m[1].x * 50.0f) * dt;
        d.y = (m[1].y * 50.0f) * dt;
        view_pos.x = d.x + view_pos.x;
        view_pos.y = d.y + view_pos.y;
    }
    if (g_keyboard.down[0x1f]) {
        d.x = (m[1].x * 50.0f) * dt;
        d.y = (m[1].y * 50.0f) * dt;
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
// Rotate the 2x2 matrix m by `angle`: first row -> c*row0 + s*row1, second row -> c*row1 - s*row0.
// The original keeps sin/cos in x87 registers, spills one float copy of the sine and the m[1]/m[3]
// products to the stack: m[0]/m[2] therefore round their products once, m[1]/m[3] twice, and only
// the m[2] sine product uses the unrounded register value.  The volatile temps reproduce exactly
// that (the values are bit-identical to the original); the direct stores keep the original's order
// (the last one leaves the new m[2] in eax, which a caller may read).
void rot2::rotate_2x2(float angle)
{
    float s = sinf(angle);
    volatile float sr = s;
    float c = cosf(angle);
    volatile float t0 = c * m[0];
    volatile float t2 = sr * m[2];
    volatile float a = c * m[1];
    volatile float b = sr * m[3];
    float r0 = t0 + t2;
    float r1 = a + b;
    volatile float e = sr * m[1];
    volatile float d = c * m[3];
    float r2 = c * m[2] - s * m[0];
    float r3 = d - e;
    m[0] = r0;
    m[1] = r1;
    m[2] = r2;
    m[3] = r3;
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
    pos.x = m.u.x * zoom;
    pos.y = m.u.y * zoom;
    pos.z = m.u.z * zoom;                                   // the eye vector doubles as the temporary
    float sx = pos.x + m.o.x;
    float sy = m.o.y + pos.y;
    float sz = m.o.z + pos.z;
    m.o.x = sx;
    m.o.y = sy;
    m.o.z = sz;

    rgl::frustum* f = &cam;                  // page +0x70
    f->m = m;
    f->build_planes();                    // 0x41e4f0: thiscall on this+0x70
    *(FrustumBlock*)&view->scene3d.fr = *(FrustumBlock*)f;             // 0x120 bytes: page +0x70 -> world +0x1ac
    g_terrain.update_particles(&m);        // 0x4f3e10
}
