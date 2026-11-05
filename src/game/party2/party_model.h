// party_model.h: the map party's model (Party::model, an rgl entity of type 9) as the map-party code (party2/party_2.h) and the
// campaign tick (inventory_panel/game_tick.cpp) use it: the slots, fields and members they reach.
#ifndef GAME_PARTY2_PARTY_MODEL_H
#define GAME_PARTY2_PARTY_MODEL_H
#include "../../rgl/rgl_math.h"
#include "../../include/ptr_vec.h"
#include "../../rgl/mesh.h"
namespace rgl { struct psys; }
using rgl::vector4;

struct Poser {
    void add_extra(int val);   // 0x442110 (agent3/agent_fx_sound.cpp, ret 4)
};
struct Rot3 {                                           // the three rows of a frame (side, forward, up)
    vector4 s, f, u;
    void rotate_about_u(const float* angle) { ((rgl::matrix*)this)->rotate_about_u(angle); }   // 0x40e770 (ret 4: rows s and f about u)
};
inline Rot3* identity3(Rot3* out) { return (Rot3*)rgl::identity3((rgl::mat3*)out); }
// the party's map model (Party::model, an rgl entity of type 9): the slots, fields and members the update code uses
struct PartyModel {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void set_color(const float* shade);         // +0x18   0x4b7840
    virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11(); virtual void s12();
    virtual void s13(); virtual void s14(); virtual void s15(); virtual void s16(); virtual void s17();
    virtual void set_mode(int mode);                    // +0x48   0x4b7840
    virtual void s19(); virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void add_emitter(rgl::psys* e);                  // +0x64   0x4b7840
    char pad4[0x58 - 4];
    Rot3 basis;                                         // +0x58 orientation rows   0x4b80d0
    vector4 origin;                                     // +0x88 position (the sound source)   0x4b80d0
    char pad98[0xc0 - 0x98];
    float f_c0;                                         // +0xc0   0x4b7840
    vector4 color;                                      // +0xc4   0x4b7840
    char padd4[0x118 - 0xd4];
    ptr_vec ents;                                       // +0x118 std::vector<rgl::mesh*> (first at +0x11c)   0x4b7840, 0x4b80d0
    char pad128[0x140 - 0x128];
    char visible;                                       // +0x140   0x4b80d0
    char pad141[0x15c - 0x141];
    float f15c;                                         // +0x15c  0x4b1800
    float f160;                                         // +0x160
    float f164;                                         // +0x164
    char pad168[0x294 - 0x168];
    float f294;                                         // +0x294  0x4b7840, 0x4b80d0
    void add_extra(rgl::mesh* m) { ((Poser*)this)->add_extra((int)m); }       // 0x442110 (ret 4)
    void set_fade(const float* p);                      // 0x4b1800 (ret 4)
};
#endif // GAME_PARTY2_PARTY_MODEL_H
