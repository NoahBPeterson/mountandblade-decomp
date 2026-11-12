//-----------------------------------------------------------------------------
// File: mission_4.cpp  (0x48ef40..0x48f790)
//
// Desc: the container frame's release, the rider-row comparison, the field panel's text relayout and the widget terrain
//       refresh, the party record's reset, the tableau stack's camera / colour setters, the widget manager's text and position
//       setters. Names provisional. Functions in ascending address order (cl's bottom-up
//       emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: class changes only (SlotAPI | UiFrame at 0x48ef40, Tableau | Party at 0x48f790); no forward call in or out.
//-----------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../ui/ui_common.h"
#include "../ui/item_ref.h"
#include "../ui/mission_ui.h"
#include "../ui/container.h"
#include "../config/widget_mgr.h"
#include "./mission_4.h"
#include "../../include/tableau_table.h"   // TableauTbl: the camera / colour setters of the tableau on top (0x48f450..0x48f5a0)
#include "./field_panel.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../item_ref.h"
#include "../../include/frustum.h"
#include "../../include/party.h"

// @ 0x48ef40
void ContainerBase::update_children()
{
    MissionUi** p = kids._Myfirst;
    while (p != kids._Mylast) {
        MissionUi* o = *p;
        if (o)
            delete o;
        p++;
    }
    if (kids._Myfirst)
        operator delete(kids._Myfirst);
    kids._Myfirst = 0;
    kids._Mylast = 0;
    kids._Myend = 0;
}

// @ 0x48efa0
bool NavData::shares(const int& a, const int& b) const
{
    const NavCell* ra = &cells[a];
    int ai = ra->ids[0x11];
    int aj = -1;
    int id = ai;
    if (id == 0x5f5e0ff) {
        ai = ra->ids[0x12];
        aj = ra->ids[0x13];
    }
    const NavCell& rb = cells[b];
    int bi = rb.ids[0x11];
    int bj = -2;
    int id2 = bi;
    if (id2 == 0x5f5e0ff) {
        bi = rb.ids[0x12];
        int t = rb.ids[0x13];
        if (t >= 0)
            bj = t;
    }
    return ai == bi || bj == ai || aj == bi || aj == bj;
}

// FieldHost's view slot is element 0 of a std::vector<FieldVisual*> at +0x178 (f178 + view are its header): the
// original reads it through operator[] (base -> _Myfirst -> element chained in one scratch register). FieldHost in
// field_panel.h should declare the vector; until then the two readers here view it in place.
#define HOST_VIEWS(h) (*(std::vector<FieldVisual*>*)&(h)->f178)

// @ 0x48f020
void FieldPanel::relayout(const tstring* text)
{
    // sub esp,0x10 in the original: 16 bytes, not 12. text_size writes three floats, and the trailing
    // get_state(&v[2]) writes *two* more (a 2D state) over v[2] and v[3] - the original's last dword. A float v[3]
    // here makes that second dword overwrite the saved frame (return address -> the state's float bit pattern).
    float v[4];
    HOST_VIEWS(scene())[0]->text_size(v, text);
    float ty = tail.y, tx = tail.x;
    if (sqrt(ty * ty + tx * tx) > 0.0f) {
        HOST_VIEWS(scene())[0]->set_xy(&tail.x);
        HOST_VIEWS(scene())[0]->get_state(v);
        HOST_VIEWS(scene())[0]->set_xy(v);
        HOST_VIEWS(scene())[0]->f13c = v[1] - f144 * 0.63f - 0.001f;
        HOST_VIEWS(scene())[0]->get_state(&v[2]);
    }
    update();
}

// @ 0x48f120
void WItem::terrain_refresh(const float* off)
{
    FieldPanel* p = (FieldPanel*)(void*)this;
    float v[2];
    float a = off[0] * 0.02f;
    FieldVisual* w = HOST_VIEWS(p->scene())[0];
    v[0] = a;
    v[1] = off[1] * 0.02f;
    w->set_size(v);
    *(int*)&p->fD8 = ((const int*)off)[0];
    *(int*)&p->fDC = ((const int*)off)[1];
    p->update();
}

// the party record as the reset addresses it (names in src/include/party.h: pos, dirx.., radius, scale, attached, ...)
#define PI(off) (*(int*)((char*)this + (off)))
#define PF(off) (*(float*)((char*)this + (off)))
#define PB(off) (*(char*)((char*)this + (off)))

