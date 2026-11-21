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
#include "../rgl/mesh.h"                       // rgl::mesh: clone 0x41c6d0, set_color 0x47af30
#include "./party2/camera_class.h"              // Camera: the question box is the choices' scrolling list (make_editor_list 0x4f62f0)

struct Rec1208 : head_rec {                              // vtable 0x595bfc, size 0x1208: slot 2 is the opener 0x4f66e0 (char_sheet/conv_open.cpp), slot 3 the event handler 0x525160 (mission5/mission_5.cpp)
    NodeA* talk;                                       // +0x2c
    MapWidget* window;                                   // +0x30
    rgl::frustum fr;                                     // +0x34 (0x120 bytes, built by the creation function; canonical src/include/frustum.h)
    rgl::vector4 f154;                                   // +0x154 (w never written)
    int f164, f168;                                      // +0x164
    float f16c;                                          // +0x16c
    char pad170[4];                                      // +0x170 evidence: the loads of +0x170 in the screen's functions (refresh 0x4f73c0, handler 0x525160) are on the choice list's scroll record (this->question's +0x484 sub-object), not on this record; none reaches this one
    MissionUi* label_speaker;                               // +0x174
    MissionUi* label_text;                                  // +0x178
    MissionUi* input_label;                                 // +0x17c (the refresher called it body)
    text_box* bar_reln;                                   // +0x180
    char pad184[0x188 - 0x184];                          // +0x184 evidence: the store at +0x184 in the opener 0x4f66e0 is the constructor of the new bar_reln widget, not this record; no other access
    Camera* question;                                       // +0x188 the choices' scrolling list (follow[1] its scroll bar)
    ContainerBase* choice;                                     // +0x18c
    MissionUi* buttons[0x400];                              // +0x190
    int n_lines;                                         // +0x1190
    rgl::matrix world;                                   // +0x1194 (camera world transform, 0x40)
    float blend;                                         // +0x11d4 the camera's blend toward the look-at frame (update_camera 0x5257f0; the opener zeroes it)
    char started;                                        // +0x11d8 (byte flag)
    char pad11d9[0x11e0 - 0x11d9];                       // +0x11d9 (3 bytes of alignment after `started`, 4 unread) evidence: none of the screen's functions reaches +0x11d9..0x11df
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
    void update_camera();                                // 0x5257f0: blend world toward the speaker / listener look-at frame
    int find_speaker_agent();                            // 0x526070
    int find_listener_agent();                           // 0x526130
};
typedef Rec1208 ConvScreen;

#endif // MB_GAME_CONV_SCREEN_H
