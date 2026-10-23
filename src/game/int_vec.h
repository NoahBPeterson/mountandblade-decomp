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

#endif // GAME_INT_VEC_H
