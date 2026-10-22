// clang-format off
// Options dialog window procedure: reads/applies the render config on init, lays out the video tab,
// answers the resolution spin control (0x114), and saves (OK), discards (cancel) or re-reads (apply)
// rgl_config.txt. A WM_COMMAND from the tab control first refreshes the tab and page text, then
// dispatches the child id (a tab notification falls through to that same dispatch).
//
// The tail of the WH_WM_COMMAND case is written with inline __asm: the compiler rejects the C
// spelling (a `return` of DefWindowProcA's result from a __thiscall member whose prologue was
// rewritten through __asm and whose `switch` jump table it cannot model), so this only describes
// the intended behaviour, not compilable C (hence the slice lives in partial.txt).
#include <windows.h>

// skinmagic ordinal 9: skins a dialog window with a registered skin class name.
int __stdcall SkinDialog(HWND hwnd, const char* cls);    // SkinMagic ord9, thunk 0x535446

char* _sprintf(char* buf, const char* fmt, ...);         // 0x536f8f
int _fclose(void* file);                                  // 0x537168

void user_path(char* out);                       // 0x4bab70
bool mb_file_exists(const char* path);           // 0x415120
void* mb_open_user_file(const char* name, const char* mode);   // 0x415160
void cfg_load(void* file);                       // 0x4bbf70 (thiscall on &g_render_cfg)
void cfg_save(void* file);                       // 0x415fb0 (thiscall on &g_render_cfg)
void cfg_init(int reset);                        // 0x4b9b90 (thiscall on &g_render_cfg)
void gfx_query(int dummy);                       // 0x4bee10 (fastcall on this)
void cfg_apply();                                // 0x4bf170 (fastcall on this)
void FUN_004b9790(int v);                        // 0x4b9790
void FUN_004c6f90(int v);                        // 0x4c6f90
void fill_video_tab();                           // 0x4cf7b0 (thiscall, fills the video tab)

const char k_config[] = "rgl_config.txt";        // 0x54b048

class Dialog {
public:
    char pad[0x380];
    HWND m_hwnd;      // 0x380
    LRESULT proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// @ 0x4d0240
LRESULT Dialog::proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        m_hwnd = hwnd;
        SkinDialog(hwnd, "Dialog");
        char path[256];
        _sprintf(path, "%s", k_config);
        user_path(path);
        if (mb_file_exists(path)) {
            void* f = mb_open_user_file(k_config, "r");
            cfg_load(f);
            _fclose(f);
        } else {
            *(int*)0x5aa0ac = 0x46;
        }
        cfg_init(0);
        fill_video_tab();
        gfx_query(0);
        return 1;
    case 0x114: {
        int v = (int)SendMessageA(GetDlgItem(m_hwnd, 0x43e), 0x400, 0, 0);
        char text[256];
        _sprintf(text, "%d", v);
        SetWindowTextA(GetDlgItem(m_hwnd, 0x450), text);
        return 1;
    }
    case WM_COMMAND:
        if (wParam != 0x43d) {
            DefWindowProcA(hwnd, msg, wParam, lParam);
            __asm pop edi
            __asm pop esi
            __asm add esp, 208h
            __asm ret 10h
        }
        FUN_004b9790((int)SendMessageA(GetDlgItem(m_hwnd, 0x43d), 0x130b, 0, 0));
        // falls through to the control dispatch: a tab notification also runs it
        switch (wParam & 0xffff) {
        case 1: {               // ok: write the config back
            cfg_apply();
            void* f = mb_open_user_file(k_config, "w");
            cfg_save(f);
            _fclose(f);
            EndDialog(hwnd, 2);
            return 1;
        }
        case 2:                 // cancel
            EndDialog(hwnd, 2);
            return 1;
        case 0x448:             // refresh-rate edit: keep 10..999, else restore
            if ((wParam >> 16) == 0x200) {
                BOOL ok;
                UINT v = GetDlgItemInt(m_hwnd, 0x448, &ok, 0);
                if (ok && v >= 10 && v < 1000) {
                    *(int*)0x5aa0b4 = v;
                } else {
                    SetDlgItemInt(m_hwnd, 0x448, *(int*)0x5aa0b4, 0);
                }
            }
            return 1;
        case 0x45e:             // auto adjust: re-detect and re-read the config
            cfg_apply();
            *(int*)0x5aa0a8 = 0;
            cfg_init(1);
            gfx_query(0);
            return 1;
        case 0x45f:             // display-mode list changed
            if ((wParam >> 16) == 9) {
                FUN_004c6f90(0);
                return 1;
            }
            break;
        }
        return 1;
    }
    return 0;
}
// clang-format on
