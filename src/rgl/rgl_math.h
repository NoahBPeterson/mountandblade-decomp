// rgl_math.h: rgl::vector4 / rgl::matrix, shared by src/rgl and src/game (the former match/include/rgl.h
// declarations of vector4/matrix live here; the inline bodies matter for codegen, keep them as they are).
// A TU whose original calls the out-of-line copies 0x407ca0 / 0x407cd0 / 0x407d00 defines
// RGL_VECTOR4_OPS_OUT_OF_LINE before including this header. rgl::string is rgl_string.h.
// The matrix members after transform_point_to_local come from the mesh slices (s00412a90..s00413e50).
#ifndef RGL_MATH_H
#define RGL_MATH_H
#include <math.h>

namespace rgl {

// 16 bytes: x, y, z + an unused 4th lane that whole-struct copies carry along.
struct vector4 {
    float x, y, z, w;
    vector4() {}
    vector4(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    vector4& set(const float& x_, const float& y_, const float& z_);        // 0x407c30
    float length() const { return (float)sqrt(x * x + y * y + z * z); }
    float length_squared() const { return x * x + y * y + z * z; }
    float out_length() const;                                               // 0x407c50
    float out_length_squared() const;                                       // 0x407c80
#ifdef RGL_VECTOR4_OPS_OUT_OF_LINE
    vector4 operator-(const vector4& o) const;                              // 0x407ca0 (defined in shapes/shapes.cpp)
    vector4 operator*(const float& s) const;                                // 0x407cd0
#else
    // Inline in the original's header: 0x407ca0 / 0x407cd0 / 0x407d00 are only the out-of-line copies that the
    // giant functions (0x4465e0, 0x448eb0, 0x457b40 ...) call when cl's inline budget is spent; everywhere else the
    // arithmetic is expanded in place. shapes/shapes.cpp (the TU of the copies) defines RGL_VECTOR4_OPS_OUT_OF_LINE.
    vector4 operator-(const vector4& o) const { return vector4(x - o.x, y - o.y, z - o.z); }   // 0x407ca0
    vector4 operator*(const float& s) const { return vector4(x * s, y * s, z * s); }           // 0x407cd0
#endif
    vector4 operator+(const vector4& o) const { return vector4(x + o.x, y + o.y, z + o.z); }   // w left unset
    float normalize();          // returns the old length; (0,1,0) if <= 1e-10  0x407d60
};

#ifdef RGL_VECTOR4_OPS_OUT_OF_LINE
vector4 operator*(const float& s, const vector4& v);                       // 0x407d00
#else
inline vector4 operator*(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }   // 0x407d00
#endif
float length(const vector4& v);                                             // 0x407d30
vector4 cross(const vector4& a, const vector4& b);                         // 0x407dc0
void cross(vector4& r, const vector4& a, const vector4& b);                 // 0x407eb0
void normalize(vector4& out, const vector4& in);  // (0,1,0) if length <= 0  0x407e00
float dot(const vector4& a, const vector4& b);                              // 0x407e90

// 3x3 rotation, rows s(ide), f(orward), u(p) padded to 4 floats (48 bytes). rot3 is the plain triple; mat3 adds the by-value
// constructor (src/rgl/shapes/shapes.cpp builds results with it).
struct rot3 { vector4 s, f, u; };
struct mat3 {
    vector4 s, f, u;
    mat3() {}
    __forceinline mat3(const vector4& s_, const vector4& f_, const vector4& u_) : s(s_), f(f_), u(u_) {}
};

// position/frame: s(ide), f(orward), u(p) rows and origin o (the Module System `pos`). 64 bytes. The one class every
// module (rgl scene, game, shapes, the interpreter) uses for it; the 64-byte layout is verified by entity (0x41fcd0 /
// 0x41fda0 / 0x41fe50 copy the frame at +0x18 / +0x58), renderer (0x461630, 0x462170), frustum::to_clip (0x4621f0),
// entity::render 0x420310 (transform_point), FxDef::skin 0x4291c0 (rot_t, rot3), game_util 0x4329d0 (relative).
// No constructors: the scene code keeps it an aggregate.
struct matrix {
    vector4 s, f, u, o;
    void orthonormalize();                                                  // 0x407ef0
    vector4 transform_point_to_local(const vector4& p) const;               // 0x407f70
    matrix operator*(const matrix& b) const;                                // 0x40e870 (this = left operand, hidden return; rotation via rot3, origin via transform_point)
    void rotate_about_s(const float* angle);                                // 0x40e570 (rows f and u about s)
    void rotate_about_f(const float* angle);  // rotates rows s and u about f   0x40e670
    void rotate_about_u(const float* angle);  // rotates rows s and f about u   0x40e770
    __declspec(noinline) vector4 transform_point(const vector4& p) const;   // 0x408460 (hidden return pointer)
    mat3 rot3(const matrix& m) const;                                       // 0x40a920 (rotation rows of m through this; hidden mat3 return)
    __declspec(noinline) vector4 rot(const vector4& p) const;               // 0x40a9c0 (no translation)
    mat3* rot_t(mat3* out) const;                                           // 0x429670 (transposed rotation of this, returns out)
    matrix relative(const matrix& base) const;                              // 0x4329d0 (base expressed in this frame; hidden return + base)
    __forceinline vector4 transform_point_inl(const vector4& p) const       // the body as callers inline it
    {
        return vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                       s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                       s.z * p.x + f.z * p.y + u.z * p.z + o.z);
    }
};
// a frame whose rows can be scaled in place (entity::local, 0x41fee0)
struct smat : matrix {
    void scale_rows(const vector4* k);                                      // 0x41fee0
};
// the scene code's name for the frame
typedef matrix mat4;
mat3* __cdecl identity3(mat3* out);                                         // 0x40cd60 (cdecl; returns out; defined in shapes/shapes.cpp)
matrix* __cdecl identity_pos(matrix* out);                                  // 0x40cd00 (cdecl; returns out)
struct frame_value : matrix {};                                              // a frame with a base class: cl 13.10 builds such a temporary in place (a plain matrix is copied once more)
frame_value identity_pos();                                                  // 0x40cd00 (the hidden-return-pointer spelling of the same function: renderer::set_view's temporaries)

// packed ARGB <-> float colour (cdecl)
unsigned pack_color(const vector4* c);                                      // 0x40e440
vector4 unpack_color(const unsigned* argb);                           // 0x40e4e0 (cdecl; the colour comes back through the hidden return pointer)

} // namespace rgl
#endif
