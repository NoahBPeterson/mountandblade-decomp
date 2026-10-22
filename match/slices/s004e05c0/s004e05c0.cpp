// clang-format off
// Slice s004e05c0: the options screen (0x4e05c0, 8152 bytes).  Resets the GUI, builds the options window
// with its text options, track bars, drop-downs, the resolution list and the six quality buttons, registers
// the window with the GUI and re-selects the current settings.  Widget slot offsets, the string helpers and
// the localization table (0x5e2a18) follow the verified neighbouring slices (s004dfd50, s004e25a0,
// s004cf7b0).  Names are provisional.
#include <windows.h>
#include <stdio.h>
#include <new>
#include <vector>

// rgl::string: 16 bytes, verified in match/include/rgl.h (ctor 0x42bbe0).
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s);                                 // 0x42bbe0
    tstring(const tstring& o);                              // 0x42c110
    ~tstring();                                             // 0x42bc40
    tstring& operator=(const tstring& o);                   // 0x42bc50
    void insert(int at, const tstring& s);                  // 0x42c900
    bool equals(const char* s) const;                       // 0x42bd10
    int find(const tstring& s, int from) const;             // 0x42c440
};

// localization table at 0x5e2a18: get(key) hands back the text stored for the key.
struct text_table {
    tstring* get(tstring& key);                             // 0x43a1e0
};
extern text_table g_text;                                   // 0x5e2a18
struct text_ret {
    tstring* t;
    operator tstring*() { return t; }
    operator tstring&() { return *t; }
};
static inline text_ret tex(const char* key) {
    tstring k(key);
    text_ret r = { g_text.get(k) };
    return r;
}

struct mesh {
    mesh* clone();                                          // 0x41c6d0
    void set_color(const float* c);                         // 0x47af30 (four floats)
};

// pair of floats handed to a widget slot (the original copies the constants onto the stack)
static const float* xy(float a, float b)
{
    static float v[2];
    v[0] = a;
    v[1] = b;
    return v;
}

struct resmgr {
    mesh* get_object(const tstring& name);                  // 0x419540
};
extern resmgr g_resmgr;                                     // 0x5d6fa0

struct gui_window;
struct gui {
    void reset(int a);                                      // 0x42d1b0
    void set_window(gui_window** w);                        // 0x49b120
};
extern gui g_gui;                                           // 0x5d7080

// plain GUI window (vptr 0x592848, ctor 0x4931a0, 0x190 bytes)
struct gui_window {
    gui_window();
    virtual void d0(); virtual void d1(); virtual void d2(); virtual void d3(); virtual void d4();
    virtual void d5(); virtual void d6(); virtual void d7(); virtual void d8(); virtual void d9();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23();
    virtual void set_pos(const float* p);                   // +0x58
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33(); virtual void d34();
    virtual void d35(); virtual void d36(); virtual void d37();
    virtual void set_size(const float* p);                  // +0x64
    virtual void d39(); virtual void d40(); virtual void d41(); virtual void d42(); virtual void d43();
    virtual void d44(); virtual void d45();
    virtual void set_mesh(mesh* m, int layer);              // +0x7c
    virtual void d47(); virtual void d48(); virtual void d49(); virtual void d50(); virtual void d51();
    virtual void d52(); virtual void d53(); virtual void d54(); virtual void d55(); virtual void d56();
    virtual void d57(); virtual void d58();
    virtual void set_color(const unsigned& c);              // +0x98
    virtual void d60(); virtual void d61(); virtual void d62(); virtual void d63(); virtual void d64();
    virtual void set_caption(tstring* a, tstring* b);       // +0xa4
    char tail[0x188 - 0xac];
    char f188;                                              // +0x188
};

