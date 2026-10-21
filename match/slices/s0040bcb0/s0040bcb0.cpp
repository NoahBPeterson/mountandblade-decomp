// Slice s0040bcb0: shape-vs-shape contact dispatcher (collision), sphere clone / copy ctor, and the
// libcpmt string.obj instantiations that the linker placed right behind them: std::string erase / assign /
// _Copy / dtor / ctor(const char*), std::logic_error and a derived std::length_error (the _Xlen error).
//
// Default flags (no /EHsc): _Copy's own try/catch still produces its EH frame, the rest has none.
#include <string>
#include <exception>
#include "rgl.h"
using namespace rgl;

// ---- std::string members (EH frames) -----------------------------------------------------------
typedef std::string str;
// @ 0x40c0c0
template str& str::erase(size_t, size_t);
// @ 0x40c140
template void str::_Copy(size_t, size_t);
// 0x40c1b1: catch(...) funclet of _Copy (retry with _Newres = _Newsize), emitted by cl inside _Copy
// 0x40c257: catch(...) funclet of _Copy ({ _Tidy(true); throw; }), emitted by cl inside _Copy
// @ 0x40c290
template str::~basic_string();
// @ 0x40c360
template str& str::assign(const str&, size_t, size_t);
// @ 0x40c450
template str& str::assign(const char*, size_t);
// @ 0x40c540
template str::basic_string(const char*);

// ---- std exceptions (copies of <stdexcept>, out-of-line so that they are emitted) ----------------
class logic_error : public std::exception {
public:
    explicit logic_error(const std::string& m);
    logic_error(const logic_error&);
    virtual ~logic_error();
    virtual const char* what() const;               // 0x40c300 (libcpmt)
private:
    std::string _Str;
};
class length_error : public logic_error {
public:
    explicit length_error(const std::string& m);
    length_error(const length_error&);
    virtual ~length_error() {}                      // ??_G at 0x40c5c0
};
// @ 0x40c2c0
logic_error::~logic_error() {}
// 0x40c310
// (scalar deleting destructor of logic_error, vtable 0x59c978 slot 0: generated; forced into the object here)
void* force_vtables(const std::string& s) { logic_error* p = new logic_error(s); delete p; length_error* q = new length_error(s); delete q; return 0; }
// @ 0x40c580
logic_error::logic_error(const std::string& m) : _Str(m) {}
// @ 0x40c5c0
// (length_error scalar deleting destructor, vtable 0x59c984 slot 0: generated from the inline ~length_error)
// @ 0x40c5f0
logic_error::logic_error(const logic_error& o) : std::exception(o), _Str(o._Str) {}
// @ 0x40c630
length_error::length_error(const length_error& o) : logic_error(o) {}

// ---- sphere (vtable 0x54a4f0) -------------------------------------------------------------------
namespace rgl {
struct shape {                       // vtable 0x54a4b8 (abstract base, 0x20 bytes)
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape();
    virtual void pure1() = 0;
    virtual void pure2() = 0;
    virtual void pure3() = 0;
    virtual void pure4() = 0;
};
}
struct sphere : shape {              // vtable 0x54a4f0, 0x44 bytes
    vector4 v20, v30;
    float radius;                    // 0x40
    sphere(const sphere& o);                         // 0x40c030
    virtual void pure1(); virtual void pure2(); virtual void pure3(); virtual void pure4();
    virtual sphere* clone();                         // slot 5   0x40c010
};
// @ 0x40c010
sphere* sphere::clone() { return new sphere(*this); }
// @ 0x40c030
sphere::sphere(const sphere& o) : shape(o), v20(o.v20), v30(o.v30), radius(o.radius) {}

// ---- shape contact dispatcher -------------------------------------------------------------------
struct contact {                     // 0x24 bytes
    vector4 normal;
    vector4 point;
    float depth;
};
struct coll_poly;
struct coll_shape {                  // views of the shape classes (provisional)
    virtual ~coll_shape();
    virtual int type();              // slot 1: 0 sphere, 1 capsule, 2 polygon, 3 compound
    char pad[0x1c];
    coll_shape** kids;               // 0x20 (compound: child shapes)
    int nkids;                       // 0x24
};
void sphere_sphere_contact(coll_shape* a, coll_shape* b, int max, contact* out, int* count);    // 0x409980
void capsule_sphere_contact(coll_shape* a, coll_shape* b, int max, contact* out, int* count);   // 0x409ac0
void capsule_capsule_contact(coll_shape* a, coll_shape* b, int max, contact* out, int* count);  // 0x409e90
void polygon_sphere_contact(coll_poly* a, coll_shape* b, int max, contact* out, int* count);    // 0x40a000
void polygon_capsule_contact(coll_poly* a, coll_shape* b, int max, contact* out, int* count);   // 0x40b0f0

// @ 0x40bcb0
__declspec(noinline) void shape_contacts(coll_shape* a, coll_shape* b, int max, contact* out, int* count)
{
    if (a->type() == 3) {
        if (b->type() == 3) {
            for (int i = 0; i < a->nkids; i++) {
                if (*count != 0 && *count >= max) break;
                for (int j = 0; j < b->nkids; j++) {
                    if (*count != 0 && *count >= max) break;
                    shape_contacts(a->kids[i], b->kids[j], max, out, count);
                }
            }
        } else {
            for (int i = 0; i < a->nkids; i++) {
                if (*count != 0 && *count >= max) break;
                shape_contacts(a->kids[i], b, max, out, count);
            }
        }
    } else if (b->type() == 3) {
        for (int i = 0; i < b->nkids; i++) {
            if (*count != 0 && *count >= max) break;
            shape_contacts(a, b->kids[i], max, out, count);
        }
    } else if (a->type() == 0) {
        if (b->type() == 0) {
            sphere_sphere_contact(a, b, max, out, count);
        } else {
            int first = *count;
            shape_contacts(b, a, max, out, count);
            for (int i = first; i < *count && i < max; i++) {
                out[i].normal.x = out[i].normal.x * -1.0f;
                out[i].normal.y = out[i].normal.y * -1.0f;
                out[i].normal.z = out[i].normal.z * -1.0f;
            }
        }
    } else if (a->type() == 1) {
        if (b->type() == 0) {
            capsule_sphere_contact(a, b, max, out, count);
        } else if (b->type() == 1) {
            capsule_capsule_contact(a, b, max, out, count);
        } else {
            int first = *count;
            shape_contacts(b, a, max, out, count);
            for (int i = first; i < *count && i < max; i++) {
                out[i].normal.x = out[i].normal.x * -1.0f;
                out[i].normal.y = out[i].normal.y * -1.0f;
                out[i].normal.z = out[i].normal.z * -1.0f;
            }
        }
    } else if (a->type() == 2) {
        if (b->type() == 0) {
            polygon_sphere_contact((coll_poly*)a, b, max, out, count);
        } else if (b->type() == 1) {
            polygon_capsule_contact((coll_poly*)a, b, max, out, count);
        } else {
            b->type();
        }
    }
}
// @ 0x40bfe0
int shape_contact_count(coll_shape* a, coll_shape* b, int max, contact* out)
{
    int n = 0;
    shape_contacts(a, b, max, out, &n);
    return n;
}
