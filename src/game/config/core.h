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

struct Core : CD3DApplication {
    // the framework's protected members that 0x4b99c0 calls from outside the class
    using CD3DApplication::ChooseInitialD3DSettings;    // 0x402100
    using CD3DApplication::ToggleFullscreen;            // 0x402be0 (the slice called it HandleSizeChange)
    using CD3DApplication::DisplayErrorMsg;             // 0x4014f0
    using CD3DApplication::m_bWindowed;                 // +0xa4 = 0x8ee3cec: the 'settings probed' flag of 0x4b99c0 and the
                                                        // crop flag of 0x4bace0 / 0x4bb9c0 (the slices' g_settings_probed / g_screenshot_cropped)

    char pad348[0x590 - 0x348];
    D3DDISPLAYMODE mode;                                // +0x590 adapter display mode filled by init_d3d 0x4bad70

    void update_view_rect();                            // 0x4bace0 (config.cpp)
    int  init_d3d();                                    // 0x4bad70
    bool choose_display_mode(bool kind1, bool kind2);   // 0x4badf0
};
extern Core g_app_obj;                                  // 0x8ee3c48

#endif
