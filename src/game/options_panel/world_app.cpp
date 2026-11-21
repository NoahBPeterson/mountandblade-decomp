// world_app.cpp: the mission object's constructor / destructor, restore_defaults, a hash-table insert, the
// application constructor and main window procedure (0x4d3720..0x4d4b80), and the vector fill helpers that sit
// after them (T-regime region: helpers right after the first caller). Names provisional.
// The mission object is g_mission (0x5e292c; Mission, ../mission.h: its destructor is in hud.cpp) and `GameApp` is the
// application object (../config/core.h).
#define GLYPH_HASH_INSERT_INSTANTIATED_HERE   // glyph_hash.h: the glyph table's insert 0x4d3c00 is this file's
#include "../../include/image_addrs.h"
#include "../../include/layout32.h"
#include "../ini_file.h"                      // ini_file, g_module_info (0xa18238)
#include "../config/config_block.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include <windows.h>
#include <shellapi.h>
#include <new>
#include <vector>
#include <memory>
#include <algorithm>
#include "../tstring.h"
#include "../font.h"                    // GlyphHash, the glyph table (insert 0x4d3c00)
#include "../party/game.h"
#include "../../include/rec_dtors.h"
#include "../../include/skin.h"            // Skin (the destructor 0x4d3720 is defined here)
#include "../config/core.h"                 // GameApp : CD3DApplication, the application object (defined below, g_app_obj)

// std::vector<tstring> (_Tidy 0x42d390) is declared out of line in src/include/rec_dtors.h

// ---- callees -------------------------------------------------------------------------------------------
tstring g_module_path;                                          // 0x8ee3c28 (constructed by the compiler: tstring::tstring 0x42bbd0)
extern tstring g_module_name;                                      // 0x8ee3c38

#include "../../include/crt.h"   // canonical: operator delete (0x535712)
// The vector elements of mesh_2.cpp (RagdollBone 0x50, E68 0x68, FaceTexture 0xb0) and the std algorithms the original keeps out of line for them;
// the explicit specialization declarations (no bodies here) keep every one of them a call.
#include "./elem_calls.h"
#include "../glyph_table_calls.h"   // the helpers of the glyph table's insert 0x4d3c00
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/skinmagic.h"
namespace std {
template<> FaceTexture* _Copy_opt<FaceTexture*, FaceTexture*>(FaceTexture*, FaceTexture*, FaceTexture*, _Nonscalar_ptr_iterator_tag);   // 0x4cd620
template<> void _Destroy_range<FaceTexture, allocator<FaceTexture> >(FaceTexture*, FaceTexture*, allocator<FaceTexture>&, _Nonscalar_ptr_iterator_tag);   // 0x4ca7e0
template<> void _Destroy_range<E68, allocator<E68> >(E68*, E68*, allocator<E68>&, _Nonscalar_ptr_iterator_tag);   // 0x4cd570
}
#include "../arr_set.h"   // ArrSet (std::_Destroy_range<ArrSet> is terrain_b/poly_mesh.cpp's 0x4727a0)
#include "../str_table.h"                                      // StrTable (0x5e2a18; was StrTab here: lookup 0x43a1e0)

// ---- globals -------------------------------------------------------------------------------------------
#include "../../include/fx.h"   // canonical: g_ticks (0x5e2958)
extern int g_pending_action;                                // 0x5a9f18
#include "../mission.h"   // canonical: g_mission (0x5e292c)
#include "../../rgl/scene/scene_entity.h"   // rgl::entity: the mission's scene entities

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_pending_action = -1;                                  // 0x5a9f18 -1 = nothing pending

#include "../../include/render_flags.h"
#include "../window_mgr.h"                                     // g_windows (0xa73980)

// ---- the mission object (../mission.h) --------------------------------------------------------------------------------
#include "../missile.h"   // Missile, the record of Pool<Missile> (Mission::poolJ)
#include "../placed_obj.h"                      // PlacedObj: Mission::props

