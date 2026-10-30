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
#include "./overlay.h"
#include "./map_widget.h"

struct base_mesh {
    base_mesh* clone();                                  // 0x41c6d0
    void set_color(const float* c);                      // 0x47af30 (four floats)
};

struct Choices : WidgetObj {                             // vtable 0x5924a0, ctor 0x48ae20, 0x188 bytes
    int f178;                                            // +0x178 (the allocator slot of the vec that the shared constructor 0x4931a0 leaves in front of the three zeroed words)
    int f17c, f180, f184;                                // +0x17c/+0x180/+0x184
};

struct ConvScreen {
    char pad0[0x2c];
    Overlay* talk;                                       // +0x2c
    MapWidget* window;                                   // +0x30
    char pad34[0x174 - 0x34];
    Widget* label_speaker;                               // +0x174
    Widget* label_text;                                  // +0x178
    Widget* input_label;                                 // +0x17c (the refresher called it body)
    TextBox* bar_reln;                                   // +0x180
    char pad184[0x188 - 0x184];
    Widget* question;                                    // +0x188
    Choices* choice;                                     // +0x18c
    Widget* buttons[0x400];                              // +0x190
    int n_lines;                                         // +0x1190
    rgl::matrix world;                                   // +0x1194 (camera world transform, 0x40)
    int quest_mode;                                      // +0x11d4
    char started;                                        // +0x11d8 (byte flag)
    char pad11d9[0x11e0 - 0x11d9];
    TextBox* msg_window;                                 // +0x11e0
    Widget* reln_pointer;                                // +0x11e4
    Widget* relation_bar;                                // +0x11e8
    Widget* lines[5];                                    // +0x11ec
    int target;                                          // +0x1200
    int speaker_id;                                      // +0x1204
    void open(int dialog, int* out);
    void refresh(int sel, int* out);                     // 0x4f73c0
};

#endif // MB_GAME_CONV_SCREEN_H
