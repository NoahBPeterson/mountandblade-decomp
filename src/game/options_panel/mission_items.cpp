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

// opaque record types used by the loops at the end of the slice
struct rec68 { int pad[0x68 / 4]; };       // E68 (stride 0x68)
struct recb0 { int pad[0xb0 / 4]; };       // FaceTexture (stride 0xb0)
#include "./elem_calls.h"


// The frustum sub-object at +0xf8 is built in the ctor body, not by a member ctor (the original calls rgl::frustum::frustum,
// 0x41ef60, after the first group of stores): constructed on the raw bytes of the record.
#include "../world_data/raw_place.h"
#define CONSTRUCT_FRUSTUM(p) ((void)new ((void*)(p), raw_place()) rgl::frustum)

// ---- 0x60-byte item base (vtbl 0x5947d8, id 99, tstring at +0x1c) ----------------------------------------
// The family base is src/game/head_rec.h - the nine real slots of the image's table (see the table there), not
// placeholders. This TU used to declare its own `item_base` with eight empty virtuals, which put stubs in the
// vtable where the original has functions; deriving from head_rec keeps the layout (id at +8, name at +0x1c) and
// the slots real. `item_base` stays as the name this file uses.
#include "../head_rec.h"
#include "../strat/gui_manager.h"     // g_gui
#include "../menu_rec.h"   // g_menus 0x5e29b0
#include "./mission_item_recs.h"
#include "../../include/game_data.h"                        // GameData g_game (0x5e2930)
#include "../../include/clock_table.h"
// @ 0x4ce960
MissionBlocker::MissionBlocker()
{
    shield_mesh = 0;
    shadow_mesh = 0;
}

char* g_mission_params;                                         // unknown owner; init called from here

void MissionBlocker::reset_screen() { g_gui.reset(); }

// @ 0x4cea10
void MissionBlocker::slot_empty_4()
{
    presentation = -1;
    cheat_slowmo = 0;
    cheat_freeze = 0;
    refresh_slots = 0;
    grow_health = 0;
    order_state = 0;
    order = -1;
    fr.m.o = vector4(0, 0, 10);
    head = vector4(0, 0, 10);
    cam_dist = 1.1f;
    cam_pull = 1.1f;
    zoom = 97.0f;
    yaw = 0.0f;
    pitch = -0.1f;
    roll = 0.0f;
    g_game->f1a20c = 0;
    placement_adjusted = 0;
    yaw_rate = 0;
    pitch_rate = 0;
    lag.x = -1e6f;
    scene_dirty = 0;
    f2f9 = 0;
    free_cam_speed = 0.0f;
    down_repeat = 0;
    up_repeat = 0;
    variant = 0;
    prev_variant = -1;
}

// ---- the remaining item constructors (0x4ceb30 / 0x4cef20 / 0x4cef80) --------------------------------------

// @ 0x4ceb30
ItemA::ItemA()
{
}

// @ 0x4cef20
ItemB::ItemB()
{
}

// @ 0x4cef80
ItemC::ItemC()
{
}

// ---- 0x9c-byte item with four empty vector-ish pairs (0x4cee50) -------------------------------------------
// 0x4ceea0 / 0x4ceec0 are the deleting dtor / releasing dtor of a class whose vtable takes precedence, so this
// variant carries a virtual dtor of its own (the shared base vtable 0x5947d8 + id 99, then its own vtable).
// The item records' virtual destructor, laid over item_base: the body releases the two blocks and the inlined
// item_base dtor then restores the base vtable 0x5947d8 and destroys the name (the original order).
// novtable: the original dtor has no store of its own vtable at the top (only cl's ctor/dtor scaffolding emits one).
static inline void triple_zero(owned_triple& t) { t.p0 = 0; t.p1 = 0; t.p2 = 0; }   // through a reference: the stores stay after the vptr
// @ 0x4cee50 (the two owned triples are built by their member constructors, as in 0x4ceec0)
ItemD::ItemD()
{
    triple_zero(blk0);
    triple_zero(blk1);
}


