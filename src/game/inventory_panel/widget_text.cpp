// Slice s0050cc60 (batch ds1). Editor helpers: the widget flag getter (0x50cc60), the text box ctor
// (0x50d0b0, vtable 0x597050), the index-list sorter (0x50cc70) and pusher (0x50d0e0), vector<Skin>::resize
// (0x50d170) and the 0x1f8-record scene reader (0x50d520). The engine vector is the VC7.1 std::vector whose allocator carries one dword of state, so
// _Myfirst/_Mylast/_Myend sit at +4/+8/+0xc behind the allocator word. Names provisional; the engine
// string is called `tstring`.
#include "../../include/image_addrs.h"
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <string.h>
#include <new>
#include <vector>
#include "../../rgl/rgl_math.h"

// ------------------------------------------------------------------ engine string (0x10 bytes) -----------
#include "../tstring.h"
#define TEXT_BOX_OUT_OF_LINE      // 0x50d0b0 is defined below
#define CONTAINER_DTOR_EXPANDED   // ??_Gtext_box, slot 0 of 0x597050 (the code of 0x5021f0), see ui/container.h
#include "../ui/container.h"                          // MissionUi (flag1 0x50cc60), text_box (0x50d0b0)
#include "../shared_vars.h"                             // g_show_party_ids_instead_of_names 0xa181b8

namespace c49aca0 {                                                     // game/party2/camera.cpp
struct NodeA;
struct PVec {
    void _Insert_n(NodeA** where, unsigned n, NodeA* const& x);   // 0x49b1c0
};
}
// ------------------------------------------------------------------ records --------------------------------
// (vector<RagModel>::_Insert_n 0x50d260, _Ufill 0x50d140 and _Destroy 0x50d230, placed after this file's functions, are the header's
// templates instantiated by push_back 0x50fbf0, inventory_panel/loading_screen.cpp; RagModel is src/include/rag_model.h.)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)


// ------------------------------------------------------------------ the 0x1f8 model record ---------------
#include "../arr_set.h"
#include "../../include/vec_elems.h"
#include "../../include/skin.h"                      // Skin: the 0x1f8-byte skin record

struct vec_model {                                  // vector<Skin>
    void* alloc;                                    // +0
    Skin* first;                                 // +4
    Skin* last;                                  // +8
    Skin* end;                                   // +0xc
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    void resize(unsigned n, Skin val = Skin());   // 0x50d170
};

#include "../int_vec.h"                                 // IntVec (was vec_int here)
#include "../../include/game_data.h"   // GameData, g_game 0x5e2930

struct vec_widget {                                 // vector<widget*> (allocator word not initialised)
    void* alloc;                                    // +0
    void** first;                                   // +4
    void** last;                                    // +8
    void** end;                                     // +0xc
};

void  tstring_dtor(tstring* s);                                          // 0x42bc40

// ------------------------------------------------------------------ globals --------------------------------

#include "../str_table.h"                                       // StrTable: the translated-string table 0x5e2a18 (find 0x4c6010)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
GameData* g_game;                                           // 0x5e2930 the game state object

// 0x48ad00 is Map10007::lookup (ui/mission.h, ui/mission_ui.cpp: the id -> index hash at the start of the party manager)
struct AccessStream {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48();
    virtual void read_int(int* out);                            // +0x4c
};

// ------------------------------------------------------------------ the widget base ------------------------
// @ 0x50cc60 MissionUi::flag1
unsigned MissionUi::flag1() const
{
    return f104 & 1;
}

// the text box (vtable 0x597050, size 0x188): ui/container.h's container with nothing added. Its slot 0 is the container's
// destroying destructor (0x5021f0, the copy party_window.cpp's object holds).

// this file's text_box constructor expands the container constructor (0x50d0b3); the other sites call 0x4931a0
inline ContainerBase::ContainerBase() {}

// @ 0x50d0b0 text_box::text_box
text_box::text_box()
{
}

