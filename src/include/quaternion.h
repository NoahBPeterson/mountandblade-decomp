// quaternion.h: the engine's quaternion, memory order w, x, y, z (verified: 0x422db0 stores x y z w in this order of
// arguments; 0x4339d0 stores (0,0,0,1) as w=1 last in the constructor call order). Global namespace: the symbol
// maps (and the manifest rows of src/rgl/dynamic/fx_anim, src/game/agent) bind it by that name.
// Users: src/rgl/dynamic/fx_anim.cpp (the members below, all defined there), src/game/agent/curve.cpp and
// agent.cpp (KeyTable::sample 0x436ae0 returns one by value).
// Every member stays out of line (callers in fx_anim call them; the original compiled them in the same object).
#ifndef MB_QUATERNION_H
#define MB_QUATERNION_H

struct quaternion {
    float w, x, y, z;
    quaternion() {}
    quaternion(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
    __declspec(noinline) quaternion& set(const float& x_, const float& y_, const float& z_, const float& w_);   // 0x422db0
    __declspec(noinline) void add(const quaternion& o);                                                         // 0x422de0
    __declspec(noinline) quaternion operator+(const quaternion& o) const;                                       // 0x422e10
    __declspec(noinline) void normalize();                                                                      // 0x422e40
    __declspec(noinline) quaternion mul(const quaternion& o) const;                                             // 0x422eb0
    __declspec(noinline) quaternion mul_conj(const quaternion& o) const;                                        // 0x422f60
    __declspec(noinline) float dot(const quaternion& o) const;                                                  // 0x423010
    __declspec(noinline) bool dot_negative(const quaternion& o) const;                                          // 0x423040
};

#endif // MB_QUATERNION_H
