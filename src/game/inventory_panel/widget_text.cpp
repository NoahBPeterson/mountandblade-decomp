// Slice s0050cc60 (batch ds1). Editor helpers: the widget flag getter (0x50cc60), the text box ctor
// (0x50d0b0, vtable 0x597050), the index-list sorter (0x50cc70) and pusher (0x50d0e0), and five members of
// the engine's vector template (0x50d140/0x50d170/0x50d230/0x50d260 + the 0x1f8-record scene reader
// 0x50d520). The engine vector is the VC7.1 std::vector whose allocator carries one dword of state, so
// _Myfirst/_Mylast/_Myend sit at +4/+8/+0xc behind the allocator word. Names provisional; the engine
// string is called `tstring`.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <string.h>
#include <new>
#include <vector>
#include "rgl.h"

// ------------------------------------------------------------------ engine string (0x10 bytes) -----------
#include "../tstring.h"

// ------------------------------------------------------------------ records --------------------------------
// 0x50-byte item stored inside `named_vec::items` (name + tail pointer, see 0x4d10f0).
struct item50 {
    tstring name;                                   // +0
    char pad10[0x34];
    void* tail;                                     // +0x44
};

// Scratch argument the folded STL helpers take by value (the original's slots at -0x2c/-0x30/-0x34/-0x38/-0x3c are read-only).
#include "../junk_tag.h"

void destroy_items(item50* first, item50* last, void* self, junk_tag tag);   // 0x4d10f0

struct sub_vec {                                    // vector<item50>
    void* alloc;                                    // +0
    item50* first;                                  // +4
    item50* last;                                   // +8
    item50* end;                                    // +0xc
    sub_vec();                                      // implicit
    sub_vec(const sub_vec& o);                      // 0x4da1f0
    ~sub_vec() { destroy(); }
    void destroy()
    {
        if (first != 0) {
            destroy_items(first, last, this, junk_tag());
            ::operator delete(first);
        }
        first = 0;
        last = 0;
        end = 0;
    }
};

struct named_vec {
    tstring name;                                   // +0
    sub_vec items;                                  // +0x10
};

// engine vector with the allocator dword in front (see the file comment).
struct vec_named {                                  // vector<named_vec>
    void* alloc;                                    // +0
    named_vec* first;                               // +4
    named_vec* last;                                // +8
    named_vec* end;                                 // +0xc
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    unsigned max_size() const { return 0x7ffffff; }
    named_vec* _Ufill(named_vec* ptr, unsigned n, const named_vec& val);   // 0x50d140
    named_vec* _Ucopy(named_vec* first_, named_vec* last_, named_vec* dst); // 0x4fe840
    void _Destroy(named_vec* first_, named_vec* last_);                    // 0x50d230
    void _Insert_n(named_vec* where, unsigned n, const named_vec& val);    // 0x50d260
    void _Xlen() const;                                                    // 0x43e2a0
};
void vec_named::_Xlen() const { throw std::length_error("vector<T> too long"); }

// ------------------------------------------------------------------ the 0x1f8 model record ---------------
#include "../model_b.h"

struct vec_model {                                  // vector<model_b>
    void* alloc;                                    // +0
    model_b* first;                                 // +4
    model_b* last;                                  // +8
    model_b* end;                                   // +0xc
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    void clear();                                   // 0x4eb810
    void resize(unsigned n, model_b val = model_b());   // 0x50d170
};

#include "../int_vec.h"                                 // IntVec (was vec_int here)

struct vec_widget {                                 // vector<widget*> (allocator word not initialised)
    void* alloc;                                    // +0
    void** first;                                   // +4
    void** last;                                    // +8
    void** end;                                     // +0xc
};

// ------------------------------------------------------------------ callees --------------------------------
void  destroy_named(named_vec* first, named_vec* last, void* self, void* extra);   // 0x4f6680
void  destroy_named_t(named_vec* first, named_vec* last, void* self, junk_tag tag); // 0x4f6680
void  insert_models(model_b** out, unsigned n, const model_b* val);      // 0x4fe520
void  erase_models(model_b* out, model_b* first, model_b* last);         // 0x4eb860
void* uninit_copy(void* first, void* last, void* dst, void* self, junk_tag tag);   // 0x4ebe10
void* uninit_fill(void* dst, unsigned n, const void* val, void* self, const void* val2);  // 0x4f65f0
void  fill_named(named_vec* first, named_vec* last, const named_vec* val);         // 0x4e5f80
void  copy_back(void* first, void* last, void* dst, junk_tag tag);        // 0x4e42b0
void  destroy_items(item50* first, item50* last, void* self, junk_tag tag);         // 0x4d10f0
void  scene_read(void* self, void* stream);                              // 0x4e7b30
void  tstring_dtor(tstring* s);                                          // 0x42bc40

