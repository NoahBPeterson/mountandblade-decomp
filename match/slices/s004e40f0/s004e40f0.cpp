// Slice s004e40f0: array helpers (std::vector<...>::resize) of the character-sheet exporter, the copy
// helpers of the same container, and the exporter's label routine (0x4e43e0).
// The resize helpers are the vector member cl 13.10 inlined:
//   if (size() < n) insert_n(end, n - size(), value);  else if (begin && n < size) erase(begin + n, end)
// with size() = begin ? (end - begin) / sizeof(T) : 0 (a signed division by a constant, as in the original).
// Element types come from the neighbouring slices:
//   0x68  Cls_map      (s004c7bf0: map block, materials freed with 0x4c8330)
//   0xb0  world record (s004d3720: destroyed with 0x4d3720, range insert 0x4d3e70)
//   0x18  a generic record (copy ctor 0x4dc030, range copy 0x4d5a20, in-place copy 0x4e4340)
//   0x20  tstring + a 0x10-byte list (copy ctor 0x4e42f0, copy 0x4e42b0, fill 0x4e43e0)
//   0x1f8 loading_config (s004dbf7e).  Names provisional.
#include <windows.h>
#include <new>
#include <vector>
#include "rgl.h"

extern "C" int _fltused = 0;

void* __cdecl operator new(unsigned n);                     // 0x53545e
extern "C" void __cdecl _free(void* p);                     // 0x535712

// ---- engine string (0x10 bytes, tstring.h of s0046ad50) -------------------------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    tstring& operator=(const tstring& o);               // 0x42bc50
    void assign(const char* s);                         // 0x42bcb0
};
typedef std::vector<tstring> list10;                    // 0x10 bytes: a small list of strings

// ---- records --------------------------------------------------------------------------------------------
// 0x68-byte map block (s004c7bf0)
struct Cls_map {                                        // 0x68 bytes
    void free_materials();                              // 0x4c8330
};
typedef std::vector<Cls_map> Cls_map_vec;
Cls_map_vec Cls_map_insert(Cls_map_vec* self, const Cls_map* at, const Cls_map* end);  // 0x4d9390
void Cls_map_fill(Cls_map_vec* self, Cls_map* at, unsigned n, Cls_map& v);             // 0x4db9d0

// 0xb0-byte world record (s004d3720)
struct WorldRec {                                       // 0xb0 bytes
    void destroy();                                     // 0x4d3720
};
typedef std::vector<WorldRec> WorldRec_vec;
void WorldRec_insert(WorldRec_vec* self, WorldRec* at, WorldRec* end);                 // 0x4d3e70
WorldRec* WorldRec_fill(WorldRec_vec* self, WorldRec* at, unsigned n, WorldRec& v);    // 0x4dbcc0

// 0x18-byte record
struct row18 {                                          // 0x18 bytes
    int f0, f4, f8, fc, f10, f14;
    row18(const row18& o);                              // 0x4dc030
    row18& operator=(const row18& o);                   // 0x4dc060
};
row18* row18_copy(row18* dst, row18* first, row18* last);           // 0x4d5a20
row18* row18_copy_inplace(row18* dst, row18* first, row18* last);   // 0x4e4340 (EH frame)

// 0x20-byte record: a name and a 0x10-byte list
struct row20 {
    tstring name;                                       // +0x00
    list10 list;                                        // +0x10
    row20(const row20& o);                              // 0x4e42f0
};
tstring* tstring_assign(tstring* dst, const tstring* src);         // 0x42bc50
list10* list10_assign(list10* dst, const list10* src);              // 0x4dc1c0
row20* row20_copy(row20* dst, row20* first, row20* last);           // 0x4e42b0
row20* row20_fill(row20* dst, unsigned n, const row20& v);          // 0x4e43e0 (ret 0xc, EH frame)

// 0x1f8-byte loading config (s004dbf7e)
struct loading_config {
    loading_config(const loading_config& o);            // 0x4e4320
};

// ---- externals used by the label routine ---------------------------------------------------------------
struct rgl_mesh;
struct ExporterPanel;
void set_dynamic_mesh(void* self, void* m, bool own);                       // 0x42cdb0 (ECX = self, pushed arguments)
struct ResMgr { void* get_material(tstring* name); };                        // 0x418a70 (ECX = this, ret 4)
extern ResMgr g_resmgr;                                                     // 0x5d6fa0
struct Font {
    void layout_text(ExporterPanel* self, float* out, const tstring* text, const float* pos_in,
                     const float* scale_in, rgl_mesh* m, const int* arg, const float* xmin,
                     const float* xmax, unsigned flags);                    // 0x4520d0 (ECX = font, ret 0x24)
};
extern Font* g_party_recs;                                                  // 0x5e2934 (Font object)
void mesh_bounds(ExporterPanel* self, rgl_mesh* m, float* out, float* minmax);   // 0x438d80 (ECX = mesh)
void loading_config_assign(ExporterPanel* self, float* a, float* b);         // 0x4dc350 (ECX = self)
void panel_refresh(ExporterPanel* self, const int* v);                      // 0x4c5190 (ECX = self)
void panel_update(ExporterPanel* self);                                     // 0x4da900 (ECX = self)

