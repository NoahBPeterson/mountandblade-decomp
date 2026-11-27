//-----------------------------------------------------------------------------
// File: script_support.cpp  (0x49b1c0..0x49cc60)
//
// Desc: Mission::place, set_font_mesh, GameData gold / quest / label helpers, battle icon, order arrow, check_slot, Mission::order (the slices c49aca0..c49c2c0)
//       One part of what was game/script/script_interpreter.cpp (the object's functions in address order; the cut points and the evidence for
//       them are in docs/source_tree.md section 3). The prelude and the helper definitions below are the
//       parts of the old file's prelude and earlier sections this part's code uses.
//-----------------------------------------------------------------------------

#define RGL_VECTOR4_OPS_OUT_OF_LINE   // this object holds the out-of-line copy 0x407cd0 of vector4::operator* (defined below)
#include <math.h>
#include "engine/core/layout32.h"
#include "game/data/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40; listed directly for the resolution audit)
#include "game/mission/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): listed directly (the resolution audit reads only the source's own includes)
#include "game/config/config.h"   // mesh_record (set_background_mesh)
#include "game/data/env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "engine/audio/music_player.h"   // g_music (0xa73a10)
#include "game/agent/animator.h"      // Animator g_anim (0x8ed4f60)
#include "game/config/config_block.h"      // Config g_cfg (0x5a9f98)
#include <stdlib.h>
#include "game/script/script_interpreter.h"
#include "game/ui/gui_manager.h"          // gui_manager: g_gui
#include "game/dialog/dialog_manager.h"
#include "game/agent/agent_queries.h"
#include "engine/core/tstring.h"
#include "game/data/str_table.h"
#include "engine/math/mat4_include.h"
#include "engine/math/quaternion.h"
#include "engine/core/timer.h"
#include "engine/math/vec.h"
#include "engine/render/light.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/mission/placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/resource/skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/party/slot_array.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/agent/agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/mission/mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/game_recs.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/agent/blow.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/campaign/event.h"           // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include <vector>
#include "game/data/mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/string_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/party/party_x.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/party/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "engine/scene/world.h"      // rgl::world (the scene object at mission +0 and in g_script.level)
#include "engine/audio/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "game/data/item_kind_include.h"   // g_item_kind_count 0x5e29d4
#include "platform/image_addrs.h"
#include "game/agent/animator.h"      // Animator g_anim (0x8ed4f60)
#include "engine/render/skin.h"      // Skin (g_anim.skin_vec)
#include "platform/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include <new>
#include <string.h>
#include "engine/math/rgl_math.h"
#include "engine/core/identity_value.h"   // rgl::identity3_value (0x40cd60 by value)
#include "engine/core/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "game/data/scene_object.h"   // SceneObject: the tableau scene (TableauTbl::worlds)
#include "game/common/game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include <vector>
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include "engine/core/tstring.h"
#include "engine/render/font.h"
#include "game/data/game_data.h"
#include "engine/fx/fx.h"          // FxDef (the effect definition of a scene entity)
#include "game/dialog/dialog_rec.h"
#include "engine/render/mesh.h"
#include "engine/resource/resource.h"             // resmgr g_resmgr (0x5d6fa0)
#include "game/party/party.h"
#include "game/mission/camera.h"
#include "game/mission/mesh_build.h"   // build_appearance (0x482240), setup_item_meshes (0x47cfe0)
#include "engine/resource/skeleton.h"                // node_groups / node_list (cap_all 0x438f90)
#include "game/mission/camera_class.h"      // Camera (the one declaration of the 0x4a8-byte view object)
#include "engine/audio/sound_mgr.h"      // g_sound_mgr 0x5aa128 (listener at +0x40 = 0x5aa168)
#include "game/data/str_table.h"                  // StrTable::lookup 0x43a1e0 (the canonical class the views below forward to)
#include "game/ui/widgets/container.h"                 // ContainerBase (game/ui/widgets/widget_base.h)
#include "game/campaign/event.h"                         // Event: the 0x98-byte map event / battle record (GameData::events, pages at +0xe4, 0x49bbb0)
#include "game/ui/gui_manager.h"       // gui_manager (set_font_mesh 0x49b630 is defined here)
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "game/data/game_recs.h"
#include "game/common/level_global.h"      // g_level (0xa732b4)
#include "game/data/mode_rec.h"   // g_mission_templates 0x5e2a00
#include "engine/math/float_consts.h"   // shared float constants kept in memory (0x549cdc ...)
#include "game/agent/animator.h"
#include "engine/core/clock_table.h"
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "engine/io/text_stream.h"
#include "game/common/shared_vars.h"
#include "game/agent/animator.h"
#include "game/script/script_globals.h"      // canonical declarations of the interpreter's globals (listed directly: the resolution audit reads only the source's own includes)
#include "game/ui/ui_globals.h"      // UI / core globals
#include "engine/core/vec_xlen.h"                                                // 0x43e2a0 std::vector<int>::_Xlen (agent3/agent_combat.cpp)

