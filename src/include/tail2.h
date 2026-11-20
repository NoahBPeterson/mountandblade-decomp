// tail2.h: Tail2, the 0x20-byte tail object of the agent AI block (game/agent_ai.h, before it became a std::deque there) and of the world records (rgl/xmlcfg/records.h,
// embedded there with size 0x20): a std::deque<int> (allocator word at +0, map +4, map size +8, offset +0xc, size +0x10) whose
// destructor tail-calls std::deque<int>::_Tidy, out of line at 0x4c4d10 (rgl/mesh2/gui_edit.cpp). The fields are the deque's: the view's
// constructor 0x475e50 clears map / mapsize / offset / size by hand.
#ifndef MB_TAIL2_H
#define MB_TAIL2_H
#include <deque>

struct Tail2 : std::deque<int> {                // the deque's destructor is out of line (_Tidy 0x4c4d10)
    char pad14[0x20 - 0x14];   // +0x14 (size 0x20: records.h)
};

#endif // MB_TAIL2_H
