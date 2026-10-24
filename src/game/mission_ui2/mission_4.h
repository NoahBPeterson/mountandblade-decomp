// mission_4.h: classes of mission_4.cpp (0x48ef40..0x48f790): the container frame, the rider table, the tableau stack and the
// widgets as the text / position setters see them. Names provisional; every offset cites the function that reads it. The
// panel's members are in field_panel.h, the party's in src/include/party.h, the game data's in src/include/game_data.h,
// the widget manager's in src/game/config/widget_mgr.h.
#ifndef GAME_MISSION_UI2_MISSION_4_H
#define GAME_MISSION_UI2_MISSION_4_H
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../ui/mission_ui.h"
#include "../../include/frustum.h"
#include "./field_panel.h"

// ---- 0x48ef40: a container widget that owns a vector of objects (the same object as UiFrame in party/party_window.h) ----------------
struct VDtor { virtual ~VDtor(); };         // an object with a virtual deleting destructor in slot 0
struct FrameBox : MissionUi {
    int f178;                               // +0x178 the vector's allocator word
    VDtor** first;                          // +0x17c
    VDtor** last;                           // +0x180
    VDtor** cap;                            // +0x184
    void release();                         // 0x48ef40 (deletes every object, frees the block, clears the vector)
};

// ---- 0x48efa0: the rider rows (0x88 bytes each) of an object ---------------------------------------------------------------
struct RiderRow {
    char pad0[0x48];
    int id;                                 // +0x48 the rider's id, or 0x5f5e0ff when the row is a pair
    int a;                                  // +0x4c
    int b;                                  // +0x50
};
struct RiderTable {
    char pad0[0x14];
    RiderRow* rows;                         // +0x14 array of 0x88-byte rows
    int shares(const int& a, const int& b) const;   // 0x48efa0 (ret 8: do the two rows name a common rider; the arguments are row indices)
};

// ---- 0x48f450..0x48f5a0: the stack of tableaus being rendered (object 0xa75f40; TableauVec in script/script_interpreter.h) ----------
struct TabCurrent {                         // the tableau on top of the stack
    char pad0[0x36c];
    rgl::frustum fr;                        // +0x36c the camera (0x48f480, 0x48f4c0)
    int background;                         // +0x48c (0x48f570)
    float ambient[4];                       // +0x490 (0x48f5a0)
};
struct TableauStack {
    char pad0[0x10];
    std::vector<TabCurrent*> stack;         // +0x10 (allocator word, first at +0x14)
    TabCurrent* current();                  // 0x48f450
    void set_camera_position(const rgl::matrix* m);     // 0x48f480 (ret 4)
    void set_camera_parameters(const char* perspective, const float* w, const float* h, const float* near_, const float* far_);   // 0x48f4c0 (ret 0x14)
    void set_background_color(int c);       // 0x48f570 (ret 4)
    void set_ambient_light(const float* c); // 0x48f5a0 (ret 4)
};

// ---- 0x48f5f0, 0x48f6b0: the widget slots the text and position setters call -----------------------------------------------------
struct TxVt {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54();
    virtual void set_pos(float* p);         // +0x58
    virtual void v5c(); virtual void v60(); virtual void v64(); virtual void v68();
    virtual void v6c(); virtual void v70(); virtual void v74(); virtual void v78();
    virtual void v7c(); virtual void v80(); virtual void v84(); virtual void v88();
    virtual void v8c(); virtual void v90();
    virtual void get_state(float* out);     // +0x94
    virtual void v98(); virtual void v9c(); virtual void va0();
    virtual void text_size(float* out, const tstring* s);   // +0xa4
    virtual void va8(); virtual void vac(); virtual void vb0(); virtual void vb4(); virtual void vb8();
    virtual void vbc();
    virtual int type();                     // +0xc0 widget kind (7 = the field panel)
};
struct TxObj : TxVt {
    char pad4[0xf0 - 4];
    float w;                                // +0xf0 widget size (screen[0..1])
    float h;                                // +0xf4
    char padf8[0x148 - 0xf8];
    unsigned flags148;                      // +0x148 (0x10: centred horizontally, 0x1000: vertically)
};

#endif // GAME_MISSION_UI2_MISSION_4_H
