// Slice s00417850: binary (memory/file) serialisation streams, the 12-byte vec_array record, the
// engine's 32-slot vector table and small STL-style algorithm instantiations over those records.
// Functions needing an EH frame (std::list/_Uninit_* instantiations) are in s00417850_stl.cpp.
#include "mbbin.h"
#include <string.h>
#include <new>

// 0x42cc80 / vec_ctor helpers are compiler-generated; the 16-byte element's default ctor is out of line.
struct f4 { float x, y, z, w; };       // plain 16-byte value
struct vec16 : f4 {                    // 16 bytes: x, y, z + a 4th lane
    vec16();                           // 0x407c20 (returns this)
    vec16(float x_, float y_, float z_) { x = x_; y = y_; z = z_; }
};

// 12-byte record: parallel arrays of n 16-byte vectors and n ints (a vector4 key list + indices).
struct vec_array {
    vec16* v;                          // +0
    int* idx;                          // +4
    int n;                             // +8
    void operator=(const vec_array& o);    // 0x417850
};

// @ 0x417850
void vec_array::operator=(const vec_array& o)
{
    n = o.n;
    if (idx) { delete[] idx; idx = 0; }
    if (v) { delete[] v; v = 0; }
    if (o.n) {
        idx = new int[n];
        v = new vec16[n];
        for (int i = 0; i < n; i++)
            v[i] = o.v[i];
        memcpy(idx, o.idx, n * 4);
    }
}

// ---- binary stream slots ---------------------------------------------------------------------
// @ 0x417930
stream_base& mem_stream::w_stream(stream_base* o)
{
    int n = o->buf.len - o->buf.pos;
    buf.write(&n, 4);
    if (n > 0) {
        const char* p;
        if (o->buf.buf)
            p = o->buf.pos + o->buf.buf;
        else
            p = (const char*)&o->buf.buf;
        buf.write(p, n);
    }
    return *this;
}
// @ 0x417990
stream_base& mem_stream::w_cstr(const char* s)
{
    int n = strlen(s);
    buf.write(&n, 4);
    buf.write(s, n);
    return *this;
}
// @ 0x4179e0
stream_base& mem_stream::w_int(int* v) { buf.write(v, 4); return *this; }
// @ 0x417a00
stream_base& mem_stream::w_i64(__int64* v) { buf.write(v, 8); return *this; }
// @ 0x417a20
stream_base& mem_stream::w_tstr(const tstring* s)
{
    int n = s->len;
    buf.write(&n, 4);
    buf.write(s->c_str(), n);
    return *this;
}
// @ 0x417a70
stream_base& mem_stream::w_byte(unsigned char* v) { buf.write(v, 1); return *this; }
// @ 0x417a90
stream_base& mem_stream::r_tstr(tstring* v)
{
    int n;
    r_int(&n);
    v->assign("");
    for (int i = 0; i < n; i++) {
        char c;
        buf.read(&c, 1);
        v->push_back(c);
    }
    return *this;
}
// @ 0x417af0
stream_base& mem_stream::r_byte(char* v) { buf.read(v, 1); return *this; }
// @ 0x417b10
stream_base& mem_stream::r_int(int* v) { buf.read(v, 4); return *this; }
// @ 0x417b30
stream_base& mem_stream::r_i64(__int64* v) { buf.read(v, 8); return *this; }
// @ 0x417b50
stream_base& mem_stream::r_stream(stream_base* o)
{
    int n;
    r_int(&n);
    o->reset();
    for (int i = 0; i < n; i++) {
        char c;
        buf.read(&c, 1);
        o->buf.push_back(c);
    }
    return *this;
}
// @ 0x417bb0
int bin_writer::flush()
{
    int r = 0;
    if (is_open) {
        int len = buf.len;
        if (flushed < len) {
            int bad = 0;
            const char* p = buf.c_str();
            size_t n = fwrite(flushed + p, 1, len - flushed, fp);
            if (n != buf.len - flushed)
                bad = 1;
            fflush(fp);
            flushed = buf.len;
            r = bad;
        }
    }
    return r;
}

// ---- constructors / destructor ---------------------------------------------------------------
// @ 0x417c10
bin_writer::bin_writer() {}
// @ 0x417c50
bin_reader::bin_reader() {}
// @ 0x417c90
mem_stream::~mem_stream() {}

