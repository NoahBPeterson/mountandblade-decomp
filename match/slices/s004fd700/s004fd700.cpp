// Slice s004fd700: two std::vector<vector4> helpers, plus the overlay renderer 0x4fd700 they serve.
//   0x4fdff0  vector<vector4>::push_back(const vector4&): fast path -> 0x40df60 uninitialized fill of one
//             element at _Mylast, else insert(_Mylast, 1, val).
//   0x4fe060  vector<vector4>::insert(iterator where, unsigned count, const vector4& val): index = 0 when
//             _Myfirst == 0 or _Mylast == _Myfirst (the empty-vector guard from the original's
//             `sub ecx, edx; sar 2` truncation), else where - _Myfirst; then _Insert_n (0x40dc50) and
//             *out = _Myfirst + index * 0x10.  `this` is in ECX in both functions.
//   0x4fd700  Font::layout -- the "map text" row renderer.  `this` (the font) is in ECX, 17 stack arguments
//             (arg1 = the caller's result block, arg2 = the row rect, arg3 = a scratch vector4, arg4 = the
//             font-metric rect, arg5 = the mesh, arg6/arg7/arg8 = the colours, arg9/arg10 = the x extent
//             pair, arg11 = the flag mask, arg12 = the current row, arg13/arg14/arg16 = the three vectors
//             that are cleared, arg15/arg17 = the two that are wiped) and `ret 0x44`.  It clears the mface
//             vectors, wipes the output vectors, decodes the text, measures it with the font, breaks it into
//             rows honouring <tag> markup and \t/\n, appends the row rectangles + row indices to the caller's
//             vectors, draws every glyph through 0x44f5d0, recomputes the mesh normals and finally writes
//             {width, height} into arg1.  The original supplies `this` in ECX and the result through arg1;
//             cl 13.10 cannot spell that shape (it would pass a hidden by-value return pointer instead), so
//             the function is declared as a Font member whose first stack argument is the caller's result
//             block: the ECX `this` and the 17 stack arguments are then identical to the original's, which
//             is what the callers (s004fd1d0) and the equivalence harness need.
#include <windows.h>
#include "rgl.h"

using rgl::vector4;

// ---- vector<T> (s0040d7d0): allocator object at +0, _Myfirst at +4, _Mylast at +8, _Myend at +0xc -----
template <class T>
struct vec {
    T* f0;
    T* first;                                       // +4
    T* last;                                        // +8
    T* end;                                         // +0xc
};

void vec4_uninit_copy(vector4* dst, unsigned count, const vector4& val);                        // 0x40df60
vector4* vec4_insert_n(vec< vector4 >* self, vector4* where, unsigned count, const vector4& val);  // 0x40dc50
void vec4_insert(vec< vector4 >* self, vector4** out, vector4* where, unsigned count,
                 const vector4& val);                                                          // 0x4fe060

// @ 0x4fdff0  (this in ECX, element by address at [esp+8])
void vec4_push_back(vec< vector4 >* self, const vector4& x)
{
    vector4* p = self->first;
    if (p != 0 && ((self->last - p) & 0x3fffffff) < ((self->end - p) & 0x3fffffff)) {
        vector4* last = self->last;
        vec4_uninit_copy(last, 1, x);
        self->last = last + 1;
        return;
    }
    vec4_insert(self, (vector4**)self->last, self->last, 1, x);
}

// @ 0x4fe060  (this in ECX; out slot, where, count and val on the stack)
void vec4_insert(vec< vector4 >* self, vector4** out, vector4* where, unsigned count, const vector4& val)
{
    unsigned idx = 0;
    if (self->first != 0 && ((self->last - self->first) & 0x3fffffff) != 0)
        idx = (unsigned)(where - self->first);
    vec4_insert_n(self, where, count, val);
    vector4* p = self->first;
    *out = p + idx;
}

// ---- the 8-byte layout result ---------------------------------------------------------------------------
struct MapText { int a; float w; };

namespace rgl {
struct mesh {
    char pad0[0x2c];
    void* res;                                      // +0x2c
    char pad30[0x260 - 0x30];
    mesh();                                         // 0x412a90
    void clear_geometry();                          // 0x428e80
};
}
using rgl::mesh;

