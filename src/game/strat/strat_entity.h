// strat_entity.h: the strategic entity (StratEnt, vtable 0x54b5d0) and the scene/world object it lives in
// (Ctx), with the lights and ground mesh they use. Functions: src/game/strat/strat_entity.cpp (0x42a090..0x42bac0).
// Every offset below is cited by the function(s) that read or write it (addresses in the comments; the byte-exact
// functions fix the layout, the ones marked "nm" are the nonmatching-but-equivalent ones). ALL NAMES PROVISIONAL.
#ifndef STRAT_ENTITY_H
#define STRAT_ENTITY_H
#include "./strat_ext.h"

// ---- scene lights (record 0x70+ bytes). type 2 = directional (direction at +0x44), else a point light at +0x24.
#include "../light.h"                   // Light (shared with agent2 / script)
OFFSET_CHECK3(Light, dir, 0x44);
OFFSET_CHECK3(Light, flags, 0x6c);

struct Psys;

// ---- the strategic entity (vtable 0x54b5d0). MeshEnt is the base part (0x420310 is the base render); its other
// functions are in the fx/entity modules. Fields below +0x58 are the ones this object's functions touch.
struct MeshEnt {
    virtual ~MeshEnt();
    virtual int type();         // slot 1 (9 = strategic entity)
    virtual void s2(); virtual void s3(); virtual void s4();
    virtual float get_radius();                         // slot 5 (0x41fd80)
    virtual void s6(); virtual void s7(); virtual void s8();
    virtual void render(scene* sc, render_info* out);   // 9: 0x420310 (base), 0x42a090 (StratEnt)
    rgl::world* ctx;            // 0x04   render 0x42a090 (first_dir_light), 0x42a480, 0x42b080, 0x42b790 (find_entity)
    int id;                     // 0x08   find_entity 0x42ba50
    char padc[0x58 - 0xc];
    matrix pos;                 // 0x58 (origin at 0x88)   render 0x42a090 (&pos to fx->draw), build_shadow_frame 0x42aaf0, update_shadows 0x42a1f0 (origin)
    char pad98[0xc0 - 0x98];
    float radius;               // 0xc0   update_shadows 0x42a1f0, build_shadow_frame 0x42aaf0, update_ground_cache 0x42a480
    vector4 off;                // 0xc4 (shadow-caster offset in the local frame)   build_shadow_frame 0x42aaf0 (0xc4..0xcc)
    char padd4[0x138 - 0xd4];
    mesh* shadow_a;             // 0x138   update_shadows 0x42a1f0, create_shadow_mesh caller
    mesh* shadow_b;             // 0x13c   update_shadows 0x42a1f0
    char f140;                  // 0x140   render 0x42a090 (visible flag)
    char pad141[3];
    float f144;                 // 0x144 (passed by address to the fx draw)   render 0x42a090, shadow_tick 0x42b790
    unsigned mode;              // 0x148   render 0x42a090 (mode & 3), update_shadows 0x42a1f0 (mode & 2)
    vector4 scale;              // 0x14c
};
struct StratEnt : MeshEnt {
    float f15c;                 // 0x15c   update_shadows 0x42a1f0 (-1/(f15c*25)), create_shadow_mesh 0x42b080 (sample count)
    float f160, f164;           // 0x160 / 0x164   render 0x42a090 (shadow strengths), create_shadow_mesh 0x42b080
    int f168, f16c, f170;       // 0x168 / 0x16c / 0x170   f170: update_shadows 0x42a1f0 (= 1)
    int ids[64];                // 0x174   update_ground_cache 0x42a480 (stores), create_shadow_mesh 0x42b080 (reads)
    int nids;                   // 0x274   0x42a480, 0x42b080
    vector4 last_pos;           // 0x278   update_shadows 0x42a1f0
    skin* p288;                 // 0x288   update_shadows 0x42a1f0, shadow_tick 0x42b790
    scene* sc28c;               // 0x28c   shadow_tick 0x42b790
    char pad290[0x294 - 0x290];
    float f294;                 // 0x294   render 0x42a090, create_shadow_mesh 0x42b080
    int f298;                   // 0x298 (id of a linked entity, -1 if none)   shadow_tick 0x42b790
    char pad29c[0x2a0 - 0x29c];
    int f2a0;                   // 0x2a0   update_shadows 0x42a1f0 (clock index)
    char pad2a4[4];
    __int64 t2a8;               // 0x2a8   update_shadows 0x42a1f0 (shadow-mesh refresh time)
    int f2b0;                   // 0x2b0   update_shadows 0x42a1f0 (clock index)
    char pad2b4[4];
    __int64 t2b8;               // 0x2b8   update_shadows 0x42a1f0 (shadow-map refresh time)
    char pad2c0[0x304 - 0x2c0];
    FxDef* fx;                  // 0x304   render 0x42a090, shadow_tick 0x42b790
    char pad308[4];
    Psys** emitters_first;      // 0x30c   render 0x42a090
    Psys** emitters_last;       // 0x310   render 0x42a090
    char pad314[0x31c - 0x314];
    int f31c;                   // 0x31c   render 0x42a090 (++)

    void build_shadow_frame(matrix* m, vector4* amb);   // 0x42aaf0
    void update_ground_cache();                         // 0x42a480
    void update_shadows(scene* sc, bool a, bool b);     // 0x42a1f0
    void create_shadow_mesh(mesh** slot, char flag);    // 0x42b080
    void shadow_tick();                                 // 0x42b790
};
OFFSET_CHECK3(MeshEnt, ctx, 4);
OFFSET_CHECK3(MeshEnt, pos, 0x58);
OFFSET_CHECK3(MeshEnt, radius, 0xc0);
OFFSET_CHECK3(MeshEnt, shadow_a, 0x138);
OFFSET_CHECK3(MeshEnt, mode, 0x148);
OFFSET_CHECK3(StratEnt, f15c, 0x15c);
OFFSET_CHECK3(StratEnt, ids, 0x174);
OFFSET_CHECK3(StratEnt, nids, 0x274);
OFFSET_CHECK3(StratEnt, last_pos, 0x278);
OFFSET_CHECK3(StratEnt, p288, 0x288);
OFFSET_CHECK3(StratEnt, f294, 0x294);
OFFSET_CHECK3(StratEnt, t2a8, 0x2a8);
OFFSET_CHECK3(StratEnt, t2b8, 0x2b8);
OFFSET_CHECK3(StratEnt, fx, 0x304);
OFFSET_CHECK3(StratEnt, emitters_first, 0x30c);
OFFSET_CHECK3(StratEnt, f31c, 0x31c);

// the point transform of a frame (0x408460), as callers inline it: rgl::tp (rgl/scene/scene_math.h)
using rgl::tp;

#endif // STRAT_ENTITY_H
