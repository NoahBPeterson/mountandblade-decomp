// Slice s00407c10: rgl vector4 / matrix basics (pilot slice).
#include "rgl.h"

namespace rgl {

// @ 0x407c30
vector4& vector4::set(const float& x_, const float& y_, const float& z_) { x = x_; y = y_; z = z_; return *this; }
// @ 0x407c50
float vector4::out_length() const { return length(); }
// @ 0x407c80
float vector4::out_length_squared() const { return length_squared(); }
// @ 0x407ca0
vector4 vector4::operator-(const vector4& o) const { return vector4(x - o.x, y - o.y, z - o.z); }
// @ 0x407cd0
vector4 vector4::operator*(const float& s) const { return vector4(x * s, y * s, z * s); }
// @ 0x407d00
vector4 operator*(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
// @ 0x407d30
float length(const vector4& v) { return v.length(); }
// @ 0x407d60
float vector4::normalize()
{
    float len = length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
// @ 0x407dc0
vector4 cross(const vector4& a, const vector4& b) { return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
// @ 0x407e00
void normalize(vector4& out, const vector4& in)
{
    float len = in.length();
    if (len > 0.0f) {
        float inv = 1.0f / len;
        out.x = in.x * inv; out.y = in.y * inv; out.z = in.z * inv;
    } else {
        out = vector4(0.0f, 1.0f, 0.0f);
    }
}
// @ 0x407e90
float dot(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
// @ 0x407eb0
void cross(vector4& r, const vector4& a, const vector4& b) { r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x; }
// @ 0x407ef0
void matrix::orthonormalize()
{
    normalize(f, f);
    cross(s, f, u);
    normalize(s, s);
    cross(u, s, f);
}
// @ 0x407f70
vector4 matrix::transform_point_to_local(const vector4& p) const
{
    vector4 d(p.x - o.x, p.y - o.y, p.z - o.z);
    return vector4(s.x * d.x + s.y * d.y + s.z * d.z, f.x * d.x + f.y * d.y + f.z * d.z, u.x * d.x + u.y * d.y + u.z * d.z);
}

} // namespace rgl

// ---- unidentified small helpers ------------------------------------------------------------
struct ptr_or_self { int a; void* p; void* get(); };
// @ 0x407c10
void* ptr_or_self::get() { return p ? p : (void*)&p; }
struct self_ref { self_ref* self(); };
// @ 0x407c20
self_ref* self_ref::self() { return this; }

// ---- abstract shape base (vtable 0x54a4b8, 24 slots; slots 1-4 are _purecall). Name provisional: slot 16
// returns "sphere". Identical bodies share an address (link /OPT:ICF), e.g. slots 7/8 and 10/11.
namespace rgl {

struct sphere {
    int m4, m8, mc;
    int min_x, min_y, max_x, max_y;
    vector4 v20, v30;
    int m40;
    sphere();
    virtual ~sphere();                               // 0x40a540
    virtual void pure1() = 0;
    virtual void pure2() = 0;
    virtual void pure3() = 0;
    virtual void pure4() = 0;
    virtual int get_0();                             // 0x408020
    virtual void slot6();                            // 0x477b70
    virtual void ignore(int);                        // 0x408030
    virtual void ignore2(int);                       // 0x408030
    virtual void set_mc(const int& v);               // 0x408040
    virtual void clear_vector(vector4& v) const;     // 0x408050
    virtual void clear_vector2(vector4& v) const;    // 0x408050
    virtual void set_m8(int v);                      // 0x408070
    virtual void or_m8(int v);                       // 0x408080
    virtual void slot14();                           // 0x40a540
    virtual int get_0b();                            // 0x408020
    virtual string type_name() const;                // 0x408090
    virtual void slot17();                           // 0x4084c0
    virtual void slot18();                           // 0x408540
    virtual void slot19();                           // 0x40c010
    virtual void slot20();                           // 0x408110
    virtual void slot21();                           // 0x4083f0
    virtual void slot22();                           // 0x408420
    virtual void set_mc2(const int& v);              // 0x408040
    void copy_from(const sphere& o);
};

// @ 0x407ff0
sphere::sphere() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
// @ 0x408020
int sphere::get_0() { return 0; }
// @ 0x408030
void sphere::ignore(int) {}
// @ 0x408040
void sphere::set_mc(const int& v) { mc = v; }
// @ 0x408050
void sphere::clear_vector(vector4& v) const { v.x = 0; v.y = 0; v.z = 0; }
// @ 0x408070
void sphere::set_m8(int v) { m8 = v; }
// @ 0x408080
void sphere::or_m8(int v) { m8 |= v; }
// @ 0x408090
string sphere::type_name() const { return "sphere"; }
// @ 0x4080b0
void sphere::copy_from(const sphere& o) { m4 = o.m4; v20 = o.v20; v30 = o.v20; m40 = o.m40; m8 = o.m8; }
} // namespace rgl
