// Slice s004ce960: item / mission-object record types and the per-frame updater that lives next to them.
// 0x4ce960 / 0x4ceb30 / 0x4cef20 / 0x4cef80 / 0x4cee50 / 0x4cee50 are constructors of 0x60..0x2c8-byte
// records that all start with the same "vtbl then tstring at +0x1c, id 99" base; 0x4cea10 is the big
// initialiser of the 0x328-byte mission-parameter block, 0x4ceb90 the per-frame "process the placed
// objects" pass, and 0x4ceea0 / 0x4ceec0 its deleting dtor and releasing dtor. The three loops at the end
// (0x4cf070 / 0x4cf0a0 / 0x4cf0d0) walk arrays of 0x50 / 0x68 / 0xb0-byte records and call their readers.
// ALL NAMES PROVISIONAL. Layouts and field offsets reproduced from the disassembly of this slice only.
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <new>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../include/frustum.h"
#include "../tstring.h"
using rgl::vector4;

// ---- globals --------------------------------------------------------------------------------------------
extern int g_5e2930_dummy;

// The string table is game/str_table.h's StrTable (find 0x4c6010, lookup 0x43a1e0); the script state (0xa70440) is dialog_manager.h's ScriptState
// (expand 0x4606d0, thiscall, ret 8: hidden return + token). Those headers declare g_game and friends in other types than this TU's, so the
// member is repeated under the class name.
#include "../str_table.h"                                       // StrTable g_strtable (0x5e2a18)
#include "../../include/script_state.h"                      // ScriptState g_script (0xa70440): expand 0x4606d0

#include "../../include/crt.h"   // canonical: operator delete (0x535712)
void conv_clear(void* p);                                    // 0x4c8bf0 (releases an owned block)

// opaque record types used by the loops at the end of the slice
struct rec50 { int pad[0x50 / 4]; };       // E50 (stride 0x50)
struct rec68 { int pad[0x68 / 4]; };       // E68 (stride 0x68)
struct recb0 { int pad[0xb0 / 4]; };       // E176 (stride 0xb0)
#include "./elem_calls.h"

// A compiler fence (no code): pins the store order the original has (base id/vptr stores before the zero and
// load/store runs) where cl 13.10 otherwise sinks them (docs/codegen.md 10.9).
extern "C" void _ReadWriteBarrier(void);

// The frustum sub-object at +0xf8 is built in the ctor body, not by a member ctor (the original calls rgl::frustum::frustum,
// 0x41ef60, after the first group of stores): constructed on the raw bytes of the record.
#include "../world_data/raw_place.h"
#define CONSTRUCT_FRUSTUM(p) ((void)new ((void*)(p), raw_place()) rgl::frustum)                                       // E50 / E68 / E176: the out-of-line copy assignments

// ---- 0x60-byte item base (vtbl 0x5947d8, id 99, tstring at +0x1c) ----------------------------------------
// The family base is src/game/head_rec.h - the nine real slots of the image's table (see the table there), not
// placeholders. This TU used to declare its own `item_base` with eight empty virtuals, which put stubs in the
// vtable where the original has functions; deriving from head_rec keeps the layout (id at +8, name at +0x1c) and
// the slots real. `item_base` stays as the name this file uses.
#include "../head_rec.h"
#include "../strat/gui_manager.h"     // g_gui
#include "../menu_rec.h"   // g_menus 0x5e29b0
#include "./mission_item_recs.h"
struct GameData;
extern GameData* g_game;                                    // 0x5e2930
#include "../../include/clock_table.h"
// @ 0x4ce960
MissionBlocker::MissionBlocker()
{
    _ReadWriteBarrier();
    w04 = 0;
    f34 = 0; f38 = 0; f3c = 0; f48 = 0;
    clock = ((int*)g_ticks)[0];
    clock_hi = ((int*)g_ticks)[1];
    f58 = 1.0f;
    f40 = -1;
    f5c = 0;
    CONSTRUCT_FRUSTUM(fst);
    f_2a8 = 0;
    f_2b0 = ((int*)g_ticks)[0];
    f_2b4 = ((int*)g_ticks)[1];
    f_2b8 = 0;
    f_2c0 = ((int*)g_ticks)[0];
    f_2c4 = ((int*)g_ticks)[1];
    _ReadWriteBarrier();
    f_d4 = 0;
    f_d8 = 0;
}

