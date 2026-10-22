// Slice s004bf870: the editor's script debugger dialog procedure (one function, ~2873 bytes). It handles
// WM_INITDIALOG (layout + load the script list from ID_scripts.py), WM_COMMAND (script combo change, index
// combo change, the debug/stop toggle, "reload module_scripts.py") and WM_MOVE (re-layout). Part of the editor
// panel class from s004bf170: the dialog hwnd lives at +0x378; the game state is the global at 0x5e2930.
// Names are provisional; only the offsets this code touches are modelled.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <richedit.h>
#include <stdio.h>
#include "tstring.h"

// The A-suffixed imports the original reaches through its import table (windows.h only has the macros).
extern "C" __declspec(dllimport) HWND __stdcall GetDlgItemA(HWND, int);              // 0x54932c
extern "C" __declspec(dllimport) BOOL __stdcall SetDlgItemTextA(HWND, int, LPCSTR);   // 0x549328

int __cdecl _sprintf(char*, const char*, ...);   // 0x536f8f

// ---- engine text log stream (global object at 0x5a9f60, see s004b8f40): slot 10 (+0x28) is w_cstr ---------
struct stream_base {
    virtual void reset();
    virtual void rewind();
    virtual stream_base& w_stream(void* o) = 0;
    virtual stream_base& w_float(void* v) = 0;
    virtual stream_base& w_bool(void* v) = 0;
    virtual stream_base& w_u64(void* v) = 0;
    virtual stream_base& w_i64(void* v) = 0;
    virtual stream_base& w_uint(void* v) = 0;
    virtual stream_base& w_int(int* v) = 0;
    virtual stream_base& w_byte(void* v) = 0;
    virtual stream_base& w_cstr(const char* s) = 0;
};
struct log_stream : stream_base {
    virtual stream_base& w_stream(void* o);
    virtual stream_base& w_float(void* v);
    virtual stream_base& w_bool(void* v);
    virtual stream_base& w_u64(void* v);
    virtual stream_base& w_i64(void* v);
    virtual stream_base& w_uint(void* v);
    virtual stream_base& w_int(int* v);
    virtual stream_base& w_byte(void* v);
    virtual stream_base& w_cstr(const char* s);
};
extern log_stream g_log;                        // 0x5a9f60

// ---- engine file helpers (Ghidra FUNs; not part of the slice's match targets) -------------------------------
long long file_length(FILE* f)                  // 0x42cbc0
{
    long p = ftell(f);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, p, SEEK_SET);
    return (long long)n;
}

// ---- registry helpers of the editor (0x4b96d0 / 0x4b9720) ---------------------------------------------------
void keys_open(HKEY* out)                       // 0x4b96d0
{
    DWORD disp;
    RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\MountAndBladeKeys", 0, 0, 0, KEY_ALL_ACCESS, 0, out, &disp);
    if (disp == 1)
        g_log.w_cstr("Creating new Registry Key");
}

void keys_read(HKEY* key, const char* value, tstring* out)   // 0x4b9720
{
    DWORD cb = 0x100;
    DWORD type = 0;
    char buf[256];
    buf[255] = 0;
    out->assign("");
    if (*key != (HKEY)0 && RegQueryValueExA(*key, value, 0, &type, (BYTE*)buf, &cb) == 0)
        out->assign(buf);
}

// ---- the editor panel (only the offsets these paths touch) -------------------------------------------------
struct EditorPanel {
    char pad0[0x378];
    HWND dlg;                   // +0x378  the script debugger dialog
    void msg_script_debugger(HWND h, UINT msg, WPARAM wParam, LPARAM lParam);
};

// ---- game state (object at global 0x5e2930): only the debugger's fields are modelled ------------------------
void* g_game;                       // 0x5e2930
struct GameDebugState {
    tstring module_folder;          // +0x1a060 module system folder (also the FILE* slot at +0)
    int     paused;                 // +0x1a068 "Stop"/"Debug" toggle
    tstring script_text;            // +0x1a06c text of the selected line
    int     script_index;           // +0x1a07c selected script index (CB_GETITEMDATA)
    tstring script_list;            // +0x1a080 module_scripts.py contents
    FILE*   debugger_file;          // +0x1a08c the debugger's script file
};
#define GD (*(GameDebugState*)((char*)g_game + 0x1a060))

