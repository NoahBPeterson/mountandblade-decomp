// Slice s004f3570: the terrain "edit" page's field reading, camera simulation, two small rotation helpers and
// the clipboard "paste".
//   0x4f3570  read_fields: decode the terrain bit words back into the page fields (enables, positions, layer
//             bits, water level/height, terrain type/size)
//   0x4f37f0  simulate(dt): keyboard camera steering (pitch, zoom, strafe, pan), clamping
//   0x4f3b40  rotate a 2x2 rotation matrix (m[0..3]) by an angle
//   0x4f3be0  write a 2x2 identity into four floats
//   0x4f3c30  paste_clipboard: read CF_TEXT from the clipboard, parse the terrain words from it, set the hex
//             field's text, then read the fields back and refresh the page
//   0x4f3cf0  path(dt): build the map camera's frame and hand it to the terrain renderer
// All names provisional. The page layout is the one recovered in slice s004f2d70 (TerrainPage); the terrain bit
// words live in the global object at 0xa76630 (slice s004f25a0 / s00464a60).
#include <windows.h>
#include <stdio.h>
#include <math.h>
#pragma intrinsic(sqrt, fabs, sin, cos, tan, fsin, fcos)
// the x87 inline sin/cos used by rotate_2x2 (inline FP is off in this build, so the intrinsics need the asm form)

#include "rgl.h"

using rgl::vector4;

struct vector2 { float x, y; };                             // rgl::vector2 (the 2x2 camera axes, +0 = strafe)

// ---- tstring / rgl::string ---------------------------------------------------------------------------
struct tstring : rgl::string {
    tstring(const char* s) : rgl::string(s) {}                              // 0x42bbe0
    tstring();                                                              // 0x42bbd0
    tstring(const tstring& o);                                              // 0x42c110
    ~tstring();                                                             // 0x42bc40
    const char* c_str() const { return p ? p : (const char*)&p; }
};

// ---- controls -----------------------------------------------------------------------------------------
// Only the accessor this slice calls is declared.  cl 13.10 numbers virtuals in declaration order, one slot per
// declaration, but a reference parameter opens a family numbered 9 slots above the no-parameter virtuals.
struct Widget {
    char pad00[0xa0];
    virtual void put(const tstring& s);                     // +0xa4  set_text
};

// ---- terrain bit words (the object at 0xa76630) -------------------------------------------------------
struct BitWords {
    unsigned w[8];                                          // w[0..5] are the six hex words
    void set(const unsigned& lo, const int& hi, const unsigned& v);   // 0x464960
    unsigned get(const unsigned& lo, const int& hi);                  // 0x4649b0
    void build_hex_text(tstring& out);                                // 0x4f29b0 (slice s004f25a0)
    void parse_hex(const tstring& s);                                 // 0x4658c0 fills w[0..5] from "0x..."
    void submit_frame(const float* rect);                             // 0x4f3e10 (next slice)
};
extern BitWords g_bits;                                     // 0xa76630
extern unsigned g_type_mask;                                // 0x5a8e88 (= 4: the terrain type is 4 bits wide)
extern unsigned g_size_mask;                                // 0x5a8e78 (= 4: the terrain size is 2 bits wide)

// ---- the world / scene --------------------------------------------------------------------------------
struct Block { int d[0x48]; void build_planes(); };   // build_planes = 0x41e4f0 (thiscall)
struct World {                                              // the page's +0x6c pointer target
    char pad00[0x1ac];
    int block[0x48];                                        // +0x1ac  the 0x120-byte map block (copy of the page's +0x70)
};

// 0x40cd00 has a hidden return pointer and returns it; the two rotation helpers are thiscall on the matrix and
// take a pointer to the angle (thiscall, ret 4).
struct mat_self : rgl::matrix {
    void rotate_fu(const float* angle);                     // 0x40e570 (rows s/f about u)
    void rotate_su(const float* angle);                     // 0x40e670 (rows s/u about f)
};
mat_self identity_pos();                                    // 0x40cd00 (cdecl, hidden return)
void rotate_fu(void* m, const float* angle);                // 0x40e570 (same thiscall, for callers holding a raw ptr)
void rotate_su(void* m, const float* angle);                // 0x40e670

void rotate_2x2(float* m, float angle);                     // 0x4f3b40
void identity_2x2(float* m);                                // 0x4f3be0