// The item records' slot 1 (tables 0x5961e4 / 0x596208 / 0x5961c0): reset_gui 0x4c6e00. Slots 2 / 3 are the records' own functions
// (progress_bar.cpp, exchange_screens.cpp, view_open.cpp, agent_hud.cpp); ItemA's slot 1 is gui_edit.cpp's 0x4c6e10.
void ItemB::reset_screen() { g_gui.reset(); }
void ItemC::reset_screen() { g_gui.reset(); }
void ItemD::reset_screen() { g_gui.reset(); }

// ---- per-frame update of the placed objects (0x4ceb90) --------------------------------------------------
// The updater's sixteen row widgets (+0x60), the shown count (+0xa0) and the record index per row (+0xa4) fill the menu's
// list: the widgets are ui_widget.h Widgets (the calls are the original's vtable offsets: hide +0xb0, enable +0xbc,
// set_text +0xa4, show +0xb4, disable +0xb8, set_pos +0x58, set_scale +0x64).
#include "../ui_widget.h"                                     // MissionUi / MissionUi (the widget interface, ctor 0x48ae20)
#include "../party2/camera_class.h"      // Camera (draw 0x488d50)
#include "../../include/dialog_rec.h"                         // DialogBlock: play 0x4875e0 (thiscall, ret 4)


// FieldPanel (mission_ui2/field_panel.h, which this TU's headers clash with): the scene widget's relayout
struct FieldPanel {
    void relayout(const tstring* text);                         // 0x48f020 (thiscall, ret 4; mission_ui2/mission_4.cpp)
};

// @ 0x4ceb90
void ItemA::s_last()
{
    int i = g_game->current_menu;
    MenuRec* g = &g_menus[i];
    tstring* t = g_strtable.find(&g->id);
    if (t == 0)
        t = &g->text;
    g_game->nameb8 = g_script.expand(*t);           // the game's current menu title (assign 0x42bc50)
    ((FieldPanel*)tree)->relayout(&g_game->nameb8.strip_tags());
    for (int k = 0; k < 16; k++) {
        kids[k]->hide();
        kids[k]->flag2_clear();
    }
    for (int idx = 0; idx < g->count; idx++) {
        if (count >= 0x10)
            break;
        g_game->menu_option_disabled = 0;
        if (g->items[idx].cond.play(8)) {
            menu_item* r = &g->items[idx];
            tstring* nm = g_strtable.find(&r->key);
            if (nm == 0)
                nm = &r->name;
            int ret[2];                                         // the 8-byte result slot set_text fills (never read)
            kids[count]->set_text(g_script.expand(*nm).strip_tags());
            if ((g->flags & 0x10) == 0)
                kids[count]->show();
            if (g_game->menu_option_disabled)                 // menu_option_disabled (script_interpreter.h +0x14054)
                kids[count]->flag2_set();
            ids[count] = idx;
            count++;
        }
    }
    {
        float f0 = (float)count * 0.04f;                        // 0x54ab1c
        float f;
        if (f0 > 0.24f)                                         // 0x5961bc
            f = f0;
        else
            f = 0.24f;
        for (int k = 0; k < count; k++) {
            kids[k]->move_a(V2(0.325f - kids[k]->rect_max.x * 0.5f,    // 0x5961b8, 0x549d70
                               f - (float)(k + 1) * 0.04f));
        }
    }
    {
        float h = (float)count * 0.04f;
        if (!(h > 0.24f))
            h = 0.24f;
        slider->target->set_xy(V2(0.75f, h));
    }
    slider->draw();
}

// ---- the array walkers (0x4cf070 / 0x4cf0a0 / 0x4cf0d0) --------------------------------------------------
// @ 0x4cf070 (std::vector<RagdollBone>'s fill: the header body, operator= 0x4cd270 out of line)
template void std::fill(RagdollBone*, RagdollBone*, const RagdollBone&);

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
        ((FaceTexture*)begin)->operator=(*(const FaceTexture*)arg);
}

// std::vector<CapDef> (a bone's capsules, rgl/core/bone.h): its insert (0x4ccef0, mesh_2.cpp) stays out of line
namespace std { template<> vector<CapDef>::iterator vector<CapDef>::insert(vector<CapDef>::iterator _P, const CapDef& _X); }   // 0x4ccef0

// @ 0x4cefe0 (RagdollBone::load's push_back; _Uninit_fill_n<CapDef*> is folded onto gui_edit.cpp's 0x4c5010)
template void std::vector<CapDef>::push_back(const CapDef&);
