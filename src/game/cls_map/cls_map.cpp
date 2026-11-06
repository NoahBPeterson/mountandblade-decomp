// cls_map.cpp: 0x4d92c0..0x4e3eb0 (TU map 107), merged from the slices s004d8c20 (tail), s004d9450, s004d9aa0,
// s004da410, s004dada0, s004db660, s004dbf7e, s004dc630, s004dcef0, s004dd830, s004de1b0, s004de6f0, s004dfd50,
// s004e05c0, s004e25a0, s004e2ea0 and s004e3780. T-regime region: helpers sit right after their first caller;
// the ones the original does not inline are kept out of line (noinline) in binary order.
#define UIB_CTOR_OUT_OF_LINE   // make_slider 0x4dfd50 calls the constructor 0x48b4b0
#include "../../include/image_addrs.h"
#include "../env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "../music_player.h"   // music_player g_music (0xa73a10): listed directly (the resolution audit reads only the source's own includes)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "./cls_map.h"
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40)
#include "../../include/prsnt_table.h"   // g_presentations (0xa75f2c): used[0..2] are the menu kinds
#include "./progress_bar.h"
// the headers whose `// 0x...` annotations the resolution audit has to see are included directly (it scans only the direct includes)
#include "../config/config.h"
#include "../config/widget_mgr.h"
#include "../config/ui_widgets_c.h"
#include "../strat/gui_manager.h"
#include "../font.h"
#include "../../include/e68.h"
#include "../../include/vec_elems.h"
#include "../arr_set.h"
#include "../../include/ptr_tag.h"
#include "../../include/game_data.h"
#include "../../rgl/mesh2/mesh2_common.h"
#include "../../rgl/mesh.h"
#include <process.h>
#include <shlobj.h>
#include "../../tinyxml/tinyxml.h"
#include "../ui/ui_frame.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../shared_vars.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
struct OptionsDlg {
    void choose_dx_settings(Config* cfg, bool overwrite);   // 0x4b9b90 (config/options_dlg.h pulls winsock2.h: member-only stub)
};
#include "../../dxframework/dxutil.h"   // DXUtil_Timer 0x407660 (stdcall, TIMER_COMMAND)
#include "../../rgl/options.h"       // options::save 0x4181c0
#include <hash_map>
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/game_recs.h"
#include "../map_icon.h"      // g_map_icons (0x5e29f0)
#include "../../include/rec78.h"
#include "../ptype.h"
#include "../../include/render_flags.h"
#include "../module_name.h"      // g_module_name (0x8ee3c38)
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "../mode_rec.h"   // g_mission_templates 0x5e2a00
#include "../../include/skinmagic.h"
struct Presentation;
extern Presentation* g_presentation;                        // 0x5e299c

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----

Font* g_font;                                               // 0x5e2934 the game font
int g_video_played;                              // 0x5e2a10 (1 once the intro video started, 2 once it was skipped)
float g_5a9f24 = 0.65f;                          // 0x5a9f24 (the meeting window columns' top)
extern const float g_time_scale = 60.0f;         // 0x590e28 read by SaveWindow::list_saves (progress_bar.cpp)
extern float g_5a9f20;                                      // 0x5a9f20 the meeting window columns' left edge (progress_bar.cpp: a definition in this TU would be folded)
extern float g_5a9be0;                                      // 0x5a9be0 (progress_bar.cpp)
float g_5a9be8 = 1.0f - g_5a9be0;                           // 0x5a9be8 computed at startup
float g_5a9f28[2] = { 1.0f - g_5a9f20, 0.21450001f };       // 0x5a9f28 {x, y} column size (x is computed at startup)


// ---- globals and callees of 0x4d9450 (WinMain) and of the font code ----------------------------------------------

// ---- 0x4d92c0: the glyph hash table's operator[] ------------------------------------------------------------------
// 0x43b210 is stdext::hash_map<int, HashVal>::lower_bound (agent2/agent_ai.cpp), shared by every int-keyed hash map (folded): the glyph table
// calls that instance. Its explicit specialization is only declared.
#include "../glyph_lookup.h"
// the 4d3c00 insert is world_app.cpp's HashTab::insert (the glyph table is a hash table of that layout)
struct HashTab { void insert(HOut* out, unsigned* key); };   // 0x4d3c00
// @ 0x4d92c0
Glyph* GlyphMap::at(const int& key)
{
    GlyphNode* it;
    glyph_lower_bound(*this, &it, key);
    if (it == head) {
        HOut out;
        node_pair v = { key, Glyph() };
        ((HashTab*)this)->insert(&out, (unsigned*)&v.key);
        return &out.node->val;
    }
    return &it->val;
}

// ---- the main window procedure thunk (0x4d9440): enters the shared handler with this = the application object -------
// 0x4d4140: the application window procedure, GameApp::msg_proc (options_panel/world_app.cpp; thiscall with this = g_app_obj)

// @ 0x4d9440   main window procedure thunk
int __stdcall msg_proc_thunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return g_app_obj.msg_proc(hwnd, msg, wParam, lParam);
}


// ---- 0x4d9450: WinMain --------------------------------------------------------------------------------------------
// @ 0x4d9450
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WL_MARKER(12)
    (void)hPrevInstance;

    // skip the command line
    for (char* p = lpCmdLine; *p > 0; p++)
        ;

    g_app_obj.hinstance = hInstance;
    g_app_obj.hbmp = 0;
    g_strtable.language.assign("en");

    {
        WL_MARKER(13)
        tstring empty("");
        g_strtable.load(&empty);
    }   // destroyed here, once (0x4d94dd); an explicit ~tstring() plus the scope exit freed the buffer twice

    LoadLibraryA("riched20.dll");
    InitSkinMagicLib(hInstance, "SCRIBBLE", 0, 0);

    char skin_path[0x104];
    sprintf(skin_path, ".\\Skins\\mb_skin_%s.smf",
            g_strtable.language.buf ? g_strtable.language.buf : (char*)&g_strtable.language.buf);

    WIN32_FIND_DATAA fd;
    HANDLE hfind = FindFirstFileA(skin_path, &fd);
    const char* chosen;
    if (hfind == INVALID_HANDLE_VALUE) {
        chosen = ".\\Skins\\mb_skin_int.smf";
    } else {
        FindClose(hfind);
        chosen = skin_path;
    }
    LoadSkinFile(chosen);

    WL_MARKER(12)
    int r = g_app_obj.init_d3d();
    if (r < 0)
        return 0;

    WL_MARKER(13)
    INT_PTR dlg = DialogBoxParamA(hInstance, MAKEINTRESOURCEA(0xaa), NULL, (DLGPROC)msg_proc_thunk, 0);
    if (dlg != 0x410)
        return (int)dlg;

    FILE* f = mb_open_user_file("rgl_config.txt", "r");
    if (f != NULL) {
        g_cfg.load_file(f);
        fclose(f);
    }
    ((options_record*)&g_options)->load();
    ((OptionsDlg*)&g_app_obj)->choose_dx_settings(&g_cfg, false);

    DWORD_PTR proc_mask = 0, sys_mask = 0;
    GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask);
    (void)sys_mask;     // 0x4d9613 tests [esp+0x1c], the PROCESS mask (2nd out-parameter), not the system mask
    if (g_cfg.force_single_threading == 0 && proc_mask > 1) {
        g_mt_inited = 1;
        InitializeCriticalSection(&g_mt_cs);
        g_mt_event = CreateEventA(NULL, FALSE, FALSE, NULL);
        _beginthread((void (__cdecl*)(void*))loader_thread_body, 0x40000, NULL);
    }

    g_app_obj.m_bStartFullscreen = (g_cfg.start_windowed == 0);
    g_app_obj.m_fMinFrameTime = 1.0f / (float)g_cfg.max_framerate;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    float t0 = DXUtil_Timer(TIMER_GETAPPTIME);

    hfind = FindFirstFileA("paradox.bik", &fd);
    if (hfind != INVALID_HANDLE_VALUE) {
        FindClose(hfind);
        BOOL ok = CreateProcessA("binkplay.exe",
                                 " paradox.bik /P /I2 /Z /J /U1 /W-1 /H-1 /C /B2",
                                 NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (ok) {
            g_video_played = 1;
            WaitForSingleObject(pi.hProcess, 0x57e4);
            CloseHandle(pi.hProcess);
        }
    }

    if (g_video_played == 1 && DXUtil_Timer(TIMER_GETAPPTIME) - t0 > 15.0f) {
        STARTUPINFOA si2;
        PROCESS_INFORMATION pi2;
        memset(&si2, 0, sizeof(si2));
        si2.cb = sizeof(si2);
        memset(&pi2, 0, sizeof(pi2));

        hfind = FindFirstFileA("taleworlds_intro.bik", &fd);
        if (hfind != INVALID_HANDLE_VALUE) {
            FindClose(hfind);
            BOOL ok = CreateProcessA("binkplay.exe",
                                     " taleworlds_intro.bik /P /I2 /Z /J /U1 /W-1 /H-1 /C /B2",
                                     NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2);
            if (ok) {
                g_video_played = 2;
                WaitForSingleObject(pi2.hProcess, 0x1c84);
                CloseHandle(pi2.hProcess);
            }
        }
    }

    r = g_app_obj.Create(hInstance);
    if (r < 0)
        return 0;

    SetActiveWindow(g_app_obj.m_hWnd);
    SetForegroundWindow(g_app_obj.m_hWnd);
    UpdateWindow(g_app_obj.m_hWnd);

    r = g_app_obj.Create3DEnvironment((int)hInstance);
    if (r < 0)
        return 0;

    return g_app_obj.Run();
}

// ---- 0x4d9840: std::vector<E176>::operator= (explicit instantiation of the whole class, as the slice did) ----------
template class std::vector<E50>;
template class std::vector<E68>;
template class std::vector<E176>;
template class std::vector<int>;

// ---- 0x4d99d0: Font::Font (a real constructor: eax = this) ---------------------------------------------------------
// The glyph table is a list (sentinel at +0x4c from list_int_buynode) plus a bucket vector of 9 copies of the sentinel.
// @ 0x4d99d0
Font::Font()
{
    dflt = Glyph();
    map.head = (GlyphNode*)list_int_buynode((list_int*)&map.pad0[1]);
    map.count = 0;
    new (&map.buckets) std::vector<int>(9, (int)map.head);
    map.i20 = 1;
    map.i24 = 1;
    for (int i = 0; i < 0x200; i++)
        ascii[i] = Glyph();
}

// ---- 0x4d9aa0: the font definition loader (TinyXML) ---------------------------------------------------------------
// @ 0x4d9aa0
void Font::load(const char* filename)
{
    int font_scale_i = 100;
    int line_spacing_i = 100;

    width = 0x200;
    height = 0x200;
    padding = 0;

    // the argument is the address of the caller's tstring (editor_ui.cpp: `load((const char*)&n)`); the original
    // reads its buffer, or the characters held in place of the buffer pointer when it is null (add eax, 4)
    const tstring* name = (const tstring*)filename;
    TiXmlDocument doc(name->buf ? name->buf : (const char*)&name->buf);
    if (!doc.LoadFile((TiXmlEncoding)0)) {
        mb_error("Unable To Load Font File!");
        return;
    }

    font_size = 0x4b;

    TiXmlHandle docHandle(&doc);
    TiXmlElement* root = docHandle.FirstChildElement().Element();
    if (!root)
        return;
    TiXmlHandle rootHandle(root);                    // the FontDetails chain hangs off the root element, not the document

    root->QueryIntAttribute("padding", &padding);
    root->QueryIntAttribute("width", &width);
    root->QueryIntAttribute("height", &height);
    root->QueryIntAttribute("font_scale", &font_scale_i);
    root->QueryIntAttribute("line_spacing", &line_spacing_i);
    root->QueryIntAttribute("font_size", &font_size);

    font_scale = (float)font_scale_i * 0.01f;
    line_spacing = (float)line_spacing_i * 0.01f;
    x_ratio = (float)font_size / (float)width;
    y_ratio = (float)font_size / (float)height;

    font_details fd;                                // staged glyph record: its ctor zeroes it once, before the loop

    TiXmlElement* details = rootHandle.FirstChild("FontDetails").FirstChild().Element();
    if (!details)
        return;

    for (TiXmlElement* e = details; e; e = e->NextSiblingElement()) {
        int code = 0;
        if (e->QueryIntAttribute("code", &code) != 0)
            continue;

        int u, v, w, h, preshift, yadjust, postshift;

        fd.b = fd.c = fd.d = fd.e = fd.f = fd.g = 0.0f;       // (0x4d9c89..0x4d9ccc: the first float, a, is not cleared per glyph: it keeps the previous glyph's value)

        if (e->QueryIntAttribute("u", &u) == 0)
            fd.a = (float)(u - 4) / (float)width;
        if (e->QueryIntAttribute("v", &v) == 0)
            fd.d = (float)((height - v) + 4) / (float)height;
        if (e->QueryIntAttribute("w", &w) == 0)
            fd.c = (float)((w - padding) + 6) / (float)width;
        if (e->QueryIntAttribute("h", &h) == 0)
            fd.b = (float)((height - h) - 6 + padding) / (float)height;
        if (e->QueryIntAttribute("preshift", &preshift) == 0)
            fd.e = (float)(preshift - 4) / (float)width;
        if (e->QueryIntAttribute("yadjust", &yadjust) == 0)
            fd.f = (float)(yadjust + 4) / (float)height;
        if (e->QueryIntAttribute("postshift", &postshift) == 0)
            fd.g = (float)(postshift + 4) / (float)width;

        font_details* dst;
        if (code < 0x200)
            dst = (font_details*)&ascii[code];
        else
            dst = (font_details*)map.at(code);
        *dst = fd;
    }
}

