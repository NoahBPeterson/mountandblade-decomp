//-----------------------------------------------------------------------------
// File: dialog_block_runner.cpp  (0x4b0940..0x4b1160)
//
// Desc: DialogBlock::run_line and DialogBlock::emit: the control-opcode block runner (opcodes below 30)
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


// g_script_error (0x5e259c) is a file static of operation_execute.cpp (one object with this file in the original)
int script_error_get();
void script_error_set(int v);

// ---- definitions from the old file's prelude and earlier sections that this part's code uses (copies; each is a static
// or inline helper, a type or a macro: none of them emits code or data of its own)
namespace rgl { struct mesh; }
namespace rgl { struct matrix; } typedef rgl::matrix pos;
// ======================================================================================
// slice s0049b630 (namespace c49b630)
// ======================================================================================
namespace c49b630 {
// Slice s0049b630: six Mission methods from one translation unit.
//   [0x49b630] rebuild the mission's UI "font mesh" (material lookup + layout_text)
//   [0x49b720] add "value" to an army slot and, for slot 0, log the gained amount
//   [0x49b820] subtract "value" from an army slot (clamped at 0) and log the lost amount
//   [0x49b930] mark a quest record as taken (write +0x118/+0x11c, log ui_new_quest_taken)
//   [0x49baa0] layout one text row into a mesh with the non-wrap/scale preset
//   0x49bbb0: the battle icon for one 0x98-byte army record (Mission::spawn_battle_icon)
// Names and field names are provisional; every offset is read off the disassembly.
// The class layout is shared with the neighbouring slices s0049bea0 / s0049c2c0 (Mission).
// (tstring.h inlined below)


using rgl::vector4;
using rgl::matrix;
using rgl::mesh;

// ---- engine tables / globals ---------------------------------------------------------------------

struct SoundMgr { int play(int idx, int flags, const vector4* pos, float freq, int* out); };  // 0x442650


struct TickPair {                                   // g_ticks[i].lo + g_ticks[i].hi * 2^32
    int lo, hi;
    operator __int64() const { return ((long long)hi << 32) | (unsigned)lo; }
};

extern "C" int __cdecl sprintf(char*, const char*, ...);        // 0x536f8f

// ---- scene objects -------------------------------------------------------------------------------

// the map entity world::create_entity(9) builds: rgl::strategic_entity (engine/scene/scene_entity.h)
typedef rgl::strategic_entity EntityFull;



} // namespace c49b630
// ======================================================================================
// slice s0049bea0 (namespace c49bea0)
// ======================================================================================
// GameData members of slice s0049b630 (the slices called the game-data object `Mission`): the value counters, the quest
// "taken" stamp and the label text layout. Fields not in game_data.h are reached by offset.
// ======================================================================================
#include "game/campaign/event.h"           // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
namespace c49b630 {
using rgl::vector4;
using rgl::matrix;
using rgl::mesh;

} // namespace c49b630
// ======================================================================================
namespace c49bea0 {
// Slice s0049bea0: 0x49bea0 (order arrow creation) and 0x49c170 (reinforcement slot check).
// 0x49bea0: look an agent/order slot up in the mission's agent table, create a scene object
//   (type 9) for it, give it a 0.3-radius sphere shape and an identity frame, refresh the order's
//   text entries and then attach the "ui_..." arrow label through 0x49baa0.
// 0x49c170: validate a reinforcement packet id against the current set and apply its flag bits
//   (reset the unit's copied frame, clear two counters, run 0x4677a0, then set the team slot flags).
// Names are provisional; every offset comes from these two functions and the neighbouring mission slices.

#define MB_NOTHROW throw()


struct vector4 { float x, y, z, w; };
struct matrix { float m[4][4]; };
inline matrix* identity_pos(matrix* out) { return (matrix*)rgl::identity_pos((rgl::matrix*)out); }   // 0x40cd00 (cdecl; returns out)



// scene object for one order entry (the mission order contour): rgl::strategic_entity (engine/scene/scene_entity.h)
typedef rgl::strategic_entity OrderObj;


struct OrderObjHost {
    virtual OrderObj* create_entity(int type);       // 0x428440 (ret 4)
};


// 0xa732b4 is g_level; the original calls the *direct* create_entity (0x49bea0: mov ecx,[0xa732b4]; push 9;
// call 0x428440), not a virtual slot - the object at g_level is not polymorphic (its first dword is data).
static OrderObj* order_create_entity(int type)
{
    return (OrderObj*)g_script.level->create_entity(type);
}

} // namespace c49bea0
// ======================================================================================
// slice s0049c2c0
// ======================================================================================
// Slice s0049c2c0: Mission::order (0x49c2c0, ~2.4 KB, `ret 8`). One `case` per army order: each of
// the three order slots belonging to the selected order leader is armed with a per-order state,
// the order's localised caption is fetched from the string table, and (only for the local agent's
// own order group) the caption plus the leader's order text is pushed to the message log. Finally
// every agent in the same order group gets its three order-parameter fields re-seeded from the
// global order table. `index` selects a leader (0..2), `order` the order (0..0xd).
// Names, and the two frame slots the compiler reads before writing on the advance/fall-back paths,
// are provisional.

