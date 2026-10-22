// Slice s004c7bf0: the map editor's object-properties dialog (0x4c7bf0: refill the entry list of the chosen
// object type, 0x4c7e20: refill the placed-object list), the deleting destructor of the record array
// (0x4c7ee0), the terrain apron (0x4c7f90), the triangle normal under the mouse (0x4c8000) and a mesh block
// (0x4c80c0 clear, 0x4c8110 read, 0x4c8330 free its materials, 0x4c83a0 read its terrain block).
// Names provisional; offsets from the disassembly. windows.h from SDK 6.0A does not parse under cl 13.10,
// so the few Win32 entry points are declared here.
#include <stdio.h>
#include "rgl.h"
typedef struct HWND__* HWND;
typedef unsigned int UINT;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef long LRESULT;
#define WINAPI __stdcall
#define DLLI extern "C" __declspec(dllimport)
DLLI HWND WINAPI GetDlgItem(HWND, int);
DLLI LRESULT WINAPI SendMessageA(HWND, UINT, WPARAM, LPARAM);
#define LB_ADDSTRING 0x180
#define LB_GETCURSEL 0x188
#define LB_RESETCONTENT 0x184
#define LB_SETCURSEL 0x186
#define LB_SETITEMDATA 0x19a
#define CB_GETCURSEL 0x147
#define WM_USER_LIST 0x150
#define WM_USER_SEL 0x199

// tstring: the engine's 16-byte string with a read cursor (layout from s00429890/tstring.h; copied locally
// because the shared include directory is not on cl.sh's include path).
struct tstring {
    int pos;            // +0   read cursor
    char* buf;          // +4   (null while empty)
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    void underscores_to_spaces();               // 0x42bf80
    bool differs(const char* s) const;          // 0x42bdb0
    void assign(const char* s);                 // 0x42bcb0
    tstring substr(int start, int end) const;   // 0x42c4a0
    char* str() const { return buf ? buf : (char*)this + 4; }
};

// a translation table (keys -> localized text); find returns null when the key is absent
struct StrTable {
    tstring* find(const tstring* key);
};
extern StrTable g_strtable;                             // 0x5e2a18
extern void mb_error(const char* fmt, ...);             // 0x42cc80

// the resource manager
struct res_manager {
    void* get_material(const tstring& name);            // 0x418a70
    void release(void* res);                            // 0x4175f0
};
extern res_manager g_res_manager;                       // 0x5d6fa0

// the reader a block is deserialized from (an access-code stream); only the slots this slice calls
struct AccessCodeStream {
    virtual void v00() { for (int k = 0; k < 1; k++) { (void)k; } }
    virtual void v04() { for (int k = 0; k < 2; k++) { (void)k; } }
    virtual void v08() { for (int k = 0; k < 3; k++) { (void)k; } }
    virtual void v0c() { for (int k = 0; k < 4; k++) { (void)k; } }
    virtual void v10() { for (int k = 0; k < 5; k++) { (void)k; } }
    virtual void v14() { for (int k = 0; k < 6; k++) { (void)k; } }
    virtual void v18() { for (int k = 0; k < 7; k++) { (void)k; } }
    virtual void v1c() { for (int k = 0; k < 8; k++) { (void)k; } }
    virtual void v20() { for (int k = 0; k < 9; k++) { (void)k; } }
    virtual void v24() { for (int k = 0; k < 10; k++) { (void)k; } }
    virtual void v28() { for (int k = 0; k < 11; k++) { (void)k; } }
    virtual void v2c() { for (int k = 0; k < 12; k++) { (void)k; } }
    virtual void v30() { for (int k = 0; k < 13; k++) { (void)k; } }
    virtual void v34() { for (int k = 0; k < 14; k++) { (void)k; } }
    virtual void f32(float* f);                         // 0x38  the float reader
    virtual void v3c() { for (int k = 0; k < 16; k++) { (void)k; } }
    virtual void v40() { for (int k = 0; k < 17; k++) { (void)k; } }
    virtual void v44() { for (int k = 0; k < 18; k++) { (void)k; } }
    virtual void v48() { for (int k = 0; k < 19; k++) { (void)k; } }
    virtual void i32(int* i);                           // 0x4c  the int reader
    virtual void v50() { for (int k = 0; k < 21; k++) { (void)k; } }
    virtual void str(tstring* s);                       // 0x54  the string reader
};
void AccessCodeStream::str(tstring* s)
{
    for (int i = 0; i < 1; i++)
        s = s;                                          // body (the layout is what matters here)
}
void AccessCodeStream::i32(int* i)
{
    *(int*)i = 0;
}
void AccessCodeStream::f32(float* f)
{
    *f = 0.0f;
}
// the game frame the dialog drives: +0x1a0b8 the selected list type, +0x1a0bc the selected item
extern char* g_5e2930;
// the map: its record array (+0x668c, 0x1c8-byte records)
extern char* g_5e292c;
extern void record_name(char* out, void* r);            // 0x4c6620
extern void record_free(void* r);                       // 0x4c65d0
// the editor's own mesh record (the terrain apron)
extern void build_terrain(int idx);                     // 0x4c54f0
// the count of the map's records: (end - begin) / 0x1c8
inline int map_record_count()
{
    char** end = (char**)(g_5e292c + 0x6690);
    char* begin = *(char**)(g_5e292c + 0x668c);
    return (*end - begin) / 0x1c8;
}

