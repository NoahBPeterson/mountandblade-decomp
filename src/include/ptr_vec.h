// ptr_vec.h: std::vector<int*> as VC7.1 lays it out (allocator dword at +0, _First +4, _Last +8, _End +0xc), seen
// through the two member instances the migrated modules define.
//   pop_back_if 0x438bf0: first = [this+4]; if (first) { if (([this+8] - first) >> 2) [this+8] -= 4 }
//   erase_range 0x4c88a0: [this+8] is _Last: moves [last, _Last) down to `first`, stores the new _Last at [this+8]
//                         and returns `first` through the hidden return pointer (one-pointer iterator)
// The slices of src/rgl/mesh2 (gui_edit) had the first pointer at +0 and called _Last "end": the same record seen
// without its allocator dword.
// Users: src/game/agent2/agent_stats.cpp, src/rgl/mesh2/gui_edit.cpp.
#ifndef MB_PTR_VEC_H
#define MB_PTR_VEC_H

// the returned iterator of erase_range (one pointer, returned by value)
struct ptr_iter {
    int* p;                                                  // +0
};

struct ptr_vec {
    int al;                                                  // +0 allocator (empty class, one dword)
    int* first;                                              // +4
    int* last;                                               // +8
    int* end;                                                // +0xc
    unsigned size() const { return first == 0 ? 0 : last - first; }
    bool empty() const { return size() == 0; }
    void pop_back_if();                                      // 0x438bf0
    ptr_iter erase_range(int* first, int* last);             // 0x4c88a0 (thiscall, ret 0xc, hidden return)
};

#endif // MB_PTR_VEC_H
