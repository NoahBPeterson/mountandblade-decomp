// raw_place.h: constructing a canonical class in raw storage. The widget factories and the pool allocators of this module build
// objects whose classes' constructors are defined elsewhere (MissionUi::MissionUi 0x48ae20, ...) and whose own derived views must
// not emit a vtable here; std's placement operator new is throw(), so it makes the compiler add a null test; this one does not.
#ifndef GAME_WORLD_DATA_RAW_PLACE_H
#define GAME_WORLD_DATA_RAW_PLACE_H
#include <stddef.h>
struct raw_place {};
inline void* operator new(size_t, void* p, raw_place) { return p; }
inline void operator delete(void*, void*, raw_place) {}
#endif
