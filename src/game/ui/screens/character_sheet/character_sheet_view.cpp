// Slice s00509980: the character-sheet window's 3D portrait view.
//   0x509980  CharSheet::open_view     - build the portrait world (3D view) or the flat shadow fallback
//   0x50a140  ItemD::init_screen       - clear the selection, drop the buffers, rebuild the view
// The widget slots, tstring / rgl::frustum / face-body / world / wind layouts and the callee conventions
// follow the verified neighbours s004f49d0 (the same 0x358 face body, 0x344 world and 0x7c wind trio),
// s004f8c80 (frustum) and s00502300 (widget vtables).  Names are provisional; every offset comes from
// this slice's disassembly.
#define UIA_CTOR_OUT_OF_LINE   // the flat branch's new XyzButton calls the constructor 0x48b1c0 (call at 0x509a5f)
#include <stdlib.h>
#include <windows.h>
#include <new>
#include <vector>
#include "engine/math/rgl_math.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ----------------------------------------------------------------------------
#include "engine/core/tstring.h"

// ---- engine mesh -----------------------------------------------------------------------------------------
#include "engine/render/mesh.h"
using rgl::mesh;

// ---- widgets ---------------------------------------------------------------------------------------------
// 0x178-byte widget base (ctor 0x48ae20 installs vptr 0x591c80).  The slots used here are add_image (+0x7c),
// add_image2 (+0x80) and add_child (+0xc8); the rest are placeholders that keep the offsets honest.
#include "game/ui/widgets/container.h"                                 // MissionUi (the 0x178-byte widget base, ctor 0x48ae20), ContainerBase
#include "game/ui/widgets/ui_widgets.h"                                // XyzButton (the portrait icon, vtable 0x591ee8)

// load the widget's "<name>_*" parameters from the ini table (0x4e4c70)
void label_refresh(MissionUi* w, tstring name);                    // 0x4e4c70 (tstring by value)

// ---- frustum (0x120 bytes) -------------------------------------------------------------------------------
#include "engine/scene/frustum.h"                   // rgl::frustum (0x120 bytes; the slices called it rgl::frustum)
// ---- entities --------------------------------------------------------------------------------------------

// entity +0x108: the resource list the animation collect fills
struct named_object;
template<> void std::vector<named_object*, std::allocator<named_object*> >::push_back(named_object* const&);   // 0x41b750 (engine/resource/resource_manager.cpp's instance; the entity's +0x108 list is a std::vector<named_object*>)

// ---- wind track (0x7c bytes) -----------------------------------------------------------------------------
#include "engine/render/light.h"                             // Light (shared with game/terrain/wind_helpers.cpp)

// ---- the world (0x344 bytes) -----------------------------------------------------------------------------
#include "engine/core/engvec.h"                                     // engvec<T>

#include "game/ui/screens/character_sheet/cs_common.h"
#include "engine/scene/world.h"                      // rgl::world: the 0x344-byte scene (step 0x427f40, reset 0x421390, create_entity 0x428440)

// ---- the 0x358 portrait body (ctor 0x4ed630) -------------------------------------------------------------
#include "game/campaign/map_widget.h"                 // MapWidget (the slice's face_body)
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
typedef MapWidget face_body;

// ---- the object table and the resource manager -----------------------------------------------------------
#include "game/data/tableau_table.h"            // TableauTbl g_tableaus (0xa75f40)

#include "engine/resource/res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules

// ---- the global animator ---------------------------------------------------------------------------------
#include "game/agent/animator.h"                                  // Animator, g_anim
#include "game/ui/widgets/ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
int __cdecl clamp_index(const int* v, const int* lo, const int* hi);       // 0x432510

// ---- the game singleton ----------------------------------------------------------------------------------
#include "game/data/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)
#include "game/common/game_globals.h"      // canonical global declarations
#include "game/ui/ui_globals.h"
#include "game/ui/gui_manager.h"      // g_gui.fov_k (0x5e20a8)
#include "game/terrain/cell_grid.h"      // CellGrid (ctor 0x478ad0, dtor 0x475d50, spawn_wind 0x478f30): the one definition


#include "game/ui/screens/character_sheet/char_sheet_panel.h"            // CharSheet: open_view 0x509980 builds its portrait