// @ 0x48f180
void Party::init()
{
    int u_w;
    PI(0x30) = 0;                           // pos
    PI(0x34) = 0;
    PI(0x38) = 0;
    PI(0x3c) = u_w;                         // pos.w is never set: the reset stores a stack value
    PI(0x964) = 0;                          // dest_x
    PI(0x960) = 0;                          // goal_y
    PI(0x95c) = 0;                          // goal_x
    PI(0x8f0) = 0;                          // state_f8f0
    PF(0x40) = 1.0f;                        // dirx
    PI(0x968) = 0;                          // dest_y
    PI(0x8ec) = 0;                          // state_f8ec
    PI(0x44) = 0;                           // diry
    PF(0x48) = 0.1f;                        // oldx
    PI(0x4c) = 0;                           // oldy
    PI(0x88) = -1;                          // f88 battle record
    PI(0x59c0) = 0;                         // model
    PI(0x8c) = 0;                           // owner
    PI(0x90) = 0;
    PI(0x70) = -1;                          // f70
    PI(0x74) = 0;                           // f74
    PI(0x934) = 0;                          // order_state
    PI(0x8e0) = 0;                          // interval
    PI(0x8e4) = 0;                          // state
    PI(0x918) = 0;                          // action
    PI(0x920) = 0;                          // f920
    PF(0x64) = 0.15f;                       // radius
    PI(0x8ac) = 0;                          // f8ac
    PF(0x8b0) = 1.0f;                       // f8b0
    PI(0xa0) = 0;                           // flags_lo
    PI(0xa4) = 0;                           // flags_hi
    PI(0x596c) = 0;                         // cursor
    PI(0x5970) = 0;                         // npath
    PI(0x5974) = 0;                         // mesh
    PI(0x8a8) = 0;                          // nstacks
    PF(0x7c) = 0.5f;                        // f7c
    PI(0x80) = 0;                           // zoom
    PI(0x84) = 0;
    PI(0x50) = 0;                           // angle
    PI(0x5998) = 0;                         // kind
    PI(0x5980) = 0;                         // moving
    PI(0x5984) = -1;                        // f5984
    PI(0x5988) = 0;                         // f5988
    PI(0x598c) = 0;                         // f598c
    PF(0x8f4) = 10.0f;                      // f8f4
    PF(0x68) = -1000.0f;                    // scale
    PI(0x6c) = 0;
    PF(0x5990) = -100000.0f;                // f5990
    PF(0x5994) = -1000000.0f;               // f5994
    PB(0x8b4) = 1;                          // dirty
    PB(0x8b5) = 1;                          // f8b5
    PB(0x1c) = 0;                           // flag
    if (PI(0x59dc))                         // the slot array's vector (+0x59d8)
        operator delete((void*)PI(0x59dc));
    PI(0x59dc) = 0;
    PI(0x59e0) = 0;
    PI(0x59e4) = 0;
    PB(0x8cc) = 0;                          // f8cc
    PI(0x8c8) = -1;                         // f8c8 attached-to
    if (PI(0x8bc))                          // attached ids
        operator delete((void*)PI(0x8bc));
    PI(0x8bc) = 0;
    PI(0x8c0) = 0;
    PI(0x8c4) = 0;
    PB(0x5a04) = 0;                         // f5a04
    PI(0x94) = 0;
    PF(0x98) = 1.0f;
    PF(0x958) = 1.0f;                       // f958
    float a = (rand() % 0x3dc9) * 6.322312e-05f;
    float b = (rand() % 0x3dc9) * 6.322312e-05f;
    PF(0x59c4) = (b + a) * 0.7f + 0.5f;     // trail_len
    PF(0x92c) = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 2.0f;   // vx
    PF(0x930) = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 2.0f;   // vy
    PI(0x59e8) = 0;                         // f59e8
    PI(0x59ec) = 0;                         // e59ec
    PI(0x59f0) = 0;                         // e59f0
}

// @ 0x48f450
TabCurrent* TableauTbl::current()
{
    return (TabCurrent*)worlds[worlds.size() - 1];
}

// @ 0x48f480
void TableauTbl::set_camera_position(const rgl::matrix* m)
{
    rgl::frustum* f = &((TabCurrent*)worlds[worlds.size() - 1])->fr;
    f->m = *m;
    f->build_planes();
}

// @ 0x48f4c0
void TableauTbl::set_camera_parameters(const char* perspective, const float* w, const float* h, const float* near_, const float* far_)
{
    TabCurrent* c = (TabCurrent*)worlds[worlds.size() - 1];
    float fz = *far_;
    float nz = *near_;
    float top = *h * 0.5f;
    float bottom = *h * -0.5f;
    float right = *w * 0.5f;
    float left = *w * -0.5f;
    rgl::frustum* f = &c->fr;
    f->perspective = *perspective;
    f->nearz = nz;
    *(int*)&f->left = *(int*)&left;
    *(int*)&f->right = *(int*)&right;
    f->farz = fz;
    *(int*)&f->bottom = *(int*)&bottom;
    *(int*)&f->top = *(int*)&top;
    f->build_planes();
}

// @ 0x48f570
void TableauTbl::set_background_color(int c)
{
    ((TabCurrent*)worlds[worlds.size() - 1])->background = c;
}

// @ 0x48f5a0
void TableauTbl::set_ambient_light(const float* c)
{
    TabCurrent* t = (TabCurrent*)worlds[worlds.size() - 1];
    rgl::vector4* d = (rgl::vector4*)t->ambient;
    *d = *(const rgl::vector4*)c;
}

// the widget `idx` is re-read from the manager's vector at every use
#define OW(idx) ((TxObj*)widgets[idx])

// @ 0x48f5f0
void widget_mgr::set_text(int idx, const tstring* text)
{
    if (OW(idx)->type() == 0 || OW(idx)->type() == 1 || OW(idx)->type() == 9 || OW(idx)->type() == 6) {
        float v[2];
        OW(idx)->text_size(v, text);
    } else if (OW(idx)->type() == 7) {
        ((FieldPanel*)(void*)OW(idx))->relayout(text);
    }
}

// @ 0x48f6b0
void widget_mgr::set_position(int idx, const float* pos)
{
    if (OW(idx)->type() == 7) {
        TxObj* o = OW(idx);
        o->set_pos((float*)pos);
        ((FieldPanel*)(void*)o)->update();
        return;
    }
    float wx = (OW(idx)->flags148 & 0x10) ? OW(idx)->w * 0.5f : 0.0f;
    float wy = (OW(idx)->flags148 & 0x1000) ? OW(idx)->h * 0.5f : 0.0f;
    TxObj* o = OW(idx);
    float v[2] = { pos[0] - wx, pos[1] - wy };
    o->set_pos(v);
    OW(idx)->get_state(v);
}
