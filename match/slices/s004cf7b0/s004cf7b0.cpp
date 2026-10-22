// clang-format off
// Options dialog: fills the video options tab, sizes the mode list and labels every control.
#include <windows.h>
#include <vector>

// rgl::string: 16 bytes, verified in match/include/rgl.h (ctor 0x42bbe0).
struct tstring {
    char* p; int size, cap, len;     // 0x0 / 0x4 / 0x8 / 0xc
    tstring(const char* s);          // 0x42bbe0
    tstring(const tstring& o);       // 0x42c110
    ~tstring();                      // 0x42bc40
};

// 0x8ee3c48: localized string table (16-byte entries).
extern tstring g_strtab[];

// 0x433bb0: hashes a key object into an index of the table.
int strtab_hash(const tstring& key);

// 0x43a1e0: localized text of a table key (text at entry+4; the empty case falls back to that address).
const char* strtab_lookup(tstring& key);

// 0x4cf100: fills the list of display mode names.
void FUN_004cf100(std::vector<tstring>* out);

void FUN_004c6f90(int v);      // 0x4c6f90
void FUN_004b9790(int v);      // 0x4b9790

const float g_width_scale = 0.0023752970155328512f;    // 0x59642c
const float g_height_scale = 0.0020833334419876337f;   // 0x596430

class Dialog {
public:
    char pad[0x380];
    HWND m_hwnd;      // 0x380
    int m_ctrl;       // 0x384
    void fill_video_tab();
};

// A table key whose object lives only for the full expression that uses it (the original's
// `tstring key("..."); strtab_lookup(key)` pairs are an inlined helper of exactly this shape).
struct key_text {
    char* text;
    key_text(const char* s) { tstring k(s); text = (char*)strtab_lookup(k); }
};

// @ 0x4cf7b0
void Dialog::fill_video_tab()
{
    int item[3];
    item[0] = 1;
    item[2] = -1;
    m_ctrl = 0;

    HWND tabs = GetDlgItem(m_hwnd, 0x43d);
    item[1] = (int)key_text("ui_video").text;
    SendMessageA(tabs, 0x1307, 0, (LPARAM)item);
    item[1] = (int)key_text("ui_audio").text;
    SendMessageA(tabs, 0x1307, 1, (LPARAM)item);
    item[1] = (int)key_text("ui_game").text;
    SendMessageA(tabs, 0x1307, 2, (LPARAM)item);
    item[1] = (int)key_text("ui_advanced").text;
    SendMessageA(tabs, 0x1307, 3, (LPARAM)item);

    RECT dlg;
    RECT rc;
    GetWindowRect(m_hwnd, &dlg);
    GetWindowRect(tabs, &rc);
    rc.top -= 0x3c;
    rc.left -= 10;
    int top = rc.top;
    int left = rc.left;
    int h = (int)((float)(rc.bottom - 0x3c) * g_height_scale) - top;
    int w = (int)((float)(rc.right - 0x3c) * g_width_scale) - rc.left;
    SetWindowPos(tabs, NULL, left - dlg.left, top - dlg.top, w, h, 0x14);

    HWND modes = GetDlgItem(m_hwnd, 0x45f);
    if (modes) {
        SendMessageA(modes, 0x14b, 0, 0);
        std::vector<tstring> list;
        FUN_004cf100(&list);
        for (unsigned i = 0; i < list.size(); i++)
            SendMessageA(modes, 0x14a, i, (LPARAM)list[i].p);
    }
    SendMessageA(modes, 0x14a, 0, (LPARAM)key_text("ui_use_desktop_resolution").text);

    HWND quality = GetDlgItem(m_hwnd, 0x467);
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)key_text("ui_m_ultra_high").text);
        SendMessageA(quality, 0x151, i, 2);
    }
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)key_text("ui_m_high").text);
        SendMessageA(quality, 0x151, i, 1);
    }
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)key_text("ui_m_low").text);
        SendMessageA(quality, 0x151, i, 0);
    }
    {
        int i = SendMessageA(GetDlgItem(m_hwnd, 0x468), 0x14a, 0, (LPARAM)key_text("ui_directx_9").text);
        SendMessageA(GetDlgItem(m_hwnd, 0x468), 0x151, i, 1);
    }
    {
        int i = SendMessageA(GetDlgItem(m_hwnd, 0x468), 0x14a, 0, (LPARAM)key_text("ui_directx_7").text);
        SendMessageA(GetDlgItem(m_hwnd, 0x468), 0x151, i, 0);
    }

    FUN_004c6f90(1);

    {
        tstring caption(key_text("ui_configure_caption").text);
        char* p = caption.p;
        if (!p)
            p = (char*)&caption;
        SendMessageA(m_hwnd, 0xc, 0, (LPARAM)p);
    }

    SetDlgItemTextA(m_hwnd, 0x43f, key_text("ui_texture_detail").text);
    SetDlgItemTextA(m_hwnd, 0x449, key_text("ui_max_frame_rate").text);
    SetDlgItemTextA(m_hwnd, 0x460, key_text("ui_screen_size").text);
    SetDlgItemTextA(m_hwnd, 0x462, key_text("ui_antialiasing").text);
    SetDlgItemTextA(m_hwnd, 0x492, key_text("ui_shadow_quality").text);
    SetDlgItemTextA(m_hwnd, 0x493, key_text("ui_enable_vertex_shaders").text);
    SetDlgItemTextA(m_hwnd, 0x44b, key_text("ui_create_brf").text);
    SetDlgItemTextA(m_hwnd, 0x443, key_text("ui_disable_blood").text);
    SetDlgItemTextA(m_hwnd, 0x446, key_text("ui_disable_music").text);
    SetDlgItemTextA(m_hwnd, 0x452, key_text("ui_disable_sound").text);
    SetDlgItemTextA(m_hwnd, 0x442, key_text("ui_enable_cheats").text);
    SetDlgItemTextA(m_hwnd, 0x44c, key_text("ui_enable_edit_mode").text);
    SetDlgItemTextA(m_hwnd, 0x44a, key_text("ui_force_single_threading").text);
    SetDlgItemTextA(m_hwnd, 0x441, key_text("ui_enable_pixel_shaders").text);
    SetDlgItemTextA(m_hwnd, 0x44d, key_text("ui_enable_sound_variation").text);
    SetDlgItemTextA(m_hwnd, 0x445, key_text("ui_show_framerate").text);
    SetDlgItemTextA(m_hwnd, 0x487, key_text("ui_force_vsync").text);
    SetDlgItemTextA(m_hwnd, 0x444, key_text("ui_start_windowed").text);
    SetDlgItemTextA(m_hwnd, 2, key_text("ui_cancel2").text);
    SetDlgItemTextA(m_hwnd, 1, key_text("ui_ok2").text);
    SetDlgItemTextA(m_hwnd, 0x45e, key_text("ui_auto_adjust").text);

    m_ctrl = SendMessageA(GetDlgItem(m_hwnd, 0x43e), 0x406, 0, 0x64000a);
    FUN_004b9790(m_ctrl);
}
// clang-format on