// @ 0x509980 -----------------------------------------------------------------------------------------------
void CharSheet::open_view()
{
    f_4cc = 0;
    f_4d0 = 0;
    int tab = g_tableaus.game_tableaus[0];
    if (!(g_tableaus.game_tableaus[0] < 0)) {
        // ---- flat fallback: a shadow image plus the empty character icon ----
        if (mode == -1) {
            f_4d0 = new MissionUi;
            label_refresh(f_4d0, tstring("char_i_character_shadow"));
            {
                tstring nm("character_window_shadow");
                f_4d0->set_mesh(0, g_resmgr.get_object(&nm)->clone());
            }
            window->add_child(f_4d0);
            f_4cc = new XyzButton;                                  // ctor 0x48b1c0, vptr 0x591ee8
            label_refresh(f_4cc, tstring("char_i_character"));
            f_4cc->set_track(0, g_tableaus.make_mesh_from(&tab, &w14));
            window->add_child(f_4cc);
        }
    } else {
        // ---- 3D portrait: build the body, the world, the wind tracks and the camera ----
        face_body* b = new face_body;
        label_refresh((MissionUi*)b, tstring("char_i_character"));
        rgl::world* old = scene;
        if (old != 0) {
            old->~world();      // 0x475d50 (the CellGrid destructor body), then operator delete (0x535712, call at 0x509b28)
            ::operator delete(old);
            scene = 0;
        }
        float lo[2];
        lo[0] = -80.0f;
        lo[1] = -80.0f;
        float hi[2];
        hi[0] = 80.0f;
        hi[1] = 80.0f;
        int zero = 0;
        int zero2 = 0;
        int zero3 = 0;
        rgl::world* w = new rgl::world(lo, hi, &zero, &zero2, &zero3);   // 0x478ad0
        scene = w;
        w->ambient = rgl::vector4(0.23f, 0.25f, 0.27f);
        w->ambient.w = 1.0f;
        {
            float z2 = 0.0f;
            float five = 5.0f;
            int   col = (int)0xff221815;
            w->set_fog(&z2, &five, &col);   // 0x488cd0
        }
        {
            Light* wd = new Light;
            w->lights.push_back(wd);
            wd->color = rgl::vector4(0.1f, 0.15f, 0.2f);
            wd->base_color = rgl::vector4(0.1f, 0.15f, 0.2f);
            wd->pos = rgl::vector4(-2.0f, -2.0f, 12.0f);
            wd->type = 2;
            wd->range = 20.0f;
            wd->dir = rgl::vector4(-0.5f, 0.0f, -1.0f);
        }
        {
            Light* wd = new Light;
            w->lights.push_back(wd);
            wd->color = rgl::vector4(0.45f, 0.42f, 0.4f);
            wd->base_color = rgl::vector4(0.4f, 0.4f, 0.4f);
            wd->pos = rgl::vector4(2.0f, 2.0f, 2.0f);
            wd->type = 2;
            wd->range = 20.0f;
            wd->dir = rgl::vector4(0.6f, -1.0f, -2.0f);
        }
        b->world = w;
        b->scene3d.flags |= 0x1200;
        b->scene3d.clear_color = 0xff221815;
        // the camera frame: the body's aspect scaled by the global fov factor
        float aspect = b->rect_max.y / b->rect_max.x * g_gui.fov_k;
        fr.init(0.2617994f, aspect, 0.01f, 20.0f);
        rgl::matrix mat;
        identity_pos((pos*)&mat);
        // identity_pos (0x40cd00) has its worker 0x40cd60 copy its OWN uninitialised stack locals into the matrix' w
        // lanes, so s.w/u.w/f.w/o.w hold whatever an earlier callee left on the stack (the original's own uninitialised
        // read; build_planes below then computes with them).  [bm9fv 2026-11-08: the earlier `mat.u.w = 0x1200` pin,
        // which only reproduced one particular leftover for the harness, is removed; the harness stubs build_planes.]
        mat.o.x += mat.u.x * 1.2f;
        mat.o.y += mat.u.y * 1.2f;
        mat.o.z += mat.u.z * 1.2f;
        fr.m = mat;
        fr.build_planes();
        view_rect = rgl::vector4(0.0f, 0.0f, 0.0f);
        view_x = -0.3f;
        view_y = -0.09f;
        view_z = 0.7f;
        b->scene3d.fr = fr;
        rgl::entity* e = w->create_entity(9);
        rgl::matrix m2;
        identity_pos((pos*)&m2);
        m2.o.y = -0.12f;
        float ang = -0.2f;
        m2.rotate_about_f(&ang);
        ang = 0.1f;
        m2.rotate_about_s(&ang);
        e->set_local_dirty(&m2);
        window->add_child(b);
        if (mode == -1) {
            // seed the body's face from the troop record and start the idle animation
            Troop* rec = &g_game->chars[w14];
            rgl::lod_mesh* ra = 0;
            int skin = rec->flags32 & 0xf;
            int flags = 0;
            rgl::lod_mesh* rc = 0;
            g_anim.cur = clamp_index(&skin, &flags, &g_anim.char_count);
            g_anim.unpack(&rec->face_keys);
            rgl::lod_mesh* rb = 0;
            g_anim.collect(g_module_settings.limit_hair_colors != 0, &ra, &rb, &rc);
            e->clear();
            e->lods.push_back(ra);
            if (rc)
                e->lods.push_back(rc);
            if (rb)
                e->lods.push_back(rb);
        }
    }
}

// ---- the portrait view state (a different class from char_window: +0x7c/+0x8c are heap buffers, not
//      tstrings) --------------------------------------------------------------------------------------------
#include "game/data/mission_item_recs.h"          // ItemD: the screen record 0x4d0620 allocates (0xb4 bytes)

// @ 0x50a140 -----------------------------------------------------------------------------------------------
void ItemD::init_screen()
{
    page = -1;
    arg = -1;
    scroll_pos = -1.0f;
    typedef std::vector<int> IntVec;
    back_pages.~IntVec();                      // the vector's _Tidy: free the storage, zero the three pointers
    fwd_pages.~IntVec();
    build(g_game->notes_type, g_game->notes_object);
}
