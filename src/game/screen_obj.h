// screen_obj.h: a screen of the window manager (g_windows / g_screens, 30 objects from 0xa73980) as a vtable. Slot 0 is the
// deleting destructor (cls_map.cpp deletes the 30 entries of g_screen.items through it), slots 2 / 7 / 8 are the
// queries world_data.cpp calls (open-screen deque 0x494260 / 0x495700). One definition for world_data.h and cls_map.cpp.
#ifndef GAME_SCREEN_OBJ_H
#define GAME_SCREEN_OBJ_H
struct ScreenObj {
    virtual ~ScreenObj();                                       // slot 0: deleting destructor
    virtual void s1(); virtual char s2();
    virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual char s7();                                          // +0x1c
    virtual char s8();                                          // +0x20
};
#endif // GAME_SCREEN_OBJ_H