// ------------------------------------------------------------------ the selection list ----------------------
struct sel_list {
    IntVec v;                                       // +0
    char pad[0x1708 - 0x10];
    char dirty;                                     // +0x1708
    void add(int val);                              // 0x50d0e0
};

// std::vector::insert(where, val) as VC7.1 writes it: the offset of `where` is computed (and dropped by the caller),
// which makes `where` a two-use value coloured before the &val temporary (eax for end(), edx for &val)
static inline int* iv_insert(IntVec* v, int* where, const int& val)
{
    int off = v->count() == 0 ? 0 : (int)(where - v->_Myfirst);
    ((c49aca0::PVec*)v)->_Insert_n((c49aca0::NodeA**)where, 1, *(c49aca0::NodeA* const*)&val);   // 0x49b1c0
    return v->_Myfirst + off;
}
// @ 0x50d0e0 sel_list::add
void sel_list::add(int val)
{
    dirty = 1;
    if (v.count() < v.capacity()) {
        int* p = v._Mylast;
        *p = val;
        v._Mylast = p + 1;
    } else {
        iv_insert(&v, v._Mylast, val);
    }
}

// ------------------------------------------------------------------ name lookups used by the sorter --------
// the editor's name tables hang off the game object as pointer members; the compiler folds the member
// load into the record-address arithmetic (`add esi,[eax+0x13a00]`).
static inline const char* name_of_1(int id)
{
    Troop* rec = &g_game->troops[id];
    const tstring* t = &rec->name;
    const tstring* q = g_strtable.find(&rec->key);
    if (q != 0)
        t = q;
    return t->c_str();
}

static inline const char* name_of_2(int id)
{
    FactionRec* rec = &g_game->factions[id];
    const tstring* t = &rec->name;
    if (rec->f10 == 0) {
        const tstring* q = g_strtable.find(&rec->key);
        if (q != 0)
            t = q;
    }
    return t->c_str();
}

// expanded at both sort_list sites in the original; plain inline becomes a call once the unit's budget is spent
__forceinline const char* name_of_3(int id)
{
    PartyMgr* mgr = &g_game->parties;
    int v = mgr->lookup(id);
    Party* rec = (Party*)((char*)mgr->pages._Myfirst[v >> 4] + (v & 0xf) * sizeof(Party));      // the page table is a pointer (0x50ce86)
    const tstring* t = &rec->name;
    if (g_module_settings.show_party_ids_instead_of_names) {
        t = &rec->key;
    } else if (!rec->flag) {
        const tstring* q = g_strtable.find(&rec->key);
        if (q != 0)
            t = q;
    }
    return t->c_str();
}

struct row138 {
    char pad0[0x40];
    tstring text;                                   // +0x40
};
static inline const tstring& name_of_4(int id)
{
    return *(const tstring*)(((char*)g_game->quests) + id * 0x138 + 0x40);
}

static inline void swap_index(sel_list* l, unsigned i, unsigned j)
{
    int t = l->v._Myfirst[i];
    l->v._Myfirst[i] = l->v._Myfirst[j];
    l->v._Myfirst[j] = t;
}

// @ 0x50cc70 sel_list::sort
struct note_window { void sort_list(sel_list* l, int mode); };    // the screen itself is defined in note_window.cpp
void note_window::sort_list(sel_list* l, int mode)
{
    for (unsigned i = 0; i < l->v.count(); i++) {
        for (unsigned j = i + 1; j < l->v.count(); j++) {
            bool do_swap = false;
            switch (mode) {
            case 1:
                do_swap = strcmp(name_of_1(l->v._Myfirst[i]), name_of_1(l->v._Myfirst[j])) > 0;
                break;
            case 2:
                do_swap = strcmp(name_of_2(l->v._Myfirst[i]), name_of_2(l->v._Myfirst[j])) > 0;
                break;
            case 3:
                do_swap = strcmp(name_of_3(l->v._Myfirst[i]), name_of_3(l->v._Myfirst[j])) > 0;
                break;
            case 4:
                // one full expression, as the original: the arguments evaluate right to left, so j's
                // strip_tags() temporary is built first and both die after the strcmp
                do_swap = strcmp(name_of_4(l->v._Myfirst[i]).strip_tags().c_str(),
                                 name_of_4(l->v._Myfirst[j]).strip_tags().c_str()) > 0;
                break;
            }
            if (do_swap)
                swap_index(l, i, j);
        }
    }
}

