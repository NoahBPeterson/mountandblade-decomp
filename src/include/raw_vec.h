// raw_vec.h: the std::vector<T*> triple with its allocator dword, in its own header (script_state.h holds the map scene's material lists of this type;
// game/int_vec.h, which includes this, also defines IntVec).
#ifndef MB_RAW_VEC_H
#define MB_RAW_VEC_H

// a std::vector<T*> triple kept as raw pointers (allocator dword at +0, then first / last / end): the freshly created physics record of
// agent4 (PhysRecInit bones / bodies, zeroed in place, no cleanup) and the map scene's material lists g_script.mat_type0 .. mat_sea2 (0xa732c0, 0xa732d0,
// 0xa732e0, 0xa732f0: stride 0x10) and the cells' mesh lists, which mission5 pushes onto through _Insert_n 0x410610 (this = the vector)
struct RawVec {
    int pad;                                // +0 the allocator dword
    void** first;                           // +4
    void** last;                            // +8
    void** end;                             // +0xc
    RawVec() : first(0), last(0), end(0) {}
    int count() const { return first ? (int)(last - first) : 0; }
    template <class T> T* at(unsigned i) const { return (T*)first[i]; }
};

#endif // MB_RAW_VEC_H