// ---- the engine string (0x10 bytes) ---------------------------------------------------------------------
struct tstring {
    int pos;                                        // +0
    char* buf;                                      // +4
    int cap;                                        // +8
    int len;                                        // +0xc
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    ~tstring();                                     // 0x42bc40
    void push_back(char c);                         // 0x42be70
    void to_lower();                                // 0x42c020
    int find(const tstring& sub, int start) const;  // 0x42c440
    int find(char c, int start) const;              // 0x42c080
    tstring substr(int a, int b) const;             // 0x42c4a0 (hidden return pointer)
};

// ---- the font (this) ------------------------------------------------------------------------------------
struct Font {
    char pad00[0x18];
    float f18, f1c, f20, f24;                       // +0x18, +0x1c, +0x20, +0x24 (font scale metrics)
    char f28[0x4c - 0x28];                          // +0x28 (the default glyph block)
    int f4c;                                        // +0x4c (key of the default glyph block)
    char pad50[0x6c - 0x50];
    char glyphs[1];                                 // +0x6c (glyph records, stride 0x1c)

    // 0x43e200: measure `len` characters of the decoded text; arg3 receives the end index, arg4 the y
    // advance; returns the run width.
    float measure(const int* buf, int len, int* out, float* arg) const;

    // 0x4fd700: `this` in ECX, 17 stack arguments (arg1 = the caller's result block), `ret 0x44`.
    MapText* layout(void* out, const void* p2, const void* p3, const void* p4, mesh* p5, const void* p6,
                    const void* p7, const void* p8, const void* p9, const void* p10, int p11, int p12,
                    const void* p13, const void* p14, const void* p15, const void* p16, const void* p17);
};

// ---- engine / CRT declarations used by the renderer ------------------------------------------------------
// The four markup tag names, at the addresses the original built them into.
extern "C" const char tag_alpha[];                  // 0x599684 ("alpha")
extern "C" const char tag_font[];                   // 0x59968c ("font")
extern "C" const char tag_size[];                   // 0x599690 ("size")
extern "C" const char tag_color[];                  // 0x599698 ("color")
void __cdecl vec_clear(void* self);                 // 0x41f4b0 (?clear@?$vec@Umface@@@@QAEXXZ, ECX = self)
void __cdecl font_decode(int* buf, const char* s);  // 0x487b00 (decode_utf8)
int __cdecl grid_round();                           // 0x535a64
void __cdecl vec_insert_n_int(vec<int>* self, int* where, unsigned n, const int* val);  // 0x43e7b0 (ECX = self)
void __cdecl vec_push_back_a(void* self, const int* val);   // 0x4fe0b0 (ECX = self; vector<int>::push_back)
void __cdecl vec_push_back_b(void* self, const int* val);   // 0x440b30 (ECX = self; the ICF'd twin)

// A vector<int>::push_back, inlined exactly as the original has it: the fast path stores the element at
// _Mylast, the slow path goes to _Insert_n (0x43e7b0).
static void vec_int_push_back(vec<int>* self, const int* val)
{
    int* p = self->first;
    if (p != 0 && ((self->last - p) & 0x3fffffff) < ((self->end - p) & 0x3fffffff)) {
        *self->last = *val;
        self->last = self->last + 1;
        return;
    }
    vec_insert_n_int(self, self->last, 1, val);
}
void __cdecl glyph_lookup(void* self, int* out, const int* key);   // 0x43b210 (ECX = self)
void __cdecl glyph_draw(int glyph, const float* pos, const float* scale, const void* mesh, unsigned color); // 0x44f5d0
void __cdecl mesh_compute_normals(mesh* m);         // 0x451c50
long __cdecl imp_atol(const char* s);               // 0x53734d
const char tag_alpha[] = "alpha";
const char tag_font[]  = "font";
const char tag_size[]  = "size";
const char tag_color[] = "color";