// widget base: the slots the options screen reaches (comments give the vtable offset).
struct gui_obj {
    virtual void d0();
    virtual void d1();
    virtual void d2();
    virtual void clear(int a);                              // +0x0c
    virtual void d4();
    virtual void d5();
    virtual void d6();
    virtual void d7();
    virtual void d8();
    virtual void d9();
    virtual void d10();
    virtual void select(const int& v);                      // +0x2c
    virtual void d12();
    virtual void set_value(const float& v);                 // +0x34
    virtual void d14(); virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18();
    virtual void set_pos(const float* p);                   // +0x4c
    virtual void set_scale(const float* v);                 // +0x50
    virtual void set_scale2(const float* v);                // +0x54
    virtual void d22(); virtual void d23(); virtual void d24(); virtual void d25(); virtual void d26();
    virtual void d27(); virtual void d28(); virtual void d29();
    virtual void set_width(const int& v);                   // +0x78
    virtual void set_mesh(mesh* m, int layer);              // +0x80
    virtual void d32(); virtual void d33(); virtual void d34();
    virtual void set_color(const unsigned& c);              // +0x8c
    virtual void set_mesh_color(const unsigned& c);         // +0x90
    virtual void d37(); virtual void d38(); virtual void d39(); virtual void d40();
    virtual void set_caption(tstring* a, tstring* b);       // +0xa4
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void select_last();                             // +0xbc
    virtual void d48(); virtual void d49();
    virtual void add_item(const tstring& s);                // +0xc8
    char tail[0x178 - 0xcc];
    int f178;                                               // +0x178
};

// text option (ctor 0x48ae20, +0x184 holds the flag used by the quality buttons) and track bar
// (ctor 0x48b4b0, range at +0x154 / +0x158)
struct option_widget : gui_obj {
    int f184;                                               // +0x184
};
struct track_bar : gui_obj {
    char tail2[0x154 - 0xcc];
    float f154;                                             // +0x154 (minimum)
    float f158;                                             // +0x158 (maximum)
};

// drop-down box (ctor 0x4931d0, 0x1d0 bytes): the item vector starts at +0x190
struct drop_down : gui_obj {
    char a0[0x10c - 0xcc];
    mesh* f10c;                                             // +0x10c (mesh under the label)
    mesh* f110;                                             // +0x110
    mesh* f114;                                             // +0x114
    char b0[0x118 - 0xd8];
    float f118[4];                                          // +0xd8 row extents
    char c2[0x190 - 0xe8];
    tstring** items_begin;                                  // +0x190
    tstring** items_end;                                    // +0x194
    int item_count() { return items_begin ? (int)(items_end - items_begin) : 0; }
};

// widget factories
struct Win;
option_widget* make_option(gui_window* parent, float x, float y, float w, float h, tstring* text); // 0x4dada0
track_bar* make_track_bar(gui_window* parent, float x, float y);                                    // 0x4dfd50
drop_down* make_drop_down(gui_window* parent, tstring* name, float x, float y);                     // 0x4dfec0
void* make_button(gui_window* parent, tstring* name, float x, float y, float w, float h);           // 0x4dce70
void* make_medium_button(gui_window* parent, float x, float y, float w, float h, float k,
                         tstring* a, tstring* b);                                               // 0x4dcc50
void set_button_text(option_widget* o);                 // 0x4d6dd0
void update_quality(Win* w);                            // 0x4d6b80

// display mode
struct mode {
    tstring name; int w, h, bpp;
};
void list_modes(std::vector<mode>* out);                // 0x4cf100

extern int g_day;                   // 0x5a9f9c
extern int g_res_w;                 // 0x5a9fe4
extern int g_res_h;                 // 0x5a9fe8
extern int g_bpp;                   // 0x5a9fec
extern int g_shadow_q;              // 0x5a9fc4
extern int g_dx9;                   // 0x5a9fcc
extern int g_corpses;               // 0x5aa004
extern int g_ragdolls;              // 0x5aa008
extern int g_blood;                 // 0x5aa00c
extern int g_vsync;                 // 0x5aa020
extern int g_char_detail;           // 0x5aa024
extern int g_char_shadow;           // 0x5aa028
extern int g_ragdoll_q;             // 0x5aa030
extern int g_plant_shadow;          // 0x5aa034
extern int g_ragdoll_lim;           // 0x5aa03c
extern int g_particle;              // 0x5aa040
extern float g_gamma;               // 0x5aa084
extern float g_scale_x;             // 0x5aa088
extern float g_scale_y;             // 0x5aa08c
extern float g_scale_z;             // 0x5aa090
extern unsigned char g_style;       // 0x5d8788
extern int g_env_flag;              // 0x5e28fc

