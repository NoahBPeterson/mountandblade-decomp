// result_window.h: the message (result) window, vtable 0x595740: ONE class, a head_rec record (game/head_rec.h).
// Its members live in three places of the image, as one class's members can: the constructor and destructors with the
// other head_rec records (rgl/mesh2/rec_tables.cpp, 0x4c2c50), init() with the window makers (game/cls_map/cls_map.cpp,
// 0x4dc630) and the display-mode handler in the GUI editor's region (rgl/mesh2/gui_edit.cpp, 0x4ca550). The slice
// tooling had given each part its own class (C4c2c50, ResultWindow, screen_text) because only the vtable ties them; the
// layouts agree (the same tstrings at +0x54/+0x68/+0x78/+0x88, the window at +0x2c, the buttons at +0x3c/+0x40, the
// flag and text pointers at +0x4c/+0x50 that the constructor clears) and slot 1 (0x4c30b0, also the slot 1 of the
// 0x4c3050 record) is the one-store function below.
#ifndef MB_GAME_RESULT_WINDOW_H
#define MB_GAME_RESULT_WINDOW_H
#include "./head_rec.h"

struct ContainerBase;
struct MissionUi;
struct UiE;

struct ResultWindow : head_rec {              // vtable 0x595740; outcome (w04): 0 ok, 1 two medium buttons, 2 two long, 3 yes/no
    ContainerBase* win;                       // +0x2c
    MissionUi* panel30;                       // +0x30
    MissionUi* panel34;                       // +0x34
    UiE* box38;                               // +0x38 the party-name box (outcome 3)
    MissionUi* b3c;                           // +0x3c
    MissionUi* b40;                           // +0x40
    char pad44[8];                            // +0x44
    int* flag;                                // +0x4c cleared by the constructor
    tstring* text;                            // +0x50
    tstring s54;                              // +0x54
    unsigned char f64;                        // +0x64
    char pad65[3];
    tstring s68;                              // +0x68
    tstring s78;                              // +0x78
    tstring s88;                              // +0x88
    ResultWindow();                           // 0x4c2c50
    // the dialog box (slot 0 of the window manager, g_windows.box(): trigger_records.cpp allocates a ResultWindow there)
    void setup(const tstring* a, const tstring* b);                                  // 0x48a3a0 (ret 8): a message (w04 0)
    void setup2(const tstring* a, const tstring* b, const tstring* c, int* answer);  // 0x48a3d0 (ret 0x10): a question (w04 1) whose answer lands at *answer
    // the script interpreter's spelling: the strings are temporaries (the callee takes pointers)
    __forceinline void show(const tstring& title, const tstring& text) { setup(&title, &text); }                                  // 0x48a3a0 (ret 8)
    __forceinline void ask(const tstring& text, const tstring& yes, const tstring& no, int* answer) { setup2(&text, &yes, &no, answer); }   // 0x48a3d0 (ret 0x10)
    // the same stores as setup, inlined by the compiler into the option pages
    inline void set_inline(const tstring* hdr, const tstring* txt)
    {
        s68 = *hdr;
        name = *txt;
        flag = 0;
        text = 0;
        w04 = 0;
    }
    __declspec(noinline) ~ResultWindow();     // 0x4c2cd0 (not inlined into the deleting destructor 0x4c2cb0)
    virtual void reset_screen();              // slot 1  0x4c30b0: g_gui.f1709 = 1
    virtual void init_screen();              // slot 2  0x4dc630: builds the window and its buttons
    virtual void frame_update(float);       // slot 3  0x4ca550: the fullscreen / windowed toggle (the argument is unused)
    virtual char pure_7() { return 0; }       // slot 7: 0x4280e0 (xor al,al; ret), not head_rec's return 1
    // the dialog box: g_windows.box() (slot 0 of the window manager). w04 = 0 is a message, 1 a question whose answer lands at *flag.
    // setup 0x48a3a0 (ret 8, ui/mission_ui.cpp) fills a message, setup2 0x48a3d0 (ret 0x10) a question.
    void setup(const tstring* a, const tstring* b);                                  // 0x48a3a0 (ret 8)
    void setup2(const tstring* a, const tstring* b, const tstring* c, int* answer);  // 0x48a3d0 (ret 0x10)
    // the script interpreter's spelling: the strings are temporaries (the callee takes pointers)
    __forceinline void show(const tstring& title, const tstring& text) { setup(&title, &text); }                                  // 0x48a3a0 (ret 8)
    __forceinline void ask(const tstring& text, const tstring& yes, const tstring& no, int* answer) { setup2(&text, &yes, &no, answer); }   // 0x48a3d0 (ret 0x10)
    // the same stores as setup, inlined by the compiler into the option pages
    inline void set_inline(const tstring* hdr, const tstring* msg)
    {
        s68 = *hdr;
        name = *msg;
        flag = 0;
        this->text = 0;
        w04 = 0;
    }
};

#endif