// ---- 32-entry table of vec16, zero-initialised -----------------------------------------------
struct vec_table {
    vec16* data;                       // +0
    int count;                         // +4
    vec_table();                       // 0x417cb0
};
// @ 0x417cb0
vec_table::vec_table()
{
    count = 0x20;
    data = (vec16*)operator new[](0x200);
    vec16 tmp(0.0f, 0.0f, 0.0f);
    for (int j = 0; j < 4; j++)
        for (int k = 0; k < 8; k++) {
            data[j * 8 + k] = tmp;
            data[j * 8 + k].w = 0.0f;
        }
}

// ---- named resources ---------------------------------------------------------------------------
// vtable 0x54aeb4 (slot 0 0x41aee0 deleting dtor, slot 1 0x49df30 returns 7): a resource owning the 32-entry
// table at +0x20. The +0x14 field is set in an inline ctor of an intermediate class (vptr stored last).
struct res_base : named_object {
    int f14;                                   // +0x14
    res_base() { f14 = 0; }
};
struct frame_set : res_base {
    int f18;                                   // +0x18
    int pad1c;                                 // +0x1c
    vec_table table;                           // +0x20
    int f28, f2c, f30;
    frame_set();                               // 0x417e10
    virtual ~frame_set();                      // 0x41aee0
    virtual int type();                        // 0x49df30
};
// @ 0x417e10
frame_set::frame_set()
{
    f18 = 0;
    f28 = 0;
    f2c = 0;
    f30 = 0;
}

// Reads a vec_array through the stream, then flips each key so neighbouring 4-vectors have a non-negative dot
// product (quaternion hemisphere continuity).
stream_base* read_vec_array(stream_base* s, vec_array* a);     // 0x4173b0 (cdecl, returns s)
// @ 0x417e50
stream_base* read_continuous_quats(stream_base* s, vec_array* a)
{
    read_vec_array(s, a);
    f4 prev;
    if (a->n > 0)
        prev = a->v[0];
    for (int i = 1; i < a->n; i++) {
        vec16* q = &a->v[i];
        if (prev.z * q->z + (prev.w * q->w + (prev.y * q->y + prev.x * q->x)) < 0.0f) {
            q->y = -q->y;
            q->z = -q->z;
            q->w = -q->w;
            q->x = -q->x;
        }
        prev = *q;
    }
    return s;
}

// vtable 0x54aaac (slot 0 0x417f20 vector deleting dtor, slot 1 0x48b590 returns 2): the texture record
// (named texture in s00416540), 0x54 bytes; its dtor calls release() (0x4167c0).
struct texture : named_object {
    char pad[0x54 - 0x14];
    texture();
    int release();                             // 0x4167c0
    virtual ~texture();                        // 0x417f20
    virtual int type();                        // 0x48b590
};
// @ 0x417f20
texture::~texture() { release(); }

// vtable 0x54ac08 (slot 0 0x417fb0 vector deleting dtor, slot 1 0x417040): the 0x2c-byte effect technique set
// (effect_set in s00416ef0); its dtor clears the resolved flag and the four handles.
struct effect_set : named_object {
    int f14;
    char loaded;                               // +0x18
    int box[4];                                // +0x1c .. +0x28 (zeroed by a loop the compiler unrolls)
    effect_set();
    virtual ~effect_set();                     // 0x417fb0
    virtual int type();                        // 0x417040
};
// @ 0x417fb0
effect_set::~effect_set()
{
    for (int i = 0; i < 4; i++)
        box[i] = 0;
    loaded = 0;
}

// ---- std algorithms over vec_array (12-byte, operator= at 0x417850) ----------------------------
#include <algorithm>
// @ 0x4180d0
vec_array* copy_vec_arrays(vec_array* first, vec_array* last, vec_array* dest) { return std::copy(first, last, dest); }
// @ 0x418110
vec_array* copy_backward_vec_arrays(vec_array* first, vec_array* last, vec_array* dest) { return std::copy_backward(first, last, dest); }

// ---- vtable anchors: cl emits a class's vtable and its deleting destructor in the TU that defines a ctor ------
texture::texture() {}
effect_set::effect_set() {}
// An array new of the class makes cl emit the vector deleting destructor (??_E, flags 2 = array) as vtable slot 0.
texture* make_texture_array(int n) { return new texture[n]; }
effect_set* make_effect_set_array(int n) { return new effect_set[n]; }

// Equivalence-harness hook: tools/difftest compiles only <id>.cpp; build with /EHa /DEQUIV_EH to pull in the
// EH-frame functions of s00417850_eh.cpp (they need /EHa, so they cannot live in this file's default build).
#ifdef EQUIV_EH
#include "s00417850_eh.cpp"
#endif