// ---- definitions from the old file's prelude and earlier sections that this part's code uses (copies; each is a static
// or inline helper, a type or a macro: none of them emits code or data of its own)
// ---- data this module owns (docs/standalone_data.md) ----

extern Font* g_font;                                // 0x5e2934
#include "game/script/script_interpreter_shared.h"   // the definitions this file shares with its sibling parts

rgl::mesh* __stdcall build_legend_mesh(rgl::entity* parent);                               // 0x495b20 game/ui/widgets/hud_bar_widgets.cpp

// ======================================================================================
// slice s0049aca0 (namespace c49aca0)
// ======================================================================================
namespace c49aca0 {
// Slice s0049aca0: a mission-UI "text list" panel.  The panel (`this` of 0x49aca0/0x49ad20/0x49b4f0, a
// MissionUi-derived widget) owns a container object (vptr 0x592848, base ctor 0x4931a0) that it builds in
// create() and a std::vector<E100> of text rows; create() also builds one flag-widget child per row (UiA,
// vptr 0x591d58, base ctor 0x48ae20).  Also here: the container's scalar deleting destructor, the vector
// fast path / insert / _Insert_n, a Mission::add_placed trampoline and the text-rect layout wrapper.
// Names and field names are provisional; every offset is read off the disassembly.

using rgl::vector4;

// ---- shared engine types -------------------------------------------------------------------------
using rgl::mesh;

// ---- std::vector<NodeA*> (the container's rows): push_back 0x49b120 / insert 0x49b170 are game/ui/widgets/drop_down_list.cpp's instances;
// _Insert_n 0x49b1c0 is the real vector's, instantiated below (after the namespace)
// ---- function definitions (in VA order) ----------------------------------------------------------

// 0x49b37d is not a function: it is the catch(...) funclet inside the vector<NodeA*>::_Insert_n (std::vector<NodeA*>::_Insert_n 0x49b1c0, whose COMDAT really spans
// 0x49b1c0..0x49b4af); see nonmatching.txt.

// 0x49b4f0 is MissionUi::get_xy (game/ui/widgets/widget_manager.cpp): the base class's text-rect slot, not this panel's

} // namespace c49aca0

struct NodeA;                                    // the vector's element points at the container (DropDown's rows, game/ui/widgets/widget_manager.cpp)
// @ 0x49b1c0
template void std::vector<NodeA*>::_Insert_n(std::vector<NodeA*>::iterator, size_t, NodeA* const&);

// @ 0x49b4b0
// Mission::place: place the prop `item` (an ItemRef: id, modifier word) at the trigger block's frame.
int Mission::place(const ItemRef* p)
{
    int a = -1;
    int b = 6;
    return add_placed(&b, &p->item, &p->word, &::g_game->trigger_frame, &a);
}

// @ 0x49b630
void gui_manager::set_font_mesh(const tstring* text, int arg)
{
    {
        tstring name("ui_font");
        m1480.res = (skin*)g_resmgr.get_material(name);
    }
    m1480.clear_geometry();
    if (text->len <= 0) {
        m1480.mask = 0;
        m1220.mask = 0;
        return;
    }
    float   out[2];
    vector4 pos;
    float   scale[2];
    float   xmin = -0.35f;
    float   xmax = 0.35f;
    pos.x = -0.35f; pos.y = 0.0f; pos.z = 0.0f;
    scale[0] = 0.017f; scale[1] = 0.017f;
    g_font->layout_text(out, text, &pos, scale, &m1480, (const unsigned*)&arg, &xmin, &xmax, 0x100);
    m1480.mask = 1;
    m1220.mask = 1;
}


