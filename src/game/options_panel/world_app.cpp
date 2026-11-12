// world_app.cpp: the mission object's constructor / destructor, restore_defaults, a hash-table insert, the
// application constructor and main window procedure (0x4d3720..0x4d4b80), and the vector fill helpers that sit
// after them (T-regime region: helpers right after the first caller). Names provisional.
// `Big` is the mission object g_mission (0x5e292c; Mission, ../mission.h) and `GameApp` is the application object
// (Core, ../config/core.h): their constructors / destructors are the only functions here, written against partial
// views because mission.h / core.h declare neither (see MIGRATION.md).
#include "../../include/image_addrs.h"
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
#include "../party/game.h"
#include "../../include/rec_dtors.h"
#include "../../include/skin.h"            // Skin (the destructor 0x4d3720 is defined here)
#include "../config/core.h"                 // GameApp : CD3DApplication, the application object (defined below, g_app_obj)

// std::vector<tstring> (_Tidy 0x42d390) is declared out of line in src/include/rec_dtors.h

// ---- callees -------------------------------------------------------------------------------------------
tstring g_module_path;                                          // 0x8ee3c28 (constructed by the compiler: tstring::tstring 0x42bbd0)
extern tstring g_module_name;                                      // 0x8ee3c38

#include "../../include/crt.h"   // canonical: operator delete (0x535712)
// The vector elements of mesh_2.cpp (E50 0x50, E68 0x68, E176 0xb0) and the std algorithms the original keeps out of line for them;
// the explicit specialization declarations (no bodies here) keep every one of them a call.
#include "./elem_calls.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/skinmagic.h"
namespace std {
template<> E176* _Copy_opt<E176*, E176*>(E176*, E176*, E176*, _Nonscalar_ptr_iterator_tag);   // 0x4cd620
template<> void _Destroy_range<E176, allocator<E176> >(E176*, E176*, allocator<E176>&, _Nonscalar_ptr_iterator_tag);   // 0x4ca7e0
template<> void _Destroy_range<E68, allocator<E68> >(E68*, E68*, allocator<E68>&, _Nonscalar_ptr_iterator_tag);   // 0x4cd570
template<> void vector<scalar4, allocator<scalar4> >::resize(size_t, scalar4);   // 0x4cd010
template<> void allocator<E50>::destroy(E50*);   // 0x4cd6a0
}
// 0x4727a0 is terrain_b's destroy_arrsets (ArrSet is records.h's 0x38-byte record), defined with two parameters; the original calls
// it as std::_Destroy_range(first, last, allocator&, tag) and the last two words are never read
#include "../arr_set.h"
void __cdecl destroy_arrsets(ArrSet* first, ArrSet* last);   // 0x4727a0
namespace std {
template<> inline void _Destroy_range<ArrSet, allocator<ArrSet> >(ArrSet* f, ArrSet* l, allocator<ArrSet>& al, _Nonscalar_ptr_iterator_tag t)   // 0x4727a0
{
    ((void (__cdecl*)(ArrSet*, ArrSet*, allocator<ArrSet>&, _Nonscalar_ptr_iterator_tag))destroy_arrsets)(f, l, al, t);
}
}
void* hash_new_node(void* node, void* hint, void* key);                   // 0x4c4e50
// 0x4c4e50 is rgl/mesh2/gui_edit.cpp's list_int_make_node: thiscall on the table's node list (ecx = this+4) with the
// original's (node, hint, key); gui_edit defines it __fastcall with an unused edx word, declared here as it is defined.
struct list_int;
void* __fastcall list_int_make_node(list_int* self, void* edx, void* node, void* hint, void* key);   // 0x4c4e50
// The sub-objects the mission constructor builds in place: the 0x130-byte mark records (rec_tables.h's C4c2d10, ctor 0x4c2d10),
// config.h's clip_table (ctor 0x4be6b0), module_load.h's growable_array (ctor 0x4d1360) and a tstring.
#include "../../rgl/mesh2/rec_tables.h"
#include "../config/config.h"
#include "./growable_array.h"
#include "../str_table.h"                                      // StrTable (0x5e2a18; was StrTab here: lookup 0x43a1e0)

// ---- globals -------------------------------------------------------------------------------------------
#include "../../include/fx.h"   // canonical: g_ticks (0x5e2958)
extern int g_pending_action;                                // 0x5a9f18
#include "../mission.h"   // canonical: g_mission (0x5e292c)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_pending_action = -1;                                  // 0x5a9f18 -1 = nothing pending

