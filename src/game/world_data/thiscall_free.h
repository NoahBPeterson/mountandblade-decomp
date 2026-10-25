// thiscall_free.h: calling a free __stdcall function with an object in ECX, as the original does. Several resource functions are
// compiled as free stdcall functions (clear_f14 0x4175f0, the row destructor 0x4ebdc0) but their callers load the owning object into
// ECX before pushing the one stack argument. C++ has no spelling for that on a free function; this is the nearest: a __fastcall
// function type taking the object (ECX) and one class-typed argument, which a fastcall never passes in a register, so it is pushed
// on the stack and popped by the callee, exactly as a stdcall function takes it. The call goes to the named function itself.
#ifndef GAME_WORLD_DATA_THISCALL_FREE_H
#define GAME_WORLD_DATA_THISCALL_FREE_H
struct StackWord {
    void* v;
    StackWord(void* p) : v(p) {}
};
#define CALL_STDCALL_ON(fn, self, arg) ((void (__fastcall*)(void*, StackWord))(fn))((void*)(self), StackWord((void*)(arg)))
#endif