struct rgl_mesh {
    char pad_mesh[0x260];
};
rgl_mesh* init_mesh(rgl_mesh* m);                                   // 0x412a90

// TiXml handles: a handle wraps a node pointer; FirstChild(name) / FirstChild() are 4 bytes wide and return
// their handle through the hidden slot (ret 8 / ret 4). ToElement() is the node's virtual at vtable +0x18.
struct TiXmlElement {
    int vtable;
    int type;                                                       // +4
    char pad[0x14];
    int value;                                                      // +0x1c (a TiXmlString: 4 bytes)
    TiXmlElement* NextSiblingElement() const;                       // 0x42dc80
};
struct TiXmlNode {
    TiXmlElement* ToElement() const;                                // virtual, vtable +0x18
};
struct TiXmlHandle {                                                // a node pointer
    TiXmlNode* node;
    TiXmlHandle FirstChild(const char* name) const;                 // 0x42dff0 (ret 8)
    TiXmlHandle FirstChild() const;                                 // 0x42dfc0 (ret 4)
};
const char* attr_name(const TiXmlNode* e, const char* n);           // 0x42f0b0

// a row of the label's bone list: tstring + the 0x40 bytes its ctor 0x4cd7a0 inits (element 0x50 bytes)
struct pb_row {
    tstring text;                       // +0x00
    char pad10[0x34];
    int f44, f48, f4c;                  // +0x44, +0x48, +0x4c
    void load(const TiXmlElement* e);   // 0x4d3330
};
pb_row* new_row();                                                  // 0x4cd7a0 (ECX = storage)
struct pb_rows {                        // 0x50-byte elements
    pb_row* begin;
    pb_row* cur;
    pb_row* end;
    void push(const pb_row* v);         // 0x4e4060
};

// The exporter panel: a tstring name at +0, a list of 0x50-byte rows at +0x10 (begin/cur/end at
// +0x10/+0x14/+0x18, the bone records at +0x1c), the text mesh at +0x568 (0x260 bytes) and 0x50 per-label
// records: the name at +0xcd0+0x10*i, the flag at +0xcd0+i, the height at +0xd20+4*i and an int at
// +0xe60+4*i. The count is at +0xfa0, the widest line at +0xfa4, the running offset at +0xfa8, the pen at
// +0x1700.
struct ExporterPanel {
    tstring name;                       // +0x000
    pb_rows rows;                       // +0x010
    pb_row* bones;                      // +0x01c
    char pad20[0x568 - 0x20];
    rgl_mesh* f568;                     // +0x568 dynamic text mesh
    char pad56c[0xcd0 - 0x56c];
    tstring labels[0x50];               // +0xcd0 per-label names (i's name at +0xcd0+0x10*i)
    char fcd0[0x50];                    // +0xd10 per-label flags
    float fd20[0x50];                   // +0xd20 per-label heights
    int fe60[0x50];                     // +0xe60 per-label ints
    int fa00;                           // +0xfa0 label count (max 0x50)
    float fa04;                         // +0xfa4 widest line so far
    float fa08;                         // +0xfa8 running offset
    char pf0c[0x16e8 - 0xfb0];
    int f16e8;                          // +0x16e8
    int f16ec;                          // +0x16ec
    char p16f0[0x1700 - 0x16f0];
    float f1700;                        // +0x1700 pen
    void add_label(const tstring& label, int f, float height, bool wrap);   // 0x4e43e0
    void load(const TiXmlHandle& handle);                                   // 0x4e4690
};

// ---- code -----------------------------------------------------------------------------------------------
// @ 0x4e40f0   vector<Cls_map>::resize(n)
void resize_cls_maps(Cls_map_vec* self, unsigned n)
{
    Cls_map v;
    unsigned size = 0;
    if (!self->empty())
        size = (unsigned)((self->end() - self->begin()));
    if (size < n)
        Cls_map_fill(self, &*self->end(), n - size, v);
    else if (!self->empty() && n < size)
        Cls_map_insert(self, &*self->begin() + n, &*self->end());
}

