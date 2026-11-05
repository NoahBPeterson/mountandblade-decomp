// glyph_lookup.h: calls of the out-of-line int hash lookup 0x43b210 (the explicit specialization is only declared; agent_ai.cpp defines it)
// for the glyph table. Include only where the original calls it (agent3/agent_combat.cpp expands the library template inline instead).
#ifndef GAME_GLYPH_LOOKUP_H
#define GAME_GLYPH_LOOKUP_H
#include "./font.h"
#include "./glyph_hash.h"
template<> IntHashBase::iterator IntHashBase::lower_bound(const int&);     // 0x43b210
// the node of `key` (== map.head when absent), through the iterator the library returns by hidden pointer
static __forceinline void glyph_lower_bound(GlyphMap& m, GlyphNode** out, const int& key)
{
    *(IntHashBase::iterator*)out = ((IntHashBase&)m).lower_bound(key);
}
#endif
