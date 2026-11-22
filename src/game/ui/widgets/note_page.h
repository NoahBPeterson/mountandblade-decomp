// note_page.h: NotePage, the page a map-text link opens in the notes window. Font::layout_map 0x4fd700 pushes one per
// <tag=value> span into the widget's list at MWidget +0x198 (game/ui/widgets/map_list.h pages), the tag value goes to +0x1a8 (page_args)
// and the span index to +0x1b8 (rect_links); the notes window (game/ui/screens/scroll_list_screen.cpp) reads pages[sel] / page_args[sel] back as
// build(page, arg). Its vector is std::vector<NotePage> (push_back 0x4fe0b0, insert 0x4fe190, _Insert_n 0x4fe1e0,
// _Uninit_fill_n 0x4fe120, _Uninit_copy 0x4fe4b0: src/game/agent/hud/hud_vector_instances.cpp).
// Evidence for the shape: 4 bytes (the instances step by 4, `sar 2`), a class (_Ptr_cat non-scalar, so the fills go
// through 0x4fe120 / 0x4fe4b0, which vector<int> never calls), copied bitwise, and a user-declared destructor that does
// nothing: _Insert_n 0x4fe1e0 keeps the empty element-destroy loop at 0x4fe358 (an enum or a trivial struct compiles
// without it), while ~vector (0x41f4b0, ICF-folded with vector<int>'s) frees without one.
#ifndef GAME_UI_NOTE_PAGE_H
#define GAME_UI_NOTE_PAGE_H

struct NotePage {
    enum { troop = 1, faction = 2, party = 3, quest = 4 };     // <trp=..>, <fac=..>, <p=..>, <qst=..>
    int page;
    NotePage(int p) : page(p) {}
    ~NotePage() {}
};

// The instances of std::vector<NotePage> the original keeps out of line where this compiler would expand them inline:
// push_back 0x4fe0b0 (layout_map calls it), insert 0x4fe190 (push_back's slow path calls it) and the two fill helpers 0x4fe120 / 0x4fe4b0 (_Insert_n calls them, and
// they in turn inline the element copy, which cl 13.10 here leaves as an allocator::construct call inside their try blocks).
// push_back is the header's own instance, declared `extern template` (the MS extension) so a user calls it instead of
// instantiating it; the other three are explicit specializations of the real <vector> / <memory> templates with the
// header's own bodies. All are defined in src/game/agent/hud/hud_vector_instances.cpp; declared here so every user binds to them.
#include <memory>
#include <vector>
namespace std {
template<> void _Uninit_fill_n(NotePage* _First, unsigned _Count, const NotePage& _Val,
                               allocator<NotePage>& _Al, _Nonscalar_ptr_iterator_tag);                       // 0x4fe120
template<> NotePage* _Uninit_copy(NotePage* _First, NotePage* _Last, NotePage* _Dest,
                                  allocator<NotePage>& _Al, _Nonscalar_ptr_iterator_tag);                     // 0x4fe4b0
#ifndef NOTE_PAGE_INSTANCES                            // (hud_vector_instances.cpp, which instantiates it, defines this)
extern template void vector<NotePage>::push_back(const NotePage& _Val);                                    // 0x4fe0b0
#endif
template<> void vector<NotePage>::_Xlen() const;                     // 0x43e2a0 (folded with vector<int>'s, agent3/agent_combat.cpp): an ordinary call, as 0x4fd490 makes it
#ifndef NOTE_PAGE_BUY_HERE                             // (agent_hud.cpp, which instantiates it, defines this: cl 13.10 refuses both in one unit)
extern template bool vector<NotePage>::_Buy(size_type _Capacity);                                        // 0x4fd490 (game/ui/hud/agent_hud.cpp)
#endif
template<> __declspec(noinline) vector<NotePage>::~vector();         // 0x41f4b0 (the linker folded it into the shared 4-byte _Tidy body; game/ui/hud/agent_hud.cpp)
template<> __declspec(noinline) vector<NotePage>::iterator vector<NotePage>::insert(
    vector<NotePage>::iterator _Where, const NotePage& _Val);                                                // 0x4fe190
}

#endif // GAME_UI_NOTE_PAGE_H
