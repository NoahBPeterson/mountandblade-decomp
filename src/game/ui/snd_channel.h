// snd_channel.h: one sound channel (0x1c bytes): play 0x4875e0 (thiscall, ret 4), emit 0x4b09e0 (thiscall, ret 0x14). Two
// of them make a snd_slot (+0x10 / +0x2c, ui/item_ref.h), one is the voice of a TimerChild (+4, ui/mission_ui.h) and the
// placed scene records of options_panel/mission_items.cpp carry one at +0x30 (play 0x4875e0).
#ifndef GAME_UI_SND_CHANNEL_H
#define GAME_UI_SND_CHANNEL_H
struct snd_channel {                // 0x1c bytes
    char pad[0x1c];
    bool play(int arg);             // 0x4875e0 (thiscall, ret 4)
    char emit(int idx, int zero, int* count, void* vals, int tag);   // 0x4b09e0 (thiscall, ret 0x14; mission_ui.cpp 0x488e90)
};
#endif // GAME_UI_SND_CHANNEL_H
