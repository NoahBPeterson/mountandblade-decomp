// tail2.h: Tail2, the 0x20-byte tail object of the agent view (game/view_state.h) and of the world records (rgl/xmlcfg/records.h,
// embedded there with size 0x20): a std::deque<int> (allocator word at +0, map +4, map size +8, offset +0xc, size +0x10) whose
// destructor tail-calls std::deque<int>::_Tidy, out of line at 0x4c4d10 (rgl/mesh2/gui_edit.cpp). The fields are the deque's: the view's
// constructor 0x475e50 clears map / mapsize / offset / size by hand.
#ifndef MB_TAIL2_H
#define MB_TAIL2_H
#include <deque>

struct Tail2 {
    void*        vec;      // +0
    void**       arr;      // +4  the map: an array of owned blocks
    unsigned int count;    // +8  the map size
    int          refbase;  // +0xc the offset
    int          refcap;   // +0x10 the size
    char         pad14[0x20 - 0x14];   // +0x14 (size 0x20: records.h)
    ~Tail2() { reinterpret_cast<std::deque<int>*>(this)->~deque(); }   // _Tidy 0x4c4d10
};

#endif // MB_TAIL2_H