extern const char s_empty[];            // 0x54a480  ""
extern const char s_mode_r[];           // 0x54a8fc  "r"
extern const char s_error[];            // 0x594a84  "ERROR"
extern const char s_courier[];          // 0x594a78  "Courier New"
extern const char s_script_prefix[];    // 0x594940  "script_"
extern const char s_id_scripts[];       // 0x594948  "ID_scripts.py"
extern const char s_module_scripts[];   // 0x594b0c  "module_scripts.py"
extern const char s_msf[];              // 0x594b20  "module_system_folder"
extern const char s_stop[];             // 0x594b38  "Stop"
extern const char s_debug[];            // 0x594594  "Debug"
extern const char s_help[];             // 0x594958  long "When the game freezes, press 1 ..." text
extern const char s_file_missing[];     // 0x594a90  "File %s could not be found..."
extern const char s_fmt[];              // 0x54ba40  "%s"

enum {  // control ids of the debugger dialog
    IDC_DBG_VIEW   = 0x4b0, IDC_DBG_STOP = 0x4b3, IDC_DBG_HELP = 0x4b4,
    IDC_DBG_SCRIPT = 0x4bb, IDC_DBG_INDEX = 0x46c,
};

#define CBN_SELCHANGE_N 9   // HIWORD(wParam) of a combo selection change

// Load "<module_system_folder>\\<name>" into 'path' and open it. On failure the caller reports it.
static FILE* load_module_file(const char* name, tstring& path)
{
    HKEY key;
    tstring folder;
    keys_open(&key);
    if (key != (HKEY)0) {
        keys_read(&key, s_msf, &folder);
        if (key != (HKEY)0)
            RegCloseKey(key);
    }
    folder.~tstring();
    tstring fname(name);
    path.append(fname);
    fname.~tstring();
    return fopen(path.buf ? path.buf : (const char*)&path, "r");
}

// Report "file %s could not be found" for the given path.
static void report_missing(const tstring& path)
{
    char msg[300];
    _sprintf(msg, s_file_missing, path.buf ? path.buf : (const char*)&path);
    MessageBoxA(0, msg, s_error, 0);
}

