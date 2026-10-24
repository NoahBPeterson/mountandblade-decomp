// i64_vec.h: vector of 8-byte integers (std::vector<__int64>, VC7.1 layout with the allocator slot): the tail of the
// quest / faction / party-template records (QuestRec +0x128, FactionRec +0x24c, PartyTemplate +0xa0). ONE definition for
// src/game/party (the save-game loaders) and the record header game_recs.h; resize is the out-of-line instance 0x493090.
#ifndef MB_I64_VEC_H
#define MB_I64_VEC_H
struct stream_base;
struct I64Vec {
    int pad0;                               // +0
    __int64* _Myfirst;                      // +4   0x51e0b0 (element i read at _Myfirst + i)
    __int64* _Mylast;                       // +8
    __int64* _Myend;                        // +0xc
    void resize(unsigned n, __int64 val);   // 0x493090 (ret 0xc)
    void read(stream_base* s);              // 0x51e0b0
};
#endif // MB_I64_VEC_H
