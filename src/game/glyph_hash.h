// glyph_hash.h: the int-keyed stdext::hash_map the font's glyph table (GlyphMap, font.h) is. HashVal / IntHashBase are the mapped type and
// hash base the 0x43b210 lookup was instantiated for (src/game/agent2/agent_ai.cpp); the linker folded every int-keyed map's lookup into it.
#ifndef GAME_GLYPH_HASH_H
#define GAME_GLYPH_HASH_H
#include <hash_map>
struct HashVal { int v; };
typedef stdext::_Hash<stdext::_Hmap_traits<int, HashVal, stdext::hash_compare<int, std::less<int> >,
                                          std::allocator<std::pair<const int, HashVal> >, false> > IntHashBase;
#endif
