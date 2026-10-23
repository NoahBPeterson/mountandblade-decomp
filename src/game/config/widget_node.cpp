//-----------------------------------------------------------------------------
// File: widget_node.cpp  (0x49ad20..0x49b1c0)
//
// Desc: the drop-down button's list: DropDown::create builds the list container (NodeA) and one UiA row widget per
//       text row and registers the container with the UI root's widget vector; NodeA's scalar deleting destructor;
//       and the two std::vector<NodeA*> members that create() needs and that the linker kept from this object
//       (push_back 0x49b120, insert 0x49b170; _Insert_n 0x49b1c0 follows in the next range).
//       Names provisional. Flags: /O2 /MT /Gy /TP.
//
// Boundary: DropDown::on_event (0x49aca0) calls create (0x49ad20) forward: the object starts at 0x49ad20 (high).
// create -> push_back -> insert -> _Insert_n are further forward calls, but all four are the vector members
// std::vector<NodeA*> (template COMDATs land after their first caller), not separate objects; see MIGRATION.md.
//-----------------------------------------------------------------------------
#include "./widget_mgr.h"

extern char g_ui_dirty;                         // 0x5d8788 set when the widget list changed (0x49ad20)

// the UI root's widget vector sits at the start of the sound object (0x5d7080; handle at +0x170c). Its push_back
// is the out-of-line instance the original calls (0x49b120): the wrapper keeps it from being inlined.
struct NodeVec : std::vector<NodeA*> {
    // @ 0x49b120
    __declspec(noinline) void push_back(NodeA* const& x)                            // 0x49b120
    {
        if (size() < capacity())
            _Mylast = _Ufill(_Mylast, 1, x);
        else
            insert(end(), x);
    }
    // @ 0x49b170
    __declspec(noinline) iterator insert(iterator where, NodeA* const& x)           // 0x49b170 (std::vector::insert)
    {
        size_type off = size() == 0 ? 0 : where - begin();
        _Insert_n(where, 1, x);                                                     // 0x49b1c0 (next range)
        return begin() + off;
    }
};
#define g_root_widgets (*(NodeVec*)&g_sound)

#define ROWS() (rows_first ? (unsigned)(rows_last - rows_first) : 0u)       // the row vector's size(), re-read at each use

// @ 0x49ad20
NodeA* DropDown::create()
{
    NodeA* b = new NodeA;

    float wh[2];
    wh[0] = f1b0 * fD8;
    wh[1] = f1b4 * fDC;

    float pos[2];
    float nx = -(f1b8 * fD8);
    float h = (float)ROWS() * wh[1];
    pos[0] = fE0 - nx;
    pos[1] = fE4 - h;
    vt(b)->d22(pos);

    float sz[2];
    sz[0] = wh[0];
    sz[1] = (float)ROWS() * wh[1];
    vt(b)->d25(sz);

    g_ui_dirty = 1;
    NodeA* tmp = b;
    g_root_widgets.push_back(tmp);
    vt(b)->d19(0x20000);
    vt(b)->d20(4);

    for (unsigned u = 0; rows_first != 0 && u < (unsigned)(rows_last - rows_first); u++) {
        UiAInline* c = new UiAInline;

        float d[2];
        d[0] = 0.0f;
        d[1] = (float)(int)u * wh[1];
        vt(c)->d22(d);

        vt(c)->d25(wh);

        float m1[2];
        m1[0] = f1bc * fD8;
        m1[1] = f1c0 * fDC;
        vt(c)->d27(m1);

        float m2[2];
        m2[0] = f1c4 * fD8;
        m2[1] = f1c8 * fDC;
        vt(c)->d28(m2);

        vt(c)->d32(2, clone_gui(m1a4));
        ((gui_mesh*)c->tracks[2])->set_scale(vector4(fD8, fDC * 0.5f, 1.0f));
        vt(c)->d32(1, clone_gui(m1a0));
        ((gui_mesh*)c->tracks[1])->set_scale(vector4(fD8, fDC * 0.5f, 1.0f));
        vt(c)->d32(3, clone_gui(m1a8));
        ((gui_mesh*)c->tracks[3])->set_scale(vector4(fD8, fDC * 0.5f, 1.0f));

        float e[2];
        vt(c)->d41(e, rows_first + u);
        vt(c)->d19(0x10000);
        vt(c)->d20(3);
        vt(b)->d50(c);
    }
    return b;
}

// @ 0x49b0f0
void* NodeA::destroy(unsigned flags)
{
    *(void**)this = (void*)0x592848;
    ContainerBase::~ContainerBase();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

