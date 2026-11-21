// head_rec.h: the "head record" family of the editor/module tables: every object starts with the vtable
// 0x5947d8 (head_rec), a status int at +4 (0), the constant 0x63 at +8 and a name tstring at +0x1c.
// Derived classes store their own vtable after the base constructor ran (the vtables are named in
// symbols/slices/rec_tables.txt and symbols/slices/gui_edit.txt; a derived class declares one virtual so
// that cl emits its vptr store).
// Layout evidence: 0x4c2c50 (ResultWindow::ResultWindow: vt, name ctor +0x1c, +4 = 0, +8 = 0x63), 0x4c4c90 (head_rec_delete:
// vt 0x5947d8, name dtor), 0x4c49f0 (CampaignScreen::CampaignScreen), 0x4c4cc0 (CharSheet::CharSheet).
// Merged from s004c2b20 (Base0 / C4c2da8), s004c46d0 (head_rec), s004c3d90 (particle_cam).
#ifndef MESH2_HEAD_REC_H
#define MESH2_HEAD_REC_H
#include "../../game/tstring.h"
#include "../rgl_math.h"
#include "../scene/frustum.h"
#include "./mesh2_common.h"

#include "../../game/head_rec.h"

#endif