// ---- 0x4d9e80: teardown wrapper ------------------------------------------------------------------------------------
// @ 0x4d9e80
bool TearObj::teardown()
{
    calc_vertex_mode(f78);
    return on_selection_changed();
}

// ---- 0x4da410: Skin::operator= ----------------------------------------------------------------------------
// @ 0x4da410
Skin& Skin::operator=(const Skin& src)
{
    id = src.id;
    flags = src.flags;
    body = src.body;
    hand_l = src.hand_l;
    hand_r = src.hand_r;
    calf_l = src.calf_l;
    calf_r = src.calf_r;
    head_name = src.head_name;
    head = src.head;
    n_face_keys = src.n_face_keys;
    face_keys = src.face_keys;
    beard_meshes = src.beard_meshes;
    beard_maps = src.beard_maps;
    hair_maps = src.hair_maps;
    head_maps = src.head_maps;
    constraints = src.constraints;
    n_beard = src.n_beard;
    for (int i = 0; i < 0x40; i++)
        hair_meshes[i] = src.hair_meshes[i];
    n_hair = src.n_hair;
    n_face_tex = src.n_face_tex;
    n_hair_tex = src.n_hair_tex;
    n_beard_tex = src.n_beard_tex;
    hair_textures = src.hair_textures;
    beard_textures = src.beard_textures;
    face_textures = src.face_textures;
    voices.v[0] = src.voices.v[0];
    voices.v[1] = src.voices.v[1];
    voices.v[2] = src.voices.v[2];
    voices.v[3] = src.voices.v[3];
    voices.v[4] = src.voices.v[4];
    voices.v[5] = src.voices.v[5];
    voices.v[6] = src.voices.v[6];
    skeleton = src.skeleton;
    scale = src.scale;
    blood1 = src.blood1;
    blood2 = src.blood2;
    return *this;
}

// @ 0x4da880
Skin* copy_configs_forward(Skin* first, Skin* last, Skin* out)
{
    if (first != last) {
        do {
            *out = *first;
            first++;
            out++;
        } while (first != last);
    }
    return out;
}

// @ 0x4da8c0
Skin* copy_configs_backward(Skin* first, Skin* last, Skin* out)
{
    if (first != last) {
        do {
            last--;
            out--;
            *out = *last;
        } while (last != first);
    }
    return out;
}

// @ 0x4da840  (std::vector<E20b>::_Tidy, emitted by the explicit instantiation)
template class std::vector<E20b>;

// @ 0x4da900   gui_manager::rebuild: replace the text mesh by a fresh one and replay the stored lines through draw_string
extern "C" __declspec(dllimport) unsigned long __stdcall timeGetTime(void);   // 0x5493e8
void gui_manager::rebuild()
{
    int count = fa0;
    rgl::mesh* m = new rgl::mesh;
    if (overlay)
        delete overlay;
    if (m == 0) {
        panel_color = -1;
        panel_bg = 0xff101010;
    }
    fa0 = 0;
    overlay = m;
    sel_ent = 0;
    if (count > 0) {
        int i;
        for (i = 0; i < count; i++)
            draw_string(&lines[i], line_height[i], line_arg[i], line_wrap[i]);
    }
}

// @ 0x4da9c0
// the atexit destructor of g_tableaus: the world stack (a scalar vector: freed and cleared), then the records (destroyed through
// _Destroy_range<E68> 0x4cd570, freed and cleared)
TableauTbl g_tableaus;                                          // 0xa75f40 (.bss; compiler-constructed)
TableauTbl::~TableauTbl() {}

// @ 0x4daa10   the loading screen: the overlay window, the logo, the hint line and one random hint text
void C4c2fb0::init_screen()
{
    g_gui.reset();

    ContainerBase* o = new NodeA(0);
    overlay = o;

    {
        float v[2];
        v[0] = -0.001f;
        v[1] = -0.001f;
        vt(o)->d22(v);
    }
    {
        float v[2];
        v[0] = 1.002f;
        v[1] = 0.752f;
        vt(overlay)->d25(v);
    }
    {
        tstring name("load_window");
        vt(overlay)->d31(0, res_get(name)->clone());
    }
    {
        int color = 0xff442323;
        vt(overlay)->d38(&color);
    }

    MissionUi* logo_ = new MissionUi;
    logo = logo_;
    {
        float v[2];
        v[0] = 0.05f;
        v[1] = 0.6f;
        vt(logo_)->d22(v);
    }
    {
        float v[2];
        v[0] = 0.4f;
        v[1] = 0.1f;
        vt(logo)->d25(v);
    }
    {
        tstring lname("warrider_logo");
        vt(logo)->d31(0, res_get(lname)->clone());
    }

    vt(overlay)->d50(logo_);

    {
        tstring hintpos("");
        hint = make_hint(overlay, hintpos, 0.14f, 0.1f, 0.3f, 0.04f);
    }
    {
        tstring textpos("");
        text = make_hint(overlay, textpos, 0.58625f, 0.39125f, 0.3375f, 0.03f);
    }
    {
        int flag = 0x100;
        vt(text)->d29(&flag);
    }

    g_gui.dirty = 1;
    {
        void* root = overlay;
        g_gui.objs.push_back((MissionUi* const&)(root));
    }

    if (g_module_settings.num_hints != 0) {
        srand(timeGetTime());
        int idx;
        if (g_module_settings.num_hints <= 1)
            idx = 0;
        else
            idx = rand() % g_module_settings.num_hints;

        tstring prefix("hint_");
        char num[0x40];
        _itoa(idx + 1, num, 10);
        prefix.append(tstring(num));
        tstring full(*g_strtable_c.lookup(tstring("ui_hint")));
        full.append(tstring(" #"));
        full.append(tstring(num));
        full.append(tstring(": "));
        full.append(*g_strtable_c.lookup(prefix));
        struct { int a, b; } ret;
        vt(text)->d41(&ret, (void*)&full);
    }
    counter = 0;
    f40 = 0;
}

// @ 0x4dc190
void destroy_range_cfg(Skin* first, Skin* last, const Skin* src)
{
    for (; first != last; ++first)
        *first = *src;
}

// the implicit copy constructor (0x4dc030) is emitted out of line; this wrapper forces its emission
Skin* copy_skin(Skin* self, const Skin* o)
{
    new (self) Skin(*o);
    return self;
}

// (the copy constructor itself, ??0Skin@@QAE@ABU0@@Z at 0x4dc030, is the manifest row; no marker here: a
// marker with no definition after it binds to the next declaration)

// ---- 0x4da5b0: the end-of-game shutdown routine -------------------------------------------------------------------
// Views of foreign classes, reduced to the destructor / element sizes this routine needs.
struct E24   { int a, b; tstring s; };                  // 0x18 bytes, string at +8
struct NameRec48   { tstring s0; char pad[4]; tstring s14; char pad2[0x18]; TimerList m3c; };   // 0x48 bytes
// the vector deleting destructors of the global record tables (rgl/mesh2/gui_edit.cpp, rec_tables.cpp): 0x4c4f40 elem60_vector_delete and
// 0x4c7ee0 destroy_record_array / 0x4c8980 conv_item_array_dtor (fastcall: ecx = the table, flags on the stack), 0x4c3430 elem58's member
struct elem60; struct conv_rec;
void* __fastcall elem60_vector_delete(elem60* self, void* unused, unsigned flags);               // 0x4c4f40
struct elem58 {
    void* vector_deleting_delete(unsigned flags);   // 0x4c3430
};
void* __fastcall destroy_record_array(void* p, int unused, unsigned char flags);               // 0x4c7ee0
void* __fastcall conv_item_array_dtor(conv_rec* self, int unused, unsigned char flags);        // 0x4c8980
#include "../../include/list_int.h"                  // list_int::clear 0x4c4d80 (std::list<int>::clear)
#include "../window_mgr.h"                                    // g_windows (0xa73980)
#include "../screen_obj.h"                              // ScreenObj: the 30 entries of g_screen.items (0xa73980), deleted through slot 0

#include "../../include/fmod_api.h"   // FMOD 3 (one declaration per entry point)
#include "../mission.h"   // canonical: g_mission (0x5e292c)
#include "../menu_rec.h"           // MenuRec* g_menus (0x5e29b0)
#include "../prop_kind.h"         // PropKind* g_propkinds (0x5e29dc)
#include "../ptype.h"             // ptype* g_ptypes (0x5e28f8)
#include "../../include/item_kind.h"   // ItemKind* g_items (0x5e29d0)
#include "../terrain_b/emit.h"     // EnvRow* g_env_rows (0x5e28f4)
extern PartyTemplate* g_party_templates;                   // 0x5e29f8 (PartyTemplate: game_recs.h, via game_data.h; the type script_interpreter.h and party/game.h declare)
struct E24;
E24* g_5e2928;                                          // 0x5e2928 (the array shutdown() deletes)
extern Font* g_font;                                // 0x5e2934

// @ 0x4da5b0
HRESULT GameApp::FinalCleanup()
{
    FSOUND_Close();
    ((text_stream*)&g_log)->text_stream::w_cstr("Saving options...");     // 0x4153a0 (the log is a text_stream: called directly)
    g_options.save();
    if (((void*)g_game)) {
        GameData* g = g_game;
        g->world_free();                // 0x492f40
        operator delete(g);
        (*(void**)&g_game) = 0;
    }
    if (((void*)g_mission)) {
        Mission* w = g_mission;
        w->destroy();                   // 0x4d51d0
        operator delete(w);
        (*(void**)&g_mission) = 0;
    }
    if (g_items) {
        delete[] g_items;
    }
    if (g_menus) {
        elem60_vector_delete((elem60*)g_menus, 0, 3);              // ??_E, flags 3 (0x4c4f40)
    }
    if (g_5e2928) {
        delete[] (E24*)g_5e2928;
    }
    if (g_mission_templates) {
        ((elem58*)g_mission_templates)->vector_deleting_delete(3);            // 0x4c3430
    }
    if (g_propkinds) {
        destroy_record_array(g_propkinds, 0, 3);            // 0x4c7ee0
    }
    if (g_ptypes) {
        delete[] (E20b*)g_ptypes;
    }
    if (g_env_rows) {
        delete[] g_env_rows;
    }
    if (g_map_icons) {
        delete[] (NameRec48*)g_map_icons;
    }
    if (g_party_templates) {
        conv_item_array_dtor((conv_rec*)g_party_templates, 0, 3);            // 0x4c8980
    }
    for (int i = 0; i < 30; i++) {
        delete g_windows.arr[i];
        g_windows.arr[i] = 0;
    }
    if (g_font) {
        // the font's inlined destructor: the bucket vector, then the list (sentinel at +0x4c)
        Font* f = g_font;
        if (f->map.buckets._Myfirst)
            operator delete(f->map.buckets._Myfirst);
        f->map.buckets._Myfirst = 0;
        f->map.buckets._Mylast = 0;
        f->map.buckets._Myend = 0;
        ((list_int*)&f->map.pad0[1])->clear();
        operator delete(f->map.head);
        f->map.head = 0;
        operator delete(f);
    }
    ExitSkinMagicLib();
    if (g_log.is_open) {
        if (g_log.flush() == 0)
            fclose(g_log.fp);
        g_log.failed = 1;
        g_log.is_open = 0;
    }
    return 0;
}

