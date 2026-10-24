// core.h: the application object (global g_app_obj at 0x8ee3c48), the SDK framework's CD3DApplication
// (../../dxframework/d3dapp.h) plus the members Mount & Blade added in its own objects. The member functions
// 0x4bace0 / 0x4bad70 / 0x4badf0 (config.cpp) and the options dialog's start-up probe (options_dlg.cpp, 0x4b99c0)
// are methods of this class by their field accesses (the slices called it `Core` / `Application`).
// Field names are the framework's; the slices' provisional names are noted per member.
#ifndef GAME_CONFIG_CORE_H
#define GAME_CONFIG_CORE_H
#ifndef _MBCS
#define _MBCS
#endif
#include "../../dxframework/d3dapp.h"
#include "../../dxframework/d3dfont.h"
#include "../tstring.h"

struct Core : CD3DApplication {
    // the framework's protected members that 0x4b99c0 calls from outside the class
    using CD3DApplication::ChooseInitialD3DSettings;    // 0x402100
    using CD3DApplication::ToggleFullscreen;            // 0x402be0 (the slice called it HandleSizeChange)
    using CD3DApplication::DisplayErrorMsg;             // 0x4014f0
    using CD3DApplication::m_bWindowed;                 // +0xa4 = 0x8ee3cec: the 'settings probed' flag of 0x4b99c0 and the
                                                        // crop flag of 0x4bace0 / 0x4bb9c0 (the slices' g_settings_probed / g_screenshot_cropped)

    // +0x348..+0x590 (was one pad; split by the functions of src/game/triggers, offsets unchanged):
    CD3DFont* font;                                     // +0x348 deleted by the destructor 0x4bed90, device-object handlers 0x4c1940, 0x4c1a60, 0x4c1ad0
    char pad34c[0x374 - 0x34c];
    HWND page_hwnd;                                     // +0x374 the create-game page dialog 0x4bf640 (CreateGameOptionsPage::hDlg)
    HWND script_dlg;                                    // +0x378 the script debugger dialog 0x4bf870
    char pad37c[0x380 - 0x37c];
    HWND options_hwnd;                                  // +0x380 the options dialog 0x4bf170, 0x4bee10
    char pad384[0x390 - 0x384];
    char module_names[0x200];                           // +0x390 0x20 tstrings (destructor 0x4bed90 destroys them in reverse)
    D3DDISPLAYMODE mode;                                // +0x590 adapter display mode filled by init_d3d 0x4bad70; mode.Format at +0x59c (0x4bf170)

    // device-object handlers (overrides of the framework's virtuals, src/game/triggers/trigger_vec.cpp)
    virtual HRESULT RestoreDeviceObjects();             // 0x4c1940 (vtable 0x5948e0 slot 6)
    virtual HRESULT InvalidateDeviceObjects();          // 0x4c1a60 (slot 9)
    virtual HRESULT DeleteDeviceObjects();              // 0x4c1ad0 (slot 10)
    void read_options();                                // 0x4bf170 reads the options dialog's controls into the g_opt_* globals
    void show_options();                                // 0x4bee10 the inverse: writes them into the dialog
    INT_PTR script_debugger_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);   // 0x4bf870 script debugger dialog (0x4c3330 jumps to it with ecx = g_app_obj)
    tstring* mod_name(int i) { return (tstring*)module_names + i; }

    void update_view_rect();                            // 0x4bace0 (config.cpp)
    int  init_d3d();                                    // 0x4bad70
    bool choose_display_mode(bool kind1, bool kind2);   // 0x4badf0
};
extern Core g_app_obj;                                  // 0x8ee3c48

#endif