// ------------------------------------------------------------------ globals --------------------------------
char* g_game;                                       // 0x5e2930
char  g_flag_a181b8;                                // 0xa181b8

#include "../str_table.h"                                       // StrTable: the translated-string table 0x5e2a18 (find 0x4c6010)
struct PartyTable { int find(int id); };                        // 0x48ad00
struct AccessStream {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48();
    virtual void read_int(int* out);                            // +0x4c
};

// ------------------------------------------------------------------ the widget base ------------------------
#include "../widget.h"                              // widget (the 0x178-byte base, vptr 0x591c80, ctor 0x48ae20, flag1 0x50cc60; shared)

// @ 0x50cc60 widget::flag1
unsigned widget::flag1() const
{
    return flags & 1;
}

// the text box (vtable 0x597050, size 0x188)
struct text_box : widget {
    vec_widget kids;                                // +0x178
    virtual void v1();                              // vtable 0x597050
    text_box();                                     // 0x50d0b0
};

// @ 0x50d0b0 text_box::text_box
text_box::text_box()
{
    kids.first = 0;
    kids.last = 0;
    kids.end = 0;
}

// ------------------------------------------------------------------ the selection list ----------------------
struct sel_list {
    IntVec v;                                       // +0
    char pad[0x1708 - 0x10];
    char dirty;                                     // +0x1708
    void add(int val);                              // 0x50d0e0
};

// @ 0x50d0e0 sel_list::add
void sel_list::add(int val)
{
    dirty = 1;
    if (v.count() < v.capacity()) {
        int* p = v._Mylast;
        *p = val;
        v._Mylast = p + 1;
    } else {
        v.insert_n_49b1c0(v._Mylast, 1, &val);
    }
}

// ------------------------------------------------------------------ name lookups used by the sorter --------
// the editor's name tables hang off the game object as pointer members; the compiler folds the member
// load into the record-address arithmetic (`add esi,[eax+0x13a00]`).
static inline const char* name_of_1(int id)
{
    char* base = *(char**)(g_game + 0x13a00);
    char* rec = base + id * 0x508;
    const tstring* t = ((StrTable*)0x5e2a18)->find((const tstring*)(rec + 0x20));
    if (t == 0)
        t = (const tstring*)rec;
    return t->c_str();
}

static inline const char* name_of_2(int id)
{
    char* base = *(char**)(g_game + 0x13a48);
    char* rec = base + id * 0x300;
    const tstring* t = (const tstring*)rec;
    if (rec[0x10] == 0) {
        const tstring* q = ((StrTable*)0x5e2a18)->find((const tstring*)(rec + 0x14));
        if (q != 0)
            t = q;
    }
    return t->c_str();
}

static inline const char* name_of_3(int id)
{
    char* mgr = g_game + 0xf8;
    int v = ((PartyTable*)mgr)->find(id);
    char* rec = *(char**)(mgr + 0x138d0 + (v >> 4) * 4) + (v & 0xf) * 0x5ac8;
    const tstring* t = (const tstring*)(rec + 0xc);
    if (g_flag_a181b8) {
        t = (const tstring*)(rec + 0x20);
    } else if (rec[0x1c] == 0) {
        const tstring* q = ((StrTable*)0x5e2a18)->find((const tstring*)(rec + 0x20));
        if (q != 0)
            t = q;
    }
    return t->c_str();
}

struct row138 {
    char pad0[0x40];
    tstring text;                                   // +0x40
};
static inline const tstring& name_of_4(int id)
{
    return *(const tstring*)(*(char**)(g_game + 0x13a58) + id * 0x138 + 0x40);
}

static inline void swap_index(sel_list* l, unsigned i, unsigned j)
{
    int t = l->v._Myfirst[i];
    l->v._Myfirst[i] = l->v._Myfirst[j];
    l->v._Myfirst[j] = t;
}

// @ 0x50cc70 sel_list::sort
void __stdcall sort_list(sel_list* l, int mode)
{
    for (unsigned i = 0; i < l->v.count(); i++) {
        for (unsigned j = i + 1; j < l->v.count(); j++) {
            switch (mode) {
            case 1:
                if (strcmp(name_of_1(l->v._Myfirst[i]), name_of_1(l->v._Myfirst[j])) > 0)
                    swap_index(l, i, j);
                break;
            case 2:
                if (strcmp(name_of_2(l->v._Myfirst[i]), name_of_2(l->v._Myfirst[j])) > 0)
                    swap_index(l, i, j);
                break;
            case 3:
                if (strcmp(name_of_3(l->v._Myfirst[i]), name_of_3(l->v._Myfirst[j])) > 0)
                    swap_index(l, i, j);
                break;
            case 4:
                if (strcmp(name_of_4(l->v._Myfirst[i]).strip_tags().c_str(),
                           name_of_4(l->v._Myfirst[j]).strip_tags().c_str()) > 0)
                    swap_index(l, i, j);
                break;
            }
        }
    }
}

