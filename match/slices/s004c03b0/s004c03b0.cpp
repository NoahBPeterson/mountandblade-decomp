// Slice s004c03b0: the editor's "set module system folder" dialog procedure (0x4c03b0) and the game-variables
// loader (0x4c0520) that the same editor calls when a mission starts. The dialog reads/writes the
// "module_system_folder" value of the game's registry key (helpers 0x4b96d0 / 0x4b9720 / 0x4beb80, modelled here)
// and edits a 0x1000-byte path edit box. Names are provisional.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "tstring.h"

// The A-suffixed imports the original reaches through its import table (windows.h only has the macros).
extern "C" __declspec(dllimport) BOOL __stdcall SetDlgItemTextA(HWND, int, LPCSTR);   // 0x549328
extern "C" __declspec(dllimport) UINT __stdcall GetDlgItemTextA(HWND, int, LPSTR, int); // 0x5492f0
extern "C" __declspec(dllimport) INT_PTR __stdcall EndDialog(HWND, INT_PTR);          // 0x5493b0

// ---- registry helpers of the editor ------------------------------------------------------------------------
void keys_open(HKEY* out);                         // 0x4b96d0
void keys_read(HKEY* key, const char* name, tstring* out);   // 0x4b9720
void keys_write(HKEY* key, const char* name, tstring* v)     // 0x4beb80
{
    if (*key != (HKEY)0)
        RegSetValueExA(*key, name, 0, REG_SZ, (const BYTE*)(v->buf ? v->buf : (const char*)v), v->len + 1);
}

// The game-variables file loader of the editor (only referenced here, not a match target of this slice).
struct GameVars {
    char pad0[0x2c000];
    int  count;                    // +0x2c000
    void load(FILE* f);            // 0x4bd630
};
extern GameVars g_game_vars;       // 0xa44240

extern const char s_msf[];         // 0x594b20  "module_system_folder"
extern const char s_cancel_btn[];  // (control id 2)
enum { IDC_FOLDER = 0x4b8 };

// @ 0x4c03b0
INT_PTR CALLBACK folder_dlg_proc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char buf[0x1000];
    if (msg == WM_INITDIALOG) {
        tstring folder;
        HKEY key;
        keys_open(&key);
        if (key != (HKEY)0) {
            keys_read(&key, s_msf, &folder);
            if (key != (HKEY)0)
                RegCloseKey(key);
        }
        SetDlgItemTextA(dlg, IDC_FOLDER, folder.buf ? folder.buf : (const char*)&folder);
        folder.~tstring();
        return TRUE;
    }
    if (msg != WM_COMMAND)
        return FALSE;
    int id = LOWORD(wParam) - 1;
    if (id == 0) {                     // OK: store the folder
        GetDlgItemTextA(dlg, IDC_FOLDER, buf, 0x1000);
        tstring folder(buf);
        if (folder.buf[folder.len - 1] != '\\')
            folder.push_back('\\');
        HKEY key;
        keys_open(&key);
        if (key != (HKEY)0) {
            keys_write(&key, s_msf, &folder);
            if (key != (HKEY)0)
                RegCloseKey(key);
        }
        EndDialog(dlg, 2);
        folder.~tstring();
        return FALSE;
    }
    if (id == 1)                       // Cancel
        EndDialog(dlg, 2);
    return FALSE;
}

extern const char s_modules[];  // 0x591540  "Modules\"
extern const char s_vars_file[];// 0x594b40  "\\game_variables.txt"
extern const char s_mode_r[];   // 0x54a8fc  "r"
extern tstring g_module_name;   // 0x8ee3c38  the currently selected module (defines its folder name)

// @ 0x4c0520
void load_game_variables()
{
    tstring path(s_modules);
    tstring& mod = g_module_name;
    path.append(mod);
    tstring name(s_vars_file);
    path.append(name);
    name.~tstring();
    FILE* f = fopen(path.buf ? path.buf : (const char*)&path, s_mode_r);
    if (f != 0) {
        g_game_vars.load(f);
        fclose(f);
    }
    path.~tstring();
}
