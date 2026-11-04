// container.h: the container widget (vtable 0x5924a0, 0x190 bytes) and the list container a drop-down opens (NodeA, vtable
// 0x592848). ONE definition: the slices called the container ContainerW (world_data.cpp: its members), ContainerBase
// (config/widget_mgr.h: its constructor and destructor), FrameBox (mission_ui2/mission_4.h: its release) and gui_window
// (strat/gui_manager.cpp: its mouse handlers, now ui/widget_slots.cpp); they are one class, a MissionUi with a vector of child widgets at +0x178.
//
// Slots it overrides (the other 40 are MissionUi's): animate 0x4908c0 (animate every child), render 0x490710 (the children's
// bounds), clear_c8 0x4906e0, move_a 0x490570, on_child_moved 0x4905c0, on_moved 0x4906b0 (all world_data/world_data.cpp), key
// (0x42cf50) and refresh (0x42cec0), both in ui/widget_slots.cpp, update_children 0x48ef40
// (mission_ui2/mission_4.cpp) and add_child 0x493a30 (slot 50). The constructor 0x4931a0 is world_data.cpp's; the destructor
// 0x490520 is the one NodeA's deleting destructor 0x49b0f0 calls, and 0x490930 is the container's own deleting destructor.
#ifndef GAME_UI_CONTAINER_H
#define GAME_UI_CONTAINER_H
#include <vector>
#include "./mission_ui.h"

struct ContainerBase : MissionUi {
    std::vector<MissionUi*> kids;               // +0x178 the child widgets (allocator word, first +0x17c, last +0x180, end +0x184)
    ContainerBase();                            // 0x4931a0 (world_data.cpp)
    ~ContainerBase();                           // 0x490520 (the deleting destructor of this class is 0x490930)
    virtual void animate(int p);                // +0x08 0x4908c0
    virtual void render(rgl::scene* sc, float* count, int c, int d);   // +0x28 0x490710 (ret 0x10)
    virtual void clear_c8();                    // +0x40 0x4906e0
    virtual void move_a(const float* p);           // +0x58 0x490570
    virtual void on_child_moved(MissionUi* child);   // +0x5c 0x4905c0
    virtual void on_moved();                    // +0x60 0x4906b0
    virtual char key(unsigned mask);            // +0x8c 0x42cf50
    virtual void refresh();                     // +0x90 0x42cec0
    virtual void update_children();             // +0xc4 0x48ef40, vtable slot 49: deletes every child, frees the vector
    virtual void add_child(MissionUi* child);   // +0xc8 0x493a30, vtable slot 50
};

// The list container a drop-down opens (vtable 0x592848): the container, with its own key handler (0x42d170:
// a click inside is handled by the children, a click on the container itself closes it).
struct NodeA : ContainerBase {
    char f188;                                  // +0x188 (1: a click on the container itself closes it)
    char pad189[0x190 - 0x189];
    NodeA() { f188 = 1; }
    explicit NodeA(char closes) { f188 = closes; }   // the sites that clear the byte
    ~NodeA() {}
    virtual char key(unsigned mask);            // +0x8c 0x42d170
};

// The text box (vtable 0x597050, inventory_panel/widget_text.cpp): the container, nothing added; the screens that list text
// rows build one per row.
struct text_box : ContainerBase {
    text_box();                                 // 0x50d0b0
    virtual ~text_box();                        // no code of its own: slot 0 is the deleting destructor 0x5021f0 (ICF-folded with PartyPanel's), the container's destructor expanded
};

#endif // GAME_UI_CONTAINER_H