// ------------------------------------------------------------------ the dialog thunk -----------------------
#include "../editor_panel.h"
extern EditorPanel g_panel;                         // 0x8ee3c48

// @ 0x50d250 dialog_thunk
LRESULT __stdcall dialog_thunk(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return g_panel.terrain_dialog(hDlg, msg, wParam, lParam);
}

// ------------------------------------------------------------------ vector<named_vec> members --------------
// @ 0x50d140 vec_named::_Ufill
named_vec* vec_named::_Ufill(named_vec* ptr, unsigned n, const named_vec& val)
{
    const named_vec* q = &val;
    uninit_fill(ptr, n, q, this, &val);
    return ptr + n;
}

// @ 0x50d230 vec_named::_Destroy
void vec_named::_Destroy(named_vec* first_, named_vec* last_)
{
    named_vec* p = last_;
    destroy_named(first_, last_, this, p);
}

// @ 0x50d260 vec_named::_Insert_n
void vec_named::_Insert_n(named_vec* where, unsigned count, const named_vec& val)
{
    named_vec tmp = val;
    unsigned cap = capacity();

    if (count == 0)
        ;
    else if (max_size() - size() < count)
        _Xlen();
    else if (cap < size() + count) {
        cap = max_size() - cap / 2 < cap ? 0 : cap + cap / 2;
        if (cap < size() + count)
            cap = size() + count;
        named_vec* newvec = (named_vec*)::operator new(cap * sizeof(named_vec));
        named_vec* ptr = newvec;
        try {
            ptr = _Ucopy(first, where, newvec);
            ptr = _Ufill(ptr, count, tmp);
            _Ucopy(where, last, ptr);
        } catch (...) {
            _Destroy(newvec, ptr);
            ::operator delete(newvec);
            throw;
        }
        count += size();
        if (first != 0) {
            destroy_named_t(first, last, this, junk_tag());
            ::operator delete(first);
        }
        end = newvec + cap;
        last = newvec + count;
        first = newvec;
    } else if ((unsigned)(last - where) < count) {
        named_vec* newend = where + count;          // materialised first: the original builds it before
        uninit_copy(where, last, newend, this, junk_tag());   // the tag/self argument setup
        try {
            _Ufill(last, count - (last - where), tmp);
        } catch (...) {
            _Destroy(where + count, last + count);
            throw;
        }
        last += count;
        fill_named(where, last - count, &tmp);
    } else {
        named_vec* oldend = last;
        last = (named_vec*)uninit_copy(oldend - count, oldend, oldend, this, junk_tag());
        copy_back(where, oldend - count, oldend, junk_tag());
        fill_named(where, where + count, &tmp);
    }
}

// ------------------------------------------------------------------ vector<model_b> members ---------------
void __stdcall insert_models(model_b* where, unsigned n, const model_b& val);      // 0x4fe520
void __stdcall erase_models(const model_b& out, model_b* first, model_b* last);    // 0x4eb860

// @ 0x50d170 vec_model::resize
void vec_model::resize(unsigned n, model_b val)
{
    model_b* f = first;
    if (size() < n)
        insert_models(last, n - size(), val);
    else if (n < size())
        erase_models(val, f + n, last);
}

// ------------------------------------------------------------------ the scene reader -----------------------
// 0x1f8-record collection laid out as: five dwords of state, the model vector at +0x18, the count read
// from the stream at +0x28, 0x3f morph weights at +0x2c and two scale floats at +0x128/+0x12c.
struct mesh_scene {
    int f0;                                         // +0
    int f4;                                         // +4
    int f8;                                         // +8
    int fc;                                         // +0xc
    int f10;                                        // +0x10
    int f14;                                        // +0x14
    vec_model models;                               // +0x18
    int count;                                      // +0x28
    float weights[0x3f];                            // +0x2c
    float f128;                                     // +0x128
    float f12c;                                     // +0x12c
    void read(AccessStream* s);                     // 0x50d520
};

// @ 0x50d520 mesh_scene::read
void mesh_scene::read(AccessStream* s)
{
    models.clear();
    s->read_int(&count);
    unsigned n = count;
    models.resize(n);
    for (int i = 0; i < count; i++) {
        model_b* p = models.first + i;
        AccessStream* in = s;
        p->read(in);
    }
    f8 = 0;
    f0 = -1;
    f4 = -1;
    f14 = 0;
    fc = 0;
    float* w = weights;
    f12c = 0.5f;
    f128 = 0.3f;
    for (int i = 0; i < 0x3f; i++)
        w[i] = 0.5f;
    model_b* m = &models.first[f8];
    m->relax(-1, w);
}
