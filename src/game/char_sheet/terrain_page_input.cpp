#include "./terrain_page_input.h"
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f3570
void TerrainPage::read_fields()
{
    water_on = (g_bits.get(0xa2, 0xa3) != 0) ? 0 : 1;       // the "Enable water" checkbox holds the INVERSE
    river_on = (int)water_on;                               // (one flag per box; same word here)
    terrain_on = g_bits.get(0x7f, 0x7f);
    terraingen_on = g_bits.get(0x3f, 0x3f);
    camera_x = (float)g_bits.get(0x60, 0x69);
    camera_y = (float)g_bits.get(0x6a, 0x73);
    camera_z = (float)g_bits.get(0x87, 0x8d);
    fog = (float)g_bits.get(0x80, 0x86) * 0.01f;
    layer_bits[0] = g_bits.get(0x00, 0x1e);
    layer_bits[1] = g_bits.get(0x20, 0x3e);
    layer_bits[2] = g_bits.get(0x40, 0x5e);
    water_level = g_bits.get(0x8e, 0x94);
    water_height = (float)g_bits.get(0x95, 0x9b) * 0.01f;
    terrain_type = (g_bits.w[4] >> 0x1c) & (g_type_mask - 1);
    map_size = g_bits.w[5] & (g_size_mask - 1);
}

// @ 0x4f37f0
void TerrainPage::simulate(float dt)
{
    if (g_key_17204)
        pitch = pitch - dt * 1.1f;
    if (g_key_17214)
        pitch = dt * 1.1f + pitch;
    if (g_key_1720c)
        zoom = (dt * 2.1f + 1.0f) * zoom;
    if (g_key_171fc)
        zoom = (1.0f - dt * 2.1f) * zoom;

    vector2 m[2];                                           // the camera frame: m[0] = strafe axis, m[1] = forward
    identity_2x2(&m[0].x);                                  // 0x4f3be0
    float a = -yaw;
    rotate_2x2(&m[0].x, a);                                 // 0x4f3b40 turns m[0].xy / m[1].xy

    if (g_key_1714c) {
        view_x = view_x - (m[0].x * 50.0f) * dt;
        view_y = view_y - (m[0].y * 50.0f) * dt;
    }
    if (g_key_17154) {
        view_x = (m[0].x * 50.0f) * dt + view_x;
        view_y = (m[0].y * 50.0f) * dt + view_y;
    }
    if (g_key_17118) {
        view_x = (m[1].x * 50.0f) * dt + view_x;
        view_y = (m[1].y * 50.0f) * dt + view_y;
    }
    if (g_key_17150) {
        view_x = view_x - (m[1].x * 50.0f) * dt;
        view_y = view_y - (m[1].y * 50.0f) * dt;
    }
    if (view_x < 0.0f)
        view_x = 0.0f;
    else if (view_x > 250.0f)
        view_x = 250.0f;
    if (view_y < 0.0f)
        view_y = 0.0f;
    else if (view_y > 250.0f)
        view_y = 250.0f;

    if (g_mouse_y < 0.5f) {
        if (g_key_17458) {
            yaw = (g_axis_x * dt) * 1.7f + yaw;
            pitch = pitch - (g_axis_y * dt) * 1.7f;
        }
        if (g_key_17454)
            zoom = ((g_axis_y * dt) * 0.5f + 1.0f) * zoom;
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
void rotate_2x2(float* m, float angle)
{
    float s = 0, c = 0;                                     // x87 inline sin/cos (the original has no CRT call)
    __asm {
        fld [angle]
        fsin
        fstp [s]
        fld [angle]
        fcos
        fstp [c]
    }
    float x = m[0], y = m[1];
    m[2] = c * m[2] - s * x;
    m[3] = c * m[3] - s * y;
    m[0] = c * x + s * m[2];
    m[1] = c * y + s * m[3];
}

// @ 0x4f3be0
void identity_2x2(float* m)
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
    // Write the hex words and the hex field from whatever text the clipboard holds. The original opens and closes
    // the clipboard itself, and always re-reads the fields and refreshes the page (even when the clipboard is
    // empty: falling off the inner path jumps to the CloseClipboard block).
    HANDLE h = OpenClipboard(0) ? GetClipboardData(CF_TEXT) : 0;
    if (h) {
        const char* p = (const char*)GlobalLock(h);
        if (p) {
            ((HexWords*)(void*)&g_bits)->parse(tstring(p));   // 0x4658c0
            { char r[8]; hexbox->set_text(r, tstring(p)); }
            GlobalUnlock(h);
        }
    }
    read_fields();
    update();
    CloseClipboard();
}

// @ 0x4f3cf0
void TerrainPage::path(float dt)
{
    vector4 pos;                                            // the eye (w stays uninitialised, copied into m.o.w)
    mat_self m;
    identity_pos((::pos*)&m);                                 // 0x40cd00 (cdecl, returns its argument)
    pos.x = view_x;
    pos.y = view_y;
    pos.z = view_z;
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

    *(rgl::matrix*)frame = m;                               // page +0x138
    ((rgl::frustum*)map_block)->build_planes();                    // 0x41e4f0: thiscall on this+0x70
    *(FrustumBlock*)world6c->block = *(FrustumBlock*)map_block;             // 0x120 bytes: page +0x70 -> world +0x1ac
    ((DustTerrain*)(void*)&g_bits)->update_particles(&m);        // 0x4f3e10
}