// @ 0x4d3880 (the members in declaration order: the two pools, the 64 entry points 0x4c2d10, the clock-2 timers, the vectors,
// the terrain description 0x4be6b0, the nav mesh 0x4d1360 and the tstring; then the body)
Mission::Mission()
{
    scene = 0;
    scene_counter = 0;
}

// @ 0x4d3720  (the skin record's destructor: the face key and beard mesh arrays, then the members in reverse order)
Skin::~Skin()
{
    if (face_keys)
        delete[] face_keys;
    delete[] beard_meshes;
}

// @ 0x4d3a50
void restore_defaults()
{
    g_game->editor.active = 0;
    g_game->editor.rt_flag = 0;
    g_game->editor.list_top = -1;
    g_game->editor.list_sel = -1;
    g_game->editor.f1a0c0 = -1;
    g_game->editor.f1a0c4 = -1;

    // the map editor's scene entities (map_editor.h: +0x65d0 the selection cursor, +0x65dc the selection marker)
    g_mission->cursor->visible = 0;
    g_mission->sel_marker->visible = 0;
    g_mission->edit_obj->visible = 0;
    if (g_mission->marker_entity) g_mission->marker_entity->visible = 0;
    if (g_mission->special) ((rgl::entity*)g_mission->special)->visible = 0;

    if (g_game->editor.dirty) {
        g_game->editor.dirty = 0;
        g_pending_action = -1;
        {
            tstring s1("ui_no_discard_changes");
            tstring s2("ui_yes_save");
            tstring s3("ui_question_save_changes");
            const tstring* r1 = g_strtable.lookup(s1);
            const tstring* r2 = g_strtable.lookup(s2);
            const tstring* r3 = g_strtable.lookup(s3);
            ResultWindow* ui = g_windows.box();
            ui->s68.clear();
            ui->name = *r3;
            ui->s78 = *r2;
            ui->s88 = *r1;
            ui->flag = &g_pending_action;
            ui->text = 0;
            ui->w04 = 1;
        }
        g_windows.open(0);
        g_game->editor.active = 0;
    }
}

// ---- the glyph table's insert --------------------------------------------------------------------------------
// @ 0x4d3c00 (stdext::hash_map<int, Glyph>'s _Hash::insert: pair<iterator, bool> through the hidden result pointer)
template GlyphHash::_Pairib GlyphHash::insert(const GlyphHash::value_type&);

struct TreeW {                                      // a std::vector<FaceTexture> (allocator word, first, last, end)
    char pad0[8];
    void* last;                                     // +0x08 last
    void erase(void* p1, void* p2);                 // 0x4d3e50
    // the element iterator (a class with constructors, so it is returned through the hidden result pointer)
    struct It { FaceTexture* p; It() {} It(FaceTexture* q) : p(q) {} };
    It insert(It a, It b);                          // 0x4d3e70 (vector::erase's shape: returns a)
};

// @ 0x4d3e50
void TreeW::erase(void* p1, void* p2)
{
    std::_Destroy_range((E68*)p1, (E68*)p2, *(std::allocator<E68>*)(void*)this);   // 0x4cd570
}

static inline FaceTexture* copy_into(FaceTexture* first, FaceTexture* last, FaceTexture* dest)
{
    std::_Nonscalar_ptr_iterator_tag tag;
    return std::_Copy_opt(first, last, dest, tag);
}

// @ 0x4d3e70
TreeW::It TreeW::insert(It a, It b)
{
    if (a.p != b.p) {
        FaceTexture* p = copy_into(b.p, (FaceTexture*)last, a.p);
        std::_Destroy_range(p, (FaceTexture*)last, *(std::allocator<FaceTexture>*)(void*)this);
        last = p;
    }
    return a;
}

