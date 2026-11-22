// glyph_hash.h: the font's glyph table, a stdext::hash_map<int, Glyph> (VC7.1 <hash_map>: Font::map, font.h). The linker folded every
// int-keyed map's _Hash::lower_bound into the one instance 0x43b210 (agent4/agent_logic.cpp defines it for this table's type).
#ifndef GAME_GLYPH_HASH_H
#define GAME_GLYPH_HASH_H
#include <list>
#include <vector>
#include <hash_map>
#include "engine/math/vec.h"                        // V2

// ---- font glyph metrics -----------------------------------------------------------------------------------
struct Glyph {                                       // 0x1c bytes: two (x, y) pairs + 3 floats (the hash_map<int, Glyph> mapped type)
    V2 lo, hi;                                       // the glyph's two corners (Font::load 0x4d9aa0 stores them as floats; Font::draw_glyph 0x4c.. reads hi - lo as its size)
    float f10, f14, f18;
    // the glyph's constructor: the two pairs assigned from const-float& temporaries, then three zeroed floats (Font::Font
    // 0x4d99d0 runs it for dflt and the 0x200 ascii glyphs, operator[] 0x4d92c0 for the inserted value)
    Glyph() { lo = V2(0.0f, 0.0f); hi = V2(0.0f, 0.0f); f10 = 0.0f; f14 = 0.0f; f18 = 0.0f; }
};
typedef stdext::hash_map<int, Glyph> GlyphMap;
typedef stdext::_Hash<stdext::_Hmap_traits<int, Glyph, stdext::hash_compare<int, std::less<int> >,
                                          std::allocator<std::pair<const int, Glyph> >, false> > GlyphHash;   // GlyphMap's base: lower_bound 0x43b210, insert 0x4d3c00
typedef GlyphHash::iterator GlyphIt;                 // the element of the bucket vector (vector<GlyphIt>: resize 0x4cd010, _Insert_n 0x4cc430, _Construct_n 0x4cd1c0)
typedef std::list<std::pair<const int, Glyph> > GlyphList;   // the node list (clear 0x4c4d80, _Buynode 0x4c4dc0 / 0x4c4e50, _Incsize 0x4cb6f0)

// the out-of-line instances: lower_bound is agent4/agent_logic.cpp's (0x43b210), insert is game/app/world_app.cpp's (0x4d3c00)
#ifndef INT_HASH_LOWER_BOUND_INSTANTIATED_HERE
template<> GlyphHash::iterator GlyphHash::lower_bound(const int&);                     // 0x43b210
#endif
#ifndef GLYPH_HASH_INSERT_INSTANTIATED_HERE
template<> GlyphHash::_Pairib GlyphHash::insert(const GlyphHash::value_type&);         // 0x4d3c00
#endif
#endif