enum { noquote = 1 };
struct Block;

// the dialog
struct MapEditorDialog {
    char pad[0x374];
    HWND hwnd;                                          // +0x374
    void update_object_type();                          // 0x4c7bf0
    void update_objects();                              // 0x4c7e20
    void update_terrain(int idx, const float* d);       // 0x4c7f90
};

// @ 0x4c7bf0
void MapEditorDialog::update_object_type()
{
    if (!hwnd)
        return;
    HWND type = GetDlgItem(hwnd, 0x431);
    HWND list = GetDlgItem(hwnd, 0x433);
    int type_sel = SendMessageA(type, CB_GETCURSEL, 0, 0);
    int list_type = SendMessageA(type, WM_USER_LIST, type_sel, 0);
    SendMessageA(list, LB_RESETCONTENT, 0, 0);
    if (!list_type) {
        int count = *(int*)0x5e29e0;
        for (int i = 0, off = 0; i < count; i++, off += 0x78) {
            char* e = (char*)(*(int*)0x5e29dc + off);
            tstring sub = ((tstring*)e)->substr(4, *(int*)(e + 0xc));
            int item = SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)sub.str());
            sub.~tstring();
            SendMessageA(list, LB_SETITEMDATA, item, i);
        }
    } else if (list_type == 2) {
        int count = *(int*)0x5e29d4;
        for (int i = 0, off = 0; i < count; i++, off += 0x238) {
            tstring* key = (tstring*)(*(int*)0x5e29d0 + off + 0x20);
            tstring* text = g_strtable.find(key);
            if (text)
                key = text;
            int item = SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)key->str());
            SendMessageA(list, LB_SETITEMDATA, item, i);
        }
    } else if (list_type == 4) {
        int count = *(int*)0x5e29e8;
        for (int i = 0, off = 0; i < count; i++, off += 0x20) {
            tstring* s = (tstring*)(*(int*)0x5e28f8 + off + 4);
            int item = SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)s->str());
            SendMessageA(list, LB_SETITEMDATA, item, i);
        }
    } else if (list_type == 1) {
        SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)"Entry Point");
        SendMessageA(list, LB_SETITEMDATA, 0, 0);
    } else if (list_type == 5) {
        SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)"Passage");
        SendMessageA(list, LB_SETITEMDATA, 0, 0);
    }
    SendMessageA(list, LB_SETCURSEL, 0, 0);
    *(int*)(g_5e2930 + 0x1a0bc) = SendMessageA(list, LB_GETCURSEL, 0, 0);
    *(int*)(g_5e2930 + 0x1a0b8) = list_type;
}

// @ 0x4c7e20
void MapEditorDialog::update_objects()
{
    if (!hwnd)
        return;
    HWND list = GetDlgItem(hwnd, 0x438);
    SendMessageA(list, LB_RESETCONTENT, 0, 0);
    int i = 0;
    int off = 0;
    do {
        char* base = *(char**)(g_5e292c + 0x668c);
        if (!base)
            break;
        if ((unsigned)i >= (unsigned)map_record_count())
            break;
        char name[128];
        record_name(name, base + off);
        int item = SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)name);
        SendMessageA(list, LB_SETITEMDATA, item, i);
        i++;
        off += 0x1c8;
    } while (*(char**)(g_5e292c + 0x668c));
}

// the apron: after rebuilding the terrain, every vertex not pinned by a flag follows the drag
// @ 0x4c7f90
void MapEditorDialog::update_terrain(int idx, const float* d)
{
    build_terrain(idx);
    for (int i = 0, off = 0;; i++, off += 0x18) {
        char* base = *(char**)((char*)this + 0x1c);
        if (!base)
            break;
        int count = (*(char**)((char*)this + 0x20) - base) / 0x18;
        if (i >= count)
            break;
        float* p = (float*)(*(char**)((char*)this + 0x1c) + off);
        if (p[5] != 0.0f) {
            p[0] += d[0];
            p[1] += d[1];
            p[2] += d[2];
        }
    }
}

