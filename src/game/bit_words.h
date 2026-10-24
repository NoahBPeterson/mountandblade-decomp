// bit_words.h: the bit-field word array at the head of the terrain object (global 0xa76630, the same address as the Terrain
// of terrain_a/terrain.h: w[0..5] are the six hex words, w[4] = Terrain::flags, w[5] = Terrain::flags14). Fields [lo..hi] are
// addressed by bit index, widths come from the power-of-two table at 0x5a8e70 (g_pow2[n] = 2 << n). ONE definition for
// src/game: terrain_a (set/get/set_pct), char_sheet terrain page (read/write of the fields, hex text) saw it as partial views
// (and the percentage setter as a separate struct `Pct`: 0x4649f0 is a method of this object, the percentage rides in the low
// 7 bits of w[4]). No includes: it needs only forward declarations, so both header universes can include it.
#ifndef GAME_BIT_WORDS_H
#define GAME_BIT_WORDS_H

struct tstring;
struct BitWords {
    unsigned w[8];                                          // w[0..5] are the six hex words
    void set(const unsigned& lo, const int& hi, const unsigned& v);   // 0x464960
    unsigned get(const unsigned& lo, const int& hi);                  // 0x4649b0
    void set_pct(const float& f);                                     // 0x4649f0 (terrain_a.cpp; char_sheet passes &f48 as a reference)
    void set_a(const float* p);                                       // 0x464a60 (= Terrain::set_field_a)
    void set_b(const float* p);                                       // 0x464ac0 (= Terrain::set_field_b)
    void build_hex_text(tstring& out);                                // 0x4f29b0 (slice s004f25a0)
    void parse_hex(const tstring& s);                                 // 0x4658c0 fills w[0..5] from "0x..." (= HexWords::parse)
    void submit_frame(const float* rect);                             // 0x4f3e10 (dust.cpp / terrain_page_input.cpp)
};
extern BitWords g_bits;                                     // 0xa76630

#endif // GAME_BIT_WORDS_H