// ======================================================================================
// slice s0049bea0 (namespace c49bea0)
// ======================================================================================
// GameData members of slice s0049b630 (the slices called the game-data object `Mission`): the value counters, the quest
// "taken" stamp and the label text layout. Fields not in game_data.h are reached by offset.
// ======================================================================================
#include "game/campaign/event.h"           // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// ---- shared log tail of 0x49b720 / 0x49b820 (inlined in both; the source of the original
// likely had a static helper, but cl 13.10 inlines it at /O2 regardless of `noinline`) ----------
static __forceinline void value_message(GameData* m, int amount, const char* key, const int* sound_id)
{
    char buf[0x200];
    {
        tstring s(key);
        sprintf(buf, g_strtable.lookup(s)->c_str(), m->string_value(amount).c_str());
    }
    g_gui.add(tstring(buf), -1);
    g_sound_mgr.play(*sound_id, 1, &g_sound_mgr.frame.o, 1.0f, 0);
}

// @ 0x49b720
void GameData::add_gold(int gold, int troop)
{
    Troop* rec = troops + troop;
    rec->gold += gold;                             // Troop +0x3b4 (gold)
    if (troop == 0 && f13a34 != 0 && gold != 0)
        value_message(this, gold, "ui_you_got_value", &g_sound_mgr.slot[10]);
}

// @ 0x49b820
void GameData::remove_gold(int gold, int troop)
{
    Troop* rec = troops + troop;
    rec->gold -= gold;
    if (rec->gold < 0)
        rec->gold = 0;
    if (troop == 0 && f13a34 != 0 && gold != 0)
        value_message(this, gold, "ui_you_lost_value", &g_sound_mgr.slot[9]);
}

static inline bool quest_bit(unsigned v, int n) { return ((v >> n) & 1) != 0; }

// @ 0x49b930
void GameData::start_quest(int quest, int giver_troop)
{
    WL_MARKER(12)
    quests[quest].giver_troop = giver_troop;       // +0x118
    quests[quest].f11c = (float)(g_ticks[game_clock.clock] - game_clock.start) * 1e-5f;   // +0x11c (game time when started)
    QuestRec* rec = &quests[quest];
    if (quest_bit(rec->flags2, 1) || !rec->f64_flag) {
        tstring name(quests[quest].text);
        char buf[0x200];
        {
            tstring key("ui_new_quest_taken");
            const char* n = name.c_str();
            tstring* f = g_strtable.lookup(key);
            mb_snprintf(buf, 0x200, f->c_str(), n);
        }
        g_gui.add(tstring(buf), 0xffd0f0c8);
        g_sound_mgr.play(g_sound_mgr.slot[2], 1, &g_sound_mgr.frame.o, 1.0f, 0);
        rec->flags = 0x100;
        WL_MARKER(13)
    }
}

struct LabelScale { float x, y; LabelScale(float a, float b) : x(a), y(b) {} };

// @ 0x49baa0
void GameData::label_text(rgl::mesh* m, const tstring* text, char flag, int arg, const float* h)
{
    unsigned flags = 0;
    rgl::vector4 pos(0.05f, 0.1f, 0.2f);
    LabelScale scale(0.2f, 0.2f);
    float   xmin = 0.0f;
    float   xmax = 1.0f;

    if (flag) {
        float x = f13acc * *h;
        pos = rgl::vector4(0.0f, 0.1f, 0.4f);              // 0x49bb06: 0x3ecccccd
        scale = LabelScale(x * 0.6f, x * 0.6f);
        xmin = x * -1.0f;
        xmax = x;
        flags = 0x10;
    }
    float   out[2];
    int     a = arg;
    Font* font = g_font;
    m->clear_geometry();
    font->layout_text(out, text, &pos, (float*)&scale, m, (const unsigned*)&a, &xmin, &xmax, flags);
}

