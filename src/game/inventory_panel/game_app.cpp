// Slice s0050ec40: the main loop's per-frame "game update" (0x50ec40), the application's window
// procedure override (0x50f130, CD3DApplication::MsgProc at 0x403120) and the loading screen's
// step function (0x50f390).  Class layouts follow the verified wave-1 slices of the DirectX framework
// (s00401010: CD3DApplication = `Application`) and the engine's tstring (s0042a1f0) / widget vtables.
// Names are provisional.
#include "../../include/image_addrs.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "../window_mgr.h"      // g_windows slots (slot_as<T>(i))
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <math.h>

// ---- tstring (0x10 bytes: read cursor, buffer, capacity, length) -------------------------------------
#include "../tstring.h"
#include "../shared_vars.h"
#include "../key_names.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
float g_8ee42f0;                                // 0x8ee42f0
char g_saved;   // 0x5e2a08
int g_counter;   // 0x5e28cc
char g_key_states[256];   // 0x5e27c8
char g_5e2630;   // 0x5e2630


// the engine's "null or pointer" read used all over the code base (buf == 0 -> the field itself)
static __forceinline const char* c_str(const tstring* s)
{
    char* p = s->buf;
    if (!p) p = (char*)&s->buf;
    return p;
}

// ---- GameApp : CD3DApplication, the application object (../config/core.h) ---------------------------------
#include "../config/core.h"

// ---- screens: g_windows[30] (0xa73980) is the global screen table; the id stack at 0xa739f8 keeps
//      the current screen id (a ring of 4-int blocks). ------------------------------------------------
// ---- the engine widget interface: only the slots this slice dispatches ---------------------------------
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)

void __cdecl frame_hook(void);                               // 0x4ec460 (game/char_sheet/screen.cpp: the end-of-frame callback, a free function)

#include "../window_mgr.h"               // IdStack g_id_stack (0xa739f8), current_screen_id

// ---- game (0x5e2930).  Only the members touched here. ------------------------------------------------
#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)

// ---- globals ------------------------------------------------------------------------------------------


// the Direct3D9 device (0xa17f28); __stdcall virtuals -> the COM-style call pattern
struct IDev9 {
    virtual long __stdcall s0(); virtual long __stdcall s1(); virtual long __stdcall s2();
    virtual long __stdcall s3(); virtual long __stdcall s4(); virtual long __stdcall s5();
    virtual long __stdcall s6(); virtual long __stdcall s7(); virtual long __stdcall s8();
    virtual long __stdcall s9(); virtual long __stdcall s10();
    virtual void __stdcall SetCursorPosition(int x, int y, unsigned flags);   // slot 11 (+0x2c)
    virtual int  __stdcall ShowCursor(int show);                              // slot 12 (+0x30)
};

// the current editor entity (a pointer at 0xa739b8)
#include "../editor_entity.h"

// gui manager object (0x5d7080) and a few of its member functions
#include "../strat/gui_manager.h"

#include "../../rgl/resource.h"                  // resmgr g_resmgr (0x5d6fa0)

#include "../slot.h"                    // Slot, g_keyboard (0xa170c8)

#include "../music_player.h"   // g_music (0xa73a10)
#include "../../include/fmod_api.h"

struct GuiScreen {
    void process(int param);   // 0x4ebfd0 (game/char_sheet/screen.h): the gui object at 0x5d7080 (the dt rides in as its bits)
};
struct GuiCtx;
void __fastcall pick_entity(GuiCtx* e);                // 0x4ed8d0 (game/char_sheet/pick_entity.cpp): EditorEntity::pick_entity
struct MapEditorDialog {
    void update_object_type();   // 0x4c7bf0 / 0x4c7e20 (rgl/mesh2/gui_edit.h): the dialog of the application object
    void update_objects();   // 0x4c7e20
};

#include "../../rgl/stream.h"                   // g_log (log_stream)

#include "../window_mgr.h"                         // g_windows (0xa73980)

#include "../ui_globals.h"      // UI / core globals

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
#include "../../include/clock_table.h"
__int64 g_ticks[5];                                 // 0x5e2948 (clock table, 1e-5 s ticks)
__int64 g_tick_dt[5];                               // 0x5e2970 (the last step of each clock, in ticks)