#define MB_NOTHROW throw()
// the order leaders are the mission's TeamRec records (mission.h): cls[k] are the three order slots (ClassInfo; kind is the armed state,
// use the slot-in-use flag, pos the move target, target the extra word set by set_vec, nav the nearest face, dir the per-slot offset, sum the resting target)

// the agents are ::Agent (agent_obj.h): pos +0x50, team +0x1c4 (the order group, clamped to 7), the AI timer ai_ta() +0x2b0,
// ai_ta_interval +0x2c0, v56b4 (the follow direction); the table at mission+4 is ::AgentTable (pages: first at mission+0x1c)


struct TickPair { int a; int b; };
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f
// ==== error reporting and argument validators (static, same object) =========================================

// @ 0x49cc60
// "%s; LINE NO: %d:" to the on-screen message log and the text log, then raise the error flag.
static __declspec(noinline) void report(const char* msg)
{
    char buf[1024];
    sprintf(buf, "%s; LINE NO: %d:", msg, g_lineno);
    g_gui.add(tstring(buf), 0xffff5555);
    stream_base& lg = g_log;
    lg.w_cstr(buf);
    script_error_set(1);
}
// @ 0x49ccc0
__declspec(noinline) char __fastcall check_script_param(int* value, int* limit)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= *limit) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Script Parameter ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
__declspec(noinline) int PartyMgr::first_party_id()
{
    unsigned i = 0;
    while ((int)i < count && (&*pages.begin())[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        return serial;
    return (&*pages.begin())[i >> 4][i & 0xf].id;
}
__declspec(noinline) rgl::mat3* __cdecl rgl::identity3(mat3* out)
{
    vector4 a(1.0f, 0.0f, 0.0f), b(0.0f, 1.0f, 0.0f), c(0.0f, 0.0f, 1.0f);
    out->s = a;
    out->f = b;
    out->u = c;
    return out;
}
__declspec(noinline) rgl::matrix* __cdecl rgl::identity_pos(matrix* out)
{
    vector4 o(0.0f, 0.0f, 0.0f);
    mat3 m;
    *(mat3*)out = *identity3(&m);
    out->o = o;
    return out;
}
// ======================================================================================
// slice s0049deb0 (the view object Camera: camera_class.h)
// ======================================================================================
// Slice s0049deb0: the mission "view" object.  It is an engine object (base ctor 0x48ae20, base dtor
// 0x490520, descriptor 0x591c80) with a scene subobject at +0x188, a box frustum at +0x360 (0x120
// bytes) and the per-frame shake/colour/zoom fields that update() drives.  The view's own descriptor
// is 0x5931a8 and its size is 0x4a8.
//
// ctor 0x49deb0, type 0x49df30, mask 0x49df40, move 0x49df60, update 0x49dfe0,
// scalar deleting dtor 0x49e580, render 0x49e5b0.
namespace c49deb0 {   // the slice's own callee / data declarations (their link names carry this namespace)
using rgl::vector4;

// ---- callees that live in other slices -----------------------------------------------------------
inline void identity_pos(rgl::matrix* m) { rgl::identity_pos(m); }                   // 0x40cd00  out-only 4x4 identity (cdecl)

// ---- float / int constants (named by address, matching the game's own tables) ---------------------
extern const float g_549cd8;                    // 0x549cd8  4294967296.0f (unsigned->float bias)
extern const float g_k002;                    // 0x590cd0  0.02f

} // namespace c49deb0

// @ 0x4b0940
char DialogBlock::run_line(__int64* locals, int* idx, int* script)
{
    int out;
    char ok = ((Operation*)(items + *idx))->execute(locals, script, &out);
    if (out > 0) {
        char buf[512];
        _snprintf(buf, 0x200, "At %s.", text.c_str());
        g_gui.add(tstring(buf), 0xffff5555);
        stream_base& lg = g_log;
        lg.w_cstr(buf);
    }
    return ok;
}
// @ 0x4b09e0
// The mission script runner. Steps over the statements `items[idx]` of this holder. Each opcode is run
// through one of the interpreter helpers, then step_less decides whether to fall through or jump to the
// statement's skip target (f90). While the script is "inside a key frame" (the tag matches the current
// line, or the frame was opened earlier) the input frames are polled for the abort/return/continue keys.
char DialogBlock::emit(int script, int depth, int* limit, __int64* vals, unsigned tag)
{
    char last_ok = 1;                           // local_b05
    char done = 0;                              // local_b07
    char in_key = 0;                            // local_b06
    char brk = 0;                               // local_afa
    char first = 0;                             // local_af9
    char ok = 1;                                // local_af4
    int saved;                                  // local_af0
    int which;                                  // local_b04
    int idx = 0;                                // local_b00
    char advance = 1;                           // local_b08
    int pr_lo, pr_hi;                           // local_ae4 / local_ae0
    int out1;                                   // local_aec
    int out2;                                   // local_ad8
    int out3;                                   // local_ac4
    int range_lo, range_hi;                     // local_ad4 / local_adc
    int nsub;                                   // local_ae8
    long long value;                            // local_ad0 / local_acc
    DialogBlock* self = this;                   // local_af8
    __int64 locals[128];                        // local_a10: the block's local variables
    PairStack buf2;                           // local_410
    char msg[512];                              // local_610
    __declspec(align(8)) __int64 sub[16];                            // auStack_a98 + 2

    script_error_set(0);
    link();
    if (script_error_get() > 0) {
        const char* q = text.c_str();
        _snprintf(msg, 0x200, "At %s.", q);
        g_gui.add(tstring(msg), 0xffff5555);
        stream_base& lg = g_log;
        lg.w_cstr(msg);
    }
    script_error_set(0);
    buf2.count = 0;
    buf2.pad = 0;
    last_ok = 1;
    g_or_latch = 0;
    idx = 0;
    in_key = 0;
    done = 0;
    advance = 1;

    while (idx < count) {
        brk = 0;
        if ((tag & 0xffff) == (unsigned)idx || in_key) {
            in_key = 1;
            first = 0;
            for (;;) {
                g_keyboard.update(0.0f);
                if (!first) {
                    first = 1;
                    tstring s0 = renumber(locals, idx, 1, &pr_lo, &pr_hi);
                    set_progress_range(pr_lo, pr_hi);
                    refresh_settings_list(&s0, 1);
                }
                if ((g_keyboard.down[0x3b] && !g_keyboard.prev[0x3b]) || g_game->debugger.active == 0) {
                    g_game->debugger.active = 0;
                    set_debug_label();
                    tag = 0xffffffff;
                    done = 1;
                    break;
                }
                if (g_keyboard.down[0x3a] && !g_keyboard.prev[0x3a]) {
                    tag = 0xffffffff;
                    done = 1;
                    break;
                }
                if (g_keyboard.down[0x39] && !g_keyboard.prev[0x39]) {
                    int e = (int)tag >> 16;
                    if (g_scripts[e].name.equals(g_game->debugger.script))
                        tag = ((unsigned)e << 16) | (unsigned)g_game->debugger.line;
                    done = 1;
                    break;
                }
                if (g_keyboard.down[0x38] && !g_keyboard.prev[0x38]) {
                    brk = 1;
                    break;
                }
                if (g_keyboard.down[0x37] && !g_keyboard.prev[0x37])
                    break;
                pump_messages();
            }
            pump_messages();
        }

        int line = idx;
        int op = items[line].id & 0xfffffff;
        g_lineno = line;
        saved = line;
        ok = 1;
        g_cur_opcode = op;

        if (op < 0x15 && op != 1) {
            Elem* stmt = &items[line];
            int id = stmt->id;
            if (id == 3) {
                advance = 1;
                pop_try(locals, buf2.pairs, &idx);
            } else if (id == 4) {
                advance = 1;
                buf2.push(line, 0);
                idx = line + 1;
            } else if (id == 5) {
                if (advance) {
                    idx = stmt->f90;
                } else {
                    idx = line + 1;
                    advance = 1;
                }
            } else {
                int v = 0;
                advance = 1;
                if (id == 6) {
                    which = 1;
                    v = (int)stmt->get_operand(locals, &which, &out2);
                } else if (id == 7) {
                    which = 2;
                    v = (int)stmt->get_operand(locals, &which, &out2) - 1;
                } else if (id == 0xc) {
                    v = g_mission->agents.first_used();
                } else if (id == 0xb) {
                    v = (int)g_game->parties.first_party_id();
                } else if (id == 0xf) {
                    v = 0;
                } else {
                    mb_error("invalid opcode %d", id);
                }
                buf2.push(idx, v);
                value = v;
                items[line].set_lvalue(locals, value);
                if (!step_less(locals, buf2.pairs)) {
                    idx = items[line].f90;
                    goto step_end;
                }
                idx++;
            }
        } else if (op < 0x1e) {
            if (op == 1) {
                which = 0;
                int id = (int)items[line].get_operand(locals, &which, &out1);
                if (id < 0 || id >= g_scripts.count || depth >= 0x100) {
                    ok = 0;
                    goto fail;
                }
                {
                Elem* s = &items[line];
                int i = 1;
                which = i;
                for (; i < s->n; i++) {
                    sub[i - 1] = s->get_operand(locals, &which, &out1);
                    s = &items[line];
                    which = i + 1;
                }
                }
                unsigned t;
                if (brk)
                    t = 0;
                else if (g_game->debugger.active != 0 && g_scripts[id].name.equals(g_game->debugger.script))
                    t = ((unsigned)id << 16) | (unsigned)g_game->debugger.line;
                else
                    t = 0xffffffff;
                nsub = items[line].n - 1;
                ok = g_scripts[id].body.emit(script, depth + 1, &nsub, sub, t);
                goto after;
            } else if (op == 0x15) {
                which = 0;
                if (check_script_param(&which, limit))
                    items[line].set_lvalue(locals, vals[0]);
                idx++;
            } else if (op == 0x16) {
                which = 1;
                if (check_script_param(&which, limit))
                    items[line].set_lvalue(locals, vals[1]);
                idx++;
            } else if (op == 0x17) {
                which = 1;
                int k = (int)items[line].get_operand(locals, &which, &out3) - 1;
                which = k;
                if (check_script_param(&which, limit))
                    items[line].set_lvalue(locals, vals[k]);
                idx++;
            } else {
                idx++;
            }
        } else {
            ok = run_line(locals, &idx, &script);
        after:
            if (ok) {
                idx++;
            } else {
            fail:
                advance = 0;
                idx = items[line].f90;
            }
        }

    step_end:
        if (buf2.count == 0)
            last_ok = ok;
        if (in_key) {
            if (done) {
                in_key = 0;
                done = 0;
            }
            // the original reads `saved` into a register before the first call (which stores the block end back into the
            // slot through its last argument); both calls take the statement index, not the clobbered slot
            int at = saved;
            tstring s2 = renumber(locals, at, 1, &which, &saved);
            tstring s1 = renumber(locals, at, ok, &range_lo, &range_hi);
            set_progress_range(range_lo, range_hi);
            refresh_settings_list(&s1, 0);
        }
        if (script_error_get() > 0) {
            const char* q = self->text.c_str();
            _snprintf(msg, 0x200, "At %s.", q);
            g_gui.add(tstring(msg), 0xffff5555);
            stream_base& lg = g_log;
            lg.w_cstr(msg);
        }
        if (!last_ok)
            break;
    }

    return last_ok;
}
