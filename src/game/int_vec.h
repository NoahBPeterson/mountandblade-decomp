// int_vec.h: std::vector<int> as VC7.1 lays it out (the pad dword is the allocator), as the game modules use it.
// Users: src/game/party/game.cpp (0x51e840 / 0x51ea10 inline push_back, resize 0x43f310, _Insert_n 0x43e7b0),
// src/game/terrain_b/terrain_b.cpp (FaceHairRec::list at +0x1c of the 0xb0-byte hair record, read by the breed
// function 0x4746e0: candidate values picked by `g_rand_seed % size`).
#ifndef GAME_INT_VEC_H
#define GAME_INT_VEC_H

struct IntVec {
    int pad0;
    int* _Myfirst;                          // +4
    int* _Mylast;                           // +8
    int* _Myend;                            // +0xc
    IntVec() { _Myfirst = 0; _Mylast = 0; _Myend = 0; }
    void resize(unsigned n, int v);                         // 0x43f310
    void insert_n(int* where, unsigned count, const int* val);   // 0x43e7b0
    void _Insert_n(int* where, unsigned n, const int& v);   // 0x410610 (the std::vector<mesh*> instance of _Insert_n: mesh_slot/mesh_start.cpp pushes onto the entity's mesh list at +0x118 through it)
    void push_back_mesh(const int& v)       // the body mesh_start.cpp 0x484590 inlines for those lists: size and capacity first, the fast store, else _Insert_n
    {
        unsigned sz = _Myfirst == 0 ? 0 : (unsigned)(_Mylast - _Myfirst);
        unsigned cp = _Myfirst == 0 ? 0 : (unsigned)(_Myend - _Myfirst);
        if (sz < cp) { *_Mylast = v; ++_Mylast; }
        else _Insert_n(_Mylast, 1, v);
    }
    void push_back_out(const int& v);       // 0x440b30 (thiscall, ret 4): the out-of-line std::vector<int>::push_back (char_sheet/save_records.cpp calls it for the free-id list at game+0x138bc)
    void push_back(int v)                   // inlined by 0x51e840 and 0x51ea10: size, then the fast path
    {
        unsigned size;
        if (_Myfirst == 0)
            size = 0;
        else
            size = (unsigned)(_Mylast - _Myfirst);
        if (_Myfirst && size < (unsigned)(_Myend - _Myfirst)) {
            int* last = _Mylast;
            *last = v;
            _Mylast = last + 1;
        } else {
            insert_n(_Mylast, 1, &v);
        }
    }
};

// a std::vector<T*> triple kept as raw pointers (allocator dword at +0, then first / last / end): the freshly created physics record of
// agent4 (PhysRecInit bones / bodies, zeroed in place, no cleanup) and the map scene's material lists g_mat_vec[4] (0xa732c0, 0xa732d0,
// 0xa732e0, 0xa732f0: stride 0x10) and the cells' mesh lists, which mission5 pushes onto through _Insert_n 0x410610 (this = the vector)
struct RawVec {
    int pad;                                // +0 the allocator dword
    void** first;                           // +4
    void** last;                            // +8
    void** end;                             // +0xc
    RawVec() : first(0), last(0), end(0) {}
};

#endif // GAME_INT_VEC_H
