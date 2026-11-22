// patches/gfx/gfx_video_page.cpp: the gfx settings on the in-game Video Options page (patch set "gfx", opt-in).
//
// CHANGES BEHAVIOUR on purpose: the page (Recf0, built by init_screen 0x4e05c0) gets four drop-down rows of the game's
// own kind (make_dropdown 0x4dfec0, a make_hint label styled like the page's others):
//   Anisotropy Level   Off, 2x, 4x, 8x, 16x                     (gfx_anisotropy; the page's own "Anisotropic Filtering"
//                                                                toggle is the game's on/off, this is the level)
//   Render Scale       50% 67% 75% 85% 100% 125% 150% 200%      (gfx_render_scale)
//   AMD FSR 1          Off, On                                  (gfx_fsr)
//   FSR Sharpness      0.00 (sharpest), 0.20, 0.50, 1.00, 2.00  (gfx_fsr_sharpness)
// A value from rgl_config.txt that is not a preset gets its own entry. The page has no free row, so its rows (the
// labels, drop-downs, sliders, toggles and value texts of the original, at y = 0.70 - 0.04 k) are moved closer together
// and down, and the four new rows go on top, where their lists have room to open below them: see GFX_Y.
//
// This file replaces init_screen (0x4e05c0): the faithful body of src/game/app/winmain_and_menu_screens.cpp with the rows added
// before the window goes to the GUI manager, and each y through GFX_Y. gfx_video_apply.cpp replaces the page's
// frame_update (0x4d63b0), which reads the rows into g_gfx_saved and, on Done, after rgl_config::save has written
// them, applies them (gfx_apply_saved: at the next Present, live, no restart needed).
#include "../../src/platform/image_addrs.h"
#include "../../src/game/data/env_row.h"
#include "../../src/engine/audio/music_player.h"
#include "../../src/platform/wl_marker.h"
#include "../../src/game/app/winmain_and_menu_screens.h"
#include "../../src/game/data/tableau_table.h"
#include "../../src/game/data/prsnt_table.h"
#include "../../src/game/ui/screens/progress_bar_and_load_screens.h"
#include "../../src/game/config/config.h"
#include "../../src/game/ui/widgets/widget_manager.h"
#include "../../src/game/ui/widgets/text_editor_widget.h"
#include "../../src/game/ui/gui_manager.h"
#include "../../src/engine/render/font.h"
#include "../../src/game/ui/widgets/ui_frame.h"
#include "../../src/game/common/shared_vars.h"
#include "../../src/game/config/options.h"
#include "../../src/engine/render/render_flags.h"
#include "../../src/engine/render/skinmagic.h"
#include "../../src/game/ui/screens/screen_records.h"
#include "./gfx.h"

#define set_enabled_text(o) ((KeysPage*)this)->set_enabled_text((MissionUi*)(o))   // 0x4d6dd0
#define update_perf(w) ((KeysPage*)(w))->update_perf()                          // 0x4d6b80
#define COLOR_ENABLED 0xffdfcfbfu   // the ui colour word every label of the screen is tinted with
#define SL_MIN(w) (*(float*)((char*)(w) + 0x16c))   // slider range minimum (MissionUi +0x16c)
#define SL_MAX(w) (*(float*)((char*)(w) + 0x170))   // slider range maximum
static __forceinline int drop_count(DropDown* d) { return (int)d->rows.size(); }
static __forceinline UiA* new_medium_button_i(MissionUi* parent, const tstring& text, float a, float b, float c)
{
    return new_sized_button(parent, text, a, b, c, "medium_button", "medium_button_down", 3.7f, 0.072f, 0.085f);
}
#define LABEL_STYLE(m, wd) \
    { int v_[2]; v_[0] = (wd); vt(m)->d29(v_); v_[0] = COLOR_ENABLED; vt(m)->d38(v_); vt(m)->d37(v_); }
#define LABEL(m, key, x, y, w, h) \
    { tstring k_(key); m = make_hint(window, *g_strtable_c.lookup(k_), x, GFX_Y(y), w, h); } \
    LABEL_STYLE(m, 4)
#define QBTN(m, y, getval) \
    { tstring k_("ui_enabled"); m = new_medium_button_i((MissionUi*)window, *g_strtable_c.lookup(k_), 0.52f, GFX_Y(y), 0.035f); } \
    ((UiA*)m)->f184 = 1; \
    vt(m)->d11(getval); \
    set_enabled_text(m);
