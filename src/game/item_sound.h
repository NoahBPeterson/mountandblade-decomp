// item_sound.h: the sound sub-object at ItemKind +0x228 (a TimerList of ../ui/mission_ui.h; 0x10 bytes before the entries),
// played by 0x48add0 (thiscall, ret 4). ONE definition for src/game/party (party_window.h item_sound()) and
// src/game/inventory_panel (mission_ctx.cpp plays the one at game +0x139f4 the same way).
#ifndef GAME_ITEM_SOUND_H
#define GAME_ITEM_SOUND_H

struct ItemSound {
    char pad[0x10];
    void play(int id);                      // 0x48add0 (ret 4)
};

#endif // GAME_ITEM_SOUND_H