// @ 0x4fd700
MapText* Font::layout(void* out, const void* p2, const void* p3, const void* p4, mesh* p5, const void* p6,
                      const void* p7, const void* p8, const void* p9, const void* p10, int p11, int p12,
                      const void* p13, const void* p14, const void* p15, const void* p16, const void* p17)
{
    (void)p2; (void)p4; (void)p6;

    // --- prologue: clear the three vectors and wipe the two output vectors ----------------------------
    vec<vector4>* a13 = (vec<vector4>*)p13;     // arg13: cleared, receives the rect pushes
    vec<vector4>* a14 = (vec<vector4>*)p14;     // arg14: cleared, receives the rect pushes
    vec<int>*     a16 = (vec<int>*)p16;         // arg16: cleared, holds the tag indices
    vec<int>*     a15 = (vec<int>*)p15;         // arg15: wiped, holds the tag values
    vec<int>*     a17 = (vec<int>*)p17;         // arg17: wiped, holds the tag name codes
    vec_clear(a13);
    vec_clear(a14);
    if (a15->first != 0)
        ::operator delete(a15->first);
    a15->first = 0; a15->last = 0; a15->end = 0;
    vec_clear(a16);
    if (a17->first != 0)
        ::operator delete(a17->first);
    a17->first = 0; a17->last = 0; a17->end = 0;

    // --- font metrics and the row rectangle -----------------------------------------------------------
    const vector4* fr   = (const vector4*)p3;       // arg3: the row rectangle
    const float*   xext = (const float*)p9;         // arg9: the x extent pair

    float cx = fr->x, cy = fr->y;
    if ((p11 & 0x400) != 0)
        cx = xext[0];

    float adv  = (xext[0] / f18) * f20;             // per-character advance
    float step = (f20 / f1c) * xext[1];             // line step
    float max_x = cx;                               // local_80cc: the running x
    float y = 0.0f;                                 // local_80d0: the current line start
    char in_tag = 0;                                // local_80d1 / byte[esp+0x12]: the '>' parity
    char tag = 0;                                   // byte[esp+0x1b]: inside a '<...>' sequence
    float tag_start = 0.0f;                         // local_8098: the index of the '<'
    int tag_index = -1;                             // local_80ac: the current tag ordinal
    float line_y = cy;                              // local_807c: the running line y
    float widest = xext[0];                         // local_80b4: the widest x reached

    int buf[8190];
    float count_f = 0.0f;                           // local_80a0
    font_decode(buf, (const char*)0x5937ac);

    // optional shrink so the measured text fits the extent
    if ((p11 & 0x100) == 0 && (p11 & 2) == 0) {
        int arg = 0;
        float measured = measure(buf, 0, &arg, &count_f);
        float w = (measured * adv) * 1.001f;
        if ((xext[1] - xext[0] < w) && (0.0f < w)) {
            float s = (xext[1] - xext[0]) / w;
            adv *= s;
            step *= s;
        }
    }

    int total = (int)count_f;
    if (0 < total) {
        float inv_adv = 1.0f / adv;                 // local_8094
        float search = y;                           // local_80a8: the end of this row
        do {
            search = y;
            float run_w = 0.0f;                     // local_809c
            char first = 1;
            int cur;
            for (;;) {
                int stop = 0;
                int i = (int)search;
                for (; i < total; i++) {
                    int ch = buf[i];
                    if (ch == 0x20 || ch == 10 || ch == 9) {
                        if (i < total) {
                            stop = (buf[i] == 10);
                            i++;
                        }
                        break;
                    }
                }
                cur = i;
                float arg = search;
                float m = measure(buf, total, &cur, &arg);
                if (first != 0 || m * adv <= xext[1] - xext[0]) {
                    run_w = m * adv;
                    search = (float)cur;
                } else {
                    stop = 1;
                }
                if (total <= (int)search)
                    stop = 1;
                first = 0;
                if (stop)
                    break;
            }

            // horizontal placement of this row
            if ((p11 & 4) != 0) {
                max_x = xext[0];
            } else if ((p11 & 0x10) != 0) {
                max_x = (xext[0] + xext[1]) * 0.5f - run_w * 0.5f;
            } else if ((p11 & 8) != 0) {
                max_x = (xext[0] + xext[1]) - run_w;
            }

            float xpos = inv_adv * max_x;           // local_80b8 (the starting column)
            for (int i2 = (int)y; i2 < (int)search; i2++) {
                if (tag != 0) {
                    if (buf[i2] != 0x3e)                    // not '>'
                        continue;
                    in_tag = !in_tag;
                    if (in_tag) {                           // the '<...>' body
                        tag_index++;
                        tstring name;                       // 0x42bbd0
                        for (int e = (int)tag_start + 1; e < i2; e++)
                            name.push_back((char)buf[e]);
                        name.to_lower();
                        {
                            tstring t(tag_color);
                            int hit = name.find(t, 0);
                            t.~tstring();
                            if (hit != 0) { int v = 1; vec_push_back_a(a17, &v); }
                            else {
                                tstring t2(tag_size);
                                int hit2 = name.find(t2, 0);
                                t2.~tstring();
                                if (hit2 != 0) { int v = 2; vec_push_back_a(a17, &v); }
                                else {
                                    tstring t3(tag_font);
                                    int hit3 = name.find(t3, 0);
                                    t3.~tstring();
                                    if (hit3 != 0) { int v = 3; vec_push_back_a(a17, &v); }
                                    else {
                                        tstring t4(tag_alpha);
                                        int hit4 = name.find(t4, 0);
                                        t4.~tstring();
                                        if (hit4 != 0) { int v = 4; vec_push_back_a(a17, &v); }
                                    }
                                }
                            }
                        }
                        {
                            char eq = '=';
                            int pos = name.find(eq, 0);
                            tstring sub = name.substr(pos + 1, tag_index);
                            const char* p = sub.buf ? sub.buf : (const char*)((char*)&sub + 4);
                            int v = (int)imp_atol(p);
                            sub.~tstring();
                            vec_int_push_back(a17, &v);
                            vec4_push_back(a13, *(const vector4*)&max_x);
                            vec_int_push_back(a15, &tag_index);
                        }
                        tag = 0;
                    } else {                                // the closing '>': emit a rect
                        vector4 t;
                        t.x = max_x;
                        t.y = (f20 * fr->y) * f24 * 0.9f + cy;
                        t.z = fr->y;
                        t.w = 0.0f;
                        vec4_push_back(a14, t);
                        tag = 0;
                    }
                    continue;
                }

                int ch = buf[i2];
                if (ch == 0x3c) {                           // '<'
                    tag = 1;
                    tag_start = (float)i2;
                } else if (ch != 10) {
                    if (ch == 9) {                          // '\t'
                        int tab = grid_round();
                        max_x = ((((float)tab + 1.0f) * 0.25f) + xpos) * adv;
                    } else {
                        const char* g;
                        if (ch < 0x200) {
                            g = glyphs + ch * 0x1c;
                        } else {
                            int key = ch;
                            int hit = 0;
                            glyph_lookup((char*)this + 0x44, &hit, &key);
                            g = (hit == f4c) ? f28 : (const char*)(hit + 0xc);
                        }
                        if (in_tag) {
                            if (i2 == (int)y) {
                                vec_push_back_a(a16, (const int*)a16->last - 1);
                                vec_push_back_b(a17, (const int*)a17->last - 1);
                                vec4_push_back(a14, *(const vector4*)&max_x);
                                vec_push_back_b(a15, &tag_index);
                            }
                            max_x += adv * *(const float*)(g + 0x10);
                            glyph_draw(ch, &max_x, &adv, p5,
                                       (p12 == tag_index) ? (unsigned)(size_t)p8 : (unsigned)(size_t)p7);
                        } else {
                            max_x += adv * *(const float*)(g + 0x10);
                            glyph_draw(ch, &max_x, &adv, p5, (unsigned)(size_t)p7);
                        }
                        max_x += adv * *(const float*)(g + 0x18);
                    }
                }
            }

            if (in_tag) {
                vector4 t;
                t.x = max_x;
                t.y = (f20 * fr->y) * f24 * 0.9f + cy;
                t.z = fr->y;
                t.w = 0.0f;
                vec4_push_back(a14, t);
            }

            if (widest < max_x)
                widest = max_x;
            max_x = xext[0];
            y = search;
            line_y -= (f20 * fr->y) * f24 * 0.9f;
        } while ((int)search < total);
    }

    mesh_compute_normals(p5);
    ((float*)out)[0] = (widest - xext[0]) + 0.0001f;
    ((float*)out)[1] = line_y - y;
    return (MapText*)out;
}