extern const float g_row_step;      // 0x597944 = 0.00528f

float f_round(float v);                     // 0x535a64
char* f_itoa(int v, char* buf, int base);   // 0x5371f7

struct Win {
    void init();

    char pad0[0x2c];
    gui_window* window;                 // +0x2c
    void* done;                         // +0x30
    track_bar* ui_scale;                // +0x34
    track_bar* y_scale;                 // +0x38
    track_bar* z_scale;                 // +0x3c
    track_bar* days;                    // +0x40
    track_bar* ragdoll_limit;           // +0x44
    drop_down* corpses;                 // +0x48
    drop_down* ragdolls;                // +0x4c
    drop_down* blood;                   // +0x50
    drop_down* plant_shadows;           // +0x54
    drop_down* resolution;              // +0x58
    drop_down* shadow_quality;          // +0x5c
    void* change;                       // +0x60
    option_widget* quality[6];          // +0x64..0x78
    gui_obj* texture_quality;           // +0x7c
    gui_obj* shadow_quality_label;      // +0x80
    gui_obj* texture_detail;            // +0x84
    gui_obj* vertex_shaders;            // +0x88
    gui_obj* refresh_rate;              // +0x8c
    gui_obj* screen_size;               // +0x90
    gui_obj* gamma;                     // +0x94
    gui_obj* anisotropic_filtering;     // +0x98
    gui_obj* grass_density;             // +0x9c
    gui_obj* ragdoll_quality;           // +0xa0
    gui_obj* character_detail;          // +0xa4
    gui_obj* char_detail_low;           // +0xa8
    gui_obj* char_detail_high;          // +0xac
    gui_obj* char_shadow_detail;        // +0xb0
    gui_obj* char_shadow_low;           // +0xb4
    gui_obj* char_shadow_high;          // +0xb8
    gui_obj* dynamic_lighting;          // +0xbc
    gui_obj* character_shadows;         // +0xc0
    gui_obj* environment_shadows;       // +0xc4
    gui_obj* realistic_shadows;         // +0xc8
    gui_obj* particle_systems;          // +0xcc
    gui_obj* antialiasing;              // +0xd0
    gui_obj* fast_water;                // +0xd4
    gui_obj* corpses_list;              // +0xd8
    gui_obj* ragdolls_list;             // +0xdc
    gui_obj* blood_list;                // +0xe0
    gui_obj* quality_label;             // +0xe4
    int day_saved;                      // +0xe8
    char f_ec;                          // +0xec
    char f_ed;                          // +0xed
};

