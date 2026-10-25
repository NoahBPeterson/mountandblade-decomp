// ui_widgets.h: the widgets derived from the mission-UI base (mission_ui.h): UiA (toggle, vtable 0x591d58),
// UiB (slider, vtable 0x591fb0), UiC (label / icon, vtable 0x592078) and UiD (progress bar, vtable 0x592140),
// plus the UI click sound. Names provisional. Constructors call the base constructor 0x48ae20 first.
#ifndef GAME_UI_WIDGETS_H
#define GAME_UI_WIDGETS_H
#include "./mission_ui.h"

struct UiSoundHost {                           // the sound object used by the UI (global 0x5d7080)
    char pad[0x170c];
    int handle;                             // +0x170c the UI click sample    0x48b2d0
};
extern UiSoundHost g_ui_sound;                    // 0x5d7080
void __fastcall play_sound(UiSoundHost* m);    // 0x48b2d0 (FSOUND_PlaySoundEx + volume from g_master_volume)
extern float g_master_volume;               // 0x5aa014
extern float g_ui_target[2];                // 0x5d7094 slider target positions   0x48b5a0
extern void* g_ui_active;                   // 0x5d7090 widget being dragged       0x48b5a0

struct UiA : MissionUi {                    // vtable 0x591d58
    int f178;                               // +0x178   0x48b1c0, 0x48b320
    float f17c;                             // +0x17c
    char f180;                              // +0x180 (bool flag)   0x48b200
    char pad181[3];
    unsigned f184;                          // +0x184 (flags)       0x48b200
    UiA();                                  // 0x48b1c0
    int type();                             // 0x48b1f0 (also mesh's type slot: ICF)
    char key(unsigned p);                   // 0x48b200
    void update(void* a, float* b, int c, int d);   // 0x48b320
    int type9();                            // 0x48b390
    int type8();                            // 0x48b4a0
};

struct UiB : MissionUi {                    // vtable 0x591fb0
    int f178;                               // +0x178 slider index
    float f17c;                             // +0x17c tolerance
    int f180;                               // +0x180 dragging
    float f184;                             // +0x184 grab offset
    UiB();                                  // 0x48b4b0
    void show(void* a, float* b, int c, int d);   // 0x48b4f0
    float value();                          // 0x48b550
    int type2();                            // 0x48b590
    char key(unsigned p);                   // 0x48b5a0
    void set_value(float v);                // 0x48b6e0
};

struct UiC : MissionUi {                    // vtable 0x592078
    int f178;                               // +0x178
    float f17c;                             // +0x17c
    int f180;                               // +0x180
    int f184;                               // +0x184
    int f188;                               // +0x188
    int f18c;                               // +0x18c
    char f190;                              // +0x190
    char pad191[3];
    float f194;                             // +0x194
    float f198, f19c, f1a0, f1a4;           // +0x198 (1, 1, 1 and an uninitialised stack value: 0x48b760)
    int f1a8;                               // +0x1a8
    virtual void update_children();         // +0xc4, vtable slot 49 (0x497d10, called by show)
    UiC();                                  // 0x48b760
    void show(void* a, float* b, int c, int d);   // 0x48b7f0
    int type();                             // 0x48b880
};

struct UiD : MissionUi {                    // vtable 0x592140
    int f178;                               // +0x178 index
    float f17c;                             // +0x17c tolerance
    int f180;                               // +0x180 dragging
    float f184;                             // +0x184 grab offset
    virtual void update_children();         // +0xc4, vtable slot 49 (0x498390)
    UiD();                                  // 0x48b890
    void show(void* a, float* b, int c, int d);   // 0x48b8d0
    float value();                          // 0x48b930 (next object)
    char key(unsigned p);                   // 0x48b960 (next object)
    void set_value(float x);                // 0x48baa0 (next object)
};

#endif // GAME_UI_WIDGETS_H
