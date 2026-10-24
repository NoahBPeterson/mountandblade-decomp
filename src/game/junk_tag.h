// junk_tag.h: the scratch argument the folded STL helpers (0x4d10f0 destroy_items, 0x4f6680, 0x4ebe10, 0x4e42b0) take by
// value. cl materialises one stack slot per call site and never writes it (the original's slots are all read-only); the
// user ctor keeps cl from zero-initialising the temporary (a plain empty class gets a byte store). One definition for
// inventory_panel/skeleton_load.cpp and widget_text.cpp: both pass it to the same helper 0x4d10f0 as the last argument.
#ifndef GAME_JUNK_TAG_H
#define GAME_JUNK_TAG_H
struct junk_tag { int pad; junk_tag() {} };
#endif
