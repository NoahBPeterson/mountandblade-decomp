// conv_screen.h: the conversation screen (ConvScreen; `this` of the opener 0x4f66e0, the line refresher 0x4f73c0 and 0x4f7d70):
// the talk overlay (+0x2c), the 3D window (+0x30, a MapWidget), the speaker / line labels, the question box (+0x188), the
// choices box (+0x18c, a Choices widget), 1024 body buttons from +0x190, the camera world matrix (+0x1194), the message window and
// the five sentence boxes (+0x11e0..). ONE definition for char_sheet/conv_open (the opener, a full layout) and conv_refresh
// (the refresher). The refresher's view placed the choices box at +0x188 and the question at +0x18c (0x4f7cb1 reads +0x18c for the
// choices box) and called the +0x17c widget `body`. Also the mesh stub the two slices share (clone 0x41c6d0, set_color 0x47af30).
// Needs rgl::matrix and the includer's tstring.
#ifndef MB_GAME_CONV_SCREEN_H
#define MB_GAME_CONV_SCREEN_H
#include "./ui_widget.h"
#include "./head_rec.h"
#include "../include/frustum.h"
#include "./ui/container.h"
#include "./map_widget.h"
#include "../rgl/mesh_x.h"                       // mesh_x: clone 0x41c6d0, set_color 0x47af30

struct Choices : MissionUi {                             // vtable 0x5924a0, ctor 0x48ae20, 0x188 bytes
    int f178;                                            // +0x178 (the allocator slot of the vec that the shared constructor 0x4931a0 leaves in front of the three zeroed words)
    int f17c, f180, f184;                                // +0x17c/+0x180/+0x184
};

struct Rec1208 : head_rec {                              // vtable 0x595bfc, size 0x1208: slot 2 is the opener 0x4f66e0 (char_sheet/conv_open.cpp), slot 3 the event handler 0x525160 (mission5/mission_5.cpp)
    NodeA* talk;                                       // +0x2c
    MapWidget* window;                                   // +0x30
    rgl::frustum fr;                                     // +0x34 (0x120 bytes, built by the creation function; canonical src/include/frustum.h)
    char pad154[0x174 - 0x154];
    MissionUi* label_speaker;                               // +0x174
    MissionUi* label_text;                                  // +0x178
    MissionUi* input_label;                                 // +0x17c (the refresher called it body)
    text_box* bar_reln;                                   // +0x180
    char pad184[0x188 - 0x184];
    MissionUi* question;                                    // +0x188
    Choices* choice;                                     // +0x18c
    MissionUi* buttons[0x400];                              // +0x190
    int n_lines;                                         // +0x1190
    rgl::matrix world;                                   // +0x1194 (camera world transform, 0x40)
    int quest_mode;                                      // +0x11d4
    char started;                                        // +0x11d8 (byte flag)
    char pad11d9[0x11e0 - 0x11d9];
    text_box* msg_window;                                 // +0x11e0
    MissionUi* reln_pointer;                                // +0x11e4
    MissionUi* relation_bar;                                // +0x11e8
    MissionUi* lines[5];                                    // +0x11ec
    int target;                                          // +0x1200
    int speaker_id;                                      // +0x1204
    Rec1208() { target = -1; }
    virtual void reset_screen();
    virtual void init_screen();                         // 0x4f66e0
    virtual void frame_update(float);                         // 0x525160
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    void refresh(int sel, int* out);                     // 0x4f73c0
};
typedef Rec1208 ConvScreen;

#endif // MB_GAME_CONV_SCREEN_H