// ---- 0x4dada0: the hint bar maker (a text label added to an overlay) -----------------------------------------------
// @ 0x4dada0
MissionUi* make_hint(ContainerBase* o, const tstring& text, float x, float y, float w, float h)
{
    MissionUi* r = new MissionUi();
    {
        float pos[2] = { x, y };
        vt(r)->d22(pos);
    }
    {
        float sc[2] = { w, h };
        vt(r)->d25(sc);
    }
    float z = h * 0.7f;
    {
        float sz[2] = { z, z };
        vt(r)->d27(sz);
        vt(r)->d41(sz, (void*)&text);
    }
    vt(o)->d50(r);
    return r;
}

// @ 0x4db810
UiA* new_button(ContainerBase* o, const tstring& text, float px, float py, float sx, float sy)
{
    UiA* r = new UiA;
    {
        float t[2];
        t[0] = px; t[1] = py;
        vt(r)->d22(t);
    }
    {
        float t[2];
        t[0] = sx; t[1] = sy;
        vt(r)->d25(t);
    }
    {
        tstring n("button5_up");
        vt(r)->d31(2, res_get(n)->clone());
    }
    {
        tstring n("button5_down");
        vt(r)->d31(1, res_get(n)->clone());
    }
    {
        tstring n("button5_hl");
        vt(r)->d31(3, res_get(n)->clone());
    }
    {
        struct { int a, b; } ret;
        vt(r)->d41(&ret, (void*)&text);
    }
    vt(o)->d50(r);
    return r;
}

// ---- 0x4dc350: place the frame of the loading screen (a quad mesh at +0xfb0 and four corner meshes at +0x1210) --------
// @ 0x4dc350
void gui_manager::set_rect(const float* a, const float* b)
{
    float sy = b[1] + a[1];
    float sx = a[0] + b[0];
    mfb0.ensure_local();
    { rgl::vector4 v(a[0], a[1], 0.0f); rgl::point20* r = mfb0.v70._Myfirst + 0; r->b = r->a = v; }
    { rgl::vector4 v(sx, a[1], 0.0f); rgl::point20* r = mfb0.v70._Myfirst + 1; r->b = r->a = v; }
    { rgl::vector4 v(sx, sy, 0.0f); rgl::point20* r = mfb0.v70._Myfirst + 2; r->b = r->a = v; }
    { rgl::vector4 v(a[0], sy, 0.0f); rgl::point20* r = mfb0.v70._Myfirst + 3; r->b = r->a = v; }
    int& bg_color = *(int*)&mfb0.color;
    bg_color = panel_bg;
    mfb0.slots[0].f28 = 1;
    {
        tstring n("matte");
        mfb0.res = (skin*)g_resmgr.get_material(n);
    }
    corner[0]->m_cc.o.x = (a[0] - corner[0]->bounds.min.x) - 0.003f;
    corner[0]->m_cc.o.y = (sy - corner[0]->bounds.max.y) + 0.003f;
    corner[1]->m_cc.o.x = (sx - corner[0]->bounds.max.x) + 0.003f;
    corner[1]->m_cc.o.y = (sy - corner[0]->bounds.max.y) + 0.003f;
    corner[2]->m_cc.o.x = (sx - corner[0]->bounds.max.x) + 0.003f;
    corner[2]->m_cc.o.y = (a[1] - corner[0]->bounds.min.y) - 0.003f;
    corner[3]->m_cc.o.x = (a[0] - corner[0]->bounds.min.x) - 0.003f;
    corner[3]->m_cc.o.y = (a[1] - corner[0]->bounds.min.y) - 0.003f;
    corner[0]->color = -1;
    corner[1]->color = -1;
    corner[2]->color = -1;
    corner[3]->color = -1;
}

// ---- 0x4dc630: the message (result) window ------------------------------------------------------------------------------
// the long / medium button makers sit after init(); the shared maker 0x4dcc50 follows (T regime: kept out of line)

// @ 0x4dc630
void ResultWindow::init_screen()
{
    float mv[4];
    if (w04 == 0) {
        tstring key("ui_ok");
        s78 = *g_strtable_c.lookup(key);
    }
    win = new NodeA(0);
    mv[0] = 0.25f; mv[1] = 0.2f; mv[2] = 0.5f; mv[3] = 0.44f;
    vt(win)->d22(mv);
    vt(win)->d25(&mv[2]);
    rgl::mesh* m;
    {
        tstring name("message_window");
        m = res_get(name)->clone();
    }
    vt(win)->d31(0, m);

    panel30 = new MissionUi;
    {
        float v[2] = { 0.05f, 0.34f };
        vt(panel30)->d22(v);
    }
    {
        float v[2] = { 0.4f, 0.05f };
        vt(panel30)->d25(v);
    }
    { struct { int a, b; } ret; vt(panel30)->d41(&ret, &s68); }
    vt(win)->d50(panel30);

    panel34 = new MissionUi;
    {
        int fl = 0x100;
        vt(panel34)->d30(&fl);
    }
    {
        float v[2] = { 0.05f, 0.135f };
        vt(panel34)->d22(v);
    }
    {
        float v[2] = { 0.4f, 0.2f };
        vt(panel34)->d25(v);
    }
    {
        float v[2] = { 0.02f, 0.02f };
        vt(panel34)->d27(v);
    }
    { struct { int a, b; } ret; vt(panel34)->d41(&ret, &name); }
    vt(win)->d50(panel34);

    b3c = 0;
    b40 = 0;
    box38 = 0;
    if (w04 == 3) {
        b3c = new_medium_button(win, &s78, 0.15f, 0.085f, 0.045f);
        b40 = new_medium_button(win, &s88, 0.15f, 0.03f, 0.045f);
        box38 = new UiE;
        {
            float y = 0.0f;
            tstring ky("yn_tb_name_y");
            int iy = g_game_vars.find(ky);
            if (iy >= 0)
                y = (float)atof(g_game_vars.entries[iy].value.c_str());
            float x = 0.0f;
            tstring kx("yn_tb_name_x");
            int ix = g_game_vars.find(kx);
            if (ix >= 0)
                x = (float)atof(g_game_vars.entries[ix].value.c_str());
            float v[2] = { x, y };
            vt(box38)->d22(v);
        }
        {
            float v[2] = { 0.0f, 0.009f };
            vt(box38)->d28(v);
        }
        {
            float v[2] = { 0.02f, 0.02f };
            vt(box38)->d27(v);
        }
        {
            tstring n("party_button");
            vt(box38)->d32(2, res_get(n)->clone());
        }
        {
            rgl::vector4 c(0.5f, 0.5f, 1.0f);
            ((rgl::mesh*)box38->tracks[2])->set_color(c);
        }
        {
            tstring n("party_button_down");
            vt(box38)->d32(1, res_get(n)->clone());
        }
        {
            rgl::vector4 c(0.5f, 0.5f, 1.0f);
            ((rgl::mesh*)box38->tracks[1])->set_color(c);
        }
        vt(box38)->d03();
        { struct { int a, b; } ret; vt(box38)->d41(&ret, &s54); }
        box38->f180 = 1;
        g_gui.ui_active = box38;
        vt(win)->d50(box38);
    } else if (w04 == 2) {
        b3c = new_long_button(win, &s78, 0.03f, 0.085f, 0.045f);
        b40 = new_long_button(win, &s88, 0.03f, 0.03f, 0.045f);
    } else if (w04 == 1) {
        b3c = new_medium_button(win, &s78, 0.15f, 0.085f, 0.045f);
        b40 = new_medium_button(win, &s88, 0.15f, 0.03f, 0.045f);
    } else {
        b3c = new_medium_button(win, &s78, 0.15f, 0.05f, 0.045f);
    }
    {
        void* self = win;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(self));
    }
    f64 = g_gui.show_cursor;
}

// @ 0x4dcbd0
UiA* new_long_button(MissionUi* parent, const tstring* text, float a, float b, float c)
{
    tstring d("longer_button_down");
    tstring nm("longer_button");
    return new_sized_button(parent, *text, a, b, c, nm, d, 9.7f, 0.072f, 0.085f);
}

// UTF-8 -> int code points (0x487b00; custom register convention in the original: text in edi, count out in edx)
static __declspec(noinline) void decode_utf8(const tstring* s, int* out_len, int* buf, const int* maxn)   // 0x487b00
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            if (*out_len >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((((q[0] & 7) << 6) | (q[1] & 0x3f)) << 6 | (q[2] & 0x3f)) << 6) | (q[3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = ((((q[0] & 0xf) << 6) | (q[1] & 0x3f)) << 6) | (q[2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((q[0] & 0x1f) << 6) | (q[1] & 0x3f));
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[*out_len] = ch;
            (*out_len)++;
        } while (i < n);
    }
}

// (parent, text, a, b, c, mesh name, pressed mesh name, k1, k2, k3): a button sized from the text width
// @ 0x4dcc50
UiA* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
                            const tstring& name, const tstring& down, float k1, float k2, float k3)
{
    float tmp = c * k1;
    int buf[0x800];
    UiA* p = new UiA;
    {
        float v[2] = { a, b };
        vt(p)->d22(v);
    }
    {
        float v[2] = { tmp, c };
        vt(p)->d25(v);
    }
    float r = 1.0f / k2;
    float s = r * c;
    {
        rgl::mesh* m = res_get(name)->clone();
        m->scale(s);
        vt(p)->d32(2, m);
    }
    {
        rgl::mesh* m = res_get(down)->clone();
        m->scale(s);
        vt(p)->d32(1, m);
    }
    {
        int col = 0xff007700;
        vt(p)->d39(&col);                       // +0x9c highlight colour (0x4dcd7f)
    }
    float v = c * 0.65f;
    int len;
    int maxn = 0x800;
    decode_utf8(&text, &len, buf, &maxn);
    float t = (float)len * v * 0.39f;
    float e = tmp - (r * c) * k3;               // 0x4dcdc7: r, * c, * k3 in that order
    if (t > e)
        v = v * (e / t);
    {
        float sv[2] = { v, v };
        vt(p)->d27(sv);
    }
    {
        // the text's y offset uses the text size (0x4dcda2 stores v over the mesh scale's slot [esp+0xc], read at
        // 0x4dce1f whether or not the text was shrunk), not the mesh scale s
        float pv[2] = { 0.0f, c * 0.5f - v * 0.3f };
        vt(p)->d28(pv);
    }
    { struct { int a, b; } ret; vt(p)->d41(&ret, (void*)&text); }
    vt(parent)->d50(p);
    return p;
}

// @ 0x4dce70
UiA* new_medium_button(MissionUi* parent, const tstring* text, float a, float b, float c)
{
    tstring d("medium_button_down");
    tstring nm("medium_button");
    return new_sized_button(parent, *text, a, b, c, nm, d, 3.7f, 0.072f, 0.085f);
}

