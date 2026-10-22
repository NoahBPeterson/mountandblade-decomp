// Slice s004bbba0: the configuration defaults and setters, the configuration file reader, the engine's
// software cursor and the DirectInput device set. Class and field names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <math.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

struct tstring;

// The engine string (16 bytes), as in match/slices/s00463790/tstring.h.
struct tstring {
    int pos;                // +0  read cursor
    char* buf;              // +4  (null while empty)
    int cap;                // +8
    int len;                // +0xc
    tstring();                                              // 0x42bbd0
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
    char equals(const char* s) const;                       // 0x42bd10
    void clear();                                           // 0x42be50
    void to_lower();                                        // 0x42c020
};

extern "C" int atoi(const char* str);                       // 0x53734d (atoi: jmp atol)

// ---- configuration object (layout from 0x4bc020 and the setters) ----------------------------------------------
struct Config {
    int first_time;                 // +0x00
    int texture_detail;             // +0x04
    int show_framerate;             // +0x08
    int max_framerate;              // +0x0c
    int start_windowed;             // +0x10
    int skip_tutorial;              // +0x14
    int enable_edit_mode;           // +0x18
    int force_single_threading;     // +0x1c
    int debug_mode;                 // +0x20
    int cheat_mode;                 // +0x24
    int enable_blood;               // +0x28
    int fake_reflections;           // +0x2c
    int disable_frequency_variation;// +0x30
    int use_vertex_shaders;         // +0x34
    int use_pixel_shaders;          // +0x38
    int disable_music;              // +0x3c
    int disable_sound;              // +0x40
    int unk44;                      // +0x44
    int render_buffer_size;         // +0x48
    int display_width;              // +0x4c
    int display_height;             // +0x50
    int display_bits;               // +0x54
    int antialiasing;               // +0x58
    int sample_quality;             // +0x5c
    int force_vsync;                // +0x60
    int unk64;                      // +0x64
    int shadowmap_quality;          // +0x68
    int unk6c;                      // +0x6c
    void init_defaults();                           // 0x4bc020
    void set(tstring& key, tstring& value);                 // 0x4bbba0
    void load_file(FILE* f);                                // 0x4bbf70
    void set_triple_buffering(int t);                       // 0x4bc090
    void set_texture_quality(int q);                        // 0x4bc100
    void split_line(const char* line, tstring* first, tstring* second); // 0x4bba90
    int value_as_int(const tstring& s);                     // 0x4bbb70
};

// The value of a "name = value" line, 1 when the value is empty.
// 0x4bbb70
int Config::value_as_int(const tstring& s)
{
    int r = 1;
    if (s.len != 0) {
        r = atoi(s.c_str());
    }
    return r;
}

// @ 0x4bbba0
// Applies one "name = value" line. Both strings are lowered first, then every known name stores the value
// parsed from the right-hand string. Names with no handler leave the configuration untouched.
void Config::set(tstring& key, tstring& value)
{
    key.to_lower();
    value.to_lower();

    if (key.equals("start_windowed")) { start_windowed = value_as_int(value); return; }
    if (key.equals("skip_tutorial")) { skip_tutorial = value_as_int(value); return; }
    if (key.equals("enable_edit_mode")) { enable_edit_mode = value_as_int(value); return; }
    if (key.equals("force_single_threading")) { force_single_threading = value_as_int(value); return; }
    if (key.equals("debug_mode")) { debug_mode = value_as_int(value); return; }
    if (key.equals("cheat_mode")) { cheat_mode = value_as_int(value); return; }
    if (key.equals("enable_blood")) { enable_blood = value_as_int(value); return; }
    if (key.equals("disable_music")) { disable_music = value_as_int(value); return; }
    if (key.equals("disable_sound")) { disable_sound = value_as_int(value); return; }
    if (key.equals("show_framerate")) { show_framerate = value_as_int(value); return; }
    if (key.equals("max_framerate")) { max_framerate = value_as_int(value); return; }
    if (key.equals("use_pixel_shaders")) { use_pixel_shaders = value_as_int(value); return; }
    if (key.equals("use_vertex_shaders")) { use_vertex_shaders = value_as_int(value); return; }
    if (key.equals("fake_reflections")) { fake_reflections = value_as_int(value); return; }
    if (key.equals("disable_frequency_variation")) { disable_frequency_variation = value_as_int(value); return; }
    if (key.equals("texture_detail")) {
        int d = value_as_int(value);
        texture_detail = d;
        if (d < 0) texture_detail = 0;
        else if (d > 101) texture_detail = 101;
        return;
    }
    if (key.equals("render_buffer_size")) { render_buffer_size = value_as_int(value); return; }
    if (key.equals("first_time")) { first_time = value_as_int(value); return; }
    if (key.equals("display_width")) { display_width = value_as_int(value); return; }
    if (key.equals("display_height")) { display_height = value_as_int(value); return; }
    if (key.equals("display_bits")) { display_bits = value_as_int(value); return; }
    if (key.equals("antialiasing")) { antialiasing = value_as_int(value); return; }
    if (key.equals("sample_quality")) { sample_quality = value_as_int(value); return; }
    if (key.equals("force_vsync")) { force_vsync = value_as_int(value); return; }
    if (key.equals("shadowmap_quality")) {
        int q = value_as_int(value);
        shadowmap_quality = q;
        unk6c = q;
    }
}