// @ 0x4e05c0
void Win::init()
{
    f_ec = 0;
    f_ed = 0;
    day_saved = g_day;
    {
        g_gui.reset(0);
        gui_window* w = new gui_window;
        w->f188 = 0;
        w->set_pos(xy(-0.001f, -0.001f)); // @@inner@@
        w->set_size(xy(1.0101f, 0.7505f));
        {
            tstring empty("");
            tstring title("");
            w->set_caption(&title, &empty);
        }
        w->set_color(0xffdfcfbf);
        w->set_mesh(g_resmgr.get_object(tstring("options_window"))->clone(), 0);
        window = w;
    }
    done = make_button(window, tex("ui_done"), 0.24f, 0.7f, 0.76f, 0.3f);
    {   // version number, shown in place of the refresh rate
        char buf[0x100];
        f_itoa(f_round(g_scale_x * 100.0f), buf, 10);
        tstring version(buf);
        tstring dot(".");
        version.insert(1, dot);
        if (version.equals("2.20"))
            version = tex("ui_default");
        refresh_rate = make_option(window, 0.52f, 0.8f, 0.7f, 0.3f, &version);
    }

    int n4 = 4;
    {   // DirectX version
        refresh_rate = make_option(window, 0.2f, 0.8f, 0.7f, 0.3f, tex("ui_enable_vertex_shaders2"));
        refresh_rate->set_width(n4);
        refresh_rate->set_mesh_color(0xffdfcfbf);
        {
            tstring s(g_dx9 ? "DirectX 9" : "DirectX 7");
            int anchor[3] = { 0, 0, 0 };
            refresh_rate->set_caption((tstring*)anchor, &s);
        }
        refresh_rate->set_color(0xffdfcfbf);
    }
    {   // vertex shaders
        vertex_shaders = make_option(window, 0.2f, 0.8f, 0.7f, 0.3f, tex("ui_enable_vertex_shaders2"));
        vertex_shaders->set_width(n4);
        vertex_shaders->set_mesh_color(0xffdfcfbf);
        vertex_shaders->set_color(0xffdfcfbf);
    }
    {   // screen size
        screen_size = make_option(window, 0.2f, 0.76f, 0.7f, 0.3f, tex("ui_screen_size2"));
        screen_size->set_width(n4);
        screen_size->set_mesh_color(0xffdfcfbf);
        screen_size->set_color(0xffdfcfbf);
    }
    {   // gamma
        gamma = make_option(window, 0.2f, 0.74f, 0.7f, 0.3f, tex("ui_gamma"));
        gamma->set_width(n4);
        gamma->set_mesh_color(0xffdfcfbf);
        gamma->set_color(0xffdfcfbf);
    }
    {   // texture detail
        texture_detail = make_option(window, 0.2f, 0.7f, 0.62f, 0.3f, tex("ui_texture_detail"));
        texture_detail->set_width(n4);
        texture_detail->set_mesh_color(0xffdfcfbf);
        texture_detail->set_color(0xffdfcfbf);
    }
    {   // texture quality (%)
        char buf[0x100];
        const char* fmt = g_text.get(tstring("ui_percentage_value"))->buf;
        sprintf(buf, fmt, g_day);
        tstring text(buf);
        texture_quality = make_option(window, 0.62f, 0.8f, 0.7f, 0.3f, &text);
        texture_quality->set_width(n4);
        texture_quality->set_mesh_color(0xffdfcfbf);
        texture_quality->set_color(0xffdfcfbf);
    }
    {   // shadow quality label
        char buf[0x100];
        const char* fmt = g_text.get(tstring("ui_percentage_value"))->buf;
        sprintf(buf, fmt, g_ragdoll_lim);
        tstring text(buf);
        shadow_quality_label = make_option(window, 0.62f, 0.8f, 0.7f, 0.3f, &text);
        shadow_quality_label->set_width(n4);
        shadow_quality_label->set_mesh_color(0xffdfcfbf);
        shadow_quality_label->set_color(0xffdfcfbf);
    }
    {   // grass density
        grass_density = make_option(window, 0.54f, 0.8f, 0.7f, 0.3f, tex("ui_grass_density"));
        grass_density->set_width(n4);
        grass_density->set_mesh_color(0xffdfcfbf);
        grass_density->set_color(0xffdfcfbf);
    }
    {   // ragdoll quality
        ragdoll_quality = make_option(window, 0.2f, 0.72f, 0.7f, 0.3f, tex("ui_character_detail"));
        ragdoll_quality->set_width(n4);
        ragdoll_quality->set_mesh_color(0xffdfcfbf);
        ragdoll_quality->set_color(0xffdfcfbf);
    }
    {   // character detail: low
        char_detail_low = make_option(window, 0.33f, 0.12f, 0.3f, 0.3f, tex("ui_low"));
        char_detail_low->set_width(n4);
        char_detail_low->set_mesh_color(0xffdfcfbf);
        char_detail_low->set_color(0xffdfcfbf);
    }
    {   // character detail: high
        char_detail_high = make_option(window, 0.33f, 0.12f, 0.8f, 0.3f, tex("ui_high"));
        char_detail_high->set_width(n4);
        char_detail_high->set_mesh_color(0xffdfcfbf);
        char_detail_high->set_color(0xffdfcfbf);
    }
    {   // character shadow detail
        char_shadow_detail = make_option(window, 0.2f, 0.68f, 0.7f, 0.3f, tex("ui_character_shadow_detail"));
        char_shadow_detail->set_width(n4);
        char_shadow_detail->set_mesh_color(0xffdfcfbf);
        char_shadow_detail->set_color(0xffdfcfbf);
    }
    {   // character shadow detail: low
        char_shadow_low = make_option(window, 0.33f, 0.12f, 0.6f, 0.3f, tex("ui_low"));
        char_shadow_low->set_width(n4);
        char_shadow_low->set_mesh_color(0xffdfcfbf);
        char_shadow_low->set_color(0xffdfcfbf);
    }
    {   // character shadow detail: high
        char_shadow_high = make_option(window, 0.33f, 0.12f, 0.8f, 0.3f, tex("ui_high"));
        char_shadow_high->set_width(n4);
        char_shadow_high->set_mesh_color(0xffdfcfbf);
        char_shadow_high->set_color(0xffdfcfbf);
    }
    {   // number of corpses
        corpses_list = make_option(window, 0.2f, 0.6f, 0.7f, 0.3f, tex("ui_number_of_corpses"));
        corpses_list->set_width(n4);
        corpses_list->set_mesh_color(0xffdfcfbf);
        corpses_list->set_color(0xffdfcfbf);
    }
    {   // number of ragdolls
        ragdolls_list = make_option(window, 0.2f, 0.54f, 0.7f, 0.3f, tex("ui_number_of_ragdolls"));
        ragdolls_list->set_width(n4);
        ragdolls_list->set_mesh_color(0xffdfcfbf);
        ragdolls_list->set_color(0xffdfcfbf);
    }
    {   // blood stains
        blood_list = make_option(window, 0.2f, 0.48f, 0.7f, 0.3f, tex("ui_blood_stains"));
        blood_list->set_width(n4);
        blood_list->set_mesh_color(0xffdfcfbf);
        blood_list->set_color(0xffdfcfbf);
    }
    {   // dynamic lighting
        dynamic_lighting = make_option(window, 0.2f, 0.42f, 0.7f, 0.3f, tex("ui_dynamic_lighting"));
        dynamic_lighting->set_width(n4);
        dynamic_lighting->set_mesh_color(0xffdfcfbf);
        dynamic_lighting->set_color(0xffdfcfbf);
    }
    {   // character shadows
        character_shadows = make_option(window, 0.2f, 0.36f, 0.7f, 0.3f, tex("ui_character_shadows"));
        character_shadows->set_width(n4);
        character_shadows->set_mesh_color(0xffdfcfbf);
        character_shadows->set_color(0xffdfcfbf);
    }
    {   // environment shadows
        environment_shadows = make_option(window, 0.2f, 0.3f, 0.7f, 0.3f, tex("ui_environment_shadows"));
        environment_shadows->set_width(n4);
        environment_shadows->set_mesh_color(0xffdfcfbf);
        environment_shadows->set_color(0xffdfcfbf);
    }
    {   // realistic shadows on plants
        realistic_shadows = make_option(window, 0.2f, 0.24f, 0.7f, 0.3f, tex("ui_realistic_shadows_on_plants"));
        realistic_shadows->set_width(n4);
        realistic_shadows->set_mesh_color(0xffdfcfbf);
        realistic_shadows->set_color(0xffdfcfbf);
    }
    {   // particle systems
        particle_systems = make_option(window, 0.2f, 0.18f, 0.7f, 0.3f, tex("ui_particle_systems"));
        particle_systems->set_width(n4);
        particle_systems->set_mesh_color(0xffdfcfbf);
        particle_systems->set_color(0xffdfcfbf);
    }
    {   // anisotropic filtering
        anisotropic_filtering = make_option(window, 0.2f, 0.12f, 0.7f, 0.3f, tex("ui_anisotropic_filtering"));
        anisotropic_filtering->set_width(n4);
        anisotropic_filtering->set_mesh_color(0xffdfcfbf);
        anisotropic_filtering->set_color(0xffdfcfbf);
    }
    {   // fast water reflection
        fast_water = make_option(window, 0.2f, 0.06f, 0.7f, 0.3f, tex("ui_fast_water_reflection"));
        fast_water->set_width(n4);
        fast_water->set_mesh_color(0xffdfcfbf);
        fast_water->set_color(0xffdfcfbf);
    }
    {   // quality estimate
        int n16 = 16;
        quality_label = make_option(window, 0.68f, 0.08f, 0.7f, 0.3f, tex(""));
        quality_label->set_width(n16);
        quality_label->set_mesh_color(0xffdfcfbf);
        quality_label->set_color(0xffdfcfbf);
    }

    update_quality(this);

    change = make_button(window, tex("ui_change"), 0.68f, 0.65f, 0.705f, 0.03f);

    {   // resolution
        resolution = make_drop_down(window, tex("Resolution"), 0.52f, 0.66f);
        float c[4];
        c[0] = 0.7f;
        c[1] = 0.7f;
        c[2] = 1.0f;
        c[3] = 0.7f;
        resolution->f110->set_color(c);
        resolution->f10c->set_color(c);
        resolution->f114->set_color(c);
        float sc[2];
        sc[0] = 0.014f;
        sc[1] = 0.014f;
        resolution->set_scale(sc);
        sc[1] = 0.008f;
        resolution->set_scale2(sc);
        resolution->clear(0);
        resolution->f118[0] = 0.7f;
        resolution->f118[1] = 0.7f;
        resolution->f118[2] -= g_row_step;
        resolution->f118[3] -= g_row_step;

        std::vector<mode> modes;
        list_modes(&modes);
        for (int i = (int)modes.size() - 1; i >= 0; i--) {
            mode& m = modes[i];
            bool ok = false;
            if (g_bpp == 16)
                ok = m.name.find(tstring("16 bits"), 0) >= 0;
            if (!ok)
                ok = m.name.find(tstring("32 bits"), 0) >= 0;
            if (ok) {
                resolution->add_item(m.name);
                char buf[0x100];
                sprintf(buf, "%dx%d", g_res_w, g_res_h);
                ok = m.name.find(tstring(buf), 0) >= 0;
                if (ok)
                    resolution->select(resolution->item_count() - 1);
            }
        }
        resolution->add_item(tex("ui_use_desktop_resolution2"));
        if (g_res_w == 0)
            resolution->select(resolution->item_count() - 1);
    }

    ui_scale = make_track_bar(window, 0.56f, 0.625f);
    ui_scale->f154 = 1.5f;
    ui_scale->f158 = 2.5f;
    ui_scale->set_value(g_scale_x);

    {   // shadow quality
        shadow_quality = make_drop_down(window, tex("ShadowQ"), 0.52f, 0.585f);
        shadow_quality->add_item(tex("ui_m_ultra_high2"));
        shadow_quality->add_item(tex("ui_m_high2"));
        shadow_quality->add_item(tex("ui_m_low2"));
        shadow_quality->select(2 - g_corpses);
    }

    days = make_track_bar(window, 0.56f, 0.545f);
    days->f154 = 10.0f;
    days->f158 = 100.0f;
    days->set_value((float)g_day);

    ragdoll_limit = make_track_bar(window, 0.56f, 0.505f);
    ragdoll_limit->f154 = 0.0f;
    ragdoll_limit->f158 = 100.0f;
    ragdoll_limit->set_value((float)g_ragdoll_lim);

    y_scale = make_track_bar(window, 0.56f, 0.465f);
    y_scale->set_value(g_scale_y);

    z_scale = make_track_bar(window, 0.56f, 0.425f);
    z_scale->set_value(g_scale_z);

    {   // corpses
        corpses = make_drop_down(window, tex("Corpses"), 0.52f, 0.385f);
        corpses->add_item(tex("ui_unlimited"));
        corpses->add_item(tstring("40"));
        corpses->add_item(tstring("20"));
        corpses->add_item(tstring("10"));
        corpses->add_item(tstring("5"));
        corpses->add_item(tstring("3"));
        corpses->add_item(tstring("1"));
        corpses->add_item(tstring("0"));
        corpses->select(6 - g_corpses);
    }
    {   // ragdolls
        ragdolls = make_drop_down(window, tex("Ragdolls"), 0.52f, 0.345f);
        ragdolls->add_item(tex("ui_unlimited"));
        ragdolls->add_item(tstring("10"));
        ragdolls->add_item(tstring("5"));
        ragdolls->add_item(tstring("3"));
        ragdolls->add_item(tstring("1"));
        ragdolls->add_item(tstring("0"));
        ragdolls->select(5 - g_ragdolls);
    }
    {   // blood
        blood = make_drop_down(window, tex("Blood"), 0.52f, 0.305f);
        blood->add_item(tex("ui_off2"));
        blood->add_item(tex("ui_near_player_only"));
        blood->add_item(tex("ui_on"));
        blood->select(g_blood);
    }
    {   // plant shadows, upper row
        plant_shadows = make_drop_down(window, tex("PlantShadows"), 0.52f, 0.265f);
        plant_shadows->add_item(tex("ui_none"));
        plant_shadows->add_item(tex("ui_quick"));
        plant_shadows->add_item(tex("ui_full"));
        plant_shadows->select(g_plant_shadow);
    }

    quality[0] = (option_widget*)make_medium_button(window, 0.56f, 0.53f, 3.7f, 0.265f, 0.115f,
                                                   tex("ui_enabled"), &tstring("medium_button_down"));
    quality[0]->f184 = 1;
    quality[0]->select(g_char_detail);
    set_button_text(quality[0]);
    {   // plant shadows, lower row
        plant_shadows = make_drop_down(window, tex("PlantShadows"), 0.52f, 0.145f);
        plant_shadows->add_item(tex("ui_none"));
        plant_shadows->add_item(tex("ui_quick"));
        plant_shadows->add_item(tex("ui_full"));
        plant_shadows->select(g_plant_shadow);
    }

    quality[1] = (option_widget*)make_medium_button(window, 0.56f, 0.45f, 3.7f, 0.225f, 0.075f,
                                                   tex("ui_enabled"), &tstring("medium_button"));
    quality[1]->f184 = 1;
    quality[1]->select(g_ragdoll_q);
    set_button_text(quality[1]);

    quality[2] = (option_widget*)make_medium_button(window, 0.56f, 0.37f, 3.7f, 0.185f, 0.045f,
                                                   tex("ui_enabled"), &tstring("medium_button"));
    quality[2]->f184 = 1;
    quality[2]->select(g_plant_shadow);
    set_button_text(quality[2]);

    quality[3] = (option_widget*)make_medium_button(window, 0.56f, 0.29f, 3.7f, 0.145f, 0.053f,
                                                   tex("ui_enabled"), &tstring("medium_button"));
    quality[3]->f184 = 1;
    quality[3]->select(g_char_shadow);
    set_button_text(quality[3]);

    quality[4] = (option_widget*)make_medium_button(window, 0.56f, 0.21f, 3.7f, 0.065f, 0.01f,
                                                   tex("ui_enabled"), &tstring("medium_button"));
    quality[4]->f184 = 1;
    quality[4]->select(g_particle);
    set_button_text(quality[4]);

    quality[5] = (option_widget*)make_medium_button(window, 0.56f, 0.13f, 3.7f, 0.01f, 0.005f,
                                                   tex("ui_enabled"), &tstring("medium_button"));
    quality[5]->f184 = 1;
    quality[5]->select(g_shadow_q);
    set_button_text(quality[5]);

    g_style = 1;
    g_gui.set_window(&window);
    if (!g_env_flag) {
        ui_scale->select_last();
        gamma->select_last();
        anisotropic_filtering->select_last();
    }
}