// ---- 0x4dcef0..0x4ddf80: the main menu, the save scan, the menu rows, the block owner and the escape / message menus ----
// @ 0x4dcef0
void ItemRec::init_screen()
{
    BlockOwner& block = *(BlockOwner*)&owned;       // the +0x30 sub-object (index, timer, menu block) as the block owner sees it
    char text[0x200];
    int ids[9];
    char version_digits[0x40];
    int count;

    g_music.situation = 0x400000;
    g_gui.reset();

    win = new NodeA(0);

    { V2 t_(-0.001f, -0.001f); vt(win)->d22(&t_); }
    { V2 t_(1.002f, 0.752f); vt(win)->d25(&t_); }
    int base_colour = 0xff442323;
    vt(win)->d38(&base_colour);

    int r = g_module_settings.main_menu_font_color_red;
    r = r < 0 ? 0 : (r >= 0xff ? 0xfe : r);
    int g = g_module_settings.main_menu_font_color_green;
    g = g < 0 ? 0 : (g >= 0xff ? 0xfe : g);
    int b = g_module_settings.main_menu_font_color_blue;
    b = b < 0 ? 0 : (b >= 0xff ? 0xfe : b);
    int colour = ((((unsigned)r | 0xffffff00u) << 8) | (unsigned)g) << 8 | (unsigned)b;
    int down = (int)0xffffdd00;
    int tp = 0;

    float step = g_module_settings.enable_quick_battles ? 0.06f : 0.08f;

    {
        tstring k("ui_start_tutorial");
        row_tutorial = build_row(win, g_strtable_c.lookup(k), 0.33f,
                               (g_module_settings.enable_quick_battles ? 5 : 4) * step + 0.08f, 0.6f, 0.05f);
    }
    {
        tstring k("ui_start_a_new_game");
        row_new_game = build_row(win, g_strtable_c.lookup(k), 0.33f,
                               (g_module_settings.enable_quick_battles ? 4 : 3) * step + 0.08f, 0.6f, 0.05f);
    }
    {
        tstring k("ui_restore_a_saved_game");
        row_restore = build_row(win, g_strtable_c.lookup(k), 0.33f,
                              (g_module_settings.enable_quick_battles ? 3 : 2) * step + 0.08f, 0.6f, 0.05f);
    }
    if (g_module_settings.enable_quick_battles) {
        {
            tstring k("ui_custom_battle");
            row_custom = build_row(win, g_strtable_c.lookup(k), 0.33f, step + step + 0.08f, 0.6f, 0.05f);
        }
        vt(row_custom)->d38(&colour);
        vt(row_custom)->d39(&down);
        vt(row_custom)->d29(&tp);
    } else {
        row_custom = 0;
    }
    {
        tstring k("ui_options");
        row_options = build_row(win, g_strtable_c.lookup(k), 0.33f, step + 0.08f, 0.6f, 0.05f);
    }
    {
        tstring k("ui_exit_to_windows");
        row_exit = build_row(win, g_strtable_c.lookup(k), 0.68f, 0.02f, 0.3f, 0.04f);
    }
    {
        tstring k("ui_credits");
        row_credits = build_row(win, g_strtable_c.lookup(k), 0.33f, 0.08f, 0.5f, 0.05f);
    }

    vt(row_tutorial)->d38(&colour);
    vt(row_new_game)->d38(&colour);
    vt(row_restore)->d38(&colour);
    vt(row_options)->d38(&colour);
    vt(row_credits)->d38(&colour);
    vt(row_exit)->d38(&colour);

    vt(row_tutorial)->d39(&down);
    vt(row_new_game)->d39(&down);
    vt(row_restore)->d39(&down);
    vt(row_options)->d39(&down);
    vt(row_credits)->d39(&down);
    vt(row_exit)->d39(&down);

    vt(row_tutorial)->d29(&tp);
    vt(row_new_game)->d29(&tp);
    vt(row_restore)->d29(&tp);
    vt(row_options)->d29(&tp);
    vt(row_credits)->d29(&tp);
    tp = 8;
    vt(row_exit)->d29(&tp);

    logo = new MissionUi;
    { V2 t_(0.2f, 0.55f); vt(logo)->d22(&t_); }
    { V2 t_(0.6f, 0.15f); vt(logo)->d25(&t_); }
    {
        tstring k("warrider_logo");
        vt(logo)->d31(0, res_get(k)->clone());
    }
    vt(win)->d50(logo);

    pic = new MissionUi;
    { V2 t_(0.015f, 0.02f); vt(pic)->d22(&t_); }
    {
        tstring k("pic_mercenary");
        vt(pic)->d32(0, res_get(k)->clone());
    }
    vt(win)->d50(pic);

    sprintf(version_digits, "%d.%d%d%d", 1, 0, 1, 1);
    {
        tstring k("ui_version_value");
        tstring* fmt = g_strtable_c.lookup(k);
        sprintf(text, fmt->c_str(), version_digits);
    }
    {
        tstring t(text);
        version = make_hint(win, t, 0.67f, 0.5f, 0.3f, 0.04f);
    }
    vt(version)->d38(&colour);

    const char* prefix = g_module_name.c_str();
    scan_saves((int)ids, &count, &first_free, prefix);
    if (first_free == -1)
        vt(row_new_game)->d46();

    block.init(win, g_presentations.used[0]);
    if (block.index <= 0) {
        tstring k("load_window");
        vt(win)->d31(0, res_get(k)->clone());
    }

    void* wv = win;
    g_gui.dirty = 1;
    g_gui.objs.push_back((MissionUi* const&)(wv));
}

// @ 0x4dd7b0
void save_path(char* out)
{
    char dir[0x104];
    char base[0x104];
    SHGetFolderPathA(0, 5, 0, 0, dir);
    sprintf(base, "%s\\Mount&Blade Savegames", dir);
    CreateDirectoryA(base, 0);
    sprintf(base, "%s", out);
    sprintf(out, "%s\\Mount&Blade Savegames\\%s", dir, base);
}

// @ 0x4dd6e0
void scan_saves(int ids, int* count, int* first, const char* prefix)
{
    char buf[0x104];
    char file[0x200];
    *count = 0;
    *first = -1;
    unsigned i = 0;
    do {
        sprintf(buf, "");
        save_path(buf);
        mb_snprintf(file, 0x200, "%s%s\\sg%d%d.sav", buf, prefix, i / 10, i % 10);
        FILE* f = fopen(file, "r");
        if (f) {
            fclose(f);
            *(int*)(ids + *count * 4) = i;
            *count = *count + 1;
        } else if (*first == -1) {
            *first = i;
        }
        i++;
    } while ((int)i < 9);
}


// @ 0x4dd830
UiA* build_row(ContainerBase* parent, const tstring* key, float x, float y, float w, float h)
{
    UiA* p = new UiA;

    float v[2] = {x, y};
    vt(p)->d22(v);

    v[0] = w;
    v[1] = h;
    vt(p)->d25(v);

    {
        int colour = 0xffefefef;
        vt(p)->d38(&colour);
    }

    v[0] = h * 0.9f;
    v[1] = h * 0.9f;
    vt(p)->d27(v);

    {
        int colour2 = 0xff55ff50;
        vt(p)->d39(&colour2);
    }

    vt(p)->d41(v, (void*)key);

    vt(parent)->d50(p);
    return p;
}

static __forceinline void restart_inl(Timer& t) { t.start = g_ticks[t.clock]; }     // Timer::restart (0x432bf0) as the callers inline it

// @ 0x4dd9e0
void BlockOwner::activate()
{
    g_presentation = (Presentation*)this;
    if (index >= 0) {
        f14 = ((MenuKindRec*)g_presentations.records)[index].a;
        restart_inl(timer);
        ((MenuKindRec*)g_presentations.records)[index].sound.update(-60);
    } else {
        f14 = 0;
    }
}

// @ 0x4dd920
void BlockOwner::init(ContainerBase* parent, int which)
{
    if (which < 0)
        return;
    block = new UiFrame;
    vt(block)->d25(parent->screen);
    vt(block)->d22(parent->e);
    vt(parent)->d50(block);
    index = which;
    int n = ((MenuKindRec*)g_presentations.records)[which].first;
    if (n > 0)
        vt(block)->d31(0, g_meshes.placed(n));
    activate();
}

// @ 0x4dda40
void ItemRec2::init_screen()
{
    BlockOwner& block = *(BlockOwner*)&owned;
    g_gui.reset();
    ContainerBase* w = new NodeA(0);
    win = w;

    float v[2];
    v[0] = -0.001f;
    v[1] = -0.001f;
    vt(win)->d22(v);

    v[0] = 1.002f;                              // 0x4ddaa4: 0x3f804189
    v[1] = 0.752f;                              // 0x3f408312
    vt(win)->d25(v);

    int no = -1;
    vt(win)->d38(&no);

    int r = g_module_settings.main_menu_font_color_red;
    r = r < 0 ? 0 : (r >= 0xff ? 0xfe : r);
    int g = g_module_settings.main_menu_font_color_green;
    g = g < 0 ? 0 : (g >= 0xff ? 0xfe : g);
    int b = g_module_settings.main_menu_font_color_blue;
    b = b < 0 ? 0 : (b >= 0xff ? 0xfe : b);
    int color = ((((unsigned)r | 0xffffff00) << 8) | (unsigned)g) << 8 | (unsigned)b;

    {
        tstring k("ui_return_to_game");
        row0 = build_row(w, g_strtable_c.lookup(k), 0.1f, 0.42f, 0.4f, 0.05f);
    }
    {
        tstring k("ui_options");
        row2 = build_row(w, g_strtable_c.lookup(k), 0.1f, 0.36f, 0.4f, 0.05f);
    }
    {
        tstring k("ui_save_and_exit");
        row1 = build_row(w, g_strtable_c.lookup(k), 0.1f, 0.29f, 0.4f, 0.05f);
    }
    if (((char*)g_game)[0x18] != 0)
        vt(row1)->d46();
    vt(row0)->d38(&color);
    vt(row2)->d38(&color);
    vt(row1)->d38(&color);
    row3 = 0;
    row4 = 0;
    row5 = 0;

    if (*(int*)((char*)g_game + 0xb4) != 0) {
        {
            tstring k("ui_save");
            row3 = build_row(w, g_strtable_c.lookup(k), 0.1f, 0.22f, 0.4f, 0.05f);
        }
        vt(row3)->d38(&color);
        {
            tstring k("ui_save_as");
            row4 = build_row(w, g_strtable_c.lookup(k), 0.1f, 0.15f, 0.4f, 0.05f);
        }
        vt(row4)->d38(&color);
        {
            tstring k("ui_quit_without_saving");
            row5 = build_row(w, g_strtable_c.lookup(k), 0.1f, 0.08f, 0.4f, 0.05f);
        }
        vt(row5)->d38(&color);
        if (((char*)g_game)[0x18] != 0) {
            vt(row3)->d46();
            vt(row4)->d46();
        }
    }

    MissionUi* lg = new MissionUi;
    logo = lg;

    v[0] = 0.3f;
    v[1] = 0.6f;
    vt(lg)->d22(v);

    v[0] = 0.4f;
    v[1] = 0.1f;
    vt(lg)->d25(v);

    {
        tstring n("warrider_logo");
        vt(lg)->d31(0, res_get(n)->clone());
    }
    vt(win)->d50(lg);

    int pick = rand() % 0x3dc9;
    float f = (float)pick * (1.0f / 0x3dc9);
    tstring pic("pic_mb_warrior_1");
    if (f < 0.25f)
        pic.assign("pic_mb_warrior_2");
    else if (f < 0.5f)
        pic.assign("pic_mb_warrior_3");
    else if (f < 0.75f)
        pic.assign("pic_mb_warrior_4");

    vt(win)->d32(2, res_get(pic)->clone());

    block.init(win, g_presentations.used[1]);
    if (block.index < 0) {
        tstring n("load_window");
        vt(win)->d31(0, res_get(n)->clone());
    }
    void* wv = win;
    g_gui.dirty = 1;
    g_gui.objs.push_back((MissionUi* const&)(wv));
}

// @ 0x4ddf80
void C4c3050::init_screen()
{
    ContainerBase* w;
    {
        ContainerBase* p = new NodeA(0);
        w = p;
    }
    win = w;

    float v[4];
    v[0] = 0.25f;
    v[1] = 0.2f;
    v[2] = 0.5f;
    v[3] = 0.44f;
    vt(win)->d22(v);

    vt(win)->d25(v + 2);

    rgl::mesh* m;
    {
        tstring t("message_window");
        m = res_get(t)->clone();
    }
    vt(win)->d31(0, m);

    image = new MissionUi;

    v[0] = 0.05f;
    v[1] = 0.29f;
    vt(image)->d22(v);

    v[0] = 0.4f;
    v[1] = 0.1f;
    vt(image)->d25(v);

    vt(image)->d41(v, &name);
    vt(win)->d50(image);

    rows[0] = 0;
    rows[1] = 0;
    rows[2] = 0;
    rows[3] = 0;
    rows[4] = 0;
    rows[5] = 0;

    for (int i = 0; i < count; i++) {
        tstring t1("longer_button_down");
        tstring t2("longer_button");
        rows[i] = new_sized_button(win, ((tstring*)t6c)[i], 0.05f,
                                   0.3f - (float)(i - count + 6) * 0.05f,
                                   0.04f, t2, t1, 9.7f, 0.072f, 0.085f);
    }

    void* wv = win;
    g_gui.dirty = 1;
    g_gui.objs.push_back((MissionUi* const&)(wv));
    f58 = g_gui.show_cursor;
}


