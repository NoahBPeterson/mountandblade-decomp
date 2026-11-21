//-----------------------------------------------------------------------------
// File: widget_node.cpp  (0x49ad20..0x49b1c0)
//
// Desc: the drop-down button's list: DropDown::create builds the list container (NodeA) and one UiA row widget per
//       text row and registers the container with the UI root's widget vector; NodeA's scalar deleting destructor;
//       and the two std::vector<NodeA*> members that create() needs and that the linker kept from this object
//       (push_back 0x49b120, insert 0x49b170; _Insert_n 0x49b1c0 follows in the next range).
//       Names provisional. Flags: /O2 /MT /Gy /TP.
//
// Boundary: DropDown::key (0x49aca0) calls create (0x49ad20) forward: the object starts at 0x49ad20 (high).
// create -> push_back -> insert -> _Insert_n are further forward calls, but all four are the vector members
// std::vector<NodeA*> (template COMDATs land after their first caller), not separate objects; see MIGRATION.md.
//-----------------------------------------------------------------------------
#define WIDGET_VEC_INSTANTIATED_HERE   // gui_manager.h: push_back 0x49b120 and insert 0x49b170 of std::vector<NodeA*> are this file's
#include "../../include/image_addrs.h"
#include "./widget_mgr.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../font.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

#include "../strat/gui_manager.h"            // g_gui 0x5d7080

// the UI root's widget vector (gui_manager::objs, at the start of the object at 0x5d7080). Its push_back
// is the out-of-line instance the original calls (0x49b120).
// @ 0x49b120
template void std::vector<NodeA*, std::allocator<NodeA*> >::push_back(NodeA* const&);
// @ 0x49b170 (its _Insert_n is 0x49b1c0, next range)
template std::vector<NodeA*, std::allocator<NodeA*> >::iterator std::vector<NodeA*, std::allocator<NodeA*> >::insert(std::vector<NodeA*, std::allocator<NodeA*> >::iterator, NodeA* const&);

// the two-float arguments of the d22/d25/d27/d28 calls are 8-byte temporaries; the box position is the widget's
// position minus a temporary through an inline operator (both lanes stay on the x87 stack)
static inline V2 operator-(const vec2& a, const vec2& b) { return V2(a.x - b.x, a.y - b.y); }
// @ 0x49ad20
NodeA* DropDown::create()
{
    NodeA* b = new NodeA;

    V2 wh;
    wh.x = button_size.x * cur_size.x;
    wh.y = button_size.y * cur_size.y;

    b->move_a(position - V2(-(button_x * cur_size.x), (float)rows.size() * wh.y));

    b->set_xy(V2(wh.x, (float)rows.size() * wh.y));

    NodeA* tmp = b;
    g_gui.dirty = 1;
    g_gui.objs.push_back(tmp);
    b->set_d0(0x20000);
    b->set_d4(4);

    for (unsigned u = 0; u < rows.size(); u++) {
        UiA* c = new UiA;

        c->move_a(V2(0.0f, (float)(int)u * wh.y));

        c->set_xy(wh);

        c->set_size(V2(text_size.x * cur_size.x, text_size.y * cur_size.y));
        c->set_offset(V2(text_pos.x * cur_size.x, text_pos.y * cur_size.y));

        c->set_track(2, ((rgl::mesh*)(m1a4))->clone());
        ((rgl::mesh*)c->tracks[2])->set_color(vector4(cur_size.x, cur_size.y * 0.5f, 1.0f));
        c->set_track(1, ((rgl::mesh*)(m1a0))->clone());
        ((rgl::mesh*)c->tracks[1])->set_color(vector4(cur_size.x, cur_size.y * 0.5f, 1.0f));
        c->set_track(3, ((rgl::mesh*)(m1a8))->clone());
        ((rgl::mesh*)c->tracks[3])->set_color(vector4(cur_size.x, cur_size.y * 0.5f, 1.0f));

        c->set_text(rows[u]);
        c->set_d0(0x10000);
        c->set_d4(3);
        b->add_child(c);
    }
    return b;
}
