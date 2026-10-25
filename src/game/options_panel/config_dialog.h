// config_dialog.h: the launcher configuration dialog object (the application object g_app_obj 0x8ee3c48 seen as its dialog procedure's
// `this`; config_dialog.cpp defines the members, world_app.cpp's window procedure thunk calls proc).
#ifndef OPTIONS_PANEL_CONFIG_DIALOG_H
#define OPTIONS_PANEL_CONFIG_DIALOG_H
#include <windows.h>

class Dialog {
public:
    char pad[0x380];
    HWND m_hwnd;      // 0x380
    int m_ctrl;       // 0x384
    void fill_video_tab();                          // 0x4cf7b0
    LRESULT proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x4d0240
};

#endif
