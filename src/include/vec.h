// vec.h: the small float vectors of the engine (global namespace: the symbol maps and the manifest rows of
// src/game/agent, src/rgl/shapes and src/rgl/xmlcfg bind them by these names). Plain 2- and 3-float records
// (NOT padded to four lanes: vector4 of rgl is the 16-byte one).
//   vec2  : texture / plane coordinates. normalize 0x4367c0 (src/game/agent/agent.cpp) returns the old length,
//           (0,1) if <= 0. Users: rgl mesh (vertex60::uv), game agent, terrain, strat.
//   Vec2  : the same two floats as the game-state colour ramp block (0x4638b0) and the agent record (0x474e60) see them.
//   vec3  : add 0x40e420 (src/rgl/shapes/shapes.cpp); light / sun directions of terrain_b (0x24 / 0x44 of the batch).
//   Vec3  : positions and directions of src/rgl/xmlcfg/game_util.cpp (sub_assign 0x432770, vec3_sub 0x432830,
//           bounce 0x432e30; the inline operators are the ones those functions inline) and of the mesh2 editor.
#ifndef MB_VEC_H
#define MB_VEC_H

struct vec2 {
    float x, y;                                  // stays an aggregate: terrain_a embeds it in an anonymous struct and
                                                 // agent.cpp initialises it with a brace list
    float normalize();                           // 0x4367c0 (returns the old length; (0,1) if <= 0)
    void rotate_b(float ang);                    // 0x4327f0 (src/rgl/xmlcfg/game_util.cpp; agent4 calls it on a vec2 view of the heading)
};

struct Vec2 {
    float x, y;
    Vec2() {}
    Vec2(float a, float b) : x(a), y(b) {}
};

// a vec2 with constructors: temporaries passed by pointer (src/game/config/widget_mgr.h, cls_map), and members whose
// arrays go through the compiler's array-ctor helper because of the empty user ctor (src/game/triggers/trigger_vec.h MouseRec)
struct V2 : vec2 {
    V2() {}
    V2(float a, float b) { x = a; y = b; }
};

struct vec3 {
    float x, y, z;
    vec3() {}
    vec3(float a, float b, float c) { x = a; y = b; z = c; }
    void add(const vec3& o);                     // 0x40e420
};

struct Vec3 {
    float x, y, z;
    Vec3() {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3& operator-=(const Vec3& o) { x = x - o.x; y = y - o.y; z = z - o.z; return *this; }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    void sub_assign(const Vec3& o);              // 0x432770
};

#endif // MB_VEC_H