// @ 0x49bbb0
void GameData::spawn_battle_icon(unsigned id)
{
    Event* rec = events.atu(id);
    rgl::strategic_entity* e = (rgl::strategic_entity*)g_script.level->create_entity(9);      // set_local_scaled is devirtualized: vtbl+0x98 is called directly
    rec->entity = e;

    {
        float radius = 0.3f;
        rgl::vector4 zero;
        zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        e->shape = new ::sphere(zero, &radius);         // operator new(0x44), then the constructor 0x4332a0 when it is not null
    }

    {
        rgl::matrix tmp;
        ((rgl::matrix&)e->local) = *identity_pos(&tmp);         // orig: rep movsd of 0x10 dwords, no virtual call (a real call needs the c49bea0 local identity_pos)
    }
    rec->bind_host();

    if (rec->type == 1) {
        parties.get_w(rec->party_a)->ai_behavior = 7;  // ai_bhvr_in_battle
        parties.get_w(rec->party_b)->ai_behavior = 7;
        parties.get_w(rec->party_a)->battle = id;
        parties.get_w(rec->party_b)->battle = id;
    }

    if (rec->type == 1) {
        {
            tstring nm("battle_icon");
            e->add_mesh((rgl::mesh*)((::resmgr*)&::g_resmgr)->get_object(&nm)->clone());
        }
        {
            float angle = 1.5707964f;                       // 0x3fc90fdb (1.5707963f rounds to 0x3fc90fda)
            e->meshes[0]->local_frame.rotate_about_u(&angle);
        }

        rgl::mesh* label = ::build_legend_mesh(e);   // 0x495b20

        {
            float h = 1.0f;
            tstring kn((parties.get_w(rec->party_b)->flags & 0x400) ? "ui_siege" : "ui_battle");
            label_text(label, g_strtable.lookup(kn), 1, 1, &h);   // 0x49baa0
        }

        e->local.o = rgl::vector4(rec->pos.x, rec->pos.y, rec->z + 0.4f);
    }
    e->game_id = id;
    e->map_kind = 3;
}

// @ 0x49bea0
void GameData::add_order_arrow(int id)
{
    using c49bea0::OrderObj;
    rgl::vector4 zero(0.0f, 0.0f, 0.0f);
    int idx = parties.lookup(id);
    float radius = 0.3f;
    float h;
    Party* e = (&*parties.pages.begin())[idx >> 4] + (idx & 0xf);
    OrderObj* o = c49bea0::order_create_entity(9);
    o->shape = new ::sphere(zero, &radius);           // operator new(0x44), then the constructor 0x4332a0 when it is not null
    e->model = o;
    {
        c49bea0::matrix tmp;
        o->local = *(rgl::smat*)c49bea0::identity_pos(&tmp);
    }
    e->update_flags();                                // 0x49bf6d: the original calls it on the record (ecx = ebx), not e->model
    e->label = ::build_legend_mesh(o);                // 0x495b20 (the original passes e->model, which is o)
    // the original keeps h (+0x10, reused from the radius), scale (+0x14 = 1.0, passed by pointer) and a dead copy of flags_hi
    float scale = 1.0f;
    {
        if (e->flags_lo & 0x80000) {
            tstring s("");
            g_game->label_text(e->label, &s, 0, 1, &scale);
        } else {
            const tstring* nm;
            if ((e->flags_lo & 0x3000) == 0)
                h = 0.5f;
            else if ((e->flags_lo & 0x3000) == 0x1000)
                h = 0.75f;
            else
                h = 1.0f;
            if (g_module_settings.show_party_ids_instead_of_names) {
                nm = &e->key;
            } else {
                // the found tstring* (or the record's own name tstring at +0xc) goes straight to label_text
                const tstring* f = 0;
                if (e->flag == 0)
                    f = g_strtable.find(&e->key);
                nm = f ? f : &e->name;
            }
            if (e->flags_lo & 0x400) {
                g_game->label_text(e->label, nm, 1, (int)h, &scale);
                tstring mat_name("map_legend_outline");
                skin* mat = g_resmgr.get_material(mat_name);
                skin* old = e->label->res;
                if (old) old->refcount--;
                e->label->res = mat;
                if (mat) mat->refcount++;
            } else {
                g_game->label_text(e->label, nm, 0, (int)h, &scale);
            }
        }
    }
    o->game_id = id;
    o->map_kind = 1;
    if (o->get_radius() < 0.9f) {
        id = 0x3f666666;                            // the original reuses the argument slot for the 0.9f
        o->set_fbc((const float*)&id);
    }
    e->place_on_map();
}

