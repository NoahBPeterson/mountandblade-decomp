// constraint.h: one morph constraint of a skin's face keys (Skin::relax 0x4735e0; src/include/skin.h). The std::vector<Constraint> of a
// skin is instantiated by rgl/mesh2/mesh_2.cpp (copy constructor 0x4cc830, operator= 0x4ccc00, resize 0x4ccf60, _Insert_n 0x4cbf90).
#ifndef MB_CONSTRAINT_H
#define MB_CONSTRAINT_H
struct Term { float coef; int idx; };           // 8 bytes
struct Constraint {                             // 0x204 bytes
    int n;                                      // +0x000 number of terms
    Term t[63];                                 // +0x004
    int mode;                                   // +0x1fc -1: value must stay >= limit, +1: must stay <= limit
    float limit;                                // +0x200
};
#endif
