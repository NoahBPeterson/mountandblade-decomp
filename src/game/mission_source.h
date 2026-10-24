// mission_source.h: the item source embedded at +0x30 of the map screens (Hud, the exchange screens, MapScreen of
// char_sheet/save_game): an STL vector of MissionItem* at +0x4/+0x8/+0xc, the item family at +0x10 (its sound is the
// family-th ItemSound), the current clock slot at +0x18 and the 64-bit tick the slot was entered at (+0x20/+0x24).
// ONE definition for options_panel/hud.h (the definition of pick 0x4d5410), map_editor/exchange_screens and
// char_sheet/save_game.h (a caller of pick). MissionItem (a Widget with the offset vector at +0xe0) is only pointed to here.
#ifndef MB_GAME_MISSION_SOURCE_H
#define MB_GAME_MISSION_SOURCE_H

struct MissionItem;

struct MissionSource {
    char           pad0[4];
    MissionItem**  begin;                           // +0x4
    MissionItem**  end;                             // +0x8
    MissionItem**  cap;                             // +0xc
    int            family;                          // +0x10
    char           pad14[0x18 - 0x14];
    int            slot;                            // +0x18
    char           pad1c[0x20 - 0x1c];
    int            tick_lo;                         // +0x20
    int            tick_hi;                         // +0x24

    // VC7.1 vector<MissionItem*>::size(): empty vector is NULL in this build
    unsigned size() const { return begin ? (unsigned)(end - begin) : 0; }

    void pick(int* arg);                            // 0x4d5410 (ret 4; handle_buttons passes &arg; the callback ignores it)
};

#endif // MB_GAME_MISSION_SOURCE_H