#define DD_ITEM(dd, key) { tstring k_(key); vt(dd)->d50(g_strtable_c.lookup(k_)); }
#define DD_TEXT(dd, text) { tstring t_(text); vt(dd)->d50(&t_); }
struct uv_pair { float u, v; uv_pair(float a, float b) { u = a; v = b; } };   // (winmain_and_menu_screens.cpp's)

#include "../../src/game/data/list_modes.h"

// The page's rows sit at y = 0.70 - 0.04 k (k = 0 .. 17, the two-line rows' parts a few hundredths off); the window is
// 0.752 high. The four new rows take the top, 0.032 apart from GFX_TOP; GFX_Y maps every y of the original to
// y' = (top - 4 steps) - (top - y) * GFX_SQUEEZE, so the 18 rows take 0.032 each instead of 0.04 under them and the last
// ends where it did. (Drop-down lists open downwards: rows at the bottom could not show theirs.) The Done button and the
// performance caption (x 0.68 and right of it) keep their places.
#define GFX_TOP 0.70f
#define GFX_SQUEEZE 0.8f
#define GFX_STEP (0.04f * GFX_SQUEEZE)
#define GFX_Y(y) ((GFX_TOP - 4 * GFX_STEP) - (GFX_TOP - (y)) * GFX_SQUEEZE)
#define GFX_ROW0 GFX_TOP

DropDown* g_gfx_page_dd[4];         // the rows' drop-downs: anisotropy, scale, FSR, sharpness (gfx_video_apply.cpp)
int g_gfx_page_values[4][10];       // each row's entries' values, in order (anisotropy level, percent, 0/1, 1/100 stop)
int g_gfx_page_count[4];

static void add_entry(int row, const char* text, int value)
{
    DD_TEXT(g_gfx_page_dd[row], text)
    g_gfx_page_values[row][g_gfx_page_count[row]++] = value;
}

// presets, with `cur` among them when it is none of them, added largest first (a drop-down lists its last entry at the
// top, so they show in ascending order); returns the index of `cur`
static int add_presets(int row, const int* p, int n, int cur, const char* fmt, double div, const char* first, const char* last)
{
    int v[12], k = 0, sel = -1;
    bool placed = false;
    for (int i = 0; i < n; i++) {
        if (!placed && cur < p[i]) {
            v[k++] = cur;
            placed = true;
        }
        if (p[i] == cur)
            placed = true;
        v[k++] = p[i];
    }
    if (!placed)
        v[k++] = cur;
    for (int j = k - 1; j >= 0; j--) {
        char t[32];
        if (v[j] == p[0] && first)
            sprintf(t, first, v[j] / div);
        else if (v[j] == p[n - 1] && last)
            sprintf(t, last, v[j] / div);
        else
            sprintf(t, fmt, v[j] / div);
        if (v[j] == cur)
            sel = g_gfx_page_count[row];
        add_entry(row, t, v[j]);
    }
    return sel;
}

static void gfx_row(ContainerBase* window, int row, const char* label, const char* name, float y)
{
    MissionUi* m;
    { tstring t(label); m = make_hint(window, t, 0.2f, y, 0.3f, 0.035f); }
    LABEL_STYLE(m, 4)
    { tstring k(name); g_gfx_page_dd[row] = make_dropdown((MissionUi*)window, &k, 0.52f, y + 0.005f); }
    g_gfx_page_count[row] = 0;
}

static void gfx_page_rows(ContainerBase* window)
{
    const gfx_settings& s = g_gfx_saved;
    float y = GFX_ROW0;
    gfx_row(window, 0, "Anisotropy Level:", "GfxAniso", y);
    vt(g_gfx_page_dd[0])->d11(add_presets(0, g_gfx_aniso_levels, 5, s.anisotropy, "%.0fx", 1.0, "Off", 0));
    y -= GFX_STEP;
    gfx_row(window, 1, "Render Scale (3D):", "GfxScale", y);
    vt(g_gfx_page_dd[1])->d11(add_presets(1, g_gfx_scale_presets, 8, (int)floor(s.render_scale * 100.0f + 0.5f), "%.0f%%",
                                          1.0, 0, 0));
    y -= GFX_STEP;
    gfx_row(window, 2, "AMD FSR 1:", "GfxFsr", y);
    {
        tstring k("ui_on");
        DD_TEXT(g_gfx_page_dd[2], g_strtable_c.lookup(k)->c_str())
        g_gfx_page_values[2][g_gfx_page_count[2]++] = 1;
    }
    {
        tstring k("ui_off2");
        DD_TEXT(g_gfx_page_dd[2], g_strtable_c.lookup(k)->c_str())
        g_gfx_page_values[2][g_gfx_page_count[2]++] = 0;
    }
    vt(g_gfx_page_dd[2])->d11(s.fsr ? 0 : 1);
    y -= GFX_STEP;
    gfx_row(window, 3, "FSR Sharpness:", "GfxSharp", y);
    vt(g_gfx_page_dd[3])->d11(add_presets(3, g_gfx_sharp_presets, 5, (int)floor(s.fsr_sharpness * 100.0f + 0.5f), "%.2f",
                                          100.0, "%.2f (sharpest)", "%.2f (softest)"));
}

