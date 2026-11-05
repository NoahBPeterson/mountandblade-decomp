// data_files.h: the Data\*.txt record classes the game-data loader (game_2.cpp: Game::load_game_data) fills and src/rgl/xmlcfg/records.cpp
// destroys: TroopFile (troops.txt, 0x508 bytes), QuestFile (quests.txt, 0x138) and FactionFile (factions.txt, 0x300) with the
// string vector (tstring_vec) the faction record reads its note names into. ONE definition for both: the destructors 0x431c40 /
// 0x431b10 / 0x432010 are the classes' own (records.cpp), their members' destructors are the real ones.
#ifndef MISSION5_DATA_FILES_H
#define MISSION5_DATA_FILES_H
#include <vector>
#include "../tstring.h"
#include "../item_ref.h"
#include "../../include/ptr_tag.h"             // ptr_tag: std::_Nonscalar_ptr_iterator_tag (empty, passed by value)
#include "../alloc_construct.h"                // alloc_tstring 0x436c90
struct stream_base;

// The out-of-line instances of std::vector<tstring> that other objects define (agent_combat.cpp 0x43e310, gui_manager 0x42d360 and
// 0x42d390) are called through their std:: names: an explicit specialization declared without a definition makes every use a call.
namespace std {
template<> void vector<tstring, allocator<tstring> >::_Insert_n(vector<tstring, allocator<tstring> >::iterator, size_t, const tstring&);   // 0x43e310
template<> void vector<tstring, allocator<tstring> >::_Destroy(tstring*, tstring*);                                                        // 0x42d360
template<> void vector<tstring, allocator<tstring> >::_Tidy();                                                                             // 0x42d390
}
struct tstring_alloc {
    int pad;                                        // +0 allocator subobject
};
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v, alloc_tstring& al, ptr_tag tag);   // 0x438f00 (agent_stats.cpp)
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, tstring_alloc& al, ptr_tag tag);       // 0x524cd0

// The faction record's note-name vector: std::vector<tstring> with the loader's out-of-line reserve (0x524bc0) and its push_back
// (the fill and the _Insert_n 0x43e310 call kept as the original compiled them).
struct tstring_vec : std::vector<tstring> {
    void reserve(unsigned n);                                   // 0x524bc0
    void push_back(const tstring& v)
    {
        if (size() < capacity()) {
            ptr_tag t;
            tstring* p = _Mylast;
            uninit_fill_n_str(p, 1, v, *(alloc_tstring*)this, t);
            _Mylast = p + 1;
        } else {
            _Insert_n(iterator(_Mylast), 1, v);
        }
    }
};

struct TroopFile {                  // troops.txt record (0x508 bytes); dtor 0x431c40
    tstring f0;                     // 0x000
    tstring f10;                    // 0x010
    tstring f20;                    // 0x020
    unsigned __int64 u30;           // 0x030
    int f38;                        // 0x038
    int f3c;
    int f40;
    int f44;
    int f48;
    int f4c;
    int f50;
    int f54;
    int f58[4];                     // 0x058
    unsigned f68[6];                // 0x068
    float f80[7];                   // 0x080
    int f9c;                        // 0x09c
    float fa0;                      // 0x0a0
    int fa4;
    int fa8;
    int fac;
    int fb0;
    ItemRef pairs[0x60];               // 0x0b4 .. 0x3b3
    int f3b4;                       // 0x3b4
    int f3b8;                       // 0x3b8
    unsigned __int64 u3c0[4];       // 0x3c0
    unsigned __int64 u3e0[4];       // 0x3e0
    char pad400[0x20];              // 0x400
    tstring names[8];               // 0x420
    int f4a0;                       // 0x4a0
    ItemRef p10[10];                   // 0x4a4
    std::vector<int> v;             // 0x4f4
    TroopFile();                    // 0x524710
    ~TroopFile();                   // 0x431c40 (src/rgl/xmlcfg/records.cpp)
    void read_file(stream_base& s); // 0x524420
};
struct QuestFile {                  // quests.txt record (0x138 bytes); dtor 0x431b10
    tstring f0, f10, f20, f30, f40; // 0x000 .. 0x040
    int f50;                        // 0x050
    tstring f54;                    // 0x054
    int f64;                        // 0x064
    char pad68[0x20];               // 0x068
    tstring names[8];               // 0x088
    int f108;                       // 0x108
    char pad10c[0xc];
    int f118;                       // 0x118
    int f11c;                       // 0x11c
    int f120, f124;                 // 0x120
    std::vector<int> v;             // 0x128
    QuestFile();                    // 0x524860
    ~QuestFile();                   // 0x431b10 (src/rgl/xmlcfg/records.cpp)
    void read_file(stream_base& s); // 0x524690
    tstring* raw_title();        // 0x48d750
};
struct FactionFile {                // factions.txt record (0x300 bytes); dtor 0x432010
    tstring name;                   // 0x000
    char b10;                       // 0x010
    char pad11[3];
    tstring f14;                    // 0x014
    int f24;                        // 0x024
    unsigned color;                 // 0x028
    float arr[0x80];                // 0x02c .. 0x22b
    int f22c;                       // 0x22c
    int f230;                       // 0x230
    tstring_vec v;                  // 0x234
    float f244;                     // 0x244
    float f248;                     // 0x248
    std::vector<int> v2;            // 0x24c
    char pad25c[0x20];              // 0x25c
    tstring names[8];               // 0x27c
    int f2fc;                       // 0x2fc
    FactionFile();                  // 0x5249f0
    ~FactionFile();                 // 0x432010 (src/rgl/xmlcfg/records.cpp)
    void read_file(stream_base& s, int count);   // 0x524a70
};
#endif // MISSION5_DATA_FILES_H