// ---- the 0x328-byte mission parameter block (0x4cea10) ---------------------------------------------------
struct MissionParams {
    char pad0[0xd4];
    int f_d4;                                                   // +0xd4 = 0
    int f_d8;                                                   // +0xd8 = 0
    char pade0[0x114];
    vector4 v_1f0;                                              // +0x1f0 (0, 0, 10, w)
    char pad200[0x18];
    vector4 v_218;                                              // +0x218 (0, 0, 10, w)
    char pad228[0x40];
    int f_268;                                                  // +0x268 = 0
    float f_26c;                                                // +0x26c = -0.1f
    int f_270;                                                  // +0x270 = 0
    int f_274;                                                  // +0x274 = 0
    float f_278;                                                // +0x278 = 1.1f
    char pad27c[0x8];
    int f_284;                                                  // +0x284 = 0
    float f_288;                                                // +0x288 = 97.0f
    float f_28c;                                                // +0x28c = -1e6f
    char pad290[0xc];
    int f_29c;                                                  // +0x29c = 0
    int f_2a0;                                                  // +0x2a0 = -1
    char pad2a4[0x3c];
    char b_2e0;                                                 // +0x2e0 = 0
    char b_2e1;                                                 // +0x2e1 = 0
    char pad2e2[0xa];
    float f_2ec;                                                // +0x2ec = 1.1f
    char b_2f0;                                                 // +0x2f0 = 0
    char b_2f1;                                                 // +0x2f1 = 0
    char b_2f2;                                                 // +0x2f2 = 0
    char pad2f3[0x5];
    char b_2f8;                                                 // +0x2f8 = 0
    char b_2f9;                                                 // +0x2f9 = 0
    char pad2fa[0x6];
    int f_300;                                                  // +0x300 = 0
    char pad304[0x10];
    int f_314;                                                  // +0x314 = 0
    int f_318;                                                  // +0x318 = 0
    int f_31c;                                                  // +0x31c = 0
    int f_320;                                                  // +0x320 = -1
    int f_324;                                                  // +0x324 = -1
    void init();                                                // 0x4cea10
};
typedef char MissionParams_size_check[sizeof(MissionParams) == 0x328 ? 1 : -1];

char* g_mission_params;                                         // unknown owner; init called from here

// MissionBlocker's slot overrides: forwarding stubs (a bare `jmp`) to the functions the image's table 0x59616c names. The
// record is not those classes' view (MissionHud / Screen_s261e0 are other screens), so the stubs stay stubs. They sit
// before MissionParams::init's definition so cl does not inline it into slot 4.
struct MissionHud { void refresh(); };                          // 0x4f7f00
struct Screen_s261e0 { void tick(float); };                     // 0x5261e0
void MissionBlocker::slot_empty_1() { g_gui.reset(); }
void MissionBlocker::slot_empty_2() { ((MissionHud*)this)->refresh(); }
void MissionBlocker::scale_3(float a) { ((Screen_s261e0*)this)->tick(*(float*)&a); }
void MissionBlocker::slot_empty_4() { ((MissionParams*)this)->init(); }