#include "../../include/render_flags.h"
#include "../window_mgr.h"                                     // g_windows (0xa73980)

// ---- the large record (all members are nominal; accesses are by offset) --------------------------------
struct Big {
    int  f000; int f004; int f008; int f00c;
    int  f010; int f014; int f018; int f01c;
    int  f020; int f024; int f028; int f02c;
    int  f030; int f034; int f038; int f03c;
    int  f040;                                       // +0x40
    int  f044; int f048; int f04c; int f050; int f054;
    int  f058;                                       // +0x58
    char pad05c[0x670 - 0x5c];
    char blocks[0x40 * 0x130];                      // +0x670 .. +0x5270
    char pad5270[0x5de0 - 0x5270];
    int  f5de0;
    char pad5de4[4];
    int  f5de8;
    int  f5dec;
    char pad5df0[0x5e00 - 0x5df0];
    int  f5e00; int f5e04; int f5e08;
    char pad5e0c[0x62e8 - 0x5e0c];
    int  f62e8;
    char pad62ec[4];
    int  f62f0; int f62f4; int f62f8;
    char pad62fc[4];
    int  f6300; int f6304; int f6308;
    char pad630c[4];
    int  f6310; int f6314; int f6318;
    char pad631c[4];
    int  f6320; int f6324; int f6328;
    char pad632c[4];
    int  f6330; int f6334;
    char pad6338[0x63c4 - 0x6338];
    int  f63c4; int f63c8; int f63cc;
    char f63d0[0x667c - 0x63d0];                    // +0x63d0 (a clip_table)
    int  f667c; int f6680; int f6684;
    char pad6688[4];
    int  f668c; int f6690; int f6694;
    char f6698[0x67d8 - 0x6698];                    // +0x6698 (a growable_array)
    int  f67d8;
    char pad67dc[4];
    int  f67e0; int f67e4;
    char pad67e8[0x6800 - 0x67e8];
    int  f6800;
    char pad6804[4];
    int  f6808; int f680c;
    char pad6810[4];
    char f6814[0x10];                               // +0x6814 (a tstring)
    char done[0x40];                                // +0x6824 spawn-done flags (Mission::done, ../mission.h)
    char pad6864[4];                                // Mission's __int64 members round its size up to 8: new 0x6868 (0x4d4083)
    Big* init();                                    // 0x4d3880
};

typedef char big_size_check[sizeof(Big) == 0x6868 ? 1 : -1];   // operator new(0x6868) at 0x4d4083

static inline void zero3(void* v) { int* q = (int*)v; q[0] = 0; q[1] = 0; q[2] = 0; }
static inline void start_timer2(void* clk, void* st) { *(int*)clk = 2; ((int*)st)[0] = ((int*)g_ticks)[4]; ((int*)st)[1] = ((int*)g_ticks)[5]; }

