#ifndef FACEGEN_INIT_H
#define FACEGEN_INIT_H
// s004f49d0 (ds1 slice 190): the "face_gen_window" construction (face generator menu).
// Provisional names; every offset comes from the disassembly of 0x4f49d0..0x4f5d17.
#include <windows.h>
#include <new>
#include <math.h>
#include "engine/math/rgl_math.h"
#include "game/ui/screens/character_sheet/cs_common.h"                                // widget constructors, make_* / label_refresh / new_button ...
#include "engine/resource/res_mgr.h"                                 // g_resmgr
#include "engine/render/mesh.h"                           // rgl::mesh::clone 0x41c6d0

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ----------------------------------------------------------------------------
#include "engine/core/tstring.h"

// ---- widgets ---------------------------------------------------------------------------------------------
struct Entity;

#include "game/ui/widgets/container.h"                         // MissionUi, ContainerBase (the 0x188-byte element), NodeA (the 0x190-byte NodeA)
#include "game/mission/camera_class.h"      // Camera (draw 0x488d50)

// the engine view frustum (0x120 bytes)
#include "engine/core/std_vec_insert_n.h"
#include "engine/scene/frustum.h"                   // rgl::frustum (0x120 bytes; the slices called it rgl::frustum)
namespace rgl { struct world; }

// the 0x358-byte camera body (ctor 0x4ed630): widget + world link + frustum copy
#include "game/campaign/map_widget.h"                 // MapWidget (the slice's face_body)
typedef MapWidget face_body;

#include "game/ui/screens/face_generator/face_gen.h"                              // FaceGen: the face-generator window record (ONE definition, ctor 0x4f49d0 is its slot 2)

// ---- engine vector of ints (lead dword, then first/last/end) ---------------------------------------------
#include "engine/core/engvec.h"                                 // engvec<T>

// ---- globals ---------------------------------------------------------------------------------------------
#include "game/ui/gui_manager.h"
#include "game/data/str_table.h"   // 0x43a1e0
#include "game/config/ini_file.h"
#include "game/ui/ui_globals.h"      // UI / core globals
extern "C" double atof(const char* s);                  // 0x537352

#include "engine/render/skin.h"                  // Skin, g_anim.skin_vec (shared with src/game/editor/editor_selection_and_translation.h)
#include "game/data/game_data.h"            // GameData (g_game): the one definition (was a Game view here)

#include "game/agent/animator.h"                              // Animator, g_anim


// ---- callees: label_refresh, make_slider / make_scrollbar / make_editor_list, new_key_label, make_*_arrow, new_button, new_sized_button,
// clamp_index, Animator::get_text are the definitions' own declarations (cs_common.h)
#include "engine/scene/world.h"                      // rgl::world: the 0x344-byte scene (step 0x427f40, reset 0x421390, create_entity 0x428440)

// the 0x7c-byte wind track (ctor 0x461480)
#include "engine/render/light.h"                             // Light (shared with game/terrain/wind_helpers.cpp)

// the game-variable lookups: the first site calls find (0x4bd5a0), the others expand it in place
static __forceinline float config_float_called(const char* key)
{
    tstring t(key);
    float f = 0.0f;
    int idx = g_game_vars.find(t);
    if (idx >= 0)
        f = (float)atof(g_game_vars.entries[idx].value.c_str());
    return f;
}
static inline int find_var(ini_file& ini, const tstring& key)
{
    int r = -1;
    for (int i = 0; r == -1 && i < ini.count; i++)
        if (ini.entries[i].key.equals(key))
            r = i;
    return r;
}
static __forceinline float config_float(const char* key)
{
    tstring t(key);
    float f = 0.0f;
    int idx = find_var(g_game_vars, t);
    if (idx >= 0)
        f = (float)atof(g_game_vars.entries[idx].value.c_str());
    return f;
}

#endif
