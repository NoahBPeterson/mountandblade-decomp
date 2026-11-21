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
#include "../party2/camera_class.h"      // Camera: the scrolling view (relayout 0x48f020 / resize 0x48f120 are defined here)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../item_ref.h"
#include "../../include/frustum.h"
#include "../../include/party.h"

// @ 0x48ef40
void ContainerBase::update_children()
{
    for (std::vector<MissionUi*>::iterator p = kids.begin(); p != kids.end(); ++p) {
        MissionUi* o = *p;
        if (o)
            delete o;
    }
    kids.clear();
}

// @ 0x48efa0
bool NavData::shares(const int& a, const int& b) const
{
    int ai = cells[a].zone;
    int aj = -1;
    if (cells[a].zone == 0x5f5e0ff) {
        ai = cells[a].zone_pair[0];
        aj = cells[a].zone_pair[1];
    }
    int bi = cells[b].zone;
    int bj = -2;
    if (cells[b].zone == 0x5f5e0ff) {
        bi = cells[b].zone_pair[0];
        if (cells[b].zone_pair[1] >= 0)
            bj = cells[b].zone_pair[1];
    }
    return ai == bi || ai == bj || aj == bi || aj == bj;
}


// @ 0x48f020
void Camera::relayout(const tstring* text)
{
    // sub esp,0x10 in the original: 16 bytes, not 12. text_size writes three floats, and the trailing
    // get_state(&v[2]) writes *two* more (a 2D state) over v[2] and v[3] - the original's last dword. A float v[3]
    // here makes that second dword overwrite the saved frame (return address -> the state's float bit pattern).
    target->kids[0]->set_text(*text);
    float ty = tail.y, tx = tail.x;
    if (sqrt(ty * ty + tx * tx) > 0.0f) {
        target->kids[0]->set_xy(tail);
        V2 v = target->kids[0]->get_xy();
        target->kids[0]->set_xy(v);
        target->kids[0]->text_offset.y = v.y - text_scale.y * 0.63f - 0.001f;
        target->kids[0]->get_xy();
    }
    update();
}

// @ 0x48f120
void Camera::resize(const vec2& size)
{
    float a = size.x * 0.02f;
    MissionUi* w = target->kids[0];
    V2 v(a, size.y * 0.02f);
    w->set_size(v);
    cur_size = size;
    update();
}

// @ 0x48f180
void Party::init()
{
    float u_w;
    pos.x = 0;
    pos.y = 0;
    pos.z = 0;
    pos.w = u_w;                            // pos.w is never set: the reset stores a stack value
    // the 2-float members are assigned from vector2 temporaries: the original builds each one in its frame
    // (the store-then-reload of every nonzero float constant at esp+0x10 / esp+0x14) and copies it as dwords
    dir = Vec2(1.0f, 0.0f);
    old_dir = Vec2(0.1f, 0.0f);
    goal = Vec2(0.0f, 0.0f);
    dest = Vec2(0.0f, 0.0f);
    ai_target = Vec2(0.0f, 0.0f);
    battle = -1;
    model = 0;
    owner = 0;
    menu = 0;
    faction = -1;
    personality = 0;
    order_state = 0;
    ai_state = 0;
    state = 0;
    action = 0;
    f920 = 0;
    radius = 0.15f;
    base_speed = 0;
    speed_multiplier = 1.0f;
    flags_lo = 0;
    flags_hi = 0;
    cursor = 0;
    npath = 0;
    mesh = 0;
    nstacks = 0;
    morale = 0.5f;
    zoom = 0;
    f84 = 0;
    angle = 0;
    kind = 0;
    moving = 0;
    face_id = -1;
    needs_repath = 0;
    f598c = 0;
    ai_patrol_radius = 10.0f;
    alpha = -1000.0f;
    in_sight = 0;
    tgt_seen_pos = Vec2(-100000.0f, -1000000.0f);
    dirty = 1;
    label_dirty = 1;
    flag = 0;
    slots.clear();     // the slot array's vector (+0x59d8): drop the buffer
    engaged = 0;
    attached_to = -1;
    attached.clear();
    f5a04 = 0;
    banner_icon = 0;
    f98 = 1.0f;
    ai_initiative = 1.0f;
    float a = (rand() % 0x3dc9) * 6.322312e-05f;
    float b = (rand() % 0x3dc9) * 6.322312e-05f;
    trail_len = (b + a) * 0.7f + 0.5f;
    order_pos.z = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 2.0f;
    order_pos.w = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 2.0f;
    label = 0;
    e59ec = 0;
    e59f0 = 0;
}

// @ 0x48f450
SceneObject* TableauTbl::current()
{
    return worlds[worlds.size() - 1];
}

// @ 0x48f480
void TableauTbl::set_camera_position(const rgl::matrix* m)
{
    rgl::frustum* f = &worlds[worlds.size() - 1]->camera;
    f->m = *m;
    f->build_planes();
}

// @ 0x48f4c0
void TableauTbl::set_camera_parameters(const char* perspective, const float* w, const float* h, const float* near_, const float* far_)
{
    SceneObject* c = worlds[worlds.size() - 1];
    float fz = *far_;
    float nz = *near_;
    float top = *h * 0.5f;
    float bottom = *h * -0.5f;
    float right = *w * 0.5f;
    float left = *w * -0.5f;
    rgl::frustum* f = &c->camera;
    f->perspective = *perspective;
    f->nearz = nz;
    f->left = left;
    f->right = right;
    f->farz = fz;
    f->bottom = bottom;
    f->top = top;
    f->build_planes();
}

// @ 0x48f570
void TableauTbl::set_background_color(int c)
{
    worlds[worlds.size() - 1]->background = c;
}

// @ 0x48f5a0
void TableauTbl::set_ambient_light(const rgl::vector4* c)
{
    SceneObject* t = worlds[worlds.size() - 1];
    t->ambient_light = *c;
}

// the widget `idx` is re-read from the manager's vector at every use
#define OW(idx) (widgets[idx])

// @ 0x48f5f0
void widget_mgr::set_text(int idx, const tstring* text)
{
    if (OW(idx)->kind() == 0 || OW(idx)->kind() == 1 || OW(idx)->kind() == 9 || OW(idx)->kind() == 6) {
        OW(idx)->set_text(*text);
    } else if (OW(idx)->kind() == 7) {
        static_cast<Camera*>(OW(idx))->relayout(text);
    }
}

// @ 0x48f6b0
void widget_mgr::set_position(int idx, const vec2& pos)
{
    if (OW(idx)->kind() == 7) {
        MissionUi* o = OW(idx);
        o->move_a(pos);
        static_cast<Camera*>(o)->update();
        return;
    }
    float wx = (OW(idx)->f148 & 0x10) ? OW(idx)->rect_max.x * 0.5f : 0.0f;
    float wy = (OW(idx)->f148 & 0x1000) ? OW(idx)->rect_max.y * 0.5f : 0.0f;
    MissionUi* o = OW(idx);
    o->move_a(V2(pos.x - wx, pos.y - wy));
    OW(idx)->get_xy();
}