// ---- 0x4de1b0..0x4de6f0: the debrief menu --------------------------------------------------------------------------------
// @ 0x4de1b0
void Rec38::init_screen()
{
    float tv[2];
    ContainerBase* w;
    {
        ContainerBase* p = new NodeA(0);
        w = p;
    }
    win = w;

    float v[4];
    v[0] = 0.3f;
    v[1] = 0.2f;
    vt(win)->d22(v);

    v[2] = 0.5f;
    v[3] = 0.3f;
    vt(win)->d25(v + 2);

    vt(win)->d31(0, make_background(v + 2, -1));

    PartyMgr* party = &g_game->parties;
    int h = party->lookup(w10);
    Party** page = party->pages._Myfirst;
    char* rec = (char*)((h & 0xf) * 0x5ac8 + (char*)page[h >> 4]);
    const tstring* text;
    if (g_module_settings.show_party_ids_instead_of_names)
        text = (const tstring*)(rec + 0x20);
    else if (*(char*)(rec + 0x1c) != 0)
        text = (const tstring*)(rec + 0xc);
    else {
        const tstring* p = g_strtable.find((const tstring*)(rec + 0x20));
        text = p != 0 ? p : (const tstring*)(rec + 0xc);
    }
    vt(win)->d41(tv, (void*)text);

    {
        tstring t("ui_done");
        f34 = new_button(win, *g_strtable_c.lookup(t), 0.2f, 0.03f, 0.15f, 0.05f);
    }

    void* wv = win;
    g_gui.dirty = 1;
    g_gui.objs.push_back((MissionUi* const&)(wv));
}

// @ 0x4de320
rgl::mesh* make_background(float* v, int color)
{
    PanelMesh* m;
    {
        PanelMesh* p = (PanelMesh*)::operator new(0x260);
        if (p != 0)
            new (p) rgl::mesh();
        else
            p = 0;
        m = p;
    }
    {
        tstring t("background");
        m->res = g_resmgr.get_material(t);
    }

    float a[2];
    a[0] = v[0] + v[0];
    a[1] = -v[1] - v[1];

    float z[5];
    z[0] = 0.0f;
    z[1] = 0.0f;
    z[2] = 0.0f;
    z[3] = 0.0f;
    z[4] = 0.0f;

    m->build_panel(&z[2], v, z, a, &color);
    return m;
}

// @ 0x4de3e0
void PanelMesh::build_panel(const float* base, const float* size, const float* uv0, const float* uv1, int* col)
{
    float uv[2];
    int idx[4];
    rgl::vector4 p;
    int vv[4];
    int f5c;

    p.x = base[0];
    p.y = base[1];
    p.z = base[2];
    idx[0] = add_point(&p);

    p.x = base[0] + size[0];
    p.y = base[1];
    p.z = base[2];
    idx[1] = add_point(&p);

    p.x = base[0] + size[0];
    p.y = size[1] + base[1];
    p.z = base[2];
    idx[2] = add_point(&p);

    p.x = base[0];
    p.y = size[1] + base[1];
    p.z = base[2];
    idx[3] = add_point(&p);

    uv[0] = uv0[0];
    uv[1] = uv0[1] - uv1[1];
    p.x = 0.0f;
    p.y = 0.0f;
    p.z = 1.0f;
    f5c = 0;
    vv[0] = add_vertex(idx[0], (const vec2*)uv, (const unsigned*)col, &p, &f5c);

    f5c = 0;
    uv[0] = uv0[0] + uv1[0];
    uv[1] = uv0[1] - uv1[1];
    vv[1] = add_vertex(idx[1], (const vec2*)uv, (const unsigned*)col, &p, &f5c);

    uv[0] = uv0[0] + uv1[0];
    uv[1] = uv0[1];
    f5c = 0;
    vv[2] = add_vertex(idx[2], (const vec2*)uv, (const unsigned*)col, &p, &f5c);

    f5c = 0;
    vv[3] = add_vertex(idx[3], (const vec2*)uv0, (const unsigned*)col, &p, &f5c);

    add_face(vv[0], vv[1], vv[2]);
    add_face(vv[0], vv[2], vv[3]);

    *((char*)this + 0x138) = 1;
    *((char*)this + 0x164) = 1;
}

// @ 0x4de5b0
void Rec34::init_screen()
{
    g_sound_mgr.stop_all();

    ContainerBase* w;
    {
        ContainerBase* p = new NodeA(0);
        w = p;
    }
    win = w;

    { V2 t_(0.2f, 0.15f); vt(win)->d22(&t_); }
    { V2 t_(0.6f, 0.45f); vt(win)->d25(&t_); }

    rgl::mesh* m;
    {
        tstring t("debrief_window");
        m = res_get(t)->clone();
    }
    vt(win)->d31(0, m);

    {
        int c = -1;
        vt(win)->d38(&c);
    }
    {
        tstring t("ui_done");
        f30 = new_medium_button(win, g_strtable_c.lookup(t), 0.2f, 0.05f, 0.04f);
    }

    {
        void* wv = win;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(wv));
    }
}

// ---- 0x4dfd50 / 0x4dfec0: slider and drop-down makers ----------------------------------------------------------------------
// load and clone a named mesh (the original's tstring temporary lives inside the inlined helper)
static __forceinline rgl::mesh* load_mesh(const char* name)
{
    tstring n(name);
    return res_get(n)->clone();
}

// one float setting by a key string the caller owns (a temporary that lives to the end of the caller's full expression)
static __forceinline float config_float_key(const tstring& n)
{
    int i = g_game_vars.find(n);
    float r = 0.0f;
    if (i >= 0)
        r = (float)atof(g_game_vars.entries[i].value.c_str());
    return r;
}
static __forceinline V2* v2_ref(const V2& v) { return (V2*)&v; }

// read one float setting of the option table, `def` when the key is missing (the original always passes 0.0f)
static __forceinline float config_float(const char* name, float def)
{
    tstring n(name);
    int i = g_game_vars.find(n);
    float r = def;
    if (i >= 0)
        r = (float)atof(g_game_vars.entries[i].value.c_str());
    return r;
}

// @ 0x4dfd50
UiB* make_slider(MissionUi* parent, float x, float y, float k)
{
    UiB* w = new UiB;
    {
        V2 pos(x, y);
        vt(w)->d22(&pos);
    }
    {
        V2 size(k * 0.25f, k * 0.03f);              // 0x4dfd90: both from k ([esp+0x4c] / [esp+0x50] after the push)
        vt(w)->d25(&size);
    }
    rgl::mesh* track = load_mesh("slider_hor");
    rgl::mesh* handle = load_mesh("handle_hor");
    if (k != 1.0f) {
        track->set_color(rgl::vector4(k, k, k));
        handle->set_color(rgl::vector4(k, k, k));
    }
    vt(w)->d32(0, track);
    vt(w)->d32(2, handle);
    vt(parent)->d50(w);
    return w;
}

// @ 0x4dfec0
DropDown* make_dropdown(MissionUi* parent, const tstring* name, float x, float y)
{
    DropDown* b = new DropDown;
    {
        V2 pos(x + 0.0176f, y);
        vt(b)->d22(&pos);
    }

    {
        tstring n("button_drop");
        vt(b)->d32(2, res_get(n)->clone());
    }
    ((rgl::mesh*)b->tracks[2])->set_color(rgl::vector4(1.0f, 0.5f, 1.0f));

    {
        tstring n("button_drop_clicked");
        vt(b)->d32(1, res_get(n)->clone());
    }
    ((rgl::mesh*)b->tracks[1])->set_color(rgl::vector4(1.0f, 0.5f, 1.0f));

    {
        tstring n("button_drop_hl");
        vt(b)->d32(3, res_get(n)->clone());
    }
    ((rgl::mesh*)b->tracks[3])->set_color(rgl::vector4(1.0f, 0.5f, 1.0f));

    {
        tstring n("button_drop_child");
        b->m1a4 = res_get(n);
    }
    {
        tstring n("button_drop_child_clicked");
        b->m1a0 = res_get(n);
    }
    {
        tstring n("button_drop_child_hl");
        b->m1a8 = res_get(n);
    }

    // one full expression: both key strings live until d25 returns (the original destroys them after the call)
    vt(b)->d25(v2_ref(V2(config_float_key(tstring("combobox_button_size_x")),
                         config_float_key(tstring("combobox_button_size_y")))));

    b->f1b0 = config_float("combobox_child_button_size_x", 0.0f);
    b->f1b4 = config_float("combobox_child_button_size_y", 0.0f);
    b->f1b8 = config_float("combobox_child_button_position_x", 0.0f);

    {
        V2 ts(config_float_key(tstring("combobox_child_button_text_size_x")),     // both keys live to the end of the declaration
              config_float_key(tstring("combobox_child_button_text_size_y")));
        b->f1bc = ts.x;
        b->f1c0 = ts.y;
    }

    {
        V2 tp(config_float_key(tstring("combobox_child_button_text_position_x")),     // both keys live to the end of the declaration
              config_float_key(tstring("combobox_child_button_text_position_y")));
        b->f1c4 = tp.x;
        b->f1c8 = tp.y;
    }

    {
        float text_size = config_float("combobox_text_size", 0.0f);
        V2 tsize(text_size, text_size);
        vt(b)->d27(&tsize);
    }

    vt(b)->d28(v2_ref(V2(config_float_key(tstring("combobox_text_position_x")),
                         config_float_key(tstring("combobox_text_position_y")))));

    {
        rgl::vector4 out;
        vt(b)->d41(&out, (void*)name);
    }
    vt(parent)->d50(b);
    return b;
}

// ---- 0x4de6f0: OptionsScreen::open ----------------------------------------------------------------------------------------
// Every widget of the original in order: the overlay window, its header logo, the three buttons (ui_controls / ui_video_options /
// ui_done), the difficulty sliders and drop-downs, the 19 hint labels and the "ui_enabled" toggles, with the original callee
// prototypes (make_hint 0x4dada0, new_medium_button 0x4dce70, make_slider 0x4dfd50, make_dropdown 0x4dfec0).
#define set_enabled_text(o) ((KeysPage*)this)->set_enabled_text((MissionUi*)(o))   // 0x4d6dd0
#define update_perf(w) ((KeysPage*)(w))->update_perf()                          // 0x4d6b80
#define COLOR_ENABLED 0xffdfcfbfu   // the ui colour word every label of the screen is tinted with (edi in the original)

