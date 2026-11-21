// data_files.h: the Data\*.txt record classes the game-data loader (game_2.cpp: Game::load_game_data) fills and src/rgl/xmlcfg/records.cpp
// destroys: TroopFile (troops.txt, 0x508 bytes), QuestFile (quests.txt, 0x138) and FactionFile (factions.txt, 0x300) with the
// string vector (tstring_vec) the faction record reads its note names into. ONE definition for both: the destructors 0x431c40 /
// 0x431b10 / 0x432010 are the classes' own (records.cpp), their members' destructors are the real ones.
#ifndef MISSION5_DATA_FILES_H
#define MISSION5_DATA_FILES_H
#include <vector>
#include "../tstring.h"
#include "../../include/note_block.h"
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

// The three records have the layouts of the game's own records (troop.h's Troop, game_recs.h's QuestRec / FactionRec); the
// names follow those and the Data\*.txt field order read_file reads (the Module System's troops / quests / factions tuples).
struct TroopFile {                  // troops.txt record (0x508 bytes); dtor 0x431c40
    tstring name;                   // 0x000 (read second, underscores to spaces)
    tstring plural;                 // 0x010
    tstring key;                    // 0x020 the troop id (trp_...), read first
    unsigned __int64 flags;         // 0x030 tf_* flags
    int reserved;                   // 0x038 (the tuple's reserved word, read after the scene)
    int f3c;
    int f40;
    int f44;                        // 0x044 (ctor 0)
    int f48;                        // 0x048 (ctor 0; Troop::level48)
    int faction;                    // 0x04c
    int upgrade_troop[2];           // 0x050 upgrade targets
    int attributes[4];              // 0x058 strength / agility / intelligence / charisma
    unsigned skills[6];             // 0x068 packed 4-bit skill levels, 8 per word
    float proficiencies[7];         // 0x080 weapon proficiencies (read as ints)
    int exp;                        // 0x09c random within the level's experience band
    float hp;                       // 0x0a0 strength + 35 + 2 * ironflesh
    int skill_points;               // 0x0a4
    int attr_points;                // 0x0a8
    int prof_points;                // 0x0ac
    int level;                      // 0x0b0 (at least 1)
    ItemRef inv[0x60];              // 0x0b4 .. 0x3b3 (64 item / modifier pairs read)
    int gold;                       // 0x3b4
    int site;                       // 0x3b8 the troop's scene (-1 = none)
    unsigned __int64 face_key1[4];  // 0x3c0
    unsigned __int64 face_key2[4];  // 0x3e0
    SoaBlock notes;                     // 0x400 the notes block (add_*_note_from_dialog, read_soa 0x51e1f0)
    ItemRef equip[10];              // 0x4a4 equipment slots (cleared)
    std::vector<int> slots;         // 0x4f4
    TroopFile();                    // 0x524710
    ~TroopFile();                   // 0x431c40 (src/rgl/xmlcfg/records.cpp)
    void read_file(stream_base& s); // 0x524420
};
struct QuestFile {                  // quests.txt record (0x138 bytes); dtor 0x431b10
    tstring name;                   // 0x000
    tstring desc;                   // 0x010
    tstring giver;                  // 0x020
    tstring title;                  // 0x030 the raw title (QuestRec::raw_title)
    tstring text;                   // 0x040 the key's translation, else the name
    int flags;                      // 0x050 qf_* flags
    tstring key;                    // 0x054 the quest id (qst_...), read first
    int status;                     // 0x064 (QuestRec::flags: active / finished / succeeded / failed / concluded bits)
    SoaBlock notes;                     // 0x068 the notes block (add_*_note_from_dialog, read_soa 0x51e1f0)
    int troop;                      // 0x10c
    int item;                       // 0x110
    int number;                     // 0x114
    int giver_troop;                // 0x118
    int f11c;                       // 0x11c
    int f120, f124;                 // 0x120
    std::vector<int> slots;         // 0x128
    QuestFile();                    // 0x524860
    ~QuestFile();                   // 0x431b10 (src/rgl/xmlcfg/records.cpp)
    void read_file(stream_base& s); // 0x524690
    tstring* raw_title();        // 0x48d750
};
struct FactionFile {                // factions.txt record (0x300 bytes); dtor 0x432010
    tstring name;                   // 0x000
    char b10;                       // 0x010
    tstring key;                    // 0x014 the faction id (fac_...), read first
    int flags;                      // 0x024 ff_* flags (bits 8..15: 100 - the maximum player rating)
    unsigned color;                 // 0x028
    float relations[0x80];          // 0x02c .. 0x22b one per faction
    int nranks;                     // 0x22c
    int f230;                       // 0x230
    tstring_vec ranks;              // 0x234 the rank names
    float f244;                     // 0x244 (ctor -1)
    float max_rating;               // 0x248 the maximum player rating / 100 (from flags)
    std::vector<int> slots;         // 0x24c
    SoaBlock notes;                     // 0x25c the notes block (add_*_note_from_dialog, read_soa 0x51e1f0)
    FactionFile();                  // 0x5249f0
    ~FactionFile();                 // 0x432010 (src/rgl/xmlcfg/records.cpp)
    void read_file(stream_base& s, int count);   // 0x524a70
};
#endif // MISSION5_DATA_FILES_H