// @ 0x4bbf70
// Reads the file line by line; lines starting with '#' are comments. Each line is read with getc into a
// 256-byte buffer (at most 250 characters).
void Config::load_file(FILE* f)
{
    tstring first;
    tstring second;
    char line[0x100];

    while (!(f->_flag & _IOEOF)) {
        int n = 0;
        do {
            char c = (char)getc(f);
            line[n] = c;
            n++;
            if ((f->_flag & _IOEOF) || c == '\n') break;
        } while (n < 250);
        line[n] = 0;
        if (line[0] != '#') {
            split_line(line, &first, &second);
            set(first, second);
        }
    }
}

// @ 0x4bc020
// Configuration defaults (a void member, so the original keeps ECX as `this` and EAX as the zero register).
void Config::init_defaults()
{
    first_time = 1;
    show_framerate = 0;
    max_framerate = 100;
    texture_detail = 50;
    use_pixel_shaders = 1;
    use_vertex_shaders = 1;
    disable_sound = 0;
    disable_music = 0;
    enable_edit_mode = 0;
    force_single_threading = 0;
    debug_mode = 0;
    cheat_mode = 0;
    enable_blood = 1;
    unk44 = 0;
    disable_frequency_variation = 0;
    render_buffer_size = 15;
    fake_reflections = 1;
    start_windowed = 0;
    display_width = 0;
    display_height = 0;
    display_bits = 32;
    antialiasing = 1;
    sample_quality = 0;
    force_vsync = 0;
    unk64 = 1;
    shadowmap_quality = 1;
    unk6c = 1;
}

// @ 0x4bc090
// The triple-buffering choice also picks the render buffer size.
void Config::set_triple_buffering(int t)
{
    int v = t;

    first_time = t;
    switch (v) {
    case 5: disable_music = 9999999; break;
    case 4: disable_music = 40; break;
    case 3: disable_music = 20; break;
    case 2: disable_music = 10; break;
    case 1: disable_music = 5; break;
    case 0: disable_music = 0; break;
    }
}

// @ 0x4bc100
// The texture-quality choice picks the anisotropic level.
void Config::set_texture_quality(int q)
{
    int v = q;

    texture_detail = q;
    switch (v) {
    case 5: disable_sound = 9999999; break;
    case 4: disable_sound = 10; break;
    case 3: disable_sound = 5; break;
    case 2: disable_sound = 3; break;
    case 1: disable_sound = 1; break;
    case 0: disable_sound = 0; break;
    }
}

struct Settings;

// A 3D vector stored in a 16-byte slot; the fourth component is padding and is left untouched by this constructor.
struct Vec4 {
    float x, y, z, w;
    Vec4() {}
    Vec4(float a, float b, float c) { x = a; y = b; z = c; }
};

// The engine's software cursor: a position and a target position per axis, a radius and two state bytes.
struct Cursor {
    float pos_x, pos_y, pos_z;      // +0x00
    int   unk0c;                    // +0x0c
    float pos2_x, pos2_y, pos2_z;   // +0x10
    int   unk1c;                    // +0x1c
    float radius;                   // +0x20
    unsigned char moving;           // +0x24
    unsigned char unk25;            // +0x25
    int   unk28;                    // +0x28
    Cursor();
    void read(Settings* src);       // 0x4bc1f0
};

// The settings object the cursor reads its floats from (get_float at 0x42f190).
struct Settings {
    int get_float(const char* name, double* out);   // 0x42f190
};

// @ 0x4bc1f0
// Reads the cursor position, the target position and the radius from the settings, keeping any missing value.
void Cursor::read(Settings* src)
{
    double v;
    if (src->get_float("radius", &v) == 0) radius = (float)v;
    if (src->get_float("pos_x", &v) == 0) pos_x = (float)v;
    if (src->get_float("pos_y", &v) == 0) pos_y = (float)v;
    if (src->get_float("pos_z", &v) == 0) pos_z = (float)v;
    if (src->get_float("pos2_x", &v) == 0) pos2_x = (float)v;
    if (src->get_float("pos2_y", &v) == 0) pos2_y = (float)v;
    if (src->get_float("pos2_z", &v) == 0) pos2_z = (float)v;
}

// A 4x4 matrix returned by value (0x40cd00 builds one).
struct Matrix {
    float m[16];
};
Matrix* __cdecl make_matrix(Matrix* out);   // 0x40cd00 fills out and returns it

// The input state prefix shared with InputSet plus the two view matrices. Only the fields this slice touches are named.
struct DisplayState {
    char unk000[0x618];                     // +0x000
    int  unk618;                            // +0x618
    char unk61c[0x130];                     // +0x61c
    int  unk74c;                            // +0x74c
    int  unk750;                            // +0x750
    Matrix mat1;                            // +0x754
    Matrix mat2;                            // +0x794
    DisplayState();
};

