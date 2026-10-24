// rag_body.h: the ragdoll body record (0x2c bytes: a capsule of two end points and a radius), one definition for
// config.cpp (ctor 0x4bc170 and load 0x4bc1f0, which config.h called the "software cursor") and options_panel/ragdoll_models.cpp
// (RagBone::load 0x4d3330 builds a std::vector of them and calls the same two functions). Proof: the ctor and the
// loader are the same two addresses in both views, the loader's "Settings" object is TiXmlElement (its get_float is
// TiXmlElement::QueryDoubleAttribute 0x42f190, src/tinyxml), and the attribute names are pos_x/y/z, pos2_x/y/z, radius.
#ifndef GAME_RAG_BODY_H
#define GAME_RAG_BODY_H
class TiXmlElement;
struct RagBody {
    float pos_x, pos_y, pos_z;                      // +0x00 (a vector in a 16-byte slot; the ctor leaves the 4th lane alone)
    int   unk0c;                                    // +0x0c
    float pos2_x, pos2_y, pos2_z;                   // +0x10
    int   unk1c;                                    // +0x1c
    float radius;                                   // +0x20 (0.05f)
    unsigned char moving;                           // +0x24
    unsigned char unk25;                            // +0x25
    int   unk28;                                    // +0x28
    RagBody();                                      // 0x4bc170
    void load(TiXmlElement* src);                   // 0x4bc1f0 (pos_x.. / pos2_x.. / radius attributes, missing ones kept)
};
#endif // GAME_RAG_BODY_H