// ---------------------------------------------------------------- record array --------------------------------
// the array's elements are 0x78 bytes: +0x00 the pointer to the element's name, +0x18 the owner, +0x28 the
// cleared sub-object, +0x48/+0x4c/+0x50 three owned pointers, and the element's 16-byte name string sits at
// its very start, 4 bytes *before* the first of those fields
struct RecordArrayHead {
    int count;                                          // -4
};
extern void record_array_clear(void* r);                // 0x4317b0
// @ 0x4c7ee0
void* __cdecl destroy_record_array(void* p, unsigned int flags)
{
    int* head;
    int n;
    if (!(flags & 2)) {
        record_free(p);
        if (flags & 1)
            operator delete(p);
        return p;
    }
    n = ((RecordArrayHead*)p)[-1].count;
    head = (int*)p - 1;
    char* e = (char*)head + 0x48 + n * 0x78;            // &elem[n-1] + 0x48
    for (; n > 0; n--) {
        if (*(void**)(e - 0x30)) {                      // owner at elem+0x18
            void** vt = *(void***)(e - 0x30);
            ((void (__cdecl*)(void*, int))vt[0])(*(void**)(e - 0x30), 1);
        }
        record_array_clear(e - 0x20);                   // sub-object at elem+0x28
        if (*(void**)(e)) {
            operator delete(*(void**)(e));
            *(void**)(e) = 0;
        }
        *(void**)(e + 4) = 0;
        *(void**)(e + 8) = 0;
        ((tstring*)(e - 0x44))->~tstring();             // elem's name
        e -= 0x78;
    }
    if (flags & 1)
        operator delete(head);
    return head;
}

// ---------------------------------------------------------------- mesh block ----------------------------------
// a block of the map's mesh: name (+0x00), a second name (+0x10), variation count (+0x44), material
// variations (+0x58 array of 0x18-byte entries), the read pose (+0x1c8..), flags (+0x21c)
struct Block {
    int first;                                          // +0x00
    char pad0[0x1c];
    float* verts;                                       // +0x1c (0x18-byte vertices)
    char* verts_end;                                    // +0x20
    char* verts_cap;                                    // +0x24
    float terrain[4];                                   // +0x28
    float pad38;                                        // +0x38
    char* attaches;                                     // +0x3c (0x34-byte entries)
    float scale;                                        // +0x40
    int variation_count;                                // +0x44 (an int, read from the stream)
    char* pad48;                                        // +0x48
    char* pad4c;                                        // +0x4c
    char* variations;                                   // +0x58 (array of pointers)
    char* variations_end;                               // +0x5c
    char* variations_cap;                               // +0x60
    void* material;                                     // +0x64

    rgl::vector4 normal(const int* mesh);               // 0x4c8000
    void clear();                                       // 0x4c80c0
    void read(AccessCodeStream* stream);                // 0x4c8110
    void free_materials();                              // 0x4c8330
    void read_terrain(AccessCodeStream* stream);        // 0x4c83a0
};
extern void find_triangle(const int* mesh, int* a, int* b);      // 0x4c5f60
extern void block_read_base(Block* b, AccessCodeStream* s);      // 0x4607e0
extern void add_frame(AccessCodeStream* s, const char* name, int flags);   // 0x4c1e10

// @ 0x4c8000
rgl::vector4 Block::normal(const int* mesh)
{
    int a, b;
    find_triangle(mesh, &a, &b);
    float* v = (float*)((char*)verts + b * 0x18);
    float* w = (float*)((char*)verts + a * 0x18);
    float dx = w[0] - v[0];
    float dy = w[1] - v[1];
    float dz = w[2] - v[2];
    int mat = *(int*)(*(char**)((char*)this + 0x3c) + *mesh * 0x34 + 8) * 0x64;
    float* m = (float*)(mat + *(int*)((char*)this + 0x2c));
    rgl::vector4 res;
    res.x = dy * m[8] - dz * m[7];
    res.y = dz * m[6] - dx * m[8];
    res.z = dx * m[7] - dy * m[6];
    res.normalize();
    res.w = m[9];                                       // the caller's 4th lane lands here
    return res;
}

// @ 0x4c80c0
void Block::clear()
{
    tstring* var = (tstring*)*(int*)((char*)this + 0x44);
    int z = 0, k = 0xfffe7960;
    this->first = z;
    *(int*)((char*)this + 0x34) = -1;
    *(int*)((char*)this + 0x38) = z;
    *(int*)((char*)this + 0x3c) = -1;
    *(int*)((char*)this + 0x08) = k;
    *(int*)((char*)this + 0x0c) = -1;
    *(int*)((char*)this + 0x10) = k;
    *(int*)((char*)this + 0x14) = -1;
    *(int*)((char*)this + 0x18) = z;
    *(int*)((char*)this + 0x1c) = z;
    *(int*)((char*)this + 0x20) = 100;
    if (var)
        operator delete(var);
    *(int*)((char*)this + 0x44) = 0;
    *(int*)((char*)this + 0x48) = 0;
    *(int*)((char*)this + 0x4c) = 0;
}