// ---- free functions --------------------------------------------------------------------------------
void bump_counter(void);                                     // 0x42cb70 (?bump_counter@@YAXXZ)
long build_shader(void);                                    // 0x4c17c0 (game/triggers/trigger_vec.cpp, returns an HRESULT)
void mb_error(const char* fmt, ...);                          // 0x42cc80
void load_skyboxes(void);                                        // 0x4c3350
void load_data_files(void);                                        // 0x4d0c20 (options_panel/module_load.cpp)
void load_module_files(void);                                        // 0x50d5e0 (inventory_panel/module_files.cpp)
LRESULT __stdcall dialog_thunk(HWND, UINT, WPARAM, LPARAM);   // 0x50d250 (the dialog proc: inventory_panel/widget_text.cpp)
void script_dialog_proc();                                   // 0x4c3340
void global_4bf870(void);                                    // 0x4c3330 (rgl/mesh2/rec_tables.cpp: the thunk to the script debugger proc)

// ---- the loading screen (0x50f440 lives in slice s0050f440; see there) ------------------------------
#include "../screen_records.h"       // C4c3290: update 0x50f390 here (slot 3), advance 0x50f440 in loading_screen.cpp
#include "../../include/renderer.h"
#include "../../include/clock_table.h"

// ---- the per-frame update ---------------------------------------------------------------------------
// @ 0x50ec40
HRESULT GameApp::FrameMove()
{
    float dt;
    __int64 t;

    g_counter++;
    bump_counter();

    dt = m_fElapsedTime;
    if (dt > 0.1f) dt = 0.1f;
    g_8ee42f0 = dt;
    t = (__int64)(dt * 100000.0f);
    g_tick_dt[0] = t;   // (frame time step, scaled)
    g_ticks[0] += t;
    g_step_tbl[0] = dt;
    g_gui.f1718 = last_key;
    last_key = -1;

    g_gui.process(*(int*)&dt);
    ((Slot*)&g_keyboard)->update(dt);   // 0x487930
    g_music.tick();
    FSOUND_Update();

    if (g_cfg.enable_edit_mode) {
        if (g_keyboard.down[0x1f] && !g_keyboard.prev[0x21] && (g_keyboard.down[0x9d] || g_keyboard.down[0x1d])) {
            if (build_shader() < 0) {
                mb_error("Unable to compile the effect file.");
            } else {
                g_gui.add(tstring("Effect file recompiled."), -1);
            }
            g_resmgr.resolve_effect_sets(g_effect);
        }
    }
    if (g_cfg.enable_edit_mode) {
        if (g_keyboard.down[0x32] && !g_keyboard.prev[0x32]) {
            if ((g_keyboard.down[0x9d] || g_keyboard.down[0x1d]) && (g_keyboard.down[0x36] || g_keyboard.down[42])) {
                load_skyboxes();
                g_gui.add(tstring("Animation data restored."), -1);
            } else if (g_keyboard.down[0x9d] || g_keyboard.down[0x1d]) {
                load_data_files();
                load_module_files();
                g_gui.add(tstring("Module data restored."), -1);
            }
        }
    }

    frame_hook();
    slot_as<head_rec>(current_screen_id())->frame_update(dt);

    if (g_game) {
        // (0x50f057, the "hide" path, is reached whenever f1a090 is 0, on screen 0xe or not)
        if (g_game->f1a090) {
            if (current_screen_id() == 0xe) {
                if (!g_game->f1a160) {
                    if (!m_hMapEditorDlg) {
                        m_hMapEditorDlg = CreateDialogParamA(GetModuleHandleA(0), (LPCSTR)0xc0, m_hWnd,
                                                             (DLGPROC)dialog_thunk, 0);
                    } else {
                        ((MapEditorDialog*)this)->update_object_type();
                    }
                    ((MapEditorDialog*)this)->update_objects();
                    ShowWindow(m_hMapEditorDlg, 5);
                    g_game->f1a160 = 1;
                }
                if (g_game->f1a1c8) {
                    HWND item = GetDlgItem(m_hMapEditorDlg, 0x438);
                    int n = (int)SendMessageA(item, 0x18b, 0, 0);
                    int i;
                    for (i = 0; i < n; i++)
                        SendMessageA(item, 0x185, 0, i);
                    if (g_game->f1a1b4 >= 0) {
                        for (i = 0; i < n; i++) {
                            int v = (int)SendMessageA(item, 0x199, i, 0);
                            int* p = g_game->f1a1bc;
                            unsigned j = 0;
                            while (p && j < (unsigned)(g_game->f1a1c0 - p)) {
                                if (v == p[j]) {
                                    SendMessageA(item, 0x185, 1, i);
                                    break;
                                }
                                j++;
                            }
                        }
                    }
                    pick_entity((GuiCtx*)slot_as<EditorEntity>(14));
                    slot_as<EditorEntity>(14)->f2f9 = 1;
                    SetFocus(m_hWnd);
                    g_game->f1a1c8 = 0;
                }
            }
        } else {
            if (g_game->f1a160) {
                if (m_hMapEditorDlg)
                    ShowWindow(m_hMapEditorDlg, 0);
                g_game->f1a160 = 0;
            }
        }
        if (g_game->f1a060) {
            if (!g_game->f1a064) {
                if (!script_dlg) {
                    script_dlg = CreateDialogParamA(GetModuleHandleA(0), (LPCSTR)0xdf, m_hWnd,
                                                      (DLGPROC)global_4bf870, 0);
                }
                ShowWindow(script_dlg, 5);
                g_game->f1a064 = 1;
            }
        } else if (g_game->f1a064) {
            if (script_dlg) {
                EndDialog(script_dlg, 2);
                script_dlg = 0;
            }
            g_game->f1a064 = 0;
        }
    }

    m_bShowCursorWhenFullscreen = g_gui.show_cursor;
    if (g_saved) m_bQuitRequested = true;
    return 0;
}