// @ 0x4cea10
void MissionParams::init()
{
    char* base = (char*)this;
    void* p1 = base + 0x1f0;
    void* p2 = base + 0x218;
    *(vector4*)p1 = vector4(0.0f, 0.0f, 10.0f);
    *(vector4*)p2 = vector4(0.0f, 0.0f, 10.0f);
    *(float*)(base + 0x278) = 1.1f;
    *(float*)(base + 0x2ec) = 1.1f;
    *(int*)(base + 0x324) = -1;
    *(char*)(base + 0x2e1) = 0;
    *(char*)(base + 0x2e0) = 0;
    *(char*)(base + 0x2f1) = 0;
    *(char*)(base + 0x2f2) = 0;
    *(int*)(base + 0x29c) = 0;
    *(int*)(base + 0x2a0) = -1;
    *(float*)(base + 0x288) = 97.0f;
    *(int*)(base + 0x268) = 0;
    *(float*)(base + 0x26c) = -0.1f;
    *(int*)(base + 0x284) = 0;
    *(int*)((char*)g_game + 0x1a20c) = 0;
    *(char*)(base + 0x2f8) = 0;
    *(int*)(base + 0x270) = 0;
    *(int*)(base + 0x274) = 0;
    *(float*)(base + 0x28c) = -1e6f;
    *(char*)(base + 0x2f0) = 0;
    *(char*)(base + 0x2f9) = 0;
    *(int*)(base + 0x300) = 0;
    *(int*)(base + 0x314) = 0;
    *(int*)(base + 0x318) = 0;
    *(int*)(base + 0x31c) = 0;
    *(int*)(base + 0x320) = -1;
}

