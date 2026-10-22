// rgl_math.h: rgl::vector4 / rgl::matrix as src/rgl uses them. The declarations of vector4 are the ones of
// match/include/rgl.h (verified there; the inline bodies matter for codegen, keep them as they are). src/rgl
// does not include rgl.h because of its unverified rgl::string (see rgl_string.h), so they are repeated here.
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
    vector4 operator-(const vector4& o) const;                              // 0x407ca0
    vector4 operator*(const float& s) const;                                // 0x407cd0
    float normalize();          // returns the old length; (0,1,0) if <= 1e-10  0x407d60
};

vector4 operator*(const float& s, const vector4& v);                       // 0x407d00
float length(const vector4& v);                                             // 0x407d30
vector4 cross(const vector4& a, const vector4& b);                         // 0x407dc0
void cross(vector4& r, const vector4& a, const vector4& b);                 // 0x407eb0
void normalize(vector4& out, const vector4& in);  // (0,1,0) if length <= 0  0x407e00
float dot(const vector4& a, const vector4& b);                              // 0x407e90

// position/frame: s(ide), f(orward), u(p) rows and origin o (module system `pos`). 64 bytes.
struct matrix {
    vector4 s, f, u, o;
    void orthonormalize();                                                  // 0x407ef0
    vector4 transform_point_to_local(const vector4& p) const;               // 0x407f70
    matrix operator*(const matrix& b) const;                                // 0x40e870 (this = left operand)
    void rotate_about_s(const float* angle);                                // 0x40e570 (provisional)
    void rotate_about_f(const float* angle);  // rotates rows s and u about f   0x40e670
    void rotate_about_u(const float* angle);  // rotates rows s and f about u   0x40e770
};

// packed ARGB <-> float colour (cdecl)
unsigned pack_color(const vector4* c);                                      // 0x40e440
void unpack_color(vector4* out, const unsigned* argb);                      // 0x40e4e0

} // namespace rgl
#endif