// @ 0x4e41e0   vector<WorldRec>::resize(n)
void resize_world_recs(WorldRec_vec* self, unsigned n)
{
    WorldRec v;
    unsigned size = 0;
    if (!self->empty())
        size = (unsigned)((self->end() - self->begin()));
    if (size < n)
        WorldRec_fill(self, &*self->end(), n - size, v);
    else if (!self->empty() && n < size)
        WorldRec_insert(self, &*self->begin() + n, &*self->end());
}

// @ 0x4e42b0   row20* row20_copy(dst, first, last): copy [first,last) back to back, return dst
row20* row20_copy(row20* dst, row20* first, row20* last)
{
    for (; first != last; ++first)
        *dst++ = *first;
    return dst;
}

// @ 0x4e42f0   row20 copy ctor: the name then the list
row20::row20(const row20& o) : name(o.name), list(o.list)
{
}

// @ 0x4e4320   row18 copy ctor reachable through std::_Construct
void row18_copy_ctor(row18* dst, const row18* src)
{
    new (dst) row18(*src);
}

// @ 0x4e4340   row18* row18_copy_inplace(dst, first, last): copy [first,last) back to back (with an EH frame)
row18* row18_copy_inplace(row18* dst, row18* first, row18* last)
{
    for (; first != last; ++first)
        *dst++ = *first;
    return dst;
}

// @ 0x4e43b0   the compiler-emitted unwinder of row20_fill (destroys the rows built so far, rethrows)

// @ 0x4e43e0   ExporterPanel::add_label(name, f, height, wrap)
void ExporterPanel::add_label(const tstring& label, int f, float height, bool wrap)
{
    if (f568 == 0) {
        rgl_mesh* m = (rgl_mesh*)operator new(0x260);
        if (m != 0)
            m = init_mesh(m);
        set_dynamic_mesh(this, m, true);
    }
    if (fa00 >= 0x50)
        return;
    ExporterPanel* self = this;
    int i = self->fa00;
    self->labels[i] = label;
    self->fcd0[i] = wrap;
    self->fd20[i] = height;
    self->fe60[i] = f;
    self->fa00 = i + 1;
    unsigned flags;
    float x;
    if (wrap) {
        flags = 0x10;
        x = 0.0f;
    } else {
        flags = 0;
        x = self->fa08;
    }
    tstring font("ui_font");
    void* mat = g_resmgr.get_material(&font);
    *(void**)((char*)self->f568 + 0x2c) = mat;
    float out[2];
    float minmax[2];
    float sz[2];
    float at[2];
    float xmin = x + 1.0f;
    float xmax = x - 1.0f;
    int arg = 0;
    float pos[4];
    pos[0] = x;
    pos[1] = height;
    pos[2] = height;
    pos[3] = self->f1700;
    float scale[2];
    scale[0] = height;
    scale[1] = height;
    g_party_recs->layout_text(self, out, &label, pos, scale, self->f568, &arg, &xmin, &xmax, flags);
    self->f1700 = self->f1700 - out[1];
    mesh_bounds((ExporterPanel*)self->f568, self->f568, out, minmax);
    float w = out[0] - pos[0];
    float h = out[1] - pos[1];
    sz[0] = w + 0.02f;
    sz[1] = h + 0.02f;
    at[0] = pos[0] - 0.01f;
    at[1] = pos[1] - 0.01f;
    loading_config_assign((ExporterPanel*)self->f568, at, sz);
    int v[2];
    v[0] = self->f16e8;
    v[1] = self->f16ec;
    panel_refresh(self, v);
    if (!(self->fa04 >= w)) {
        if (wrap) {
            self->fa04 = w;
        } else {
            float d = self->fa04 - w;
            float n = self->fa08 + d;
            self->fa08 = n;
            if (!(n >= self->fa04 * 0.5f)) {
                n = n - d * 0.525f;
                self->fa08 = n;
                self->fa04 = n * 2.1f;
            } else {
                self->fa04 = w;
            }
        }
    }
    panel_update(self);
}

// @ 0x4e4690   ExporterPanel::load(handle): take "name", then every <Bones> child (one bone each)
void ExporterPanel::load(const TiXmlHandle& h)
{
    const char* n = attr_name(h.node, "name");
    if (n != 0)
        this->name.assign(n);
    TiXmlHandle hb = h.FirstChild("Bones");
    TiXmlNode* node = hb.FirstChild().node;
    TiXmlElement* e;
    if (node != 0 && node->ToElement() != 0 && (e = node->ToElement()) != 0) {
        for (; e != 0; e = e->NextSiblingElement()) {
            pb_row* r = new_row();
            rows.push(r);
            r->text.~tstring();
            bones[0].load(e);
        }
    }
}
