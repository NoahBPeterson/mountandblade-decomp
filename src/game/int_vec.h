// int_vec.h: std::vector<int> as VC7.1 lays it out (the pad dword is the allocator), as the game modules use it.
// Merged views: script_interpreter.h IntVector (+0x8b8 attached, tableau stack: size() 0x40f010), agent2 vec_int (fill_ptr 0x438c30),
// inventory_panel/widget_text.cpp vec_int (inline size / capacity, insert_n 0x49b1c0): all four dwords at +0..+0xc.
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
    // ---- out-of-line instances and inline helpers of the other modules (same layout: the object is std::vector<int>) ----
    int* fill_ptr(int* f, unsigned n, const int& x);        // 0x438c30 (agent2/agent_stats.cpp: the _Ufill instance; this is unused)
    unsigned size() const;                  // 0x40f010 (the out-of-line instance the script interpreter calls)
    unsigned count() const { return _Myfirst == 0 ? 0 : (unsigned)(_Mylast - _Myfirst); }       // widget_text.cpp 0x50d0e0: the inline size
    unsigned capacity() const { return _Myfirst == 0 ? 0 : (unsigned)(_Myend - _Myfirst); }
    void insert_n_49b1c0(int* pos, unsigned n, const int* val);    // 0x49b1c0 (a second instance of insert_n, widget_text.cpp)
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

#endif // GAME_INT_VEC_H
