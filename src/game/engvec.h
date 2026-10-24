// engvec.h: the engine's vector (VC7.1 std::vector layout: allocator dword, then first / last / end). ONE definition for
// char_sheet/facegen_init (vector of ints, insert_n 0x40f8a0) and inventory_panel/view_open (vector of the world's wind
// pointers, insert_n 0x410610); the out-of-line insert_n is one template instance per element type, so its address
// is per instance. push_back is the inline fast path both slices spelled out.
#ifndef MB_GAME_ENGVEC_H
#define MB_GAME_ENGVEC_H

template <class T>
struct engvec {
    int lead;                                           // +0x00
    T* first;                                           // +0x04
    T* last;                                            // +0x08
    T* end;                                             // +0x0c
    void insert_n(T* where, unsigned n, const T& val);  // 0x40f8a0 (int), 0x410610 (wind pointers)  // old name of fill_n, removed after the names merge
    void __cdecl fill_n(int* dst, unsigned n, const int* val); // 0x40f8a0
    void push_back(const T& v) {
        if (first == 0 || (unsigned)(last - first) >= (unsigned)(end - first))
            insert_n(last, 1, v);
        else
            *last++ = v;
    }
};

#endif // MB_GAME_ENGVEC_H