// ---- the application object (0x4d3ec0, 0x4d4130, 0x4d4140) ----
// ---- callees -------------------------------------------------------------------------------------------
#if MB_LP64
void* __cdecl operator new(size_t size);            // 0x53545e (port: LP64 spelling; MB_LP64 is never set for cl)
#else
void* __cdecl operator new(unsigned size);          // 0x53545e
#endif
#include "../../dxframework/d3dfont.h"            // CD3DFont (0x67c bytes; ctor 0x404980)
void  scan_module_folders(tstring* array, int* count);          // 0x4bec60
void  read_key_string(HKEY* key, const char* name, tstring* out);   // 0x4b9720
extern "C" {
void __stdcall FSOUND_SetMaxHardwareChannels(int n);          // 0x534300
int  __stdcall FSOUND_Init(int mixrate, int maxsoftwarechannels, int flags);  // 0x5342fa
void __stdcall FSOUND_SetSFXMasterVolume(int volume);         // 0x5342f4
void __stdcall FSOUND_3D_SetRolloffFactor(float factor);      // 0x5342ee
}

// ---- globals -------------------------------------------------------------------------------------------
extern int g_5e28c8;                                // 0x5e28c8
extern int g_use_shaders;                                // 0x5e28fc
#include "../ui_globals.h"      // UI / core globals (g_5e28c8)
// g_cfg.disable_sound (config/config.h Config +0x40 of g_cfg 0x5a9f98; that header brings d3dapp.h, which would redefine
// CD3DApplication here)
#define CFG_DISABLE_SOUND (g_cfg.disable_sound)
#include "../game_globals.h"      // canonical g_key_states (listed directly: the audit reads only the source's own includes)
#define g_key_states_i (*(int(*)[0x40])g_key_states)     // g_key_states (0x5e27c8, game_globals.h) as 0x40 ints

#include "../../rgl/stream.h"                   // g_log (log_stream)
#include "../music_player.h"                    // g_music (0xa73a10): root is the module path copy at +0x247c

// ---- the application object: GameApp (../config/core.h) ------------------------------------------------
GameApp g_app_obj;                                  // 0x8ee3c48 (.bss; constructed by the compiler: GameApp::GameApp 0x4d3ec0, destructor registered with atexit)

// @ 0x4d3ec0
GameApp::GameApp()
{
    first_painted = 0;
    WL_MARKER(12)

    m_dw334 = "m";
    m_strWindowTitle = (TCHAR*)"Mount&Blade";
    m_d3dEnumeration.AppUsesDepthBuffer = true;
    m_dwCreationWidth = 0x190;
    m_dwCreationHeight = 0x12c;
    m_bShowCursorWhenFullscreen = true;

    font = new CD3DFont("Arial", 9, 1);
    last_key = -1;

    for (int i = 0; i < 0x40; i++)
        g_key_states_i[i] = 0;

    g_counter = 0;
    g_5e28c8 = 0;
    scan_module_folders(module_names, &module_count);
    if (module_count > 0) {
        g_module_name = module_names[0];

        HKEY  key;
        DWORD disp;
        RegCreateKeyExA((HKEY)0x80000001, "Software\\MountAndBladeKeys", 0, 0, 0, 0x20007, 0,
                        &key, &disp);
        if (disp == 1)
            ((stream_base&)g_log).w_cstr("Creating new Registry Key");
        if (key != 0) {
            tstring modname;
            read_key_string(&key, "last_module", &modname);
            if (key != 0)
                RegCloseKey(key);
            if (modname.len > 0)
                g_module_name = modname;
        }
    }

    g_module_path.assign("Modules\\");
    g_module_path.append(g_module_name);
    {
        tstring tmp("\\");
        g_module_path.append(tmp);
    }

    g_use_shaders = 0;
    g_saved = 0;
    g_game = 0;
    g_mission = new Mission;                        // 0x4d4083: operator new(0x6868), the constructor 0x4d3880

    if (CFG_DISABLE_SOUND == 0) {
        FSOUND_SetMaxHardwareChannels(0);
        FSOUND_Init(0xac44, 0x40, 0);
        FSOUND_SetSFXMasterVolume(0xff);
        FSOUND_3D_SetRolloffFactor(0.5f);
    }

    *(tstring*)&g_music.root = g_module_path;
    hwnd = 0;
    m_hMapEditorDlg = 0;
    script_dlg = 0;
    dlg_options = 0;
    WL_MARKER(13)
}

