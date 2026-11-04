//-----------------------------------------------------------------------------
// File: widget_slots.cpp
//
// Desc: the virtual slots of the widget base MissionUi (vtable 0x591c80) and of the widgets derived from it in
//       ui_widgets.h (UiA 0x591d58, UiB 0x591fb0, UiC 0x592078, UiD 0x592140). The original's tables hold, per slot,
//       a function that our source defines under another class (the slices split the widget code between
//       "Effect", "MissionUi", "UiA" ...) or a one-instruction leaf. A slot here is either that leaf (the linker folds
//       it with the original's: `xor al,al; ret`, `ret 4` ...) or a forwarding stub that /O2 compiles to a bare `jmp`
//       to the function the slot names (same `this`, same stack arguments). The owner classes are declared
//       minimally below; the class names and signatures must match the names of the definitions.
//       A pointer-to-member cast stands in where the stub's parameter has another type than the callee's (an int
//       slot argument, a float parameter): the stack words are passed on unchanged, which is what the jmp does.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "./mission_ui.h"
#include "./ui_widgets.h"
#include "./container.h"
#include "../strat/gui_manager.h"      // g_gui (the UI root: f1709 closes the top window)

// ---- the owners of the functions the original's slots name (definitions elsewhere) ------------------------------

// ---- MissionUi, vtable 0x591c80 ------------------------------------------------------------------------------

bool MissionUi::is_active() { return false; }
void MissionUi::on_child_moved(MissionUi* src) { (void)src; }
void MissionUi::on_moved() {}
void MissionUi::on_track_set(void* a) { (void)a; }
// 0x42cd60 (the slot the base and every widget that does not override it holds)
char MissionUi::key(unsigned mask)
{
    set_key_flags(mask);
    refresh();
    return 0;
}
// 0x42ce50
void MissionUi::refresh()
{
    if (fCC & fD0) {
        unsigned w = fD4;
        if (w & 1) v15c = v15c + 1;
        else if (w & 2) v15c = v15c - 1;
        if (v15c < f160) v15c = f160;
        else if (v15c > f164) v15c = f164;
        if (w & 3) g_gui.f1709 = 1;
    }
    fCC = 0;
}
int MissionUi::kind() { return 0; }

// ---- UiB, vtable 0x591fb0 ------------------------------------------------------------------------------------

// ---- UiC, vtable 0x592078 ------------------------------------------------------------------------------------

// ---- UiD, vtable 0x592140 ------------------------------------------------------------------------------------

// ---- ContainerBase, vtable 0x5924a0, and NodeA, vtable 0x592848 ----------------------------------------------
// The container's key and refresh are 0x42cf50 and 0x42cec0, NodeA's key is 0x42d170 (the slices wrote them as gui_window::mouse_event /
// wheel_focus / handle_event over a widget view; gui_window and its widget base are gone, these are the one definition).

// @ 0x42cf50
char ContainerBase::key(unsigned mask)
{
    char r = 0;
    if (f104 & 3) return 0;
    if (mask & 7) {
        for (std::vector<MissionUi*>::iterator it = kids.begin(); it != kids.end(); ++it) {
            MissionUi* c = *it;
            if ((c->f104 & 3) == 0) {
                bool inside = false;
                if (c->screen[2] + c->fE0 <= g_gui.mouse[0] && g_gui.mouse[0] < c->screen[0] + c->fE0 &&
                    c->screen[3] + c->fE4 <= g_gui.mouse[1] && g_gui.mouse[1] < c->screen[1] + c->fE4)
                    inside = true;
                if (!(mask & 4) && inside) {
                    if (c->b121) {
                        c->key(1);
                    } else {
                        c->key(2);
                        (*it)->b121 = 1;
                    }
                } else if (c->b121) {
                    c->key(4);
                    (*it)->b121 = 0;
                }
            }
        }
    } else {
        for (int i = (int)kids.size() - 1; i >= 0 && !r; i--) {
            MissionUi* c = kids[i];
            r = 0;
            if ((c->f104 & 3) == 0) {
                if (c->fE0 + c->screen[2] <= g_gui.mouse[0] && g_gui.mouse[0] < c->fE0 + c->screen[0] &&
                    c->fE4 + c->screen[3] <= g_gui.mouse[1] && g_gui.mouse[1] < c->fE4 + c->screen[1])
                    r = c->key(mask);
            }
        }
    }
    set_key_flags(mask);
    refresh();
    return r;
}

void ContainerBase::refresh()
{
    if ((fCC & fD0) && (fD4 & 4) && f158) {
        unsigned top = (unsigned)-1;
        for (unsigned i = 0; i < kids.size(); i++) {
            if (kids[i]->test_c8(0x10000))
                top = i;
        }
        if ((int)top >= 0)
            f158->set15c(top);
        f158->key(0x10000);
    }
    MissionUi::refresh();
}

char NodeA::key(unsigned mask)
{
    if (ContainerBase::key(mask)) return 1;
    if (f188 && (mask & 0x10)) {
        g_gui.f1709 = 1;
        return 1;
    }
    return 0;
}