// @ 0x4de6f0
void Recd8::init_screen()
{
    g_gui.reset();
    root = new NodeA(0);
    {
        float p[2];
        p[0] = -0.001f; p[1] = -0.001f;
        vt(root)->d22(p);
    }
    {
        float s[2];
        s[0] = 1.002f; s[1] = 0.752f;
        vt(root)->d25(s);
    }
    {
        tstring k("");
        float o_[2];
        vt(root)->d41(o_, &k);
    }
    {
        unsigned c = COLOR_ENABLED;
        vt(root)->d38(&c);
    }
    {
        rgl::mesh* m;
        {
            tstring k("options_window");
            m = res_get(k)->clone();
        }
        vt(root)->d31(0, m);
    }
    header = new MissionUi;
    {
        float p[2];
        p[0] = 0.4f; p[1] = 0.67f;
        vt(header)->d22(p);
    }
    {
        float s[2];
        s[0] = 0.2f; s[1] = 0.05f;
        vt(header)->d25(s);
    }
    {
        tstring k("warrider_logo");
        rgl::mesh* m = res_get(k)->clone();
        vt(header)->d31(0, m);
    }
    vt(root)->d50(header);
    { tstring k("ui_controls"); b38 = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.1f, 0.07f, 0.05f); }
    { tstring k("ui_video_options"); b3c = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.1f, 0.14f, 0.05f); }
    { tstring k("ui_done"); b40 = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.65f, 0.03f, 0.05f); }
    s7c = make_slider((MissionUi*)root, 0.23f, 0.605f, 1.0f);
    vt(s7c)->d13(g_options.music_volume);
    s80 = make_slider((MissionUi*)root, 0.23f, 0.555f, 1.0f);
    vt(s80)->d13(g_options.master_volume);
    s84 = make_slider((MissionUi*)root, 0.23f, 0.455f, 1.0f);
    vt(s84)->d13(g_options.exp_arg);
    { tstring k("ui_enabled"); b88 = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.23f, 0.405f, 0.03f); }
    ((UiA*)b88)->f184 = 1;
    vt(b88)->d11(g_options.invert);
    set_enabled_text(b88);
    { tstring k("Combat dif"); dd44 = make_dropdown((MissionUi*)root, &k, 0.7f, 0.605f); }
    { tstring k("ui_reduced_to_1_over_4_easiest"); vt(dd44)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_reduced_to_1_over_2_easy"); vt(dd44)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_normal"); vt(dd44)->d50(g_strtable_c.lookup(k)); }
    vt(dd44)->d11(g_options.dmg_to_player);
    { tstring k("Friend combat dif"); dd48 = make_dropdown((MissionUi*)root, &k, 0.7f, 0.555f); }
    { tstring k("ui_reduced_to_1_over_2_easiest"); vt(dd48)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_reduced_to_3_over_4_easy"); vt(dd48)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_normal"); vt(dd48)->d50(g_strtable_c.lookup(k)); }
    vt(dd48)->d11(g_options.dmg_to_friends);
    { tstring k("Combat ai"); dd4c = make_dropdown((MissionUi*)root, &k, 0.7f, 0.505f); }
    { tstring k("ui_good"); vt(dd4c)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_average_caps"); vt(dd4c)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_poor"); vt(dd4c)->d50(g_strtable_c.lookup(k)); }
    vt(dd4c)->d11(g_options.difficulty);
    { tstring k("Combat speed"); dd50 = make_dropdown((MissionUi*)root, &k, 0.7f, 0.455f); }
    { tstring k("ui_slower"); vt(dd50)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_normal"); vt(dd50)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_faster"); vt(dd50)->d50(g_strtable_c.lookup(k)); }
    vt(dd50)->d11(g_options.agent_count);
    { tstring k(""); dd54 = make_dropdown((MissionUi*)root, &k, 0.23f, 0.305f); }
    { tstring k("ui_automatic_recommended"); vt(dd54)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_by_mouse_movement"); vt(dd54)->d50(g_strtable_c.lookup(k)); }
    vt(dd54)->d11(g_options.stick_mode);
    { tstring k(""); dd58 = make_dropdown((MissionUi*)root, &k, 0.23f, 0.255f); }
    { tstring k("ui_by_relative_enemy_position"); vt(dd58)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_by_mouse_movement"); vt(dd58)->d50(g_strtable_c.lookup(k)); }
    { tstring k("ui_by_inverse_mouse_movement"); vt(dd58)->d50(g_strtable_c.lookup(k)); }
    vt(dd58)->d11(g_options.ctl_mode);
    sl78 = make_slider((MissionUi*)root, 0.67f, 0.405f, 1.0f);
    vt(sl78)->d13(g_options.f74);
    { tstring k(""); h34 = make_hint((ContainerBase*)root, k, 0.93f, 0.4f, 0.05f, 0.035f); }
    { unsigned c = 0xffffffff; vt(h34)->d38(&c); }
    ((QuestPage*)this)->update_days();
    { tstring k("ui_music_volume"); h8c = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.05f, 0.6f, 0.16f, 0.035f); }
    { int v = 4; vt(h8c)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(h8c)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(h8c)->d37(&c); }
    { tstring k("ui_sound_volume"); h90 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.05f, 0.55f, 0.16f, 0.035f); }
    { int v = 4; vt(h90)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(h90)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(h90)->d37(&c); }
    { tstring k("ui_mouse_sensitivity"); h94 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.05f, 0.45f, 0.16f, 0.035f); }
    { int v = 4; vt(h94)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(h94)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(h94)->d37(&c); }
    { tstring k("ui_invert_mouse_y_axis"); h98 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.05f, 0.4f, 0.16f, 0.035f); }
    { int v = 4; vt(h98)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(h98)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(h98)->d37(&c); }
    { tstring k("ui_control_block_direction"); h9c = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.05f, 0.3f, 0.16f, 0.035f); }
    { int v = 4; vt(h9c)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(h9c)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(h9c)->d37(&c); }
    { tstring k("ui_control_attack_direction"); ha0 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.05f, 0.25f, 0.16f, 0.035f); }
    { int v = 4; vt(ha0)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(ha0)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(ha0)->d37(&c); }
    { tstring k("ui_damage_to_player"); ha4 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.6f, 0.16f, 0.035f); }
    { int v = 4; vt(ha4)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(ha4)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(ha4)->d37(&c); }
    { tstring k("ui_damage_to_friends"); ha8 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.55f, 0.16f, 0.035f); }
    { int v = 4; vt(ha8)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(ha8)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(ha8)->d37(&c); }
    { tstring k("ui_combat_ai"); hac = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.5f, 0.16f, 0.035f); }
    { int v = 4; vt(hac)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hac)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hac)->d37(&c); }
    { tstring k("ui_combat_speed"); hb0 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.45f, 0.16f, 0.035f); }
    { int v = 4; vt(hb0)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hb0)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hb0)->d37(&c); }
    { tstring k("ui_battle_size"); hb4 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.4f, 0.12f, 0.035f); }
    { int v = 4; vt(hb4)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hb4)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hb4)->d37(&c); }
    { tstring k("ui_show_attack_direction"); hb8 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.34f, 0.3f, 0.035f); }
    { int v = 4; vt(hb8)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hb8)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hb8)->d37(&c); }
    { tstring k("ui_show_targeting_reticule"); hbc = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.3f, 0.3f, 0.035f); }
    { int v = 4; vt(hbc)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hbc)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hbc)->d37(&c); }
    { tstring k("ui_show_names_of_friendly_troops"); hc0 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.26f, 0.3f, 0.035f); }
    { int v = 4; vt(hc0)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hc0)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hc0)->d37(&c); }
    { tstring k("ui_report_damage"); hc4 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.22f, 0.3f, 0.035f); }
    { int v = 4; vt(hc4)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hc4)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hc4)->d37(&c); }
    { tstring k("ui_report_shot_difficulty"); hc8 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.18f, 0.3f, 0.035f); }
    { int v = 4; vt(hc8)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hc8)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hc8)->d37(&c); }
    { tstring k("ui_report_casualties"); hcc = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.14f, 0.3f, 0.035f); }
    { int v = 4; vt(hcc)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hcc)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hcc)->d37(&c); }
    { tstring k("ui_report_experience"); hd0 = make_hint((ContainerBase*)root, *g_strtable_c.lookup(k), 0.51f, 0.1f, 0.3f, 0.035f); }
    { int v = 4; vt(hd0)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hd0)->d38(&c); }
    { unsigned c = COLOR_ENABLED; vt(hd0)->d37(&c); }
    { tstring k("ui_enabled"); b60 = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.83f, 0.345f, 0.035f); }
    ((UiA*)b60)->f184 = 1;
    vt(b60)->d11(g_options.i68);
    set_enabled_text(b60);
    { tstring k("ui_enabled"); b5c = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.83f, 0.305f, 0.035f); }
    ((UiA*)b5c)->f184 = 1;
    vt(b5c)->d11(g_options.i64);
    set_enabled_text(b5c);
    { tstring k("ui_enabled"); b64 = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.83f, 0.265f, 0.035f); }
    ((UiA*)b64)->f184 = 1;
    vt(b64)->d11(g_options.fx_on);
    set_enabled_text(b64);
    { tstring k("ui_enabled"); b68 = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.83f, 0.225f, 0.035f); }
    ((UiA*)b68)->f184 = 1;
    vt(b68)->d11(g_options.damage_msgs);
    set_enabled_text(b68);
    { tstring k("ui_enabled"); b6c = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.83f, 0.185f, 0.035f); }
    ((UiA*)b6c)->f184 = 1;
    vt(b6c)->d11(g_options.shot_difficulty);
    set_enabled_text(b6c);
    { tstring k("ui_enabled"); b70 = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.83f, 0.145f, 0.035f); }
    ((UiA*)b70)->f184 = 1;
    vt(b70)->d11(g_options.kill_msgs);
    set_enabled_text(b70);
    { tstring k("ui_enabled"); b74 = new_medium_button((MissionUi*)root, g_strtable_c.lookup(k), 0.83f, 0.105f, 0.035f); }
    ((UiA*)b74)->f184 = 1;
    vt(b74)->d11(g_options.show_messages);
    set_enabled_text(b74);
    { tstring k(""); hd4 = make_hint((ContainerBase*)root, k, 0.34f, 0.03f, 0.3f, 0.04f); }
    { int v = 4; vt(hd4)->d29(&v); }
    { unsigned c = COLOR_ENABLED; vt(hd4)->d38(&c); }
    update_difficulty();

    {
        void* w = root;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(w));
    }
    if (g_game)
        g_game->sl_update_avg();
}


// ---- 0x4e05c0: OptionsWin::init (the video-options page) -----------------------------------------------------------------------
// Every widget of the original in order, with its callee prototypes (make_hint 0x4dada0, new_medium_button 0x4dce70, new_sized_button
// 0x4dcc50, make_slider 0x4dfd50, make_dropdown 0x4dfec0): the overlay window, the done button, the gamma string (g_options.scale_x*100 shown
// as "2.20", or "ui_default" for the default) used for two labels, 25 text labels in two columns, update_perf, the change button, the
// resolution drop-down (filled from list_modes), the gamma / texture-detail / ragdoll-limit / y / z sliders, the shadow, corpse, ragdoll,
// blood and plant-shadow drop-downs, six toggle buttons (quality) and the shader-less tail that hides the gamma widgets.
#define SL_MIN(w) (*(float*)((char*)(w) + 0x16c))   // slider range minimum (MissionUi +0x16c)
#define SL_MAX(w) (*(float*)((char*)(w) + 0x170))   // slider range maximum
static __forceinline int drop_count(DropDown* d) { return (int)d->rows.size(); }
// the three calls every label gets: width (+0x74), colour (+0x98), mesh colour (+0x94), all with COLOR_ENABLED
#define LABEL_STYLE(m, wd) \
    { int v_ = (wd); vt(m)->d29(&v_); } \
    { unsigned c_ = COLOR_ENABLED; vt(m)->d38(&c_); } \
    { unsigned c_ = COLOR_ENABLED; vt(m)->d37(&c_); }
#define LABEL(m, key, x, y, w, h) \
    { tstring k_(key); m = make_hint(window, *g_strtable_c.lookup(k_), x, y, w, h); } \
    LABEL_STYLE(m, 4)
// a toggle button: "ui_enabled" text, the two button textures, flag 1, the current value of an option, then the "enabled"/"disabled" caption
#define QBTN(m, y, getval) \
    { tstring k_("ui_enabled"); tstring* t_ = g_strtable_c.lookup(k_); tstring d_("medium_button_down"); tstring u_("medium_button"); \
      m = new_sized_button((MissionUi*)window, *t_, 0.52f, y, 0.035f, u_, d_, 3.7f, 0.072f, 0.085f); } \
    ((UiA*)m)->f184 = 1; \
    vt(m)->d11(getval); \
    set_enabled_text(m);
#define DD_ITEM(dd, key) { tstring k_(key); vt(dd)->d50(g_strtable_c.lookup(k_)); }
#define DD_TEXT(dd, text) { tstring t_(text); vt(dd)->d50(&t_); }

