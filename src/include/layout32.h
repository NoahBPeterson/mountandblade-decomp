// layout32.h: compile-time size/offset checks that only hold for the original 32-bit x86 layout.
// Pointers, vtable pointers and size_t members are 4 bytes there; on a 64-bit target (port/) they widen and every
// absolute sizeof/offsetof figure changes, so these checks apply to 32-bit x86 builds only. They emit no code.
#ifndef MB_LAYOUT32_H
#define MB_LAYOUT32_H
#if defined(_M_IX86) || defined(__i386__)
#define MB_LAYOUT32 1
// VC7.1 has no static_assert: a failed condition is a negative array size.
#define MB_LAYOUT32_ASSERT(name, cond) typedef char name[(cond) ? 1 : -1]
#else
#define MB_LAYOUT32 0
#define MB_LAYOUT32_ASSERT(name, cond) typedef char name##_layout32_skipped
#endif
#endif // MB_LAYOUT32_H
