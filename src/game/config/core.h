// core.h: the application object (global g_app_obj at 0x8ee3c48): the SDK framework's CD3DApplication
// (../../dxframework/d3dapp.h) plus the members Mount & Blade added in its own objects, one class, GameApp, with its
// vtable 0x5948e0 the compiler's. The constructor is 0x4d3ec0 (options_panel/world_app.cpp, where the object is
// defined), the destructor 0x4bed90 (triggers/trigger_vec.cpp), the member functions 0x4bace0 / 0x4bad70 / 0x4badf0
// (config.cpp) and the options dialog's start-up probe (options_dlg.cpp, 0x4b99c0) are methods of it by their field
// accesses (the slices called it `Core` / `Application`).
#ifndef GAME_CONFIG_CORE_H
#define GAME_CONFIG_CORE_H
#ifndef _MBCS
#define _MBCS
#endif
#include <stddef.h>
#include "../../dxframework/d3dapp.h"
#include "../../dxframework/d3dfont.h"
#include "../tstring.h"

struct GameApp : CD3DApplication {
    // the framework's protected members that code outside the class uses
    using CD3DApplication::ChooseInitialD3DSettings;    // 0x402100
    using CD3DApplication::ToggleFullscreen;            // 0x402be0 (the slice called it HandleSizeChange)
    using CD3DApplication::DisplayErrorMsg;             // 0x4014f0
    using CD3DApplication::m_bStartFullscreen;          // +0x346 WinMain sets it from the config (cls_map.cpp)
    using CD3DApplication::m_fMinFrameTime;             // +0x008 WinMain sets it to 1 / max_framerate
    using CD3DApplication::m_hWnd;                      // +0xe8 the main window
    using CD3DApplication::m_pD3D;                      // +0xf4 = 0x8ee3d3c: the IDirect3D9 the options page lists the display modes from
    using CD3DApplication::m_bWindowed;                 // +0xa4 = 0x8ee3cec: the 'settings probed' flag of 0x4b99c0 and the
                                                        // crop flag of 0x4bace0 / 0x4bb9c0 (the slices' g_settings_probed / g_screenshot_cropped)

    // +0x348..+0x5a0: what Mount & Blade added to the framework's object (offsets proven by the constructor 0x4d3ec0
    // and the functions named below)
    CD3DFont* font;                                     // +0x348 deleted by the destructor 0x4bed90, device-object handlers 0x4c1940, 0x4c1a60, 0x4c1ad0
    int  last_key;                                      // +0x34c the last char / key (-1 when consumed; 0x50ec40 hands it to g_5d8798)
    char f350;                                          // +0x350 the launcher's first-paint flag (0x4d4140)
    char pad351[0x354 - 0x351];
    HBITMAP hbmp;                                       // +0x354 main.bmp / the module's bitmap (0x4d4140); WinMain clears it before the window exists
    HINSTANCE hinstance;                                // +0x358 the instance handle WinMain stores (the slices' g_hinstance)
    char pad35c[0x364 - 0x35c];
    HWND hwnd;                                      // +0x364 cleared by the constructor
    char pad368[0x374 - 0x368];
    HWND m_hMapEditorDlg;                               // +0x374 the map editor dialog (0x50f130, 0x4bf640); cleared by the constructor
    HWND script_dlg;                                    // +0x378 the script debugger dialog 0x4bf870
    void* dlg_options;                                  // +0x37c the options dialog window (set by options_dlg_proc 0x4bf5f0; untyped so the call reloads it into eax)
    HWND options_hwnd;                                  // +0x380 the options dialog 0x4bf170, 0x4bee10
    int  options_ctrl;                                  // +0x384
    char pad388[0x38c - 0x388];
    int  module_count;                                  // +0x38c the module folders found (0x4d3ec0)
    tstring module_names[0x20];                         // +0x390 the module folder names
    D3DDISPLAYMODE mode;                                // +0x590 adapter display mode filled by init_d3d 0x4bad70; mode.Format at +0x59c (0x4bf170)

    GameApp();                                          // 0x4d3ec0
    virtual ~GameApp();                                 // 0x4bed90 (the scalar deleting destructor 0x4bedf0 is the compiler's)

    // the overrides, in vtable 0x5948e0 order
    virtual bool FindBestFullscreenMode(bool bRequireHAL, bool bRequireREF);   // 0x4badf0 (slot 0, config.cpp)
    virtual HRESULT OneTimeSceneInit();                 // 0x4f63c0 (slot 4, char_sheet/editor_ui.cpp: the engine start-up)
    virtual HRESULT InitDeviceObjects();                // 0x4c3510 (slot 5, rgl/mesh2/rec_tables.cpp)
    virtual HRESULT RestoreDeviceObjects();             // 0x4c1940 (slot 6)
    virtual HRESULT FrameMove();                        // 0x50ec40 (slot 7, inventory_panel/game_app.cpp: the per-frame update)
    virtual HRESULT Render();                           // 0x4eb8e0 (slot 8, char_sheet/facegen_misc.cpp: the loading / save-load screen)
    virtual HRESULT InvalidateDeviceObjects();          // 0x4c1a60 (slot 9)
    virtual HRESULT DeleteDeviceObjects();              // 0x4c1ad0 (slot 10)
    virtual HRESULT FinalCleanup();                     // 0x4da5b0 (slot 11, cls_map/cls_map.cpp: the end-of-game shutdown)
    virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x50f130 (slot 15)

    int  msg_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);   // 0x4d4140 the launcher dialog's procedure (cls_map.cpp's thunk 0x4d9440 calls it with this = g_app_obj)
    LRESULT terrain_dialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x4fe860 the map editor's terrain dialog (agent5/editor_dialog.cpp; the dialog thunk 0x50d250 calls it on g_app_obj)
    void fill_video_tab();                              // 0x4cf7b0 (options_panel/config_dialog.cpp: the launcher configuration dialog)
    LRESULT config_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x4d0240 its procedure (world_app.cpp's thunk 0x4d4130 calls it with this = g_app_obj)
    void read_options();                                // 0x4bf170 reads the options dialog's controls into the g_opt_* globals
    void show_options();                                // 0x4bee10 the inverse: writes them into the dialog
    INT_PTR script_debugger_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);   // 0x4bf870 script debugger dialog (0x4c3330 jumps to it with ecx = g_app_obj)
    tstring* mod_name(int i) { return module_names + i; }

    void update_view_rect();                            // 0x4bace0 (config.cpp)
    int  init_d3d();                                    // 0x4bad70
};
typedef char gameapp_layout_check[(sizeof(GameApp) == 0x5a0 && offsetof(GameApp, mode) == 0x590 && offsetof(GameApp, module_names) == 0x390 && offsetof(GameApp, dlg_options) == 0x37c) ? 1 : -1];
extern GameApp g_app_obj;                               // 0x8ee3c48 (defined in options_panel/world_app.cpp: the compiler constructs it and registers the destructor)

#endif