// @ 0x49c170
void Mission::check_slot(int id, unsigned bits)
{
    if (id >= 0 && id < g_mission_templates[mode].n_spawns) {
        SpawnRec* r = &g_mission_templates[mode].recs[id];
        int eax_, ebx_;

        // the unit's mark record (::Mission::recs, mission.h MarkRec): its current frame m from the copy m1, both counts cleared
        memcpy(&recs[r->id].m, &recs[r->id].m1, 0x40);
        recs[r->id].cnt2 = 0;                           // r->id is re-read: the stores may alias it
        recs[r->id].cnt = 0;
        add_amount(id, bits);
        int f = r->flags;
        if (f & 1) { announce_enemies = 1; return; }
        if (f & 2) { announce_reinforcements = 1; return; }
        int b;
        int a = f & 0x40;
        if (((f & 0x40) == 0 && (f & 0x80) == 0) || (a != 0 && f6214 == 0) || ((b = f & 0x80) != 0 && f6214 == 1)) { announce_reinforcements = 1; return; }
        if ((a != 0 && f6214 == 1) || (b != 0 && f6214 == 0)) { announce_enemies = 1; return; }
        if ((a != 0 && f6218 == 1) || (b != 0 && f6218 == 0)) { announce_reinforcements = 1; return; }
        if ((a != 0 && f6218 == 0) || (b != 0 && f6218 == 1)) announce_enemies = 1;
    } else {
        g_gui.add(tstring("Invalid entry no for add_reinforcements"), -1);
    }
}

// the agent's nav-mesh face (0x43cb30) through a forwarder: the agent pointer is formed in a scratch register, then moved to ecx
inline int nav_face(::Agent* a) { return a->update_nav_face(); }

static ::Agent* agent_by_id(Mission* m, unsigned id) {
    return &(&*m->agents.pages.begin())[id >> 4][id & 0xf];
}

// look a localisation key up in the mission string table; the held buffer, or "" when absent
static const char* order_string(const char* key) {
    tstring tmp(key);
    tstring* r = g_strtable.lookup(tmp);
    return r->buf ? r->buf : (const char*)&r->buf;
}