// ------------------------------------------------------------------ the dialog thunk -----------------------
#include "../config/core.h"                      // GameApp g_app_obj (0x8ee3c48)

// @ 0x50d250 dialog_thunk
LRESULT __stdcall dialog_thunk(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return g_app_obj.terrain_dialog(hDlg, msg, wParam, lParam);
}

// ------------------------------------------------------------------ vector<Skin> members ---------------
// vec_model is the library's vector of 0x1f8-byte records: its growth path is std::vector<Skin>::_Insert_n (0x4fe520,
// agent_vec.cpp's instance, protected: reached through a class that makes it public) and its shrink path is BigArray::erase
// (0x4eb860, game/char_sheet/facegen_misc.cpp: erase(hidden result, first, last), thiscall)
template<> void std::vector<Skin>::_Insert_n(std::vector<Skin>::iterator, size_t, const Skin&);   // 0x4fe520
struct vec_cfg_pub : std::vector<Skin> { using std::vector<Skin>::_Insert_n; };
struct BigArray {
    struct It { Skin* p; It() {} It(Skin* q) : p(q) {} };   // the element iterator (facegen_misc.cpp)
    It erase(It first, It last);                              // 0x4eb860
};

// @ 0x50d170 vec_model::resize
void vec_model::resize(unsigned n, Skin val)
{
    vec_cfg_pub& v = *(vec_cfg_pub*)this;
    if (v.size() < n)
        v._Insert_n(v.end(), n - v.size(), val);
    else if (n < v.size())
        ((BigArray*)this)->erase(BigArray::It(&*(v.begin() + n)), BigArray::It(&*v.end()));   // the discarded iterator result
}

// ------------------------------------------------------------------ the scene reader -----------------------
// 0x1f8-record collection laid out as: five dwords of state, the model vector at +0x18, the count read
// from the stream at +0x28, 0x3f morph weights at +0x2c and two scale floats at +0x128/+0x12c.
struct mesh_scene {
    int f0;                                         // +0
    int f4;                                         // +4
    int f8;                                         // +8
    int fc;                                         // +0xc
    int f10;                                        // +0x10
    int f14;                                        // +0x14
    vec_model models;                               // +0x18
    int count;                                      // +0x28
    float weights[0x3f];                            // +0x2c
    float f128;                                     // +0x128
    float f12c;                                     // +0x12c
    void read(AccessStream* s);                     // 0x50d520
};

// @ 0x50d520 mesh_scene::read
void mesh_scene::read(AccessStream* s)
{
    ((SkinVec*)&models)->~vector();   // 0x4eb810 (the vector's destructor: the same skin vector as g_anim's)
    s->read_int(&count);
    unsigned n = count;
    models.resize(n);
    for (int i = 0; i < count; i++)
        (*(((vec_cfg_pub*)&models)->begin() + i)).read((MsStream*)s);
    f8 = 0;
    f0 = -1;
    f4 = -1;
    f14 = 0;
    fc = 0;
    float* w = weights;
    f12c = 0.5f;
    f128 = 0.3f;
    for (int i = 0; i < 0x3f; i++)
        w[i] = 0.5f;
    (*(((vec_cfg_pub*)&models)->begin() + f8)).relax(-1, w);   // 0x4735e0
}

#pragma inline_depth(3)   // for ??_Gtext_box, generated at the end of the object