// @ 0x4d3880
Big* Big::init()
{
    Big* p = this;
    p->f00c = 0; p->f010 = 0; p->f014 = 0;
    p->f01c = 0; p->f020 = 0; p->f024 = 0;
    p->f004 = 0;
    p->f028 = 0; p->f02c = 0;
    p->f038 = 0; p->f03c = 0; p->f040 = 0;
    p->f048 = 0; p->f04c = 0; p->f050 = 0;
    p->f030 = 0;
    p->f054 = 0; p->f058 = 0;
    for (int i = 0; i < 0x40; i++)
        ((C4c2d10*)(p->blocks + i * 0x130))->C4c2d10::C4c2d10();

    start_timer2(&p->f5de0, &p->f5de8);
    p->f5e00 = 0; p->f5e04 = 0; p->f5e08 = 0;

    start_timer2(&p->f62e8, &p->f62f0);
    start_timer2(&p->f62f8, &p->f6300);
    start_timer2(&p->f6308, &p->f6310);
    start_timer2(&p->f6318, &p->f6320);
    start_timer2(&p->f6328, &p->f6330);
    zero3(&p->f63c4);
    ((clip_table*)p->f63d0)->clip_table::clip_table();

    zero3(&p->f667c);
    zero3(&p->f668c);
    ((growable_array*)p->f6698)->growable_array::growable_array();

    start_timer2(&p->f67d8, &p->f67e0);
    start_timer2(&p->f6800, &p->f6808);

    ((tstring*)p->f6814)->tstring::tstring();
    p->f000 = 0;
    *(int*)((char*)p + 0x5270) = 0;
    return this;
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
    g_game_G->f1a090 = 0;
    g_game_G->f1a098 = 0;
    g_game_G->f1a0b8 = -1;
    g_game_G->f1a0bc = -1;
    g_game_G->f1a0c0 = -1;
    g_game_G->f1a0c4 = -1;

    *(char*)(*(int*)(((char*)g_mission) + 0x65d0) + 0x140) = 0;
    *(char*)(*(int*)(((char*)g_mission) + 0x65dc) + 0x140) = 0;
    *(char*)(*(int*)(((char*)g_mission) + 0x65e4) + 0x140) = 0;
    if (*(int*)(((char*)g_mission) + 0x6738)) *(char*)(*(int*)(((char*)g_mission) + 0x6738) + 0x140) = 0;
    if (*(int*)(((char*)g_mission) + 0x6740)) *(char*)(*(int*)(((char*)g_mission) + 0x6740) + 0x140) = 0;

    if (g_game_G->f1a1cc) {
        g_game_G->f1a1cc = 0;
        g_pending_action = -1;
        {
            tstring s1("ui_no_discard_changes");
            tstring s2("ui_yes_save");
            tstring s3("ui_question_save_changes");
            const tstring* r1 = g_strtable.lookup(s1);
            const tstring* r2 = g_strtable.lookup(s2);
            const tstring* r3 = g_strtable.lookup(s3);
            NamedRec* ui = g_windows.box();
            ui->name68.clear();
            ui->name1c = *r3;
            ui->name78 = *r2;
            ui->name88 = *r1;
            ui->f4c = (int)&g_pending_action;
            ui->f50 = 0;
            ui->f04 = 1;
        }
        g_windows.open(0);
        g_game_G->f1a090 = 0;
    }
}

// ---- hash table ----------------------------------------------------------------------------------------
// The container helpers below take (first, last, owner, alloc). cl packs the two trailing pointer-sized
// args: the owner arg rides in ECX (still holding `this` at the call) -- see 0x4d3e50/0x4d3e70.
struct HNode {
    HNode* next;                                    // +0
    HNode* prev;                                    // +4
    unsigned key;                                   // +8
};
struct HOut { HNode* node; char inserted; };
struct list_node {                                  // mesh_2.cpp's list_node (grow 0x4cb6f0); lives at HashTab+4
    char pad0[4];
    HNode* sentinel;                                // +4 (HashTab+8)
    HNode* copy_node(HNode* n, HNode* hint, unsigned* key);   // 0x4c4e50
    void   grow(unsigned one);                   // 0x4cb6f0
};
struct HashTab {
    int      f0;
    list_node pool;                                  // +4
    int      f0c;                                   // +0xc
    char*    f10;                                   // +0x10
    HNode**  f14;                                   // +0x14
    HNode**  f18;                                   // +0x18
    HNode**  f1c;                                   // +0x1c
    unsigned f20;                                   // +0x20
    unsigned f24;                                   // +0x24
    void insert(HOut* out, unsigned* key);          // 0x4d3c00
};