// ---- data this module owns (.bss, no initialiser) ----
int g_trial_mode;                                   // 0x5e29c4 (set when the game runs as the trial; 6 modules read it)

// @ 0x4d4130

int __stdcall wndproc_thunk(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    return (int)g_app_obj.config_proc(h, msg, wp, lp);
}

// ---- callees used by the main window procedure (0x4d4140) ----------------------------------------------
long __cdecl    get_latest_version();                          // 0x4b9aa0
void __fastcall unescape(tstring* s);                  // 0x4bac70
void __cdecl    open_keys_key(HKEY* out);                         // 0x4b96d0
void __cdecl    keys_write_value(HKEY* key, const char* name, const tstring* val);   // 0x4beb80

#define STRTAB (&g_strtable)
#define UI(s) STRTAB->lookup(tstring(s))

// @ 0x4d4140
int GameApp::msg_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    tstring path;

    HKEY key;
    switch (msg) {
    case 0xf: {                                             // WM_PAINT
        paint();
        if (g_trial_mode == 0 && first_painted == 0) {
            first_painted = 1;
            g_trial_mode = 1;
        }
        return 1;
    }
    case 0x110: {                                           // WM_INITDIALOG
        hwnd = h;
        SetSingleDialogSkin(h, "MainFrame");
        init_main_page();
        if (module_count == 0) {
            ShowWindow(GetDlgItem(hwnd, 0x410), 0);
            ShowWindow(GetDlgItem(hwnd, 0x425), 0);
            MessageBoxA(0, UI("ui_there_are_no_modules_installed")->c_str(), "ERROR", 0x10);
            return 1;
        }
        HWND list = GetDlgItem(hwnd, 0x420);
        if (list == 0)
            break;
        SendMessageA(list, 0x14b, 0, 0);
        for (int i = 0; i < module_count; i++) {
            int r = SendMessageA(list, 0x14a, 0, (LPARAM)mod_name(i)->c_str());
            SendMessageA(list, 0x151, r, i);
        }
        int r = SendMessageA(list, 0x158, (WPARAM)-1, (LPARAM)g_module_name.c_str());
        SendMessageA(list, 0x14e, r, 0);
        PostMessageA(hwnd, 0x111, 0x10000, (LPARAM)list);
        ShowWindow(GetDlgItem(hwnd, 0x420), 5);
        ShowWindow(GetDlgItem(hwnd, 0x425), 5);
        break;
    }
    case 0x111: {                                           // WM_COMMAND
        switch (LOWORD(wp)) {
        case 2:
            EndDialog(h, 2);
            return 1;
        case 0x405:
            DialogBoxParamA(0, (LPCSTR)0xc1, hwnd, (DLGPROC)wndproc_thunk, 0);
            return 1;
        case 0x406:
            ShellExecuteA(0, "open", "http://www.gamersgate.com", 0, 0, 5);
            return 1;
        case 0x410: {
            g_5e2630 = 0;
            int sel = 0;
            if (module_count > 1) {
                // the control is looked up again for each message (0x4d4140 calls GetDlgItem twice)
                int idx = SendMessageA(GetDlgItem(h, 0x420), 0x147, 0, 0);
                sel = SendMessageA(GetDlgItem(h, 0x420), 0x150, idx, 0);
            }
            g_module_name = *mod_name(sel);
            g_module_path.assign("Modules\\");
            g_module_path.append(g_module_name);
            {
                tstring slash("\\");
                g_module_path.append(slash);
            }
            open_keys_key(&key);
            keys_write_value(&key, "last_module", &g_module_name);
            if (key != 0)
                RegCloseKey(key);
            EndDialog(h, 0x410);
            return 1;
        }
        case 0x41f: {                                       // check for a newer version
            int v = get_latest_version();
            if (v > 0x3f3) {
                tstring text(*UI("ui_notification_new_version"));
                unescape(&text);
                char buf[0x254];
                sprintf(buf, text.c_str(), 1, 0, 1, 1,
                            v / 1000, (v / 100) % 10, (v / 10) % 10, v % 10);
                MessageBoxA(hwnd, buf, UI("ui_new_version_found")->c_str(), 0);
            }
            else if (v == -1) {
                tstring a("ui_new_version");
                tstring b("ui_notification_unable_to_retreive_new_version");
                const char* cap = STRTAB->lookup(a)->c_str();
                const char* txt = STRTAB->lookup(b)->c_str();
                MessageBoxA(hwnd, txt, cap, 0);
            }
            else {
                tstring a("ui_mount_and_blade_is_up_to_date");
                tstring b("ui_notification_latest_version");
                const char* cap = STRTAB->lookup(a)->c_str();
                const char* txt = STRTAB->lookup(b)->c_str();
                MessageBoxA(hwnd, txt, cap, 0);
            }
            break;
        }
        case 0x469:
            EndDialog(h, 2);
            return 1;
        case 0x46b:
            ShellExecuteA(0, "open", "http://www.paradoxplaza.com/Mount&Blade/", 0, 0, 5);
            return 1;
        case 0x420: {                                       // module selection changed
            if (HIWORD(wp) != 9)
                break;
            int idx = SendMessageA(GetDlgItem(h, 0x420), 0x147, 0, 0);    // GetDlgItem per message, as above
            int sel = SendMessageA(GetDlgItem(h, 0x420), 0x150, idx, 0);
            if (sel >= module_count)
                sel = module_count - 1;
            g_module_name = *mod_name(sel);

            path.assign("Modules\\");
            path.append(g_module_name);
            path.append(tstring("\\main.bmp"));

            DeleteObject(hbmp);
            hbmp = (HBITMAP)LoadImageA(0, path.c_str(), 0, 0, 0, 0x10);

            tstring ini("Modules\\");
            ini.append(g_module_name);
            ini.append(tstring("\\module.ini"));

            FILE* f = fopen(ini.c_str(), "r");
            if (f != 0) {
                g_module_info.load(f);
                fclose(f);
            }

            tstring v1;
            tstring v2;
            if (g_module_settings.works_with_version_min > 0x3f3) {
                MessageBoxA(hwnd, UI("ui_module_newer_than_application")->c_str(), 0, 0x30);
            }
            else if (g_module_settings.works_with_version_max < 0x3f3) {
                MessageBoxA(hwnd, UI("ui_module_older_than_application")->c_str(), 0, 0x40);
            }

            if (hbmp == 0)
                hbmp = (HBITMAP)LoadImageA(0, "main.bmp", 0, 0, 0, 0x10);
            RedrawWindow(hwnd, 0, 0, 0x105);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        return 0;
    }
    return 1;
}

// ---- std::_Uninit_fill_n helpers (0x4d49c0..0x4d4b80) ----
// The allocators' construct are module_load.cpp's instances (std::allocator<E68> / <FaceTexture>::construct 0x4d1160 /
// 0x4d1180: the element copy constructors 0x4cd100 / 0x4cd170 behind placement new's null test); E68's unwind path destroys
// through ~E68 0x4c8ba0 (rgl/mesh2/gui_edit.cpp), FaceTexture's through std::allocator<FaceTexture>::destroy 0x4c8ab0, out of
// line (rgl/mesh2/gui_edit.cpp defines it).
namespace std { template<> void allocator<FaceTexture>::destroy(FaceTexture*); }

// @ 0x4d49c0 (std::vector<RagdollBone>'s fill: the header body)
template void std::_Uninit_fill_n(RagdollBone*, unsigned, const RagdollBone&, std::allocator<RagdollBone>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x4d4a50 (std::vector<E68>'s fill)
template void std::_Uninit_fill_n(E68*, unsigned, const E68&, std::allocator<E68>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x4d4ae0 (std::vector<FaceTexture>'s fill)
template void std::_Uninit_fill_n(FaceTexture*, unsigned, const FaceTexture&, std::allocator<FaceTexture>&, std::_Nonscalar_ptr_iterator_tag);