// @ 0x4e05c0
void Recf0::init_screen()
{
    f_ec = 0;
    f_ed = 0;
    day_saved = g_cfg.texture_detail;
    g_gui.reset();
    window = new NodeA(0);
    {
        float p[2];
        p[0] = -0.001f; p[1] = -0.001f;
        vt(window)->d22(p);
    }
    {
        float s[2];
        s[0] = 1.002f; s[1] = 0.752f;
        vt(window)->d25(s);
    }
    {
        tstring k("");
        float o_[2];
        vt(window)->d41(o_, &k);
    }
    {
        unsigned c = COLOR_ENABLED;
        vt(window)->d38(&c);
    }
    {
        rgl::mesh* m;
        {
            tstring k("options_window");
            m = res_get(k)->clone();
        }
        vt(window)->d31(0, m);
    }
    { tstring k("ui_done"); done = new_medium_button((MissionUi*)window, g_strtable_c.lookup(k), 0.75f, 0.02f, 0.05f); }

    // the gamma as text: g_options.scale_x * 100 with a point after the first digit; the default 2.20 is shown as "ui_default"
    char buf[0x80];
    _itoa((int)(g_options.scale_x * 100.0f), buf, 10);
    tstring gamma_text(buf);
    { tstring dot("."); gamma_text.insert(1, dot); }
    if (gamma_text.equals("2.20")) {
        tstring k("ui_default");
        gamma_text = *g_strtable_c.lookup(k);
    }

    // the first label starts as the gamma text, then d41 replaces it with the DirectX version
    val_vertex_shaders = make_hint(window, gamma_text, 0.52f, 0.7f, 0.3f, 0.035f);
    {
        int v = 4;
        vt(val_vertex_shaders)->d29(&v);
        unsigned c[2];
        c[0] = COLOR_ENABLED;
        vt(val_vertex_shaders)->d38(c);
        {
            tstring s(g_cfg.use_vertex_shaders ? "DirectX 9" : "DirectX 7");
            vt(val_vertex_shaders)->d41(c, &s);   // c doubles as the out buffer
        }
        vt(val_vertex_shaders)->d37(c);           // ... and is what d37 receives next (the original reuses the slot)
    }
    LABEL(lbl_vertex_shaders, "ui_enable_vertex_shaders2", 0.2f, 0.7f, 0.3f, 0.035f)
    LABEL(lbl_screen_size, "ui_screen_size2", 0.2f, 0.66f, 0.3f, 0.035f)
    LABEL(lbl_gamma, "ui_gamma", 0.2f, 0.62f, 0.3f, 0.035f)
    val_gamma = make_hint(window, gamma_text, 0.823f, 0.615f, 0.3f, 0.035f);
    LABEL_STYLE(val_gamma, 4)
    LABEL(lbl_shadow_quality, "ui_shadow_quality2", 0.2f, 0.58f, 0.3f, 0.035f)
    LABEL(lbl_texture_detail, "ui_texture_detail", 0.2f, 0.54f, 0.3f, 0.035f)
    {
        {
            tstring k("ui_percentage_value");
            sprintf(buf, g_strtable_c.lookup(k)->c_str(), g_cfg.texture_detail);
        }
        tstring t(buf);
        val_texture_detail = make_hint(window, t, 0.823f, 0.535f, 0.3f, 0.035f);
    }
    LABEL_STYLE(val_texture_detail, 4)
    LABEL(lbl_grass_density, "ui_grass_density", 0.2f, 0.5f, 0.3f, 0.035f)
    {
        {
            tstring k("ui_percentage_value");
            sprintf(buf, g_strtable_c.lookup(k)->c_str(), g_options.i34);
        }
        tstring t(buf);
        val_grass_density = make_hint(window, t, 0.823f, 0.495f, 0.3f, 0.035f);
    }
    LABEL_STYLE(val_grass_density, 4)
    LABEL(lbl_char_detail, "ui_character_detail", 0.2f, 0.46f, 0.3f, 0.035f)
    LABEL(lbl_char_low, "ui_low", 0.52f, 0.45f, 0.03f, 0.05f)
    LABEL(lbl_char_high, "ui_high", 0.823f, 0.45f, 0.03f, 0.05f)
    LABEL(lbl_char_shadow_detail, "ui_character_shadow_detail", 0.2f, 0.42f, 0.3f, 0.035f)
    LABEL(lbl_shadow_low, "ui_low", 0.52f, 0.41f, 0.03f, 0.05f)
    LABEL(lbl_shadow_high, "ui_high", 0.823f, 0.41f, 0.03f, 0.05f)
    LABEL(lbl_corpses, "ui_number_of_corpses", 0.2f, 0.38f, 0.3f, 0.035f)
    LABEL(lbl_ragdolls, "ui_number_of_ragdolls", 0.2f, 0.34f, 0.3f, 0.035f)
    LABEL(lbl_blood_stains, "ui_blood_stains", 0.2f, 0.3f, 0.3f, 0.035f)
    LABEL(lbl_dynamic_lighting, "ui_dynamic_lighting", 0.2f, 0.26f, 0.3f, 0.035f)
    LABEL(lbl_character_shadows, "ui_character_shadows", 0.2f, 0.22f, 0.3f, 0.035f)
    LABEL(lbl_environment_shadows, "ui_environment_shadows", 0.2f, 0.18f, 0.3f, 0.035f)
    LABEL(lbl_realistic_shadows, "ui_realistic_shadows_on_plants", 0.2f, 0.14f, 0.3f, 0.035f)
    LABEL(lbl_particle_systems, "ui_particle_systems", 0.2f, 0.1f, 0.3f, 0.035f)
    LABEL(lbl_anisotropic, "ui_anisotropic_filtering", 0.2f, 0.06f, 0.3f, 0.035f)
    LABEL(lbl_fast_water, "ui_fast_water_reflection", 0.2f, 0.02f, 0.3f, 0.035f)
    { tstring k(""); quality_label = make_hint(window, k, 0.68f, 0.08f, 0.3f, 0.035f); }
    LABEL_STYLE(quality_label, 16)

    update_perf(this);

    { tstring k("ui_change"); change = new_medium_button((MissionUi*)window, g_strtable_c.lookup(k), 0.65f, 0.705f, 0.035f); }

    {   // resolution
        { tstring k("Resolution"); resolution = make_dropdown((MissionUi*)window, &k, 0.52f, 0.665f); }
        // The original passes 3-argument vector4 temporaries (w never set, 0x4e1751 / 0x4e1797 / 0x4e17dd copy the temp's
        // unwritten 4th float); set_color 0x47af30 reads only x, y, z
        ((rgl::mesh*)resolution->tracks[2])->set_color(rgl::vector4(0.7f, 0.7f, 1.0f));
        ((rgl::mesh*)resolution->tracks[1])->set_color(rgl::vector4(0.7f, 0.7f, 1.0f));
        ((rgl::mesh*)resolution->tracks[3])->set_color(rgl::vector4(0.7f, 0.7f, 1.0f));
        float sc[2];
        sc[0] = 0.0139999995f;   // 0x3c656041 (0.014f would be 0x3c656042)
        sc[1] = 0.0139999995f;
        vt(resolution)->d27(sc);
        sc[0] = 0.0139999995f;
        sc[1] = 0.00805f;
        vt(resolution)->d28(sc);
        vt(resolution)->d03();
        resolution->fD8 = 0.7f;
        resolution->fDC = 0.7f;
        resolution->e[0] -= 0.00528f;
        resolution->e[1] -= 0.00528f;

        std::vector<tstring> modes = list_modes();
        for (int i = (int)modes.size() - 1; i >= 0; i--) {
            DisplayMode& m = *(DisplayMode*)&modes[i];
            bool ok = false;
            if (g_cfg.display_bits == 16)
                ok = m.name.find(tstring("16 bits"), 0) >= 0;
            if (!ok)
                ok = m.name.find(tstring("32 bits"), 0) >= 0;
            if (ok) {
                vt(resolution)->d50(&m.name);
                char mb[0x80];
                sprintf(mb, "%dx%d", g_cfg.display_width, g_cfg.display_height);
                ok = m.name.find(tstring(mb), 0) >= 0;
                if (ok)
                    vt(resolution)->d11(drop_count(resolution) - 1);
            }
        }
        DD_ITEM(resolution, "ui_use_desktop_resolution2")
        if (g_cfg.display_width == 0)
            vt(resolution)->d11(drop_count(resolution) - 1);
    }

    ui_scale = make_slider((MissionUi*)window, 0.56f, 0.625f, 1.0f);
    SL_MIN(ui_scale) = 1.5f;
    SL_MAX(ui_scale) = 2.5f;
    vt(ui_scale)->d13(g_options.scale_x);

    {   // shadow quality
        { tstring k("ShadowQ"); shadow_quality = make_dropdown((MissionUi*)window, &k, 0.52f, 0.585f); }
        DD_ITEM(shadow_quality, "ui_m_ultra_high2")
        DD_ITEM(shadow_quality, "ui_m_high2")
        DD_ITEM(shadow_quality, "ui_m_low2")
        vt(shadow_quality)->d11(2 - g_cfg.shadow_quality_req);   // 0x5aa004
    }

    days = make_slider((MissionUi*)window, 0.56f, 0.545f, 1.0f);
    SL_MIN(days) = 10.0f;
    SL_MAX(days) = 100.0f;
    vt(days)->d13((float)g_cfg.texture_detail);

    ragdoll_limit = make_slider((MissionUi*)window, 0.56f, 0.505f, 1.0f);
    SL_MIN(ragdoll_limit) = 0.0f;
    SL_MAX(ragdoll_limit) = 100.0f;
    vt(ragdoll_limit)->d13((float)g_options.i34);

    y_scale = make_slider((MissionUi*)window, 0.56f, 0.465f, 1.0f);
    vt(y_scale)->d13(g_options.scale_y);

    z_scale = make_slider((MissionUi*)window, 0.56f, 0.425f, 1.0f);
    vt(z_scale)->d13(g_options.scale_z);

    {   // corpses (0x5aa008)
        { tstring k("Corpses"); corpses = make_dropdown((MissionUi*)window, &k, 0.52f, 0.385f); }
        DD_ITEM(corpses, "ui_unlimited")
        DD_TEXT(corpses, "40")
        DD_TEXT(corpses, "20")
        DD_TEXT(corpses, "10")
        DD_TEXT(corpses, "5")
        DD_TEXT(corpses, "0")
        vt(corpses)->d11(5 - g_options.i00);
    }
    {   // ragdolls (0x5aa00c)
        { tstring k("Ragdolls"); ragdolls = make_dropdown((MissionUi*)window, &k, 0.52f, 0.345f); }
        DD_ITEM(ragdolls, "ui_unlimited")
        DD_TEXT(ragdolls, "10")
        DD_TEXT(ragdolls, "5")
        DD_TEXT(ragdolls, "3")
        DD_TEXT(ragdolls, "1")
        DD_TEXT(ragdolls, "0")
        vt(ragdolls)->d11(5 - g_options.i04);
    }
    {   // blood (0x5aa020)
        { tstring k("Blood"); blood = make_dropdown((MissionUi*)window, &k, 0.52f, 0.305f); }
        DD_ITEM(blood, "ui_off2")
        DD_ITEM(blood, "ui_near_player_only")
        DD_ITEM(blood, "ui_on")
        vt(blood)->d11(g_options.blood);
    }

    QBTN(quality[0], 0.265f, g_options.char_detail)
    QBTN(quality[1], 0.225f, g_options.i28)
    QBTN(quality[2], 0.185f, g_options.i2c)
    {   // plant shadows (0x5aa084)
        { tstring k("PlantShadows"); plant_shadows = make_dropdown((MissionUi*)window, &k, 0.52f, 0.145f); }
        DD_ITEM(plant_shadows, "ui_none")
        DD_ITEM(plant_shadows, "ui_quick")
        DD_ITEM(plant_shadows, "ui_full")
        vt(plant_shadows)->d11(g_options.i7c);
    }
    QBTN(quality[3], 0.105f, g_options.i20)
    QBTN(quality[4], 0.065f, g_options.i38)
    QBTN(quality[5], 0.025f, g_cfg.fake_reflections)

    {
        void* w = window;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(w));
    }
    if (!g_use_shaders) {
        vt(ui_scale)->d44();
        vt(lbl_gamma)->d44();
        vt(val_gamma)->d44();
    }
}

