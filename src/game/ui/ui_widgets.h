// ui_widgets.h: the widgets derived from the mission-UI base (mission_ui.h): UiA (toggle, vtable 0x591d58),
// UiB (slider, vtable 0x591fb0), UiC (label / icon, vtable 0x592078) and UiD (progress bar, vtable 0x592140),
// plus the UI click sound. Names provisional. Constructors call the base constructor 0x48ae20 first.
#ifndef GAME_UI_WIDGETS_H
#define GAME_UI_WIDGETS_H
#include "./mission_ui.h"
#include "../../rgl/rgl_math.h"

struct UiSoundHost {                           // the sound object used by the UI (global 0x5d7080)
    char pad[0x170c];
    int handle;                             // +0x170c the UI click sample    0x48b2d0
};
extern UiSoundHost g_ui_sound;                    // 0x5d7080
void __fastcall play_sound(UiSoundHost* m);    // 0x48b2d0 (FSOUND_PlaySoundEx + volume from g_master_volume)
extern float g_ui_target[2];                // 0x5d7094 slider target positions   0x48b5a0
extern void* g_ui_active;                   // 0x5d7090 widget being dragged       0x48b5a0

struct UiABase : MissionUi {                // the first half of the toggle button's constructor 0x48b1c0
    int f178;                               // +0x178 the state the render picks (0x48b320)
    float f17c;                             // +0x17c
    UiABase() { f178 = 0; b121 = 0; }
};
struct UiA : UiABase {                      // vtable 0x591d58: the toggle button
    char f180;                              // +0x180 pressed   0x48b200
    char f181;                              // +0x181 (UiE: has a selection)
    char pad182[2];
    unsigned f184;                          // +0x184 flags     0x48b200
    // the constructor 0x48b1c0: the base half, the vptr, then the field stores. The original inlines it into most callers; some
    // of them (the factories' UiAInline) store their own vptr right after the base half and do the field stores themselves.
    UiA();                                  // 0x48b1c0, defined out of line in mission_ui.cpp and inline below for the TUs that see the body
protected:
    struct base_only {};
    UiA(base_only) {}
public:
    void set_flag() { f184 = 1; }           // 0x499b50
    void* construct();                      // the constructor on raw memory, for the callers that do not inline it (mission_ui.cpp)
    // the slots of vtable 0x591d58 that differ from the base's
    virtual bool is_active();               // +0x04 returns 1 (0x48b170)
    virtual void render(rgl::scene* sc, float* count, int c, int d);   // +0x28 0x48b320
    virtual char key(unsigned p);           // +0x8c 0x48b200
    virtual void hide();                    // +0xb0 0x48b1a0
    virtual void flag2_set();               // +0xb8 0x48b180
    virtual int kind();                     // +0xc0 0x48b1f0 (1)
};

// The same table, for the widgets the factories build (cls_map, world_data, config/widget_node.cpp).
struct UiAInline : UiA { UiAInline() : UiA(base_only()) { f180 = 0; f184 = 0; } };

// The two toggle variants: the medium button (text fitted to the button, vtable 0x591e20) and the xyz button (vtable 0x591ee8, the
// strip icons; it animates through HudBar's update 0x4966b0).
struct MediumButton : UiA {                 // vtable 0x591e20, size 0x188
    virtual float* set_text(float* out, const tstring* s);   // +0xa4 0x48b3a0 (ui_widgets_a.cpp)
    virtual int kind();                     // +0xc0 0x48b390 (9)
};
struct XyzButton : UiA {                    // vtable 0x591ee8, size 0x188
    virtual void animate(int a);            // +0x08 0x4966b0 (world_data/mesh_hud.cpp)
    virtual int kind();                     // +0xc0 0x48b4a0 (8)
};

struct UiB : MissionUi {                    // vtable 0x591fb0
    int f178;                               // +0x178 slider index
    float f17c;                             // +0x17c tolerance
    int f180;                               // +0x180 dragging
    float f184;                             // +0x184 grab offset
    UiB();                                  // 0x48b4b0
    float value();                          // 0x48b550
    void set_value(float v);                // 0x48b6e0
    // slots of vtable 0x591fb0 that differ from the base's
    virtual void animate(int a);            // +0x08 0x497880 (world_data/mesh_hud.cpp)
    void render(rgl::scene* sc, float* count, int c, int d);   // +0x28 0x48b4f0
    char key(unsigned p);                   // +0x8c 0x48b5a0
    int kind();                             // +0xc0 0x48b590 (2)
};

struct UiC : UiB {                          // vtable 0x592078: the slider with a frame (its table inherits UiB's key 0x48b5a0)
    int f188;                               // +0x188
    rgl::mesh* src;                         // +0x18c the mesh the frame is cloned from (0x497d10)
    char f190;                              // +0x190
    char pad191[3];
    float f194;                             // +0x194 the frame the current mesh was built for
    rgl::vector4 v198;                      // +0x198 the colour the frame scales (1, 1, 1 and an uninitialised stack value: 0x48b760)
    unsigned mask;                          // +0x1a8 the alpha bits the frame mesh keeps
    virtual void update_children();         // +0xc4, vtable slot 49 0x497d10 (world_data/mesh_hud.cpp: rebuilds the frame mesh)
    UiC();                                  // 0x48b760
    // slots of vtable 0x592078 that differ from UiB's
    virtual void animate(int a);            // +0x08 0x497e30 (world_data/mesh_hud.cpp)
    void render(rgl::scene* sc, float* count, int c, int d);   // +0x28 0x48b7f0
    int kind();                             // +0xc0 0x48b880 (3)
};

struct UiD : MissionUi {                    // vtable 0x592140
    int f178;                               // +0x178 index
    float f17c;                             // +0x17c tolerance
    int f180;                               // +0x180 dragging
    float f184;                             // +0x184 grab offset
    virtual void update_children(float a);  // +0xc4, vtable slot 49 0x498390 (world_data/mesh_hud.cpp: scales the lane mesh)
    UiD();                                  // 0x48b890
    float value();                          // 0x48b930 (next object)
    void set_value(float x);                // 0x48baa0 (next object)
    // slots of vtable 0x592140 that differ from the base's
    void render(rgl::scene* sc, float* count, int c, int d);   // +0x28 0x48b8d0
    char key(unsigned p);                   // +0x8c 0x48b960
};

// The body of the constructor. The original inlines it at most call sites; the TUs whose call site calls 0x48b1c0 instead
// (mission_ui.cpp, which defines it, and party/party_window.cpp) define UIA_CTOR_OUT_OF_LINE before including this header.
#ifndef UIA_CTOR_OUT_OF_LINE
inline UiA::UiA() { f180 = 0; f184 = 0; }
#endif

#endif // GAME_UI_WIDGETS_H
