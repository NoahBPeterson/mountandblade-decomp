// launcher_options_dialog.h: the launcher's options dialog (page switching, paint, the DXDiag video-memory probe), the update
// checker's HTTP client and the key registry helpers (launcher_options_dialog.cpp, 0x4b8f40..0x4b9e70). ALL NAMES PROVISIONAL.
#ifndef GAME_CONFIG_OPTIONS_DLG_H
#define GAME_CONFIG_OPTIONS_DLG_H
#define WIN32_LEAN_AND_MEAN
#ifndef _MBCS
#define _MBCS
#endif
#include <winsock2.h>
#include <windows.h>
#include <oleauto.h>
#include "engine/core/tstring.h"

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
int __cdecl http_fetch(const char* url, const char* extra, const char* post, int post_len, http_result* out);  // 0x4b9110 (extra: header text sent after Host; post/post_len: body, POST when post != 0; `out` receives the reply)
void __cdecl open_keys_key(PHKEY out);                                  // 0x4b96d0 HKCU\Software\MountAndBladeKeys
void __cdecl read_key_string(HKEY* key, LPCSTR name, tstring* out);     // 0x4b9720
long __cdecl get_latest_version();                                      // 0x4b9aa0 (the slice's __stdcall-less long)

// (the options dialog's members are GameApp's: ./core.h set_active_tab .. choose_dx_settings)


void run_settings_probe();                      // 0x4b99c0

#endif
