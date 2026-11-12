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
// is the out-of-line instance the original calls (0x49b120): the wrapper keeps it from being inlined.
// @ 0x49b120
__declspec(noinline) void WidgetVec::push_back(MissionUi* const& x)                 // 0x49b120
{
    if (size() < capacity())
        _Mylast = _Ufill(_Mylast, 1, x);
    else
        insert(end(), x);
}
// @ 0x49b170
__declspec(noinline) WidgetVec::iterator WidgetVec::insert(iterator where, MissionUi* const& x)   // 0x49b170 (std::vector::insert)
{
    size_type off = size() == 0 ? 0 : where - begin();
    _Insert_n(where, 1, x);                                                     // 0x49b1c0 (next range)
    return begin() + off;
}

// the two-float arguments of the d22/d25/d27/d28 calls are 8-byte temporaries; the box position is the widget's
// (fE0, fE4) pair minus a temporary through an inline operator (both lanes stay on the x87 stack)
struct wn_float2 { float x, y; wn_float2(float a, float b) { x = a; y = b; } };
static inline wn_float2 operator-(const wn_float2& a, const wn_float2& b) { return wn_float2(a.x - b.x, a.y - b.y); }
// @ 0x49ad20
NodeA* DropDown::create()
{
    NodeA* b = new NodeA;

    float wh[2];
    wh[0] = f1b0 * fD8;
    wh[1] = f1b4 * fDC;

    vt(b)->d22((float*)&(*(const wn_float2*)&fE0 - wn_float2(-(f1b8 * fD8), (float)rows.size() * wh[1])));

    vt(b)->d25((float*)&wn_float2(wh[0], (float)rows.size() * wh[1]));

    MissionUi* tmp = b;
    g_gui.dirty = 1;
    g_gui.objs.push_back(tmp);
    vt(b)->d19(0x20000);
    vt(b)->d20(4);

    for (unsigned u = 0; u < rows.size(); u++) {
        UiA* c = new UiA;

        vt(c)->d22((float*)&wn_float2(0.0f, (float)(int)u * wh[1]));

        vt(c)->d25(wh);

        vt(c)->d27((float*)&wn_float2(f1bc * fD8, f1c0 * fDC));
        vt(c)->d28((float*)&wn_float2(f1c4 * fD8, f1c8 * fDC));

        vt(c)->d32(2, ((rgl::mesh*)(m1a4))->clone());
        ((rgl::mesh*)c->tracks[2])->set_color(vector4(fD8, fDC * 0.5f, 1.0f));
        vt(c)->d32(1, ((rgl::mesh*)(m1a0))->clone());
        ((rgl::mesh*)c->tracks[1])->set_color(vector4(fD8, fDC * 0.5f, 1.0f));
        vt(c)->d32(3, ((rgl::mesh*)(m1a8))->clone());
        ((rgl::mesh*)c->tracks[3])->set_color(vector4(fD8, fDC * 0.5f, 1.0f));

        float e[2];
        vt(c)->d41(e, &rows[u]);
        vt(c)->d19(0x10000);
        vt(c)->d20(3);
        vt(b)->d50(c);
    }
    return b;
}