// @ 0x4bf870
void EditorPanel::msg_script_debugger(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG) {
        dlg = h;
        RECT rc, br;
        GetWindowRect(dlg, &rc);
        SetWindowPos(GetDlgItemA(dlg, IDC_DBG_VIEW), 0, rc.left + 10, rc.top + 0x78,
                     (rc.right - rc.left) - 0x1e, (rc.bottom - rc.top) - 0xf5, 0x14);
        GetWindowRect(GetDlgItemA(dlg, 2), &br);
        SetWindowPos(GetDlgItemA(dlg, 2), 0,
                     ((rc.right - rc.left) / 2 - br.right) + br.left + rc.left,
                     rc.bottom - 0x73, br.right - br.left, br.bottom - br.top, 0x14);
        SetDlgItemTextA(dlg, IDC_DBG_VIEW, "");
        SetDlgItemTextA(dlg, IDC_DBG_HELP, s_help);
        {
            tstring path;
            FILE* f = load_module_file(s_id_scripts, path);
            if (f == 0) {
                GD.debugger_file = 0;
                report_missing(path);
            } else {
                long long n = file_length(f);
                tstring text;
                text.read_from(f, (size_t)n);
                fclose(f);
                HWND combo = GetDlgItemA(h, IDC_DBG_SCRIPT);
                SendMessageA(combo, CB_RESETCONTENT, 0, 0);
                tstring pfx(s_script_prefix);
                int i = text.find(pfx, 0);
                i += 7;
                pfx.~tstring();
                char sp = ' ';
                int e = text.find(sp, i);
                while (i > 6 && e >= 0) {
                    tstring name = text.substr(i, e);
                    SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)(name.buf ? (void*)name.buf : (void*)&name));
                    name.~tstring();
                    tstring p2(s_script_prefix);
                    i = text.find(p2, e);
                    i += 7;
                    p2.~tstring();
                    e = text.find(sp, i);
                }
                text.~tstring();
            }
            path.~tstring();
        }
        return;
    }
    if (msg != WM_COMMAND) {
        if (msg == WM_MOVE) {
            int x = ((int*)lParam)[0], y = ((int*)lParam)[1];
            int w = ((int*)lParam)[2] - x, hgt = ((int*)lParam)[3] - y;
            RECT br;
            SetWindowPos(GetDlgItemA(dlg, IDC_DBG_VIEW), 0, x + 10, y + 0x78, w - 0x1e, hgt - 0xf5, 0x14);
            GetWindowRect(GetDlgItemA(dlg, 2), &br);
            SetWindowPos(GetDlgItemA(dlg, 2), 0, (w / 2 - br.right) + br.left + x, y + hgt - 0x73,
                         br.right - br.left, br.bottom - br.top, 0x14);
            return;
        }
        return;
    }
    {
        UINT id = LOWORD(wParam);
        if (id < 0x4b4) {
            if (id == IDC_DBG_STOP) {
                GD.paused = 1 - GD.paused;
                if (GD.paused != 0)
                    SetDlgItemTextA(dlg, IDC_DBG_STOP, s_stop);
                else
                    SetDlgItemTextA(dlg, IDC_DBG_STOP, s_debug);
                return;
            }
            if (id == 2) {
                GD.module_folder.assign(s_empty);
                return;
            }
            if (id == IDC_DBG_INDEX && HIWORD(wParam) == CBN_SELCHANGE_N) {
                char buf[1024];
                WPARAM sel = SendMessageA(GetDlgItemA(dlg, IDC_DBG_SCRIPT), CB_GETCURSEL, 0, 0);
                SendMessageA(GetDlgItemA(dlg, IDC_DBG_SCRIPT), CB_GETLBTEXT, sel, (LPARAM)buf);
                GD.script_text.assign(buf);
                LRESULT ix = SendMessageA(GetDlgItemA(dlg, IDC_DBG_INDEX), CB_GETCURSEL, 0, 0);
                if (ix >= 0)
                    GD.script_index = SendMessageA(GetDlgItemA(dlg, IDC_DBG_INDEX), CB_GETITEMDATA, ix, 0);
                else
                    GD.script_index = 0;
                return;
            }
        } else if (id == IDC_DBG_SCRIPT && HIWORD(wParam) == CBN_SELCHANGE_N) {
            if (GD.debugger_file == 0) {
                tstring path;
                FILE* f = load_module_file(s_module_scripts, path);
                if (f == 0) {
                    GD.debugger_file = 0;
                    report_missing(path);
                    path.~tstring();
                    return;
                }
                long long n = file_length(f);
                tstring text;
                text.read_from(f, (size_t)n);
                fclose(f);
                // strip the comment lines ("#" .. newline)
                int end = (int)n - 1;
                while (true) {
                    char hash = '#';
                    int p = text.rfind(hash, 0, end);
                    if (p < 0)
                        break;
                    char nl = 10;
                    int q = text.find(nl, p);
                    if (q < 0)
                        break;
                    text.erase_range(p, q);
                }
                GD.script_list.operator=(text);
                SetDlgItemTextA(dlg, IDC_DBG_VIEW, text.buf ? text.buf : (const char*)&text);
                // show it in the view, then highlight the "[ ... ]" body line by line
                struct CHARRANGE { LONG cpMin; LONG cpMax; } sel = { 0, 0xfa000 };
                tstring sfmt(s_fmt);
                SendMessageA(GetDlgItemA(dlg, IDC_DBG_VIEW), EM_SETSEL, 1, 0);

                {
                    CHARFORMAT2A cf;
                    cf.cbSize = 0x3c;
                    cf.dwMask = 0xa0000000;
                    cf.dwEffects = 0;
                    _sprintf(cf.szFaceName, s_courier);
                    cf.yHeight = 0xdc;
                    SendMessageA(GetDlgItemA(dlg, IDC_DBG_VIEW), EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
                }
                LRESULT start = SendMessageA(GetDlgItemA(dlg, IDC_DBG_VIEW), EM_SETSEL, 1, 0);

                if (start >= 0) {
                    do {
                        sel.cpMin = (LONG)start + 1;
                        LRESULT stop = SendMessageA(GetDlgItemA(dlg, IDC_DBG_VIEW), EM_SETSEL, 1, 0);

                        sel.cpMax = (LONG)stop + 1;
                        if (stop < 0)
                            break;
                        {
                            CHARFORMAT2A cf;
                            cf.cbSize = 0x3c;
                            cf.dwMask = 0x40000000;
                            cf.dwEffects = 0;
                            cf.yHeight = 0xc800;

                            SendMessageA(GetDlgItemA(dlg, IDC_DBG_VIEW), EM_SETCHARFORMAT, SCF_SELECTION,
                                         (LPARAM)&cf);
                        }
                        start = SendMessageA(GetDlgItemA(dlg, IDC_DBG_VIEW), EM_SETSEL, 1, 0);

                    } while (start >= 0);
                }
                sfmt.~tstring();
                text.~tstring();
                path.~tstring();
            }
            // refresh the script view from the selected script name
            {
                char buf[1024];
                WPARAM sel = SendMessageA(GetDlgItemA(dlg, IDC_DBG_SCRIPT), CB_GETCURSEL, 0, 0);
                SendMessageA(GetDlgItemA(dlg, IDC_DBG_SCRIPT), CB_GETLBTEXT, sel, (LPARAM)buf);
                GD.script_index = 0;
                {
                    tstring text(buf);
                    tstring sfmt(s_fmt);
                    text.append(sfmt);
                    sfmt.~tstring();
                    text.~tstring();
                }
            }
        }
        return;
    }
}
