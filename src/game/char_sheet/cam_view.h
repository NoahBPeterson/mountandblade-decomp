#ifndef CAM_VIEW_H
#define CAM_VIEW_H
// Slice s004ea8e0: the character-sheet camera/rotation widget. 0x4ea8e0 updates the model preview
// (zoom slider, clipboard pose dump, reset keys); 0x4eb050 is the per-frame orbit/zoom step.
// Names provisional; all offsets come from the disassembly of this range.
#include "../../include/image_addrs.h"
#include <windows.h>
#include <math.h>
#include "../../rgl/rgl_math.h"

using rgl::vector4;

#include "../../include/mat4.h"
using rgl::mat4;
using rgl::mat3;

#include "./cs_common.h"
#include "./cs_mat.h"                                // identity_pos, UiD, WindowMgrW, FaceRace
#include "../face_gen.h"                              // FaceGen: the face-generator window record (the per-frame update 0x4ea8e0 is its slot 3)
#include "../key_names.h"

// key state / settings globals

// globals of the face-generator update (0x4ea8e0)
extern float g_scroll;                                  // 0x590d7c

#include "../../include/skin_set.h"              // SkinSet, g_anim.skin_vec.first (shared with src/rgl/mesh2/gui_edit.h)

#include "../scene_world.h"                              // World::step 0x427f40
#include "../tstring.h"
#include "../str_table.h"      // 0x43a1e0
#include "../window_mgr.h"

#include "../../rgl/text_stream.h"                    // mb_snprintf (0x415100)
#include "../animator.h"                              // Animator, g_anim
#include "../ui_globals.h"      // UI / core globals

// a UI element; only the vtable slots this function calls are named
struct Ctrl {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30();
    virtual void v34(float v);                          // +0x34
    virtual float v38();                                // +0x38
    virtual void v3c(); virtual void v40();
    virtual char v44(int key);                          // +0x44
    virtual void v48(int key);                          // +0x48
    virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void v58(); virtual void v5c(); virtual void v60(); virtual void v64();
    virtual void v68(); virtual void v6c(); virtual void v70(); virtual void v74();
    virtual void v78(); virtual void v7c(); virtual void v80(); virtual void v84();
    virtual void v88(); virtual void v8c(); virtual void v90(); virtual void v94();
    virtual void v98(); virtual void v9c(); virtual void va0(); virtual void va4();
    virtual void va8(); virtual void vac();
    virtual void vb0();                                 // +0xb0
    virtual void vb4();                                 // +0xb4
    // (set_value 0x48baa0 is UiD's: ((UiD*)ctrl)->set_value(v))
};

#define SKIN_INT(off) (*(int*)((char*)g_anim.skin_vec.first + g_anim.cur * 0x1f8 + (off)))
#define g_dummy_mat_arr (g_anim.weights)

#endif