// @ 0x4d3c00
void HashTab::insert(HOut* out, unsigned* key)
{
    unsigned count = f24;
    std::vector<scalar4>* vec = (std::vector<scalar4>*)(void*)((char*)this + 0x10);
    if (f24 <= (unsigned)f0c >> 2) {               // 0x4d3c0d: shr (unsigned)
        unsigned n = f14 ? (unsigned)(((char*)f18 - (char*)f14) >> 2) : 0u;
        if (!(count < n - 1)) {
            unsigned n2 = f14 ? (unsigned)(((char*)f18 - (char*)f14) >> 2) : 0u;
            f20 = n2 * 2 - 3;
            { scalar4 sen; sen.v = (int)pool.sentinel; vec->resize(n2 * 2 - 1, sen); }   // 0x4cd010
        } else {
            if (f20 < count)
                f20 = f20 * 2 + 1;
        }
        unsigned idx = count - (f20 >> 1) - 1;
        HNode* p = f14[idx];
        if (p != f14[idx + 1]) {
            HNode* q;
            do {
                if (((p->key ^ 0xdeadbeef) & f20) == idx) {
                    q = p->next;
                } else {
                    q = p->next;
                    if (pool.sentinel != q) {
                        unsigned u = idx;
                        while (p == f14[u]) {
                            f14[u] = q;
                            if (u == 0)
                                break;
                            u = u - 1;
                        }
                        HNode* s = pool.sentinel;
                        // stand-in: the original's std::list::_Splice size move (`if (this != &_Right)`, 0x4d3ce0
                        // `cmp ecx,ecx; je` over the call) is never taken here (one list); cl 13.10 folds that compare
                        // away (482 bytes), so an always-true pointer test keeps the call's shape. grow(0) is a no-op
                        // (0x4cb6f0: 0x7ffffff - size >= 0 unsigned, size += 0), so behaviour is unchanged.
                        list_node* pl = &pool;
                        if (pl) pl->grow(0);
                        *(HNode**)p->prev = q;
                        *(HNode**)q->prev = s;
                        *(HNode**)s->prev = p;
                        HNode* t = s->prev;
                        s->prev = q->prev;
                        q->prev = p->prev;
                        p->prev = t;
                        p = s->prev;
                        f14[f24 + 1] = s;
                    }
                    unsigned u2;
                    for (u2 = f24; idx < u2 && f14[u2] == pool.sentinel; u2 = u2 - 1)
                        f14[u2] = p;
                    if (q == pool.sentinel)
                        break;
                }
                p = q;
            } while (q != f14[idx + 1]);
        }
        f24 = f24 + 1;
    }

    unsigned k = *key;
    unsigned i = (k ^ 0xdeadbeef) & f20;
    if (f24 <= i)
        i = i + (-1 - (f20 >> 1));
    HNode** b = f14 + i;
    HNode* p = b[1];
    if (b[0] != p) {
        do {
            p = p->prev;
            if ((int)p->key <= (int)k) {
                if ((int)k <= (int)p->key) {
                    out->node = p;
                    out->inserted = 0;
                    return;
                }
                p = p->next;
                break;
            }
        } while (p != b[0]);
    }
    HNode* nn = (HNode*)list_int_make_node((list_int*)&pool, 0, p, p->prev, key);
    pool.grow(1);
    p->prev = nn;
    *(HNode**)nn->prev = nn;
    HNode* q = (HNode*)p->prev;
    b = f14 + i;
    if (p == b[0]) {
        do {
            b[0] = q;
            if (i == 0)
                break;
            i = i - 1;
            b = f14 + i;
        } while (p == b[0]);
    }
    out->node = q;
    out->inserted = 1;
}

struct TreeW {
    char pad0[8];
    void* f8;                                       // +0x08
    void erase(void* p1, void* p2);                 // 0x4d3e50
    // the element iterator (a class with constructors, so it is returned through the hidden result pointer)
    struct It { E176* p; It() {} It(E176* q) : p(q) {} };
    It insert(It a, It b);                          // 0x4d3e70 (vector::erase's shape: returns a)
};

// @ 0x4d3e50
void TreeW::erase(void* p1, void* p2)
{
    std::_Destroy_range((E68*)p1, (E68*)p2, *(std::allocator<E68>*)(void*)this);   // 0x4cd570
}

static inline E176* copy_into(E176* first, E176* last, E176* dest)
{
    std::_Nonscalar_ptr_iterator_tag tag;
    return std::_Copy_opt(first, last, dest, tag);
}

// @ 0x4d3e70
TreeW::It TreeW::insert(It a, It b)
{
    if (a.p != b.p) {
        E176* p = copy_into(b.p, (E176*)f8, a.p);
        std::_Destroy_range(p, (E176*)f8, *(std::allocator<E176>*)(void*)this);
        f8 = p;
    }
    return a;
}

