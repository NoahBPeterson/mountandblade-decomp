#include "./terrain_page_input.h"
#include "../terrain_a/terrain.h"      // g_terrain 0xa76630 (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"        // MapWidget: the preview widget whose frustum (+0x1ac) takes the camera
#include "../key_names.h"
#include "../../include/render_flags.h"

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

    vector2 m[2];                                           // the camera frame: m[0] = strafe axis, m[1] = forward
    identity_2x2(&m[0].x);                                  // 0x4f3be0
    float a = -yaw;
    ((rot2*)&m[0].x)->rotate_2x2(a);                        // 0x4f3b40 turns m[0].xy / m[1].xy

    if (g_keyboard.down[0x1e]) {
        view_pos.x = view_pos.x - (m[0].x * 50.0f) * dt;
        view_pos.y = view_pos.y - (m[0].y * 50.0f) * dt;
    }
    if (g_keyboard.down[0x20]) {
        view_pos.x = (m[0].x * 50.0f) * dt + view_pos.x;
        view_pos.y = (m[0].y * 50.0f) * dt + view_pos.y;
    }
    if (g_keyboard.down[0x11]) {
        view_pos.x = (m[1].x * 50.0f) * dt + view_pos.x;
        view_pos.y = (m[1].y * 50.0f) * dt + view_pos.y;
    }
    if (g_keyboard.down[0x1f]) {
        view_pos.x = view_pos.x - (m[1].x * 50.0f) * dt;
        view_pos.y = view_pos.y - (m[1].y * 50.0f) * dt;
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
// that (the values are bit-identical to the original), and the stores go through unsigned copies in
// the original's order (the last one leaves the new m[2] in eax, which a caller may read).
void rot2::rotate_2x2(float angle)
{
    float s = sinf(angle), c = cosf(angle);
    volatile float sr = s;                                  // the float32 copy the original spills
    float x = m[0], y = m[1], z = m[2], w = m[3];
    volatile float a = c * y, b = sr * w;                   // m[1] products (spilled -> rounded)
    volatile float d = c * w, e = sr * y;                   // m[3] products (spilled -> rounded)
    float r0 = c * x + sr * z;
    float r1 = a + b;
    float r2 = c * z - s * x;                               // the unrounded sine, as in the original
    float r3 = d - e;
    unsigned* mi = (unsigned*)m;
    mi[0] = *(unsigned*)&r0;
    mi[1] = *(unsigned*)&r1;
    mi[3] = *(unsigned*)&r3;
    mi[2] = *(unsigned*)&r2;
}

// @ 0x4f3be0
__declspec(noinline) void identity_2x2(float* m)
{
    float a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f;           // the original fills four stack temps, then the array
    m[0] = a;
    m[1] = b;
    m[2] = c;
    m[3] = d;
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
                g_terrain.parse(tstring(p));   // 0x4658c0
                { char r[8]; hexbox->set_text((float*)r, &(tstring(p))); }
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
    m.o.x = pos.x + m.o.x;
    m.o.y = m.o.y + pos.y;
    m.o.z = m.o.z + pos.z;

    cam.m = m;                               // page +0x138
    cam.build_planes();                    // 0x41e4f0: thiscall on this+0x70
    *(FrustumBlock*)&view->view = *(FrustumBlock*)&cam;             // 0x120 bytes: page +0x70 -> world +0x1ac
    g_terrain.update_particles(&m);        // 0x4f3e10
}
