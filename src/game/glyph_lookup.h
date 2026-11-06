// glyph_lookup.h: calls of the out-of-line int hash lookup 0x43b210 (the explicit specialization is only declared; agent4/agent_logic.cpp defines it)
// for the glyph table. agent4/agent_logic.cpp both instantiates the template (0x43b210) and expands it inline at 0x43e190's text code, so it
// defines INT_HASH_LOWER_BOUND_INSTANTIATED_HERE.
#ifndef GAME_GLYPH_LOOKUP_H
#define GAME_GLYPH_LOOKUP_H
#include "./font.h"
#include "./glyph_hash.h"
#ifndef INT_HASH_LOWER_BOUND_INSTANTIATED_HERE
template<> IntHashBase::iterator IntHashBase::lower_bound(const int&);     // 0x43b210
#endif
// the node of `key` (== map.head when absent), through the iterator the library returns by hidden pointer
static __forceinline void glyph_lower_bound(GlyphMap& m, GlyphNode** out, const int& key)
{
    *(IntHashBase::iterator*)out = ((IntHashBase&)m).lower_bound(key);
}
#endif