// ---- the application object (0x4d3ec0, 0x4d4130, 0x4d4140) ----
// ---- callees -------------------------------------------------------------------------------------------
void* __cdecl operator new(unsigned size);          // 0x53545e
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
    f350 = 0;
    WL_MARKER(12)

    m_dw334 = (DWORD)"m";
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
    g_game_G = 0;
    {
        // 0x4d4083..0x4d40b4: operator new(0x6868), then the constructor 0x4d3880 on a non-null block
        Big* b = (Big*)operator new(sizeof(Big));
        (*(Big**)&g_mission) = b ? b->init() : 0;
    }

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
// OptionsDlg (config/options_dlg.h, whose winsock2.h cannot follow windows.h) and gui_edit.h's ConfigDialog: the application
// object (0x8ee3c48) is their `this` (paint 0x4b9af0, init 0x4c7290)
struct OptionsDlg { void paint(); };
struct ConfigDialog { void init(); };
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
        ((OptionsDlg*)(void*)this)->paint();
        if (g_trial_mode == 0 && f350 == 0) {
            f350 = 1;
            g_trial_mode = 1;
        }
        return 1;
    }
    case 0x110: {                                           // WM_INITDIALOG
        hwnd = h;
        SetSingleDialogSkin(h, "MainFrame");
        ((ConfigDialog*)(void*)this)->init();
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

            void* f = fopen(ini.c_str(), "r");
            if (f != 0) {
                g_module_info.load((FILE*)f);
                fclose((FILE*)f);
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
struct Elem50 { char pad0[0x50]; };
struct Elem68 { char pad0[0x68]; };
struct ElemB0 { char pad0[0xb0]; };

// The allocators' members are module_load.cpp's (flora_alloc / ground_alloc / mission_alloc ::construct 0x4d1140 / 0x4d1160 /
// 0x4d1180: the element copy constructors 0x4cd080 / 0x4cd100 / 0x4cd170 behind a null test) and gui_edit.cpp's element
// destructors (conv_list_dtor 0x4c8ba0, fastcall; conv_sub_dtor 0x4c8ab0, stdcall: the original's thiscall `destroy` keeps the
// allocator in ecx, which that callee ignores); std::allocator<E50>::destroy (0x4cd6a0) is a real call.
struct flora_kind; struct ground_spec; struct mission_type; struct conv_rec;
struct flora_alloc { void construct(flora_kind* dst, const flora_kind& src); };
struct ground_alloc { void construct(ground_spec* dst, const ground_spec& src); };
struct mission_alloc { void construct(mission_type* dst, const mission_type& src); };
void __fastcall conv_list_dtor(conv_rec* self);     // 0x4c8ba0
void __stdcall conv_sub_dtor(void** self);          // 0x4c8ab0

// allocator of the 0x50-byte vector
struct Alloc50 {
    __forceinline void construct(Elem50* p, const Elem50& v) { ((flora_alloc*)(void*)this)->construct((flora_kind*)p, *(const flora_kind*)&v); }
    __forceinline void flora_kind_destroy(Elem50* p) { ((std::allocator<E50>*)(void*)this)->destroy((E50*)p); }
};
// 0x68-byte vector: destroy inlined to the dtor 0x4c8ba0
struct Alloc68 {
    __forceinline void construct(Elem68* p, const Elem68& v) { ((ground_alloc*)(void*)this)->construct((ground_spec*)p, *(const ground_spec*)&v); }
    __forceinline void destroy(Elem68* p) { conv_list_dtor((conv_rec*)p); }
};
// 0xb0-byte vector: destroy inlined to the dtor 0x4c8ab0
struct AllocB0 {
    __forceinline void construct(ElemB0* p, const ElemB0& v) { ((mission_alloc*)(void*)this)->construct((mission_type*)p, *(const mission_type*)&v); }
    __forceinline void destroy(ElemB0* p) { conv_sub_dtor((void**)p); }
};

// @ 0x4d49c0
void uninit_fill_n_50(Elem50* f, unsigned n, const Elem50& x, Alloc50& al)
{
    Elem50* next = f;
    try {
        for (; 0 < n; --n, ++f)
            al.construct(f, x);
    } catch (...) {
        for (; next != f; ++next)
            al.flora_kind_destroy(next);
        throw;
    }
}

// @ 0x4d4a50
void uninit_fill_n_68(Elem68* f, unsigned n, const Elem68& x, Alloc68& al)
{
    Elem68* next = f;
    try {
        for (; 0 < n; --n, ++f)
            al.construct(f, x);
    } catch (...) {
        for (; next != f; ++next)
            al.destroy(next);
        throw;
    }
}

// @ 0x4d4ae0
void uninit_fill_n_b0(ElemB0* f, unsigned n, const ElemB0& x, AllocB0& al)
{
    ElemB0* next = f;
    try {
        for (; 0 < n; --n, ++f)
            al.construct(f, x);
    } catch (...) {
        for (; next != f; ++next)
            al.destroy(next);
        throw;
    }
}
