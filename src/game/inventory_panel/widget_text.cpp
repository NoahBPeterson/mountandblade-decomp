// Slice s0050cc60 (batch ds1). Editor helpers: the widget flag getter (0x50cc60), the text box ctor
// (0x50d0b0, vtable 0x597050), the index-list sorter (0x50cc70) and pusher (0x50d0e0), vector<Skin>::resize
// (0x50d170) and the 0x1f8-record scene reader (0x50d520). The engine vector is the VC7.1 std::vector whose allocator carries one dword of state, so
// _Myfirst/_Mylast/_Myend sit at +4/+8/+0xc behind the allocator word. Names provisional; the engine
// string is called `tstring`.
#define WIDGET_VEC_PUSH_BACK_INLINE   // gui_manager.h: add_window expands vector<NodeA*>::push_back inline (the slow path is _Insert_n 0x49b1c0)
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
#include "../options_panel/mission_item_recs.h"   // ItemD_vt: the notes window, whose member sort_list is
#define TEXT_BOX_OUT_OF_LINE      // 0x50d0b0 is defined below
#define CONTAINER_DTOR_EXPANDED   // ??_Gtext_box, slot 0 of 0x597050 (the code of 0x5021f0), see ui/container.h
struct NodeA;
template<> void std::vector<NodeA*, std::allocator<NodeA*> >::_Insert_n(std::vector<NodeA*, std::allocator<NodeA*> >::iterator, size_t, NodeA* const&);   // 0x49b1c0 (out-of-line, party2/camera.cpp)
#include "../strat/gui_manager.h"                       // gui_manager (add_window 0x50d0e0)
#include "../ui/container.h"                          // MissionUi (flag1 0x50cc60), text_box (0x50d0b0)
#include "../shared_vars.h"                             // g_show_party_ids_instead_of_names 0xa181b8

// ------------------------------------------------------------------ records --------------------------------
// (vector<RagModel>::_Insert_n 0x50d260, _Ufill 0x50d140 and _Destroy 0x50d230, placed after this file's functions, are the header's
// templates instantiated by push_back 0x50fbf0, inventory_panel/loading_screen.cpp; RagModel is src/include/rag_model.h.)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)


// ------------------------------------------------------------------ the 0x1f8 model record ---------------
#include "../arr_set.h"
#include "../../include/vec_elems.h"
#include "../animator.h"
#include "../../include/skin.h"                      // Skin: the 0x1f8-byte skin record

#include <vector>
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
#include "../../rgl/stream.h"                                    // stream_base: the skins.txt reader

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
GameData* g_game;                                           // 0x5e2930 the game state object

// 0x48ad00 is Map10007::lookup (ui/mission.h, ui/mission_ui.cpp: the id -> index hash at the start of the party manager)

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
    std::vector<int> v;                                       // +0
};

// @ 0x50d0e0 gui_manager::add_window
void gui_manager::add_window(NodeA* w)
{
    dirty = 1;
    objs.push_back(w);   // inline: the fast path here, the shared _Insert_n 0x49b1c0 out of line
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

static inline const tstring& name_of_4(int id)
{
    return g_game->quests[id].text;
}

static inline void swap_index(sel_list* l, unsigned i, unsigned j)
{
    int t = l->v._Myfirst[i];
    l->v._Myfirst[i] = l->v._Myfirst[j];
    l->v._Myfirst[j] = t;
}

// @ 0x50cc70 sel_list::sort
void ItemD_vt::sort_list(sel_list* l, int mode)
{
    for (unsigned i = 0; i < l->v.size(); i++) {
        for (unsigned j = i + 1; j < l->v.size(); j++) {
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
// 0x50d170 is vector<Skin>::resize(n, Skin): its growth path is std::vector<Skin>::_Insert_n (0x4fe520, agent_vec.cpp's
// instance) and its shrink path std::vector<Skin>::erase (0x4eb860, game/char_sheet/facegen_misc.cpp)
template<> void std::vector<Skin>::_Insert_n(std::vector<Skin>::iterator, size_t, const Skin&);   // 0x4fe520
template<> std::vector<Skin>::iterator std::vector<Skin>::erase(std::vector<Skin>::iterator, std::vector<Skin>::iterator);   // 0x4eb860
template void std::vector<Skin>::resize(size_t, Skin);   // @ 0x50d170

// ------------------------------------------------------------------ the skin table reader -----------------------
// @ 0x50d520 Animator::read (skins.txt: the count, that many skins, then the character builder's defaults)
void Animator::read(stream_base* s)
{
    skin_vec.~vector();   // 0x4eb810 (the vector's destructor)
    s->r_int(&char_count);
    unsigned n = char_count;
    skin_vec.resize(n, Skin());
    for (int i = 0; i < char_count; i++)
        (*(skin_vec.begin() + i)).read((MsStream*)s);
    cur = 0;
    set = -1;
    item = -1;
    material = 0;
    variant = 0;
    float* w = weights;
    time = 0.5f;
    scale = 0.3f;
    for (int i = 0; i < 0x3f; i++)
        w[i] = 0.5f;
    (*(skin_vec.begin() + cur)).relax(-1, w);   // 0x4735e0
}

#pragma inline_depth(3)   // for ??_Gtext_box, generated at the end of the object