// ---- the page -----------------------------------------------------------------------------------------
struct TerrainPage {
    char    pad00[0x2c - 0x00];
    int     terrain_on;                                     // +0x2c  "Enable terrain" checkbox
    int     terraingen_on;                                  // +0x30  "Enable terrain generation" checkbox
    int     water_on;                                       // +0x34  "Enable water" checkbox
    int     river_on;                                       // +0x38  "Enable river" checkbox
    float   camera_x;                                       // +0x3c  X position
    float   camera_y;                                       // +0x40  Y position
    float   camera_z;                                       // +0x44  camera height above the ground
    float   fog;                                            // +0x48  fog (0..1)
    float   water_height;                                   // +0x4c  water height (0..1)
    int     water_level;                                    // +0x50  water level
    int     layer_bits[3];                                  // +0x54 / +0x58 / +0x5c  random bits of the water layers
    int     terrain_type;                                   // +0x60  terrain type (index into the type table)
    int     map_size;                                       // +0x64  terrain size
    char    pad68[4];                                       // +0x68
    World*  world;                                          // +0x6c
    int     map_block[(0x138 - 0x70) / 4];                  // +0x70  the 0x120-byte map block (runs on through frame[]; frustum source at
                                                            //        +0xf8 = block+0x88; copied to world +0x1ac)
    float   frame[0x10];                                    // +0x138  the frame path() builds (from a temp)
    char    pad178[0x18];                                   // +0x178
    float   view_x;                                         // +0x190  eye X (clamped to 0..250)
    float   view_y;                                         // +0x194  eye Y (clamped to 0..250)
    float   view_z;                                         // +0x198  eye Z
    char    pad19c[0x1a0 - 0x19c];
    float   yaw;                                            // +0x1a0  camera heading
    float   pitch;                                          // +0x1a4  camera pitch (0.1..1.5)
    float   zoom;                                           // +0x1a8  camera zoom (1.5..400.5)
    char    pad1ac[0x1b4 - 0x1ac];
    Widget* hexbox;                                         // +0x1b4  the hex terrain text field
    char    pad1b8[0x2c0 - 0x1b8];

    void read_fields();                                     // 0x4f3570
    void simulate(float dt);                                // 0x4f37f0
    void paste_clipboard();                                 // 0x4f3c30
    void path(float dt);                                    // 0x4f3cf0
    void update();                                          // 0x4f2730 (slice s004f25a0)
};

// the input flags the camera reads (0xa17xxx block; names provisional)
extern int g_key_17204;                                     // 0xa17204  pitch down
extern int g_key_17214;                                     // 0xa17214  pitch up
extern int g_key_1720c;                                     // 0xa1720c  zoom in
extern int g_key_171fc;                                     // 0xa171fc  zoom out
extern int g_key_1714c;                                     // 0xa1714c  strafe one way (camera-space axis)
extern int g_key_17154;                                     // 0xa17154  strafe the other way
extern int g_key_17150;                                     // 0xa17150  pan one way
extern int g_key_17118;                                     // 0xa17118  pan the other way
extern int g_key_17458;                                     // 0xa17458  turn (heading)
extern int g_key_17454;                                     // 0xa17454  roll with the vertical axis
extern float g_axis_y;                                      // 0xa178d8  second input axis
extern float g_axis_x;                                      // 0xa178d4  first input axis
extern float g_mouse_y;                                     // 0x5d7094  vertical look axis compared with 0.5

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
            g_bits.parse_hex(tstring(p));
            hexbox->put(tstring(p));
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
    mat_self m = identity_pos();                            // 0x40cd00 (hidden return pointer, cdecl)
    pos.x = view_x;
    pos.y = view_y;
    pos.z = view_z;
    m.o = pos;
    float angle = 1.5707964f;                               // +90 degrees
    m.rotate_fu(&angle);                                    // 0x40e570
    angle = -yaw;
    m.rotate_su(&angle);                                    // 0x40e670
    angle = -pitch;
    m.rotate_fu(&angle);
    pos.x = m.u.x * zoom;
    pos.y = m.u.y * zoom;
    pos.z = m.u.z * zoom;                                   // the eye vector doubles as the temporary
    m.o.x = pos.x + m.o.x;
    m.o.y = m.o.y + pos.y;
    m.o.z = m.o.z + pos.z;

    *(rgl::matrix*)frame = m;                               // page +0x138
    ((Block*)map_block)->build_planes();                    // 0x41e4f0: thiscall on this+0x70
    *(Block*)world->block = *(Block*)map_block;             // 0x120 bytes: page +0x70 -> world +0x1ac
    g_bits.submit_frame((const float*)&m);                  // 0x4f3e10
}