// @ replaces 0x4e05c0
void Recf0::init_screen()
{
    f_ec = 0;
    f_ed = 0;
    day_saved = g_cfg.texture_detail;
    g_gui.reset();
    window = new NodeA(0);
    {
        float p[2];
        p[0] = -0.001f; p[1] = -0.001f;
        vt(window)->d22(p);
    }
    {
        float s[2];
        s[0] = 1.002f; s[1] = 0.752f;
        vt(window)->d25(s);
    }
    {
        tstring k("");
        float o_[2];
        vt(window)->d41(o_, &k);
    }
    {
        unsigned c[2];
        c[0] = COLOR_ENABLED;
        vt(window)->d38(c);
    }
    {
        rgl::mesh* m;
        {
            tstring k("options_window");
            m = res_get(k)->clone();
        }
        vt(window)->d31(0, m);
    }
    { tstring k("ui_done"); done = new_medium_button((MissionUi*)window, g_strtable_c.lookup(k), 0.75f, 0.02f, 0.05f); }

    // the gamma as text: g_options.scale_x * 100 with a point after the first digit; the default 2.20 is shown as "ui_default"
    char buf[0x80];
    _itoa((int)(g_options.scale_x * 100.0f), buf, 10);
    tstring gamma_text(buf);
    { tstring dot("."); gamma_text.insert(1, dot); }
    if (gamma_text.equals("2.20")) {
        tstring k("ui_default");
        gamma_text = *g_strtable_c.lookup(k);
    }

    // the first label starts as the gamma text, then d41 replaces it with the DirectX version
    val_vertex_shaders = make_hint(window, gamma_text, 0.52f, GFX_Y(0.7f), 0.3f, 0.035f);
    {
        unsigned c[2];
        c[0] = 4;
        vt(val_vertex_shaders)->d29(c);
        c[0] = COLOR_ENABLED;
        vt(val_vertex_shaders)->d38(c);
        {
            tstring s(g_cfg.use_vertex_shaders ? "DirectX 9" : "DirectX 7");
            vt(val_vertex_shaders)->d41(c, &s);   // c doubles as the out buffer
        }
        vt(val_vertex_shaders)->d37(c);           // ... and is what d37 receives next (the original reuses the slot)
    }
    LABEL(lbl_vertex_shaders, "ui_enable_vertex_shaders2", 0.2f, 0.7f, 0.3f, 0.035f)
    LABEL(lbl_screen_size, "ui_screen_size2", 0.2f, 0.66f, 0.3f, 0.035f)
    LABEL(lbl_gamma, "ui_gamma", 0.2f, 0.62f, 0.3f, 0.035f)
    val_gamma = make_hint(window, gamma_text, 0.823f, GFX_Y(0.615f), 0.3f, 0.035f);
    LABEL_STYLE(val_gamma, 4)
    LABEL(lbl_shadow_quality, "ui_shadow_quality2", 0.2f, 0.58f, 0.3f, 0.035f)
    LABEL(lbl_texture_detail, "ui_texture_detail", 0.2f, 0.54f, 0.3f, 0.035f)
    {
        {
            tstring k("ui_percentage_value");
            sprintf(buf, g_strtable_c.lookup(k)->c_str(), g_cfg.texture_detail);
        }
    }
    {
        tstring t(buf);
        val_texture_detail = make_hint(window, t, 0.823f, GFX_Y(0.535f), 0.3f, 0.035f);
    }
    LABEL_STYLE(val_texture_detail, 4)
    LABEL(lbl_grass_density, "ui_grass_density", 0.2f, 0.5f, 0.3f, 0.035f)
    {
        {
            tstring k("ui_percentage_value");
            sprintf(buf, g_strtable_c.lookup(k)->c_str(), g_options.i34);
        }
    }
    {
        tstring t(buf);
        val_grass_density = make_hint(window, t, 0.823f, GFX_Y(0.495f), 0.3f, 0.035f);
    }
    LABEL_STYLE(val_grass_density, 4)
    LABEL(lbl_char_detail, "ui_character_detail", 0.2f, 0.46f, 0.3f, 0.035f)
    LABEL(lbl_char_low, "ui_low", 0.52f, 0.45f, 0.03f, 0.05f)
    LABEL(lbl_char_high, "ui_high", 0.823f, 0.45f, 0.03f, 0.05f)
    LABEL(lbl_char_shadow_detail, "ui_character_shadow_detail", 0.2f, 0.42f, 0.3f, 0.035f)
    LABEL(lbl_shadow_low, "ui_low", 0.52f, 0.41f, 0.03f, 0.05f)
    LABEL(lbl_shadow_high, "ui_high", 0.823f, 0.41f, 0.03f, 0.05f)
    LABEL(lbl_corpses, "ui_number_of_corpses", 0.2f, 0.38f, 0.3f, 0.035f)
    LABEL(lbl_ragdolls, "ui_number_of_ragdolls", 0.2f, 0.34f, 0.3f, 0.035f)
    LABEL(lbl_blood_stains, "ui_blood_stains", 0.2f, 0.3f, 0.3f, 0.035f)
    LABEL(lbl_dynamic_lighting, "ui_dynamic_lighting", 0.2f, 0.26f, 0.3f, 0.035f)
    LABEL(lbl_character_shadows, "ui_character_shadows", 0.2f, 0.22f, 0.3f, 0.035f)
    LABEL(lbl_environment_shadows, "ui_environment_shadows", 0.2f, 0.18f, 0.3f, 0.035f)
    LABEL(lbl_realistic_shadows, "ui_realistic_shadows_on_plants", 0.2f, 0.14f, 0.3f, 0.035f)
    LABEL(lbl_particle_systems, "ui_particle_systems", 0.2f, 0.1f, 0.3f, 0.035f)
    LABEL(lbl_anisotropic, "ui_anisotropic_filtering", 0.2f, 0.06f, 0.3f, 0.035f)
    LABEL(lbl_fast_water, "ui_fast_water_reflection", 0.2f, 0.02f, 0.3f, 0.035f)
    { tstring k(""); quality_label = make_hint(window, k, 0.68f, GFX_Y(0.08f), 0.3f, 0.035f); }
    LABEL_STYLE(quality_label, 16)

    update_perf(this);

    { tstring k("ui_change"); change = new_medium_button((MissionUi*)window, g_strtable_c.lookup(k), 0.65f, GFX_Y(0.705f), 0.035f); }

    // resolution
    { tstring k("Resolution"); resolution = make_dropdown((MissionUi*)window, &k, 0.52f, GFX_Y(0.665f)); }
    // The original passes 3-argument vector4 temporaries (w never set, 0x4e1751 / 0x4e1797 / 0x4e17dd copy the temp's
    // unwritten 4th float); set_color 0x47af30 reads only x, y, z
    { ((rgl::mesh*)resolution->tracks[2])->set_color(rgl::vector4(0.7f, 0.7f, 1.0f)); }
    { ((rgl::mesh*)resolution->tracks[1])->set_color(rgl::vector4(0.7f, 0.7f, 1.0f)); }
    { ((rgl::mesh*)resolution->tracks[3])->set_color(rgl::vector4(0.7f, 0.7f, 1.0f)); }
    {
        float sc[2];
        sc[0] = 0.0139999995f;   // 0x3c656041 (0.014f would be 0x3c656042)
        sc[1] = 0.0139999995f;
        vt(resolution)->d27(sc);
    }
    {
        float sc[2];
        sc[0] = 0.0139999995f;
        sc[1] = 0.00805f;
        vt(resolution)->d28(sc);
    }
    vt(resolution)->d03();
    { *(uv_pair*)&resolution->fD8 = uv_pair(0.7f, 0.7f); }
    {
        DropDown* r = resolution;
        *(uv_pair*)&r->e[0] = uv_pair(r->e[0] - 0.00528f, r->e[1] - 0.00528f);
    }

    {
    std::vector<tstring> modes = list_modes();
    for (int i = (int)modes.size() - 1; i >= 0; i--) {
        bool ok = g_cfg.display_bits == 16 && modes[i].find("16 bits", 0) >= 0;
        if (!ok)
            ok = modes[i].find("32 bits", 0) >= 0;
        if (ok) {
            vt(resolution)->d50(&modes[i]);
            char mb[0x80];
            sprintf(mb, "%dx%d", g_cfg.display_width, g_cfg.display_height);
            ok = modes[i].find(mb, 0) >= 0;
            if (ok)
                vt(resolution)->d11(drop_count(resolution) - 1);
        }
    }
    DD_ITEM(resolution, "ui_use_desktop_resolution2")
    if (g_cfg.display_width == 0)
        vt(resolution)->d11(drop_count(resolution) - 1);

    ui_scale = make_slider((MissionUi*)window, 0.56f, GFX_Y(0.625f), 1.0f);
    SL_MIN(ui_scale) = 1.5f;
    SL_MAX(ui_scale) = 2.5f;
    vt(ui_scale)->d13(g_options.scale_x);

    {   // shadow quality
        { tstring k("ShadowQ"); shadow_quality = make_dropdown((MissionUi*)window, &k, 0.52f, GFX_Y(0.585f)); }
        DD_ITEM(shadow_quality, "ui_m_ultra_high2")
        DD_ITEM(shadow_quality, "ui_m_high2")
        DD_ITEM(shadow_quality, "ui_m_low2")
        vt(shadow_quality)->d11(2 - g_cfg.shadow_quality_req);   // 0x5aa004
    }

    days = make_slider((MissionUi*)window, 0.56f, GFX_Y(0.545f), 1.0f);
    SL_MIN(days) = 10.0f;
    SL_MAX(days) = 100.0f;
    vt(days)->d13((float)g_cfg.texture_detail);

    ragdoll_limit = make_slider((MissionUi*)window, 0.56f, GFX_Y(0.505f), 1.0f);
    SL_MIN(ragdoll_limit) = 0.0f;
    SL_MAX(ragdoll_limit) = 100.0f;
    vt(ragdoll_limit)->d13((float)g_options.i34);

    y_scale = make_slider((MissionUi*)window, 0.56f, GFX_Y(0.465f), 1.0f);
    vt(y_scale)->d13(g_options.scale_y);

    z_scale = make_slider((MissionUi*)window, 0.56f, GFX_Y(0.425f), 1.0f);
    vt(z_scale)->d13(g_options.scale_z);

    {   // corpses (0x5aa008)
        { tstring k("Corpses"); corpses = make_dropdown((MissionUi*)window, &k, 0.52f, GFX_Y(0.385f)); }
        DD_ITEM(corpses, "ui_unlimited")
        DD_TEXT(corpses, "40")
        DD_TEXT(corpses, "20")
        DD_TEXT(corpses, "10")
        DD_TEXT(corpses, "5")
        DD_TEXT(corpses, "0")
        vt(corpses)->d11(5 - g_options.i00);
    }
    {   // ragdolls (0x5aa00c)
        { tstring k("Ragdolls"); ragdolls = make_dropdown((MissionUi*)window, &k, 0.52f, GFX_Y(0.345f)); }
        DD_ITEM(ragdolls, "ui_unlimited")
        DD_TEXT(ragdolls, "10")
        DD_TEXT(ragdolls, "5")
        DD_TEXT(ragdolls, "3")
        DD_TEXT(ragdolls, "1")
        DD_TEXT(ragdolls, "0")
        vt(ragdolls)->d11(5 - g_options.i04);
    }
    {   // blood (0x5aa020)
        { tstring k("Blood"); blood = make_dropdown((MissionUi*)window, &k, 0.52f, GFX_Y(0.305f)); }
        DD_ITEM(blood, "ui_off2")
        DD_ITEM(blood, "ui_near_player_only")
        DD_ITEM(blood, "ui_on")
        vt(blood)->d11(g_options.blood);
    }

    QBTN(quality[0], 0.265f, g_options.char_detail)
    QBTN(quality[1], 0.225f, g_options.i28)
    QBTN(quality[2], 0.185f, g_options.i2c)
    {   // plant shadows (0x5aa084)
        { tstring k("PlantShadows"); plant_shadows = make_dropdown((MissionUi*)window, &k, 0.52f, GFX_Y(0.145f)); }
        DD_ITEM(plant_shadows, "ui_none")
        DD_ITEM(plant_shadows, "ui_quick")
        DD_ITEM(plant_shadows, "ui_full")
        vt(plant_shadows)->d11(g_options.i7c);
    }
    QBTN(quality[3], 0.105f, g_options.i20)
    QBTN(quality[4], 0.065f, g_options.i38)
    QBTN(quality[5], 0.025f, g_cfg.fake_reflections)

    gfx_page_rows(window);      // gfx: the four rows of the gfx set (gfx_video_page.cpp)

    {
        void* w = window;
        g_gui.dirty = 1;
        g_gui.objs.push_back((NodeA* const&)(w));
    }
    if (!g_use_shaders) {
        vt(ui_scale)->d44();
        vt(lbl_gamma)->d44();
        vt(val_gamma)->d44();
    }
    }
}
