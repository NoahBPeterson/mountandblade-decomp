// application.h: layout-only views of the application object (global g_app_obj 0x8ee3c48, vtable 0x5948e0), the
// DirectX framework's CD3DApplication (../dxframework/d3dapp.h, ctor 0x401910) plus the members the game added in its
// own constructor (0x4d3ec0). src/game/config/core.h models the same object on top of the real CD3DApplication
// (`Core`); these two structs are the flat views the window procedure / main loop TUs were written against
// (inventory_panel/game_app.cpp: 0x50ec40 Update, 0x50f130 MsgProc; options_panel/world_app.cpp: 0x4d3ec0 ctor, 0x4d4140
// the dialog procedure). One definition for both: the two TUs' views merged, offsets proven by:
//   * vtable 0x5948e0 holds 0x50ec40 (slot 7, Update) and 0x50f130 (slot 15, MsgProc), and the ctor 0x4d3ec0 stores
//     that vtable and calls the base constructor 0x401910 -> one class, GameApp : Application;
//   * Application is 0x350 bytes (the ctor 0x4d3ec0 starts its own members at +0x350: f350, hbmp, ...);
//   * 0x50f130 reads m_hInstance +0x358 and the editor / script dialog handles +0x374 / +0x378 that the ctor clears.
#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H
#include <windows.h>
#include "tstring.h"

struct IDirect3DDevice9;

// CD3DApplication (verified layout: slice s00401010) -- only the members these TUs read; the base vtable is never
// dispatched through here (MsgProc is called non-virtually at 0x403120). Size 0x350 (constructor 0x401910).
struct Application {
    char pad0[4];
    char f4;                            // 0x4 (m_bQuitRequested: a flag set when 0x5e2a08 is set)
    char pad5[0xa4 - 5];
    char m_bWindowed;                   // 0xa4
    bool m_bActive;                     // 0xa5
    char padA6[0xe8 - 0xa6];
    HWND m_hWnd;                        // 0xe8
    char padEC[0xc];                    // 0xec
    IDirect3DDevice9* m_pd3dDevice;     // 0xf8
    char padFC[0x278 - 0xfc];
    float m_fElapsedTime;               // 0x278
    char pad27C[0x344 - 0x27c];
    char m_bShowCursorWhenFullscreen;   // 0x344
    char pad345[0x34c - 0x345];
    int  m_34c;                         // 0x34c (last char / key; -1 when consumed)

    Application();                                                           // 0x401910
    int MsgProc(HWND hWnd, unsigned msg, unsigned wparam, unsigned lparam);  // 0x403120 (base)
};

// +0x350..+0x590: the game application's own fields, with the module-string array at +0x390
struct GameApp : Application {
    char   f350;                                    // +0x350
    char   pad351[0x354 - 0x351];
    HBITMAP hbmp;                                   // +0x354 (main.bmp / the module's bitmap, 0x4d4140)
    HINSTANCE m_hInstance;                          // +0x358 (0x50f130: DialogBoxParamA)
    char   pad35c[0x364 - 0x35c];
    HWND   hwnd;                                    // +0x364 (the dialog 0x4d4140 runs in)
    char   pad368[0x374 - 0x368];
    HWND   m_hMapEditorDlg;                         // +0x374 (0x50f130: map editor dialog; cleared by the ctor)
    HWND   m_hScriptDlg;                            // +0x378 (script dialog)
    int    f37c;                                    // +0x37c
    char   pad380[0x38c - 0x380];
    int    f38c;                                    // +0x38c
    char   arr[0x200];                              // +0x390: 0x20 tstrings

    GameApp();                                                  // 0x4d3ec0
    int  msg_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);      // 0x4d4140
    int MsgProc(HWND hWnd, unsigned msg, unsigned wparam, unsigned lparam);   // 0x50f130 (vtable slot 15)
    int Update();                                                             // 0x50ec40 (vtable slot 7)
    tstring* mod(int i) { return ((tstring*)arr) + i; }
};
#endif