// @ 0x4c8330
void Block::free_materials()
{
    for (unsigned i = 0;; i++) {
        char* b = *(char**)((char*)this + 0x58);
        if (!b)
            break;
        unsigned n = (unsigned)(*(char**)((char*)this + 0x5c) - b) >> 2;
        if (i >= n)
            break;
        char* e = *(char**)(*(char**)((char*)this + 0x58) + i * 4);
        *(int*)(e + 0x64) = 0;                          // the block no longer owns the material
        g_res_manager.release(e);
    }
    if (*(char**)((char*)this + 0x58))
        operator delete(*(char**)((char*)this + 0x58));
    *(int*)((char*)this + 0x58) = 0;
    *(int*)((char*)this + 0x5c) = 0;
    *(int*)((char*)this + 0x60) = 0;
    *(int*)((char*)this + 0x64) = 0;
}

// @ 0x4c83a0
void Block::read_terrain(AccessCodeStream* stream)
{
    tstring name;
    this->free_materials();
    stream->str((tstring*)((char*)this + 0x00));
    stream->i32((int*)((char*)this + 0x14));
    stream->str(&name);
    stream->i32((int*)((char*)this + 0x18));
    stream->i32((int*)((char*)this + 0x1c));
    stream->f32((float*)((char*)this + 0x28));
    float* f2 = (float*)((char*)this + 0x2c);
    stream->f32(f2);
    stream->f32((float*)((char*)this + 0x30));
    stream->f32((float*)((char*)this + 0x34));
    *(float*)((char*)this + 0x28) *= 0.001f;
    *f2 *= 0.001f;
    *(float*)((char*)this + 0x30) *= 0.001f;
    *(float*)((char*)this + 0x34) *= 0.001f;
    block_read_base(this, stream);
    char buf[0x200];
    _snprintf(buf, 0x200, "Tableau material %s", ((tstring*)((char*)this + 0x44))->str());
    tstring mat(buf);
    ((tstring*)((char*)this + 0x44))->operator=(mat);
    mat.~tstring();
    if (name.len > 1) {
        if (name.differs("none"))
            *(void**)((char*)this + 0x64) = g_res_manager.get_material(name);
    }
    name.~tstring();
}

// @ 0x4c8110
void Block::read(AccessCodeStream* stream)
{
    if (*(char**)((char*)this + 0x34))
        operator delete(*(char**)((char*)this + 0x34));
    *(int*)((char*)this + 0x34) = 0;
    *(int*)((char*)this + 0x38) = 0;
    *(int*)((char*)this + 0x3c) = 0;
    stream->str((tstring*)((char*)this + 0x00));
    stream->str((tstring*)((char*)this + 0x10));
    stream->str((tstring*)((char*)this + 0x20));
    ((tstring*)((char*)this + 0x00))->underscores_to_spaces();
    ((tstring*)((char*)this + 0x10))->underscores_to_spaces();
    stream->i32((int*)((char*)this + 0x44));
    if (*(int*)((char*)this + 0x44) > 0x10)
        mb_error("Error too many variation on mesh %s", ((tstring*)((char*)this + 0x00))->str());
    int n = *(int*)((char*)this + 0x44);
    for (int i = 0; i < n; i++) {
        char* v = (char*)this + 0x58 + i * 0x18;
        stream->str((tstring*)(v - 0x10));
        stream->f32((float*)v);
    }
    stream->f32((float*)((char*)this + 0x1c8));
    stream->f32((float*)((char*)this + 0x1d0));
    stream->i32((int*)((char*)this + 0x1d8));
    stream->f32((float*)((char*)this + 0x1e0));
    stream->f32((float*)((char*)this + 0x208));
    stream->i32((int*)((char*)this + 0x220));
    stream->i32((int*)((char*)this + 0x1fc));
    stream->i32((int*)((char*)this + 0x200));
    stream->i32((int*)((char*)this + 0x204));
    stream->i32((int*)((char*)this + 0x1ec));
    stream->i32((int*)((char*)this + 0x1e8));
    stream->i32((int*)((char*)this + 0x1f0));
    stream->i32((int*)((char*)this + 0x1f4));
    stream->i32((int*)((char*)this + 0x20c));
    stream->i32((int*)((char*)this + 0x210));
    stream->i32((int*)((char*)this + 0x214));
    stream->i32((int*)((char*)this + 0x218));
    if (*(int*)((char*)this + 0x20) > 1) {
        char buf[0x200];
        _snprintf(buf, 0x200, "Item %s", ((tstring*)((char*)this + 0x20))->str());
        add_frame(stream, buf, 0);
    }
    *(int*)((char*)this + 0x40) = 0x3f800000;
    *(int*)((char*)this + 0x21c) = 0x3f800000;
}