// ---- the remaining item constructors (0x4ceb30 / 0x4cef20 / 0x4cef80) --------------------------------------
// @ 0x4ceb30
ItemA::ItemA()
{
    _ReadWriteBarrier();
    w04 = 0;
    f34 = 0; f38 = 0; f3c = 0; f48 = 0;
    clock = ((int*)g_ticks)[0];
    clock_hi = ((int*)g_ticks)[1];
    _ReadWriteBarrier();
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4cef20
ItemB::ItemB()
{
    _ReadWriteBarrier();
    w04 = 0;
    f34 = 0; f38 = 0; f3c = 0; f48 = 0;
    clock = ((int*)g_ticks)[0];
    clock_hi = ((int*)g_ticks)[1];
    _ReadWriteBarrier();
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4cef80
ItemC::ItemC()
{
    _ReadWriteBarrier();
    w04 = 0;
    f34 = 0; f38 = 0; f3c = 0; f48 = 0;
    clock = ((int*)g_ticks)[0];
    clock_hi = ((int*)g_ticks)[1];
    _ReadWriteBarrier();
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// ---- 0x9c-byte item with four empty vector-ish pairs (0x4cee50) -------------------------------------------
// 0x4ceea0 / 0x4ceec0 are the deleting dtor / releasing dtor of a class whose vtable takes precedence, so this
// variant carries a virtual dtor of its own (the shared base vtable 0x5947d8 + id 99, then its own vtable).
// The item records' virtual destructor, laid over item_base: the body releases the two blocks and the inlined
// item_base dtor then restores the base vtable 0x5947d8 and destroys the name (the original order).
// novtable: the original dtor has no store of its own vtable at the top (only cl's ctor/dtor scaffolding emits one).
// @ 0x4cee50 (the two owned triples are built by their member constructors, as in 0x4ceec0)
ItemD::ItemD()
{
    _ReadWriteBarrier();
    blk0.p0 = 0; blk0.p1 = 0; blk0.p2 = 0;
    blk1.p0 = 0; blk1.p1 = 0; blk1.p2 = 0;
}


// The item records' slot overrides (tables 0x596190 / 0x5961e4 / 0x596208 / 0x5961c0): forwarding stubs to the image's functions.
struct MapDialog { void reset_state(); };                       // 0x4c6e10
struct GameScreen { void build(); };                            // 0x4fbfa0
struct EventScreen { void on_event(int); };                     // 0x533070
struct SaveWinCtl { void create(); void create_flagged(); };   // 0x4e5d90 / 0x4e5e50 (0x4e5d80 create_stub is `jmp create`)
struct SourceScreen { void hook_a(int); void hook_b(int); };    // 0x534180 / 0x534230
struct view_obj { void reset(); };                              // 0x50a140
struct InputScreen { void on_input(int); };                     // 0x533aa0
void ItemA::slot_empty_1() { ((MapDialog*)this)->reset_state(); }
void ItemA::slot_empty_2() { ((GameScreen*)this)->build(); }
void ItemA::scale_3(float a) { ((EventScreen*)this)->on_event(*(int*)&a); }
void ItemB::slot_empty_1() { g_gui.reset(); }
void ItemB::slot_empty_2() { ((SaveWinCtl*)this)->create(); }     // the same `jmp create` as 0x4e5d80: the linker's ICF folds them
void ItemB::scale_3(float a) { ((SourceScreen*)this)->hook_a(*(int*)&a); }
void ItemC::slot_empty_1() { g_gui.reset(); }
void ItemC::slot_empty_2() { ((SaveWinCtl*)this)->create_flagged(); }
void ItemC::scale_3(float a) { ((SourceScreen*)this)->hook_b(*(int*)&a); }
void ItemD::slot_empty_1() { g_gui.reset(); }
void ItemD::slot_empty_2() { ((view_obj*)this)->reset(); }
void ItemD::scale_3(float a) { ((InputScreen*)this)->on_input(*(int*)&a); }

// ---- per-frame update of the placed objects (0x4ceb90) --------------------------------------------------
// The updater's sixteen row widgets (+0x60), the shown count (+0xa0) and the record index per row (+0xa4) fill the menu's
// list: the widgets are ui_widget.h Widgets (the calls are the original's vtable offsets: hide +0xb0, enable +0xbc,
// set_text +0xa4, show +0xb4, disable +0xb8, set_pos +0x58, set_scale +0x64).
#include "../ui_widget.h"                                     // MissionUi / MissionUi (the widget interface, ctor 0x48ae20)
#include "../party2/camera_class.h"      // Camera (draw 0x488d50)
#include "../ui/snd_channel.h"                               // snd_channel: play 0x4875e0 (thiscall, ret 4)

// the 0x60-byte menu record of g_menus (0x5e29b0): title key +0x10, 64-bit flags +0x20, the 0x68-byte entries at +0x54/+0x58
struct placed_rec {                                             // 0x68 bytes: name +0, translation key +0x20, condition +0x30
    char pad0[0x30];
    snd_channel cond;                                           // +0x30 (play 0x4875e0: the entry's condition)
    char pad4c[0x68 - 0x4c];
};
struct menu_rec {
    char pad0[0x20];
    unsigned __int64 flags;                                     // +0x20 (bit 4: entries start hidden)
    char pad28[0x54 - 0x28];
    int count;                                                  // +0x54
    placed_rec* recs;                                           // +0x58
};

// FieldPanel (mission_ui2/field_panel.h, which this TU's headers clash with): the scene widget's relayout
struct FieldPanel {
    void relayout(const tstring* text);                         // 0x48f020 (thiscall, ret 4; mission_ui2/mission_4.cpp)
};

struct MissionUpdater {
    char pad0[0x60];
    MissionUi* items[0x10];                                        // +0x60
    int count;                                                  // +0xa0
    int ids[0x10];                                              // +0xa4
    char pade4[0x4];
    FieldPanel* scene;                                          // +0xe8 (0x4cec0d)
    Camera* group;                                        // +0xec (0x4cee0c)
    void update();                                              // 0x4ceb90
};

// ItemA slot 9: the record IS the updater's `this` (0x4ceb90); declared before the definition so it stays a `jmp`.
void ItemA::s_last() { ((MissionUpdater*)this)->update(); }

// @ 0x4ceb90
void MissionUpdater::update()
{
    int i = *(int*)((char*)g_game + 0x13aec);
    menu_rec* g = (menu_rec*)(i * 0x60 + (int)g_menus);
    tstring* t = g_strtable.find((tstring*)((char*)g + 0x10));
    if (t == 0)
        t = (tstring*)g;
    tstring& title = *(tstring*)((char*)g_game + 0xb8);                // the game's current menu title (assign 0x42bc50)
    title = g_script.expand(*t);
    { tstring stripped = title.strip_tags(); scene->relayout(&stripped); }
    for (int k = 0; k < 16; k++) {
        items[k]->hide();
        items[k]->flag2_clear();
    }
    for (int idx = 0; idx < g->count; idx++) {
        if (count >= 0x10)
            break;
        *(char*)((char*)g_game + 0x14054) = 0;
        if (g->recs[idx].cond.play(8)) {
            placed_rec* r = &g->recs[idx];
            tstring* nm = g_strtable.find((tstring*)((char*)r + 0x20));
            if (nm == 0)
                nm = (tstring*)r;
            int ret[2];                                         // the 8-byte result slot set_text fills (never read)
            items[count]->set_text((float*)ret, &(g_script.expand(*nm).strip_tags()));
            if ((g->flags & 0x10) == 0)
                items[count]->show();
            if (*(char*)((char*)g_game + 0x14054))                 // menu_option_disabled (script_interpreter.h +0x14054)
                items[count]->flag2_set();
            ids[count] = idx;
            count++;
        }
    }
    float f = (float)count * 0.04f;                             // 0x54ab1c
    if (!(f > 0.24f))                                           // 0x5961bc
        f = 0.24f;
    for (int k = 0; k < count; k++) {
        float pos[2];
        pos[0] = 0.325f - ((MissionUi*)items[k])->screen[0] * 0.5f;    // 0x5961b8, 0x549d70
        pos[1] = f - (float)(k + 1) * 0.04f;
        items[k]->move_a((float*)pos);
    }
    float h = (float)count * 0.04f;
    if (!(h > 0.24f))
        h = 0.24f;
    float sc[2];
    sc[0] = 0.75f;
    sc[1] = h;
    group->target->set_xy(sc);
    group->draw();
}

// ---- the array walkers (0x4cf070 / 0x4cf0a0 / 0x4cf0d0) --------------------------------------------------
// @ 0x4cf070
void walk50(rec50* begin, rec50* end, void* arg)
{
    for (; begin != end; begin++)
        ((E50*)begin)->operator=(*(const E50*)arg);
}

// @ 0x4cf0a0
void walk68(rec68* begin, rec68* end, void* arg)
{
    for (; begin != end; begin++)
        ((E68*)begin)->operator=(*(const E68*)arg);
}

// @ 0x4cf0d0
void walkb0(recb0* begin, recb0* end, void* arg)
{
    for (; begin != end; begin++)
        ((E176*)begin)->operator=(*(const E176*)arg);
}

#include "./vec44.h"                                         // vec44: push_back 0x4cefe0 (defined below)

// The vector is a std::vector<R44> (include/vec_elems.h R44, 0x2c trivial; repeated here because vec_elems.h defines E50's
// operator= inline): its insert (0x4ccef0) stays out of line, as the original keeps it.
struct R44 { int v[11]; };
namespace std { template<> vector<R44>::iterator vector<R44>::insert(vector<R44>::iterator _P, const R44& _X); }   // 0x4ccef0
// block_copier (rgl/mesh2/gui_edit.h): the fill helper 0x4c5010 takes (first, n, src, copier); the original pushes a fifth,
// unread word (the empty iterator-tag argument of std::_Uninit_fill_n), which the 4-parameter definition cannot express
struct block_copier;
void __cdecl uninit_fill_n(void* first, unsigned n, const void* src, block_copier* cop);   // 0x4c5010

// @ 0x4cefe0
void vec44::push_back(char* v)
{
    int n = size();
    if (begin && (unsigned)n < capacity()) {
        uninit_fill_n(end, 1, v, (block_copier*)(void*)this);
        end += 0x2c;
        return;
    }
    ((std::vector<R44>*)(void*)this)->insert(std::vector<R44>::iterator((R44*)end), *(const R44*)v);
}
