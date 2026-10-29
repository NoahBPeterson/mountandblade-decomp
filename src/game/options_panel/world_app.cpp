// world_app.cpp: the mission object's constructor / destructor, restore_defaults, a hash-table insert, the
// application constructor and main window procedure (0x4d3720..0x4d4b80), and the vector fill helpers that sit
// after them (T-regime region: helpers right after the first caller). Names provisional.
// `Big` is the mission object g_mission (0x5e292c; Mission, ../mission.h) and `GameApp` is the application object
// (Core, ../config/core.h): their constructors / destructors are the only functions here, written against partial
// views because mission.h / core.h declare neither (see MIGRATION.md).
#include <windows.h>
#include <shellapi.h>
#include <new>
#include <vector>
#include <memory>
#include <algorithm>
#include "../tstring.h"
#include "../party/game.h"
#include "../../include/rec_dtors.h"
// application.h's flat view of the framework class is called Application there; its constructor (0x401910) is the real
// CD3DApplication::CD3DApplication, which GameApp's constructor calls first. Compile the view under that name (this TU includes
// neither d3dapp.h nor core.h) so the call binds to the definition.
#define Application CD3DApplication
#include "../application.h"
#undef Application

// Tail (0x10 bytes, dtor 0x42d390) is src/include/rec_dtors.h

struct Rec18 {                                      // 0x18-byte record with a tstring-style dtor
    char pad[0x18];
    ~Rec18();                                       // 0x42bc40
};

// Constructing a tstring in raw storage (0x42bbd0): placement new without the null test std's throw() placement operator adds
struct raw_place {};
inline void* operator new(size_t, void* p, raw_place) { return p; }
inline void operator delete(void*, void*, raw_place) {}

// ---- callees -------------------------------------------------------------------------------------------
#include "../../include/crt.h"   // canonical: operator delete (0x535712)
// The vector elements of mesh_2.cpp (E50 0x50, E68 0x68, E176 0xb0) and the std algorithms the original keeps out of line for them;
// the explicit specialization declarations (no bodies here) keep every one of them a call.
#include "./elem_calls.h"
struct scalar4 { int v; };                          // e68.h: the 4-byte element of the hash bucket vector
namespace std {
template<> E176* _Copy_opt<E176*, E176*>(E176*, E176*, E176*, _Nonscalar_ptr_iterator_tag);   // 0x4cd620
template<> void _Destroy_range<E176, allocator<E176> >(E176*, E176*, allocator<E176>&, _Nonscalar_ptr_iterator_tag);   // 0x4ca7e0
template<> void _Destroy_range<E68, allocator<E68> >(E68*, E68*, allocator<E68>&, _Nonscalar_ptr_iterator_tag);   // 0x4cd570
template<> void vector<scalar4, allocator<scalar4> >::resize(size_t, scalar4);   // 0x4cd010
template<> void allocator<E50>::destroy(E50*);   // 0x4cd6a0
}
// 0x4727a0 is terrain_b's destroy_arrsets (ArrSet is records.h's 0x38-byte record), defined with two parameters; the original calls
// it as std::_Destroy_range(first, last, allocator&, tag) and the last two words are never read
struct ArrSet;
void destroy_arrsets(ArrSet* first, ArrSet* last);   // 0x4727a0
void* hash_new_node(void* node, void* hint, void* key);                   // 0x4c4e50
// 0x4c4e50 is rgl/mesh2/gui_edit.cpp's list_int_make_node: thiscall on the table's node list (ecx = this+4) with the
// original's (node, hint, key); gui_edit defines it __fastcall with an unused edx word, declared here as it is defined.
struct list_int;
void* __fastcall list_int_make_node(list_int* self, void* edx, void* node, void* hint, void* key);   // 0x4c4e50
// The sub-objects the mission constructor builds in place: rgl/mesh2/rec_tables.h's C4c2d10 (the 0x140-byte block, ctor
// 0x4c2d10), config/config.h's clip_table (ctor 0x4be6b0) and module_load.h's growable_array (ctor 0x4d1360); those headers do
// not go with this TU's, so only the constructors are repeated under the class names.
struct C4c2d10 {
    char pad[0x140];
    C4c2d10();   // 0x4c2d10
};
struct clip_table {
    char pad[0x14c];
    clip_table();   // 0x4be6b0
};
struct growable_array {
    char pad[0x140];
    growable_array();   // 0x4d1360
};
#include "../str_table.h"                                      // StrTable (0x5e2a18; was StrTab here: lookup 0x43a1e0)
struct WindowMgrW {
    void open(int flag);   // 0x494260 (world_data/world_data.h; the main menu object 0xa73980)
};

