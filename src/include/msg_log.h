// msg_log.h: the on-screen message log (object at 0x5d7080, g_msglog in agent_fx_sound.h): the enabled byte at +0x1714
// and the 32-entry ring at +0xb0b8. ONE definition for src/game (agent3/agent_fx_sound.cpp, which also declares the
// object) and src/rgl/xmlcfg/game_util.cpp (MsgLog::push 0x4334b0). Offsets: push 0x4334b0 (ring); enabled and the ring are also read by agent_fx_sound.cpp.
#ifndef MB_MSG_LOG_H
#define MB_MSG_LOG_H
#include <vector>
#include "../game/tstring.h"

struct MsgLog {
    char pad0[0x1714];
    char enabled;                                       // +0x1714
    char pad1715[0x171c - 0x1715];
    float total_h;                                      // +0x171c summed height of the visible lines (0x452c00)
    int cur;                                            // +0x1720 index of the newest ring entry
    char pad1724[4];
    char text_buf[0x4c00];                              // +0x1728 32 rgl::mesh objects of 0x260 bytes: the line texts (0x452c00)
    char bg_buf[0x4c00];                                // +0x6328 32 meshes: their backgrounds
    int age[32];                                        // +0xaf28
    float height[32];                                   // +0xafa8
    char padb028[4];
    int color[32];                                      // +0xb02c
    char padb0ac[0xb0b8 - 0xb0ac];
    struct Ring {
        int count;                                      // +0xb0b8
        int head;                                       // +0xb0bc
        tstring names[32];                              // +0xb0c0
        int vals[32];                                   // +0xb2c0
        void pop(tstring& out, int& index);             // 0x4f13f0 (src/game/char_sheet/msg_log_ui.cpp): oldest entry, or "No messages" and -1
    } ring;
    std::vector<tstring> lines;                         // +0xb340 (first at +0xb344) the line history (0x452c00)
    void push(const tstring& s, int v);                 // 0x4334b0
    void add(tstring msg, int color);                   // 0x452c00 (thiscall, ret 0x14)
    void reset_geometry();                              // 0x4e64b0 (src/game/cls_map/progress_bar.cpp: clear the 32+32 meshes, the ages to 1e6, the heights to 0.1)
    void update(float dt);                              // 0x4e6510 (flush the pending text, age the lines, fade the total height)
    void flush();                                       // 0x4f1390 (char_sheet/msg_log_ui.cpp): pops the ring into add()
    void reset();                                       // 0x42d1b0 (char_sheet/facegen_init.cpp 0x4f49d0: the UI manager reset; the 0x5d7080 object is also the gui manager)
    void build_scene();                                 // 0x4e6050 (the frame's scene build, src/game/cls_map/progress_bar.cpp; char_sheet/facegen_misc.cpp calls it)
};

#endif // MB_MSG_LOG_H
