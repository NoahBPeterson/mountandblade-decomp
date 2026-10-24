// meta_mission.h: the meta-mission object (global 0xa73300, MetaMission::init 0x4be742 of src/game/triggers/trigger_vec.cpp; the
// original brackets it with an 18-byte WinLicense marker pair, the entry 0x4be730 jumps over the marker to 0x4be742). ONE
// definition for src/game/triggers (the layout and the function) and src/game/party (game.cpp: LoadingPanel::sub 0x51c720 calls
// init on 0xa73300; its call goes through the marker entry 0x4be730, i.e. to the same function).
#ifndef GAME_META_MISSION_H
#define GAME_META_MISSION_H

struct MetaMission {
    int   f000;             // +0x000
    int   f004;             // +0x004
    int   f008;             // +0x008
    int   f00c;             // +0x00c
    int   f010;             // +0x010
    int   f014;             // +0x014
    int   f018;             // +0x018
    int   f01c;             // +0x01c
    int   f020;             // +0x020
    int   f024;             // +0x024
    char  pad028[0x14];
    int   f03c;             // +0x03c
    char  f040;             // +0x040
    char  pad041[0xf7];
    int   f138;             // +0x138
    int   f13c;             // +0x13c
    int   f140;             // +0x140
    int   f144;             // +0x144
    int   f148;             // +0x148
    int   pad14c;
    int   f150;
    int   f154;             // +0x154
    int   f158;             // +0x158
    int   f15c;             // +0x15c
    char  pad160[8];
    int   f168;             // +0x168
    char  pad16c[0x154];
    int   f2c0;             // +0x2c0
    int   f2c4;             // +0x2c4
    char  f2c8;             // +0x2c8
    void init();            // 0x4be742 (entered at 0x4be730 through the marker jump)
};

#endif // GAME_META_MISSION_H