// 0x34-byte element (two tstrings at +0 / +0x10, a heap pointer at +0x30); not the vtable-0x59584c Rec34 of trigger_records.h.
// dxframework/d3dapp.cpp's MBRecord34: the original calls its vector deleting destructor 0x401870 (??_EMBRecord34, which has no
// source spelling) with flags 3, as a plain call; the linker binds this view's symbol to it.
struct MBRecord34 {
    char pad[0x34];
    void* vecdel(unsigned flags);   // 0x401870
};
#pragma comment(linker, "/alternatename:?vecdel@MBRecord34@@QAEPAXI@Z=??_EMBRecord34@@QAEPAXI@Z")

struct OwnedVec {                                      // allocator dword, begin/last/end
    int  alloc;
    int* first;
    int* last;
    int* end;
    void clear(void* a) {
        if (first) {
            destroy_arrsets((ArrSet*)first, (ArrSet*)last);
            operator delete(first);
        }
        first = 0; last = 0; end = 0;
    }
};
struct PlainVec {                                      // trivial elements: only the buffer is freed
    int  alloc;
    void* first;
    void* last;
    void* end;
    void clear(void* a) {
        if (first) operator delete(first);
        first = 0; last = 0; end = 0;
    }
};
struct RowVec {                                      // 0xb0-byte rows, destroyed by 0x4ca7e0
    int  alloc;
    int* first;
    int* last;
    int* end;
    void clear(void* a) {
        if (first) {
            std::_Nonscalar_ptr_iterator_tag tag;
            std::_Destroy_range((E176*)first, (E176*)last, *(std::allocator<E176>*)(void*)this, tag);
            operator delete(first);
        }
        first = 0; last = 0; end = 0;
    }
};

// ---- globals -------------------------------------------------------------------------------------------
#include "../../include/fx.h"   // canonical: g_ticks (0x5e2958)
extern int g_pending_action;                                // 0x5a9f18
#include "../mission.h"   // canonical: g_mission (0x5e292c)
extern char* g_windows;                                  // 0xa73980

// ---- the large record (all members are nominal; accesses are by offset) --------------------------------
struct Big {
    int  f000; int f004; int f008; int f00c;
    int  f010; int f014; int f018; int f01c;
    int  f020; int f024; int f028; int f02c;
    int  f030; int f034; int f038; int f03c;
    MBRecord34* f040;                                // +0x40
    Rec18* f044;                                    // +0x44
    OwnedVec v48;                                       // +0x48
    OwnedVec v58;                                       // +0x58
    OwnedVec v68;                                       // +0x68
    PlainVec v78;                                       // +0x78
    char pad088[0x19c - 0x88];
    Tail f19c;
    Tail f1ac;
    RowVec v1bc;                                      // +0x1bc
    char pad1cc[0x670 - 0x1cc];
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
    Big* init();                                    // 0x4d3880
    void destroy();                                 // 0x4d3720
};

// @ 0x4d3880
Big* Big::init()
{
    Big* p = this;
    p->f00c = 0; p->f010 = 0; p->f014 = 0;
    p->f01c = 0; p->f020 = 0; p->f024 = 0;
    p->f004 = 0;
    p->f028 = 0; p->f02c = 0;
    p->f038 = 0; p->f03c = 0; p->f040 = 0;
    p->v48.alloc = 0; p->v48.first = 0; p->v48.last = 0;
    p->f030 = 0;
    p->v48.end = 0; p->v58.alloc = 0;
    for (int i = 0; i < 0x40; i++)
        new ((void*)(p->blocks + i * 0x130), raw_place()) C4c2d10;

    p->f5de0 = 2;
    p->f5de8 = ((int*)g_ticks)[4];
    p->f5dec = ((int*)g_ticks)[5];
    p->f5e00 = 0; p->f5e04 = 0; p->f5e08 = 0;

    p->f62e8 = 2;
    p->f62f0 = ((int*)g_ticks)[4];
    p->f62f4 = ((int*)g_ticks)[5];
    p->f62f8 = 2;
    p->f6300 = ((int*)g_ticks)[4];
    p->f6304 = ((int*)g_ticks)[5];
    p->f6308 = 2;
    p->f6310 = ((int*)g_ticks)[4];
    p->f6314 = ((int*)g_ticks)[5];
    p->f6318 = 2;
    p->f6320 = ((int*)g_ticks)[4];
    p->f6324 = ((int*)g_ticks)[5];
    p->f6328 = 2;
    p->f6330 = ((int*)g_ticks)[4];
    p->f6334 = ((int*)g_ticks)[5];
    p->f63c4 = 0; p->f63c8 = 0; p->f63cc = 0;
    new ((void*)p->f63d0, raw_place()) clip_table;

    p->f667c = 0; p->f6680 = 0; p->f6684 = 0;
    p->f668c = 0; p->f6690 = 0; p->f6694 = 0;
    new ((void*)p->f6698, raw_place()) growable_array;

    p->f67d8 = 2;
    p->f67e0 = ((int*)g_ticks)[4];
    p->f67e4 = ((int*)g_ticks)[5];
    p->f6800 = 2;
    p->f6808 = ((int*)g_ticks)[4];
    p->f680c = ((int*)g_ticks)[5];

    new ((void*)p->f6814, raw_place()) tstring;
    p->f000 = 0;
    *(int*)((char*)p + 0x5270) = 0;
    return this;
}