// @ 0x49c2c0
void Mission::order(int index, int order) {
    char ordstr[0x200];                 // this+0x38: the localised order caption
    char ordtxt[0x200];                 // this+0x238: the leader's order text
    char msg[0x200];                    // this+0x438: the composed log line

    rgl::vector4 p;                  // like rgl's 3-argument vector4: w is never set, and the whole-struct copies carry it
    int k = 0;
    ordstr[0] = 0;

    if (order == 0) {                           // hold this position
        TeamRec* L = &teams[index];
        if (L->leader != -1) {
            int f = nav_face(agent_by_id(this, L->leader));
            rgl::vector4 zero;               // (0, 0, 0) with w unset, as rgl's 3-argument vector4: set_vec copies the leftover w into every armed slot
            zero.x = 0.0f;
            zero.y = 0.0f;
            zero.z = 0.0f;
            L->set_vec(&agent_by_id(this, L->leader)->pos, &f, &zero);
            if (L->cls[0].use) L->cls[0].kind = 1;
            if (L->cls[1].use) L->cls[1].kind = 1;
            if (L->cls[2].use) L->cls[2].kind = 1;
            { tstring key("ui_hold_this_position_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
        }
    } else if (order == 1) {                    // follow me
        TeamRec* L = &teams[index];
        if (L->leader != -1) {
            if (L->cls[0].use) L->cls[0].kind = 2;
            if (L->cls[1].use) L->cls[1].kind = 2;
            if (L->cls[2].use) L->cls[2].kind = 2;
            int f = nav_face(agent_by_id(this, L->leader));
            L->set_vec(&agent_by_id(this, L->leader)->pos, &f,
                    &agent_by_id(this, L->leader)->v56b4);
            { tstring key("ui_follow_me_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
        }
    } else if (order == 2) {                    // charge
        TeamRec* L = &teams[index];
        if (L->cls[0].use) L->cls[0].kind = 0;
        if (L->cls[1].use) L->cls[1].kind = 0;
        if (L->cls[2].use) L->cls[2].kind = 0;
        { tstring key("ui_charge_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 11) {                   // stand ground
        TeamRec* L = &teams[index];
        if (L->cls[0].use) L->cls[0].kind = 3;
        if (L->cls[1].use) L->cls[1].kind = 3;
        if (L->cls[2].use) L->cls[2].kind = 3;
        { tstring key("ui_stand_ground_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 3) {                    // mount horses
        TeamRec* L = &teams[index];
        L->set_v(1);
        { tstring key("ui_mount_horses_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 4) {                    // dismount
        TeamRec* L = &teams[index];
        L->set_v(2);
        { tstring key("ui_dismount_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 7) {                    // stand closer
        TeamRec* L = &teams[index];
        L->bump(-1);
        { tstring key("ui_stand_closer_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 8) {                    // spread out
        TeamRec* L = &teams[index];
        L->bump(1);
        { tstring key("ui_spread_out_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 5) {                    // advance
        TeamRec* L = &teams[index];
        for (k = 0; k < 3; ++k) {
            ClassInfo* s = &L->cls[k];
            if (s->use == 0)
                continue;
            if (s->kind == 0)
                continue;
            rgl::vector4 step(s->dir.x * 7.0f, s->dir.y * 7.0f, s->dir.z * 7.0f);        // the three products are formed (z spilled to a float temp) before the adds
            p.x = step.x + s->pos.x;
            p.y = step.y + s->pos.y;
            p.z = step.z + s->pos.z;
            if (!point_in_rect(&p))
                continue;
            s->pos = p;
            s->nav = scene->nearest_face(&p.x);
            rgl::vector4 zero;               // (0, 0, 0) with w unset: the original copies the leftover frame word into extra.w
            zero.x = 0.0f;
            zero.y = 0.0f;
            zero.z = 0.0f;
            s->target = zero;
            s->kind = 1;
        }
        { tstring key("ui_advance_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 6) {                    // fall back
        TeamRec* L = &teams[index];
        for (k = 0; k < 3; ++k) {
            ClassInfo* s = &L->cls[k];
            if (s->use == 0)
                continue;
            if (s->kind == 0)
                s->pos = s->sum;
            rgl::vector4 p;                  // like rgl's 3-argument vector4: w is never set, and the whole-struct copies carry it
            rgl::vector4 step(s->dir.x * 7.0f, s->dir.y * 7.0f, s->dir.z * 7.0f);
            p.x = s->pos.x - step.x;
            p.y = s->pos.y - step.y;
            p.z = s->pos.z - step.z;
            if (!point_in_rect(&p))
                continue;
            s->pos = p;
            s->nav = scene->nearest_face(&p.x);
            rgl::vector4 zero;               // (0, 0, 0) with w unset: the original copies the leftover frame word into extra.w
            zero.x = 0.0f;
            zero.y = 0.0f;
            zero.z = 0.0f;
            s->target = zero;
            s->kind = 1;
        }
        { tstring key("ui_fall_back_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 9) {                    // use blunt weapons
        set_weapon_order_1(index, 1);
        { tstring key("ui_use_blunt_weapons_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 10) {                   // use any weapon
        set_weapon_order_1(index, 0);
        { tstring key("ui_use_any_weapon_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 12) {                   // hold fire
        set_weapon_order_2(index, 1);
        { tstring key("ui_hold_fire_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    } else if (order == 13) {                   // fire at will
        set_weapon_order_2(index, 0);
        { tstring key("ui_fire_at_will_e_"); strcpy(ordstr, g_strtable.lookup(key)->c_str()); }
    }

    // inform the local agent, but only about its own order group
    if (player_id >= 0) {
        ::Agent* la = agent_by_id(this, player_id);
        int g = la->team;
        if (g < 0 || g >= 8)
            g = 7;
        if (g == index) {
            teams[index].make_label(ordtxt);
            sprintf(msg, ordstr, ordtxt);
            tstring line(msg);                  // copied into the by-value argument (0x42c110), destroyed after the call
            g_gui.add(line, 0xfffffefb);
        }
    }

    // re-seed the order parameters of every agent in this order group
    ::AgentTable* tbl = &agents;
    unsigned id = tbl->first_used();
    while (id < (unsigned)tbl->count) {
        ::Agent* a = agent_by_id(this, id);
        int g = a->team;
        if (g < 0 || g >= 8)
            g = 7;
        if (g == index) {
            a->ai.ta.start = g_ticks[a->ai.ta.clock];
            a->ai.ta_interval = (float)(rand() % 0x3dc9) * 6.3223117e-05f + 1.0f;
        }
        tbl->next_used(&id);
    }
}
