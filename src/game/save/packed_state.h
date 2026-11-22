// packed_state.h: PackedState, the 32-byte face code (Troop::face_keys / face_keys_b at +0x3c0 / +0x3e0, Animator::pack_state 0x4eb660 / unpack 0x473800).
#ifndef MB_PACKED_STATE_H
#define MB_PACKED_STATE_H
struct PackedState {                                      // the 32 bytes at troop record +0x3c0 = Troop::face_keys (pack_state 0x4eb660)
    union {
        struct {
            unsigned __int64 header;                       // item + 1 (bits 0..5), set + 1 (6..11), material (12..17), variant (18..23), time (24..29), scale (30..35), each in 6 bits
            unsigned __int64 weights_a;                    // the face weights, 3 bits each, 21 per word (Animator::pack_state: i * 3 below 0x3f)
            unsigned __int64 weights_b;                    // the next 21 weights (and the shift overflow of the pack loop, which never reaches a fourth word)
            unsigned __int64 weights_c;                    // cleared by pack_state; the face generator copies it to the record
        };
        unsigned __int64 word[4];                          // the same four words (the hex face code, the record copy)
    };
};
#endif // MB_PACKED_STATE_H