// @ 0x4d3720
void Big::destroy()
{
    if (f040)
        f040->vecdel(3);
    delete[] f044;

    v1bc.clear(this);

    f1ac.~Tail();
    f19c.~Tail();

    v78.clear(this);
    v68.clear(this);
    v58.clear(this);
    v48.clear(this);

    ((tstring*)&f028)->~tstring();
    ((tstring*)this)->~tstring();
}

// @ 0x4d3a50
void restore_defaults()
{
    g_game->f1a090 = 0;
    g_game->f1a098 = 0;
    g_game->f1a0b8 = -1;
    g_game->f1a0bc = -1;
    g_game->f1a0c0 = -1;
    g_game->f1a0c4 = -1;

    *(char*)(*(int*)(((char*)g_mission) + 0x65d0) + 0x140) = 0;
    *(char*)(*(int*)(((char*)g_mission) + 0x65dc) + 0x140) = 0;
    *(char*)(*(int*)(((char*)g_mission) + 0x65e4) + 0x140) = 0;
    if (*(int*)(((char*)g_mission) + 0x6738)) *(char*)(*(int*)(((char*)g_mission) + 0x6738) + 0x140) = 0;
    if (*(int*)(((char*)g_mission) + 0x6740)) *(char*)(*(int*)(((char*)g_mission) + 0x6740) + 0x140) = 0;

    if (g_game->f1a1cc) {
        g_game->f1a1cc = 0;
        g_pending_action = -1;
        {
            tstring s1("ui_no_discard_changes");
            tstring s2("ui_yes_save");
            tstring s3("ui_question_save_changes");
            const tstring* r1 = ((StrTable*)0x5e2a18)->lookup(s1);
            const tstring* r2 = ((StrTable*)0x5e2a18)->lookup(s2);
            const tstring* r3 = ((StrTable*)0x5e2a18)->lookup(s3);
            char* ui = g_windows;
            ((tstring*)(ui + 0x68))->clear();
            *(tstring*)(ui + 0x1c) = *r3;
            *(tstring*)(ui + 0x78) = *r2;
            *(tstring*)(ui + 0x88) = *r1;
            *(int*)(ui + 0x4c) = (int)&g_pending_action;
            *(int*)(ui + 0x50) = 0;
            *(int*)(ui + 0x4) = 1;
        }
        ((WindowMgrW*)0xa73980)->open(0);
        g_game->f1a090 = 0;
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
    unsigned n = f14 ? (unsigned)(((char*)f18 - (char*)f14) >> 2) : 0u;
    if (f24 <= (unsigned)f0c >> 2) {               // 0x4d3c0d: shr (unsigned)
        if (count < n - 1) {
            if (f20 < count)
                f20 = f20 * 2 + 1;
        } else {
            f20 = n * 2 - 3;
            { scalar4 sen; sen.v = (int)pool.sentinel; ((std::vector<scalar4>*)(void*)((char*)this + 0x10))->resize(n * 2 - 1, sen); }   // 0x4cd010
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
                    if (q != pool.sentinel) {
                        HNode* b = f14[idx];
                        unsigned u = idx;
                        while (p == b) {
                            f14[u] = q;
                            if (u == 0)
                                break;
                            u = u - 1;
                            b = f14[u];
                        }
                        HNode* s = pool.sentinel;
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
    HNode* p = f14[i + 1];
    if (p != f14[i]) {
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
        } while (p != f14[i]);
    }
    HNode* nn = (HNode*)list_int_make_node((list_int*)&pool, 0, p, p->prev, key);
    pool.grow(1);
    p->prev = nn;
    *(HNode**)nn->prev = nn;
    HNode* q = (HNode*)p->prev;
    if (p == f14[i]) {
        do {
            f14[i] = q;
            if (i == 0)
                break;
            i = i - 1;
        } while (p == f14[i]);                      // 0x4d3e13: cmp edi, [f14+i*4]; je back
    }
    out->node = q;
    out->inserted = 1;
}

struct TreeW {
    char pad0[8];
    void* f8;                                       // +0x08
    void erase(void* p1, void* p2);                 // 0x4d3e50
    void insert(void** out, void* a, void* b);      // 0x4d3e70
};

// @ 0x4d3e50
void TreeW::erase(void* p1, void* p2)
{
    std::_Destroy_range((E68*)p1, (E68*)p2, *(std::allocator<E68>*)(void*)this, std::_Nonscalar_ptr_iterator_tag());   // 0x4cd570
}

// @ 0x4d3e70
void TreeW::insert(void** out, void* a, void* b)
{
    if (a != b) {
        E176* p = std::_Copy_opt((E176*)b, (E176*)f8, (E176*)a, std::_Nonscalar_ptr_iterator_tag());
        std::_Destroy_range(p, (E176*)f8, *(std::allocator<E176>*)(void*)this, std::_Nonscalar_ptr_iterator_tag());
        f8 = p;
    }
    *out = a;
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
extern int g_use_shaders;                                // 0x5e28fc
extern int g_5e28c8;                                // 0x5e28c8
extern int g_counter;                                // 0x5e28cc
extern char g_saved;                               // 0x5e2a08
// g_cfg.disable_sound (config/config.h Config +0x40 of g_cfg 0x5a9f98; that header brings d3dapp.h, which would redefine
// CD3DApplication here)
#define CFG_DISABLE_SOUND (*(int*)0x5a9fd8)
#include "../game_globals.h"      // canonical g_key_states (listed directly: the audit reads only the source's own includes)
#define g_key_states_i (*(int(*)[0x40])g_key_states)     // g_key_states (0x5e27c8, game_globals.h) as 0x40 ints

#include "../log_stream.h"                       // LogStream: slot 10 (+0x28) is w_cstr

// ---- the application object: Application / GameApp (../application.h) --------------------------------
// @ 0x4d3ec0
GameApp::GameApp()
{
    tstring* arrp = (tstring*)arr;
    *(void**)this = (void*)0x5948e0;

    tstring* p = arrp;
    int n = 0x20;
    do {
        new ((void*)p, raw_place()) tstring;
        p++;
    } while (--n);

    f350 = 0;

    *(int*)((char*)this + 0x334) = 0x5a9c3c;
    *(int*)((char*)this + 0x338) = 0x5a9c54;
    *(char*)((char*)this + 0x30) = 1;
    *(int*)((char*)this + 0x33c) = 0x190;
    *(int*)((char*)this + 0x340) = 0x12c;
    *(char*)((char*)this + 0x344) = 1;

    CD3DFont* font = new CD3DFont("Arial", 9, 1);
    *(CD3DFont**)((char*)this + 0x348) = font;
    *(int*)((char*)this + 0x34c) = -1;

    for (int i = 0; i < 0x40; i++)
        g_key_states_i[i] = 0;

    g_counter = 0;
    g_5e28c8 = 0;
    scan_module_folders(arrp, &f38c);
    if (f38c > 0) {
        *(tstring*)0x8ee3c38 = *arrp;

        HKEY  key;
        DWORD disp;
        RegCreateKeyExA((HKEY)0x80000001, "Software\\MountAndBladeKeys", 0, 0, 0, 0x20007, 0,
                        &key, &disp);
        if (disp == 1)
            ((LogStream*)0x5a9f60)->w_cstr("Creating new Registry Key");
        if (key != 0) {
            tstring modname;
            read_key_string(&key, "last_module", &modname);
            if (key != 0)
                RegCloseKey(key);
            if (modname.len > 0)
                *(tstring*)0x8ee3c38 = modname;
        }
    }

    ((tstring*)0x8ee3c28)->assign("Modules\\");
    ((tstring*)0x8ee3c28)->append(*(tstring*)0x8ee3c38);
    {
        tstring tmp("\\");
        ((tstring*)0x8ee3c28)->append(tmp);
    }

    g_use_shaders = 0;
    g_saved = 0;
    g_game = 0;
    (*(Big**)&g_mission) = new Big;

    if (CFG_DISABLE_SOUND == 0) {
        FSOUND_SetMaxHardwareChannels(0);
        FSOUND_Init(0xac44, 0x40, 0);
        FSOUND_SetSFXMasterVolume(0xff);
        FSOUND_3D_SetRolloffFactor(0.5f);
    }

    *(tstring*)0xa75e8c = *(tstring*)0x8ee3c28;
    *(int*)((char*)this + 0x364) = 0;
    m_hMapEditorDlg = 0;
    m_hScriptDlg = 0;
    f37c = 0;
}

// @ 0x4d4130
#include "./config_dialog.h"                      // Dialog: proc 0x4d0240 (the dialog procedure of the application object)
int __stdcall wndproc_thunk(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    return (int)((Dialog*)0x8ee3c48)->proc(h, msg, wp, lp);
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
struct ini_file {
    void load(FILE* f);   // config/config.h: the module.ini reader (0x4bd630)
};
int __stdcall   SkinDialog(HWND h, const char* s);       // 0x535446 (SkinMagic SetSingleDialogSkin: stdcall, the callers do not pop)

extern tstring g_module_name;                                      // 0x8ee3c38
extern tstring g_module_path;                                        // 0x8ee3c28
#define STRTAB ((StrTable*)0x5e2a18)
#define UI(s) STRTAB->lookup(tstring(s))

// @ 0x4d4140
int GameApp::msg_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    tstring path;

    switch (msg) {
    case 0xf: {                                             // WM_PAINT
        ((OptionsDlg*)(void*)this)->paint();
        if (*(int*)0x5e29c4 == 0 && f350 == 0) {
            f350 = 1;
            *(int*)0x5e29c4 = 1;
        }
        return 1;
    }
    case 0x110: {                                           // WM_INITDIALOG
        hwnd = h;
        SkinDialog(h, "MainFrame");
        ((ConfigDialog*)(void*)this)->init();
        if (f38c == 0) {
            ShowWindow(GetDlgItem(hwnd, 0x410), 0);
            ShowWindow(GetDlgItem(hwnd, 0x425), 0);
            MessageBoxA(0, UI("ui_there_are_no_modules_installed")->c_str(), "ERROR", 0x10);
            break;
        }
        HWND list = GetDlgItem(hwnd, 0x420);
        if (list == 0)
            break;
        SendMessageA(list, 0x14b, 0, 0);
        for (int i = 0; i < f38c; i++) {
            int r = SendMessageA(list, 0x14a, 0, (LPARAM)mod(i)->c_str());
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
            break;
        case 0x405:
            DialogBoxParamA(0, (LPCSTR)0xc1, hwnd, (DLGPROC)0x4d4130, 0);
            break;
        case 0x406:
            ShellExecuteA(0, "open", "http://www.gamersgate.com", 0, 0, 5);
            break;
        case 0x410: {
            *(char*)0x5e2630 = 0;
            int sel = 0;
            if (f38c > 1) {
                HWND c = GetDlgItem(h, 0x420);
                int idx = SendMessageA(c, 0x147, 0, 0);
                sel = SendMessageA(c, 0x150, idx, 0);
            }
            g_module_name = *mod(sel);
            g_module_path.assign("Modules\\");
            g_module_path.append(g_module_name);
            g_module_path.append(tstring("\\"));
            HKEY key;
            open_keys_key(&key);
            keys_write_value(&key, "last_module", &g_module_name);
            if (key != 0)
                RegCloseKey(key);
            EndDialog(h, 0x410);
            break;
        }
        case 0x41f: {                                       // check for a newer version
            int v = get_latest_version();
            if (v > 0x3f3) {
                tstring text(*UI("ui_notification_new_version"));
                unescape(&text);
                char buf[0x100];
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
            break;
        case 0x46b:
            ShellExecuteA(0, "open", "http://www.paradoxplaza.com/Mount&Blade/", 0, 0, 5);
            break;
        case 0x420: {                                       // module selection changed
            if (HIWORD(wp) != 9)
                break;
            HWND c = GetDlgItem(h, 0x420);
            int idx = SendMessageA(c, 0x147, 0, 0);
            int sel = SendMessageA(c, 0x150, idx, 0);
            if (f38c <= sel)
                sel = f38c - 1;
            g_module_name = *mod(sel);

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
                ((ini_file*)0xa18238)->load((FILE*)f);
                fclose((FILE*)f);
            }

            tstring v1;
            tstring v2;
            if (*(int*)0xa181c8 > 0x3f3) {
                MessageBoxA(hwnd, UI("ui_module_newer_than_application")->c_str(), 0, 0x30);
            }
            else if (*(int*)0xa181cc < 0x3f3) {
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