// @ 0x4bc2d0
// Clears the input state and rebuilds both view matrices with a unit last column.
DisplayState::DisplayState()
{
    Matrix tmp;
    Matrix* m;
    unk618 = 0;
    unk74c = 0;
    mat1 = *make_matrix(&tmp);
    m = &mat1;
    m->m[3] = 0;
    m->m[7] = 0;
    m->m[11] = 0;
    m->m[15] = 1.0f;
    mat2 = *make_matrix(&tmp);
    m = &mat2;
    m->m[3] = 0;
    m->m[7] = 0;
    m->m[11] = 0;
    m->m[15] = 1.0f;
}

// @ 0x4bc170
// The cursor starts at the origin with the fixed radius 0.05 and the two state bytes cleared.
Cursor::Cursor()
{
    *(Vec4*)&pos_x = Vec4(0.0f, 0.0f, 0.0f);
    *(Vec4*)&pos2_x = Vec4(0.0f, 0.0f, 0.0f);
    radius = 0.05f;
    unk25 = 0;
    moving = 0;
    unk28 = 0;
}

// ---- input device set (offsets from the input code that acquires the mouse) ------------------------------------
struct InputSet {
    IDirectInput8A* dinput;                 // +0x000
    IDirectInputDevice8A* keyboard;         // +0x004
    IDirectInputDevice8A* mouse;            // +0x008
    int unk[0x1f8 / 4];                     // +0x00c .. +0x203
    int mouse_buttons[0x14 / 4];            // +0x204 .. +0x217
    float scroll_delta;                     // +0x218
    int unk2[0x3f8 / 4];                    // +0x21c .. +0x613
    unsigned char exclusive_ok;             // +0x614
    unsigned char background_ok;            // +0x615
    int unk618;                             // +0x618
    int unk3[0x130 / 4];                    // +0x61c .. +0x74b
    int unk74c;                             // +0x74c
    int unk750;                             // +0x750
    int init_devices(HWND hwnd);            // 0x4bc430
};

extern int g_dinput_enabled;                // 0x5e28fc

void __fastcall release_input_devices(InputSet* self);      // 0x4bc3d0

// @ 0x4bc350
// Re-applies the exclusive, background cooperative level to the keyboard and records which level took.
void __fastcall apply_input_settings(InputSet* self)
{
    typedef int (__stdcall *SetupFn)(void*, int, int, void*, int, int, int);
    self->exclusive_ok = 0;
    self->background_ok = 0;
    if (g_dinput_enabled != 0) {
        HRESULT hr = (*(SetupFn*)(*(int*)self->unk74c + 0x28))((void*)self->unk74c, 0, 1, (void*)self->unk750, 2, 3, 0x4b);
        if (hr >= 0) {
            self->exclusive_ok = 1;
            return;
        }
        hr = (*(SetupFn*)(*(int*)self->unk74c + 0x28))((void*)self->unk74c, 0, 1, (void*)self->unk750, 2, 3, 0x34324644);
        if (hr >= 0) {
            self->background_ok = 1;
        }
    }
}

// @ 0x4bc3d0
// Unacquires and releases the keyboard and the mouse, then the DirectInput object itself.
void __fastcall release_input_devices(InputSet* self)
{
    if (self->keyboard != 0) self->keyboard->Unacquire();
    if (self->mouse != 0) self->mouse->Unacquire();
    if (self->keyboard != 0) {
        self->keyboard->Release();
        self->keyboard = 0;
    }
    if (self->mouse != 0) {
        self->mouse->Release();
        self->mouse = 0;
    }
    if (self->dinput != 0) {
        self->dinput->Release();
        self->dinput = 0;
    }
}

// @ 0x4bc430
// Creates DirectInput, the keyboard and the buffered mouse, then starts them.
int InputSet::init_devices(HWND hwnd)
{
    DIPROPDWORD buffer_size;
    HRESULT hr;

    release_input_devices(this);
    hr = DirectInput8Create(GetModuleHandleA(0), 0x0800, IID_IDirectInput8A, (void**)&dinput, 0);
    if (hr >= 0) {
        hr = dinput->CreateDevice(GUID_SysKeyboard, &keyboard, 0);
        if (hr >= 0) {
            hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
            if (hr >= 0) {
                hr = keyboard->SetCooperativeLevel(hwnd, 0x16);
                if (hr >= 0) {
                    keyboard->Acquire();
                    hr = dinput->CreateDevice(GUID_SysMouse, &mouse, 0);
                    if (hr >= 0) {
                        hr = mouse->SetDataFormat(&c_dfDIMouse);
                        if (hr >= 0) {
                            hr = mouse->SetCooperativeLevel(hwnd, 6);
                            if (hr >= 0) {
                                buffer_size.diph.dwSize = 0x14;
                                buffer_size.diph.dwHeaderSize = 0x10;
                                buffer_size.diph.dwObj = 0;
                                buffer_size.diph.dwHow = 0;
                                buffer_size.dwData = 0x10;
                                hr = mouse->SetProperty(DIPROP_BUFFERSIZE, &buffer_size.diph);
                                if (hr >= 0) {
                                    mouse->Acquire();
                                    hr = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}