// ---- the window procedure ---------------------------------------------------------------------------
// @ 0x50f130
LRESULT GameApp::MsgProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    POINT pt;

    // the case bodies are laid out by the compiler in this order (see the disassembly at 0x50f163..)
    switch (msg) {
    case 0x101:                                 // WM_KEYUP
        g_key_states[wparam] = 0;
        break;
    case 0x100:                                 // WM_KEYDOWN
        g_key_states[wparam] = 1;
        break;
    case 0x20:                                  // WM_SETCURSOR
        if ((*(IDev9**)&g_render.dev) && m_bWindowed) {
            SetCursor(0);
            (*(IDev9**)&g_render.dev)->ShowCursor(1);
            return 1;
        }
        break;
    case 0x111:                                 // WM_COMMAND
        switch ((unsigned short)wparam) {
        case 0x9c5b:
            if (g_game) {
                g_game->save_language_files(0);
                MessageBoxA(0, "Language Template Files Generated Under new_language directory!",
                            "INFO", 0);
            } else {
                MessageBoxA(0, "Create a new game or restore a game before using this function!",
                            "INFO", 0);
            }
            break;
        case 0x9c5c:
            if (g_game) {
                g_game->save_language_files(1);
                MessageBoxA(0, "Language Template Files Generated Under new_language directory! "
                               "Non-translated texts are marked with a {***} sign. You can use this "
                               "as the search keyword.", "INFO", 0);
            } else {
                MessageBoxA(0, "Create a new game or restore a game before using this function!",
                            "INFO", 0);
            }
            break;
        case 0x9c5a:
            load_data_files();
            load_module_files();
            g_gui.add(tstring("Module data restored."), -1);
            break;
        case 0x9c56:
            if (!g_game) goto check_focus;
            g_game->f1a060 = 1;
            g_game->f1a068 = 0;
            ((tstring*)((char*)g_game + 0x1a06c))->clear();
            break;
        case 0x9c58:
            DialogBoxParamA(hinstance, (LPCSTR)0xe0, m_hWnd, (DLGPROC)script_dialog_proc, 0);
            // falls through
        }
        // falls through
    case 0x200:                                 // WM_MOUSEMOVE
    check_focus:
        if (g_game && (g_game->f1a090 || g_game->f1a060) && !g_5e2630)
            SetFocus(m_hWnd);
        if (m_bActive && m_pd3dDevice)
            GetCursorPos(&pt);
        break;
    case 0x109:
        if (wparam == 0xffff) return 1;
        last_key = wparam;
        break;
    case 0x102:                                 // WM_CHAR
        last_key = wparam;
        break;
    }
    return CD3DApplication::MsgProc(hWnd, msg, wparam, lparam);
}

// ---- the loading screen's step ----------------------------------------------------------------------
// @ 0x50f390
void C4c3290::frame_update(float)
{
    char buf[0x100];
    tstring text;
    char tmp[8];
    bool finished = advance(&step, &total, &text);
    const char* fmt = "%d";
    _snprintf(buf, 0x100, fmt, step);
    ((MissionUi*)hint)->set_text((float*)tmp, &(text));
    ((MissionUi*)bar)->set168((float)step * 0.14285715f);
    if (finished) {
        g_windows.set_state(4);
        ((stream_base&)g_log).w_cstr("Finished All...\n");
    }
}
