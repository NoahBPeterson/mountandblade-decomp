// map10007.h: the id -> index hash that opens the party manager (PartyMgr, party.h): 0x2717 {key, value} slots, probed by
// +0x4f. Map10007::lookup is the function at 0x48ad00 (src/game/ui/mission_ui.cpp).
#ifndef MB_MAP10007_H
#define MB_MAP10007_H
struct MapEntry { unsigned key; int value; };
struct Map10007 {
    MapEntry tab[0x2717];
    int lookup(unsigned id);                // 0x48ad00 (thiscall, ret 4)
};
#endif // MB_MAP10007_H