// ---- 0x4e25a0: the key-binding page -----------------------------------------------------------------------------------------------
// @ 0x4e25a0
void Rec1bc::init_screen()
{
    f1b8 = -1;
    g_gui.reset();
    quad = new NodeA(0);
    float f2[2];
    f2[0] = -0.001f;
    f2[1] = -0.001f;
    vt(quad)->d22(f2);
    f2[0] = 1.002f;
    f2[1] = 0.752f;
    vt(quad)->d25(f2);
    int v = -1;
    vt(quad)->d38(&v);
    {
        tstring name("options_window");
        rgl::mesh* c = res_get(name)->clone();
        vt(quad)->d31(0, c);
    }
    {
        tstring t("ui_set_default_keys");
        f38 = new_medium_button(quad, g_strtable_c.lookup(t), 0.82f, 0.15f, 0.04f);
    }
    {
        tstring t("ui_undo_changes");
        f34 = new_medium_button(quad, g_strtable_c.lookup(t), 0.82f, 0.1f, 0.04f);
    }
    {
        tstring t("ui_done");
        f30 = new_medium_button(quad, g_strtable_c.lookup(t), 0.82f, 0.05f, 0.04f);
    }
    {
        tstring t("ui_press_a_key");
        f164 = make_hint(quad, *g_strtable_c.lookup(t), 0.75f, 0.25f, 0.2f, 0.06f);
    }
    vt(f164)->d44();
    int iv;
    {
        tstring t("ui_battle_controls");
        f168 = make_hint(quad, *g_strtable_c.lookup(t), 0.25f, 0.705f, 0.3f, 0.04f);
    }
    iv = 0x10;
    vt(f168)->d29(&iv);
    iv = -1;
    vt(f168)->d38(&iv);
    {
        tstring t("ui_map_controls");
        f16c = make_hint(quad, *g_strtable_c.lookup(t), 0.7f, 0.705f, 0.3f, 0.04f);
    }
    iv = 0x10;
    vt(f16c)->d29(&iv);
    iv = -1;
    vt(f16c)->d38(&iv);
    {
        tstring t("ui_general_controls");
        f1a8 = make_hint(quad, *g_strtable_c.lookup(t), 0.7f, 0.355f, 0.3f, 0.04f);
    }
    iv = 0x10;
    vt(f1a8)->d29(&iv);
    iv = -1;
    vt(f1a8)->d38(&iv);
    for (int i = 0; i < 0x25; i++) {
        float x = 0.01f;
        float c = 0.677f - (float)(i % 0x13) * 0.037f;
        if (i >= 0x13)
            x = 0.35f;
        MissionUi* tb = make_hint(quad, ((OptionsMenu*)this)->slot_text(i), x, c, 0.185f, 0.03f);
        slots[i] = tb;
        iv = 8;
        vt(tb)->d29(&iv);
        iv = 0xfff0f0f0;
        vt(tb)->d38(&iv);
        tstring txt("          ");
        tstring up("medium_button");
        tstring down("medium_button_down");
        MissionUi* b = new_sized_button(quad, txt, x + 0.195f, c + 0.005f, 0.034f, up, down, 3.7f, 0.072f, 0.085f);
        slot_buttons[i] = b;
        ((UiA*)b)->f184 |= 1;
    }
    for (int i = 0; i < 7; i++) {
        float c = 0.677f - (float)i * 0.037f;
        MissionUi* tb = make_hint(quad, ((OptionsMenu*)this)->key_text(i), 0.72f, c, 0.13f, 0.03f);
        captions[i] = tb;
        iv = 8;
        vt(tb)->d29(&iv);
        iv = 0xfff0f0f0;
        vt(tb)->d38(&iv);
        tstring t;
        g_game_keys.key_name(i, &t);
        MissionUi* b = make_hint(quad, t, 0.86f, c, 0.12f, 0.03f);
        key_values[i] = b;
        iv = 0x10;
        vt(b)->d29(&iv);
        iv = 0xfff0f0f0;
        vt(b)->d38(&iv);
    }
    {
        tstring t("ui_take_screenshot");
        f1ac = make_hint(quad, *g_strtable_c.lookup(t), 0.72f, 0.327f, 0.13f, 0.03f);
    }
    iv = 8;
    vt(f1ac)->d29(&iv);
    iv = 0xfff0f0f0;
    vt(f1ac)->d38(&iv);
    {
        tstring s("Ctrl + Insert");
        f1b0 = make_hint(quad, s, 0.86f, 0.327f, 0.12f, 0.03f);
    }
    iv = 0x10;
    vt(f1b0)->d29(&iv);
    iv = 0xfff0f0f0;
    vt(f1b0)->d38(&iv);
    void* q = (void*)quad;
    g_gui.dirty = 1;
    g_gui.objs.push_back((MissionUi* const&)(q));
    g_game_keys.backup();
    f1b4 = 0;
    ((KeysPage*)this)->refresh();
}

// @ 0x4e2cb0
void GameKeys::backup()
{
    // 37 pairs of dwords at this+0 (the first key word included, as in the original) are copied to this+0x128
    int* p = (int*)this + 1;
    int n = 0x25;
    do {
        p[0x49] = p[-1];
        p[0x4a] = p[0];
        p += 2;
    } while (--n != 0);
}

// @ 0x4e2ce0  tstring key_text(int i)  (a tstring returned by value: the hidden out pointer is the first stack
// argument and the epilogue copy-constructs r into it, 0x4e2e55)
tstring OptionsMenu::key_text(int i)
{
    tstring r("unknown");
    switch (i) {
    case 0: {
        tstring k("ui_move_forward");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 1: {
        tstring k("ui_move_backward");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 2: {
        tstring k("ui_move_left");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 3: {
        tstring k("ui_move_right");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 4: {
        tstring k("ui_zoom_in");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 5: {
        tstring k("ui_zoom_out");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 6: {
        tstring k("ui_wait");
        r = *g_strtable_c.lookup(k);
        break;
    }
    }
    return r;
}

// @ 0x4e2ea0    tstring slot_text(int i)  (same return-by-value convention as key_text)
tstring OptionsMenu::slot_text(int i)
{
    tstring r("unknown");
    switch (i) {
    case 0: {
        tstring k("ui_move_forward");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 1: {
        tstring k("ui_move_backward");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 2: {
        tstring k("ui_move_left");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 3: {
        tstring k("ui_move_right");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 4: {
        tstring k("ui_action");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 5: {
        tstring k("ui_jump");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 6: {
        tstring k("ui_attack");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 7: {
        tstring k("ui_defend");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 8: {
        tstring k("ui_equip_next_weapon");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 9: {
        tstring k("ui_equip_next_shield");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 10: {
        tstring k("ui_sheath_weapon");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 11: {
        tstring k("ui_leave_location_retreat");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 12: {
        tstring k("ui_zoom");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 13: {
        tstring k("ui_view_outfit");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 14: {
        tstring k("ui_toggle_first_person_view");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 15: {
        tstring k("ui_character_window");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 16: {
        tstring k("ui_inventory_window");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 17: {
        tstring k("ui_party_window");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 18: {
        tstring k("ui_quests_window");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 19: {
        tstring k("ui_game_log_window");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 20: {
        tstring k("ui_view_orders");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 21: {
        tstring k("ui_order_hold");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 22: {
        tstring k("ui_order_follow");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 23: {
        tstring k("ui_order_charge");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 24: {
        tstring k("ui_order_dismount_toggle");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 25: {
        tstring k("ui_order_hold_fire_toggle");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 26: {
        tstring k("ui_order_advance");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 27: {
        tstring k("ui_order_fall_back");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 28: {
        tstring k("ui_order_stand_closer");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 29: {
        tstring k("ui_order_spread_out");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 30: {
        tstring k("ui_order_blunt_weapons_toggle");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 31: {
        tstring k("ui_everyone_control");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 32: {
        tstring k("ui_infantry_control");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 33: {
        tstring k("ui_archers_control");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 34: {
        tstring k("ui_cavalry_control");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 35: {
        tstring k("ui_others_control");
        r = *g_strtable_c.lookup(k);
        break;
    }
    case 36: {
        tstring k("ui_quick_save");
        r = *g_strtable_c.lookup(k);
        break;
    }
    }
    return r;
}

// ---- 0x4e3780: the meeting window (character sheet) ----------------------------------------------------------------------------------
// The original contains two 18-byte WinLicense instrumentation records (EB 10 'WL  ' ... 'WL  ') that cl 13.10 cannot emit.
extern float g_meeting_y0;                      // 0x591d44 (0.63f)
extern float g_meeting_dy;                      // 0x54b8b4 (0.03f)

// @ 0x4e3780
void Rec48::init_screen()
{
    g_gui.reset();
    bg = new NodeA(0);
    {
        float f2[2];
        f2[0] = 0.0f;
        f2[1] = 0.0f;
        vt(bg)->d22(f2);
    }
    {
        float f2[2];
        f2[0] = 1.0f;
        f2[1] = 0.75f;
        vt(bg)->d25(f2);
    }
    rgl::mesh* m;
    {
        tstring name("meeting_window");
        m = res_get(name)->clone();
    }
    vt(bg)->d31(0, m);
    {
        void* w = bg;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(w));
    }
    {
        WL_MARKER(12)
        tstring t("ui_done");
        done = new_medium_button(bg, g_strtable_c.lookup(t), 0.7f, 0.09f, 0.04f);
    }
    export_btn = 0;
    import_btn = 0;
    f40 = -1;
    f44 = -1;
    {
        WL_MARKER(13)
        tstring t("ui_export_character");
        export_btn = new_long_button(bg, g_strtable_c.lookup(t), 0.5f, 0.6f, 0.04f);
    }
    {
        tstring t("ui_import_character");
        import_btn = new_long_button(bg, g_strtable_c.lookup(t), 0.5f, 0.5f, 0.04f);
    }
    panel = new text_box;
    {
        float f2[2];
        f2[0] = 0.05f;
        f2[1] = 0.0f;
        vt(panel)->d22(f2);
    }
    {
        float f2[2];
        f2[0] = 0.43f;
        f2[1] = 0.63f;
        vt(panel)->d25(f2);
    }
    {
        float f2[2];
        f2[0] = 0.02f;
        f2[1] = 0.02f;
        vt(panel)->d27(f2);
    }
    {
        int iv = 0xff000000;
        vt(panel)->d38(&iv);
    }
    {
        tstring none("");
        float out[2];
        vt(panel)->d41(out, &none);
    }
    for (int i = 0; i < 10; i++) {
        g_script.result_string.assign("");
        g_game->emit_event(0xe, i, 0);
        float pos[2];
        pos[0] = 0.0f;
        pos[1] = 0.63f - (float)i * 0.03f;
        vt(panel)->d43(&g_script.result_string, pos);
    }
    vt(bg)->d50(panel);
}

// @ 0x4e3aa0
void C4c3290::init_screen()
{
    g_gui.reset();
    f44 = 10;
    f48 = 0;
    overlay = new NodeA(0);
    float f2[2];
    f2[0] = -0.001f;
    f2[1] = -0.001f;
    vt(overlay)->d22(f2);
    f2[0] = 1.002f;
    f2[1] = 0.752f;
    vt(overlay)->d25(f2);
    {
        tstring name("load_window");
        vt(overlay)->d31(0, res_get(name)->clone());
    }
    int iv = 0xff442323;
    vt(overlay)->d38(&iv);
    // the picture's name outlives the logo and the hint blocks (the original destroys it only after them)
    int t = rand() % 0x3dc9;
    float f = (float)t * 6.3223117e-005f;
    tstring pic("ini_pic_mb_warrior_1");
    if (f < 0.25f)
        pic.assign("ini_pic_mb_warrior_2");
    else if (f < 0.5f)
        pic.assign("ini_pic_mb_warrior_3");
    else if (f < 0.75f)
        pic.assign("ini_pic_mb_warrior_4");
    vt(overlay)->d32(2, res_get(pic)->clone());
    {
        MissionUi* logo_ = new MissionUi;
        logo = logo_;
        f2[0] = 0.05f;
        f2[1] = 0.6f;
        vt(logo_)->d22(f2);
        f2[0] = 0.4f;
        f2[1] = 0.1f;
        vt(logo_)->d25(f2);
        tstring name("warrider_logo");
        vt(logo_)->d31(0, res_get(name)->clone());
    }
    vt(overlay)->d50(logo);
    bar = new_progress_bar(overlay, 0.1f, 0.07f, 0.8f, 0.03f);
    {
        tstring t("");
        hint = make_hint(overlay, t, 0.13f, 0.12f, 0.3f, 0.04f);
    }
    {
        void* w = overlay;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(w));
    }
    step = 0;
    total = 0;
}

// ---- 0x4e3d50: the progress bar maker --------------------------------------------------------------------------------------------
// @ 0x4e3d50
progress_bar* new_progress_bar(ContainerBase* o, float x, float y, float a, float b)
{
    progress_bar* p = new progress_bar;
    p->idx = 0;
    float pos[2];
    pos[0] = x;
    pos[1] = y;
    vt(p)->d22(pos);
    // the size is the unscaled (a, b); the scaled pair (and 1.0) is the bar's colour and rect, whose w is never set
    // (0x4e3d9b..0x4e3dd8: [esp+0x10] = a, [esp+0x14] = b; a * 1.5576323, b * 50, 1.0 into the vector at [esp+0x20])
    float size[2];
    size[0] = a;
    size[1] = b;
    rgl::vector4 v;
    v.x = a * 1.5576323f;
    v.y = b * 50.0f;
    v.z = 1.0f;
    vt(p)->d25(size);
    p->ratio = v.y * 0.31f;
    rgl::mesh* c;
    {
        tstring name("progressbar");
        c = res_get(name)->clone();
    }
    c->set_color(v);
    {
        tstring name("progressbar_handle");
        p->handle = res_get(name);
    }
    p->rect = v;
    vt(p)->d32(0, c);
    vt(o)->d50(p);
    return p;
}

// ItemRec / ItemRec2 slot 2 (0x4dcef0 MainMenuScreen::create, 0x4dda40 EscapeMenu::create): the record IS the screen
// object those take in ecx, so the body forwards `this` unchanged and cl tail-calls the member.
