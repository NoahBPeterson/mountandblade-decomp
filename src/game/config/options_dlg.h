// options_dlg.h: the launcher's options dialog (page switching, paint, the DXDiag video-memory probe), the update
// checker's HTTP client and the key registry helpers (options_dlg.cpp, 0x4b8f40..0x4b9e70). ALL NAMES PROVISIONAL.
#ifndef GAME_CONFIG_OPTIONS_DLG_H
#define GAME_CONFIG_OPTIONS_DLG_H
#define WIN32_LEAN_AND_MEAN
#ifndef _MBCS
#define _MBCS
#endif
#include <winsock2.h>
#include <windows.h>
#include <oleauto.h>
#include "../tstring.h"

// ---- growable byte buffer (12 bytes: data, pos, cap) used by the HTTP reader (0x4b8f40) ----------------------
struct byte_buf {
    char* data;                                 // +0
    char* pos;                                  // +4
    int cap;                                    // +8
};

// ---- record filled by http_fetch (0x4b9110): url copy, header text, body buffer, body length ------------------
struct http_result {
    char* url;                                  // +0
    char* headers;                              // +4
    char* body;                                 // +8
    int   body_len;                             // +0xc
};

void __cdecl bytes_append(byte_buf* v, const char* src, unsigned n);    // 0x4b8f40 (grow by doubling, append)
void __cdecl sock_send_str(int sock, const char* s);                    // 0x4b8fc0
void __cdecl parse_url(const char* url, LPSTR scheme, int scheme_n, LPSTR host, int host_n,
                       LPSTR path, int path_n, long* port);             // 0x4b8ff0
int __cdecl http_fetch(const char* url, const char* extra, const char* post, int post_len, int out);  // 0x4b9110 (extra: header text sent after Host; post/post_len: body, POST when post != 0; `out` is an http_result*, passed as an int)
void __cdecl open_keys_key(PHKEY out);                                  // 0x4b96d0 HKCU\Software\MountAndBladeKeys
void __cdecl read_key_string(HKEY* key, LPCSTR name, tstring* out);     // 0x4b9720
long __cdecl get_latest_version();                                      // 0x4b9aa0 (the slice's __stdcall-less long)

// ---- the options dialog object (offset-only knowledge) ----------------------------------------------------------
struct OptionsDlg {
    char pad0[0x354];
    HBITMAP back_bitmap;                        // +0x354 background picture                 0x4b9af0
    char pad358[0x364 - 0x358];
    HWND paint_hwnd;                            // +0x364 window the picture is painted to   0x4b9af0
    char pad368[0x380 - 0x368];
    HWND hwnd;                                  // +0x380 the dialog                         0x4b9790
    void set_active_tab(int tab);               // 0x4b9790
    void paint();                               // 0x4b9af0
    void choose_dx_settings(struct Config* cfg, bool overwrite);    // 0x4b9b90 (cfg: ./config.h)
};


void run_settings_probe();                      // 0x4b99c0

#endif
