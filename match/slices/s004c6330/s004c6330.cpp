// Slice s004c6330: the reference-object editor page (its name accessors, its message formatter, reset and the
// 0x24-byte target-record array it keeps). Names provisional; offsets, vtable slots and string ids from the
// disassembly.
// NOTE: the record-array code (0x4c6900 / 0x4c6950 / 0x4c6aa0) and the placement helper 0x4c6b30 are written
// against the disassembly only; their offsets have not been checked against a compiling object yet.
#include <windows.h>

#define MB_NOTHROW throw()

// ---- tstring: the engine's 16-byte string with a read cursor (layout from s00429890/tstring.h) ----------
struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4  (null while empty)
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    char equals(const tstring& o) const;        // 0x42bdd0
    void append(const tstring& o);              // 0x42c1e0
    tstring operator+(const tstring& o) const;  // 0x42c840
};
struct StrTable {
    tstring* find(const tstring* key) MB_NOTHROW;       // 0x4c6010 (null when absent)
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0 (a shared "ui_unknown..." entry on a miss)
};
extern StrTable g_strtable;                             // 0x5e2a18

// ---- engine helpers --------------------------------------------------------------------------------------
extern void free_mem(void* p);                                          // 0x535712 operator delete
extern void free_mem_arr(void* p);                                      // 0x53570d operator delete[]
extern char* alloc_bytes(int n);                                        // 0x535482 operator new[]
extern int sprintf_c(char* out, int size, const char* fmt, ...);        // 0x535717
extern void kf_arr_dtor(void* k);                                       // 0x4317b0 (an array of keyframe tracks)
extern void ref_item_ctor(void* item, int i, void* rec, int k);         // 0x4c23f0
extern void* find_frame(void* kf, const float* t, int* lo, int* hi, float* frac);   // 0x40f0d0
extern float flt_zero;                                                  // 0x549d70 (0.0f)
struct RefRecord;                                                       // below
extern void obj_scalar_dtor(RefRecord* p);                              // a virtual scalar-deleting destructor (arg 1)

// the string constants (ids from the disassembly); "{***}" is the "_door" placeholder
extern const char s__pl[];                      // 0x5923b0 "_pl"
extern const char s__text[];                    // 0x5923b4 "_text"
extern const char s__door[];                    // 0x595c9c "{***}"
extern const char s_invalid_item[];             // 0x595cac "INVALID ITEM"
extern const char s_item_name[];                // 0x595cbc "ui_item_itemname"
extern const char s_unknown[];                  // 0x595cd0 "ui_unknown_unknownname"
extern const char s_prop[];                     // 0x595ce8 "ui_prop_propname"
extern const char s_invalid_plant[];            // 0x595cfc "INVALID PLANT"
extern const char s_plant[];                    // 0x595d0c "ui_plant_plantname"
extern const char s_passage[];                  // 0x595d20 "ui_passage_menu_item_passagename"
extern const char s_entry_point[];              // 0x595d44 "ui_entry_point_entrypointname"

// ---- the item kinds / ground specs the page names things after ------------------------------------------
struct ItemKind {               // 0x238-byte record (array pointer at 0x5e29d0, count at 0x5e29d4)
    char pad0[0x20];
    tstring name;               // +0x20
    char pad30[0x238 - 0x30];
    tstring* display_name();    // 0x43a480 (the record's own name, or itself when it has none)
};
extern ItemKind* g_items;                       // 0x5e29d0
extern int g_item_count;                        // 0x5e29d4
struct GroundSpec {             // 0x20-byte record (array pointer at 0x5e28f8, count at 0x5e29e8)
    char pad0[0x4];
    char* name;                 // +0x4
    char pad8[0x20 - 0x8];
};
extern GroundSpec* g_ground_specs;              // 0x5e28f8
extern int g_ground_spec_count;                 // 0x5e29e8

// ---- a mesh's keyframe track (a 0x24-byte record's +0x14 points at an array of these) -------------------
struct keyframes {
    char pad0[0x8];
    int* times;                 // +0x8
    int count;                  // +0xc
};
struct mesh_node {              // 0x98-byte record with a keyframe track at +0x84 (cf. s0047b3b0/mesh.h)
    char pad0[0x84];
    keyframes kf;               // +0x84
};

// ---- the records the page holds -------------------------------------------------------------------------
struct RefItem {                // 0x24-byte entry of the record array
    tstring name;               // +0
    void* refs;                 // +0x10
    void* kf;                   // +0x14 (an array of keyframes, freed with the item)
    int f18, f1c, f20;
};
struct Pad98 {                  // a 0x98-byte record whose head is a string (see clear)
    tstring name;               // +0
    char pad10[0x98 - 0x10];
};

struct RefRecord {              // the source record: [0x4c] gives two strings, [0x54] two more
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48();
    virtual void v4c(void* out);
    virtual void v50();
    virtual void v54(void* out);
};

// the record class of the array at +0x13a00 (name at +0, a second string at +0x10, a third at +0x20, a
// record key at +0x54). Class A of the three record classes the caller walks.
struct RecA {
    tstring name;               // +0
    tstring text;               // +0x10
    tstring field;              // +0x20
    char pad30[0x54 - 0x30];
    tstring key;                // +0x54

    tstring text_by_kind();                     // 0x4c6330
    tstring src_text();                         // 0x4c63f0
    tstring src_id_name();                      // 0x4c6470
};

// the record class of the array at +0x13a48 (name at +0, a one-byte flag at +0x10 and a second string at
// +0x14). Class C: the flag and the string share the +0x10 word.
struct RecC {
    tstring name;               // +0
    char flag;                  // +0x10
    tstring field;              // +0x14

    tstring field_name();                       // 0x4c6860
};

// the record class of the grid the caller walks (a string at +0xc, a one-byte flag at +0x1c and a second
// string at +0x20). Class B.
struct RecB {
    char pad0[0xc];
    tstring field;              // +0xc
    char flag;                  // +0x1c
    tstring name;               // +0x20

    tstring name_by_state();                    // 0x4c6530
};

// ---- the editor page: the message formatter and the buffers it owns --------------------------------------
struct ItemPage {
    tstring name;               // +0
    tstring text;               // +0x10
    char pad20[0x188 - 0x20];
    int kind;                   // +0x188
    int index;                  // +0x18c
    tstring field;              // +0x190
    char* text_ptr;             // +0x1a0 (kind 1: the entry-point name)
    char* text_ptr2;            // +0x1a4 (kind 5: the passage name)
    char pad1a8[0x24c - 0x1a8];
    void* extra;                // +0x24c (free_mem'd)
    void* extra_end;            // +0x250
    RefItem* records;           // placeholder for the record-array helpers below
    int rec_count;

    void reset();                               // 0x4c65d0
    void format(char* out);                     // 0x4c6620
    void retire();                              // 0x4c6900
    RefRecord* fill_records(RefRecord* rec);    // 0x4c6950
    void clear();                               // 0x4c6aa0
    void place(int e);                          // 0x4c6b30
};

// @ 0x4c6330
tstring RecA::text_by_kind()
{
    tstring* r;
    {
        tstring suffix(s__pl);
        r = g_strtable.find(&(field + suffix));
    }
    if (r)
        return *r;
    {
        tstring s = text;
        {
            tstring door(s__door);
            s.append(door);
        }
        return s;
    }
}

// @ 0x4c63f0
tstring RecA::src_text()
{
    tstring* r = g_strtable.find(&key);
    if (r)
        return *r;
    tstring s = name;
    {
        tstring door(s__door);
        s.append(door);
    }
    return s;
}

// @ 0x4c6470
tstring RecA::src_id_name()
{
    tstring* r;
    {
        tstring suffix(s__text);
        r = g_strtable.find(&(key + suffix));
    }
    if (r)
        return *r;
    {
        tstring s = text;
        {
            tstring door(s__door);
            s.append(door);
        }
        return s;
    }
}

// @ 0x4c6530
tstring RecB::name_by_state()
{
    if (flag)
        return field;
    tstring* r = g_strtable.find(&name);
    if (r)
        return *r;
    tstring s = field;
    {
        tstring door(s__door);
        s.append(door);
    }
    return s;
}

// @ 0x4c65d0
void ItemPage::reset()
{
    if (*(void**)((char*)this + 0x54))
        obj_scalar_dtor(*(RefRecord**)((char*)this + 0x54));
    kf_arr_dtor((char*)this + 0x6c);
    if (*(void**)((char*)this + 0x44))
        free_mem(*(void**)((char*)this + 0x44));
    *(int*)((char*)this + 0x44) = 0;
    *(int*)((char*)this + 0x48) = 0;
    *(int*)((char*)this + 0x4c) = 0;
    name.~tstring();
}

// @ 0x4c6620
void ItemPage::format(char* out)
{
    if (kind == 1) {
        tstring key(s_entry_point);
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, text_ptr);
    } else if (kind == 5) {
        tstring key(s_passage);
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, text_ptr2);
    } else if (kind == 4) {
        if (index < 0 || index >= g_ground_spec_count) {
            sprintf_c(out, 0x80, s_invalid_plant);
            return;
        }
        tstring key(s_plant);
        GroundSpec& gs = g_ground_specs[index];
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, gs.name ? gs.name : (char*)&gs.name);
    } else if (kind == 2 || kind == 6) {
        if (index < 0 || index >= g_item_count) {
            sprintf_c(out, 0x80, s_invalid_item);
            return;
        }
        tstring key(s_item_name);
        tstring* v = g_items[index].display_name();
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, v->buf ? v->buf : (char*)&v->buf);
    } else if (kind == 0) {
        tstring key(s_prop);
        tstring* v = &field;
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, v->buf ? v->buf : (char*)&v->buf);
    } else {
        tstring key(s_unknown);
        tstring* v = &field;
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, v->buf ? v->buf : (char*)&v->buf);
    }
}
// @ 0x4c6860
tstring RecC::field_name()
{
    if (flag)
        return name;
    tstring* r = g_strtable.find(&field);
    if (r)
        return *r;
    tstring s = name;
    {
        tstring door(s__door);
        s.append(door);
    }
    return s;
}

// @ 0x4c6900
void ItemPage::retire()
{
    if (!records)
        return;
    int n = ((int*)records)[-1];
    RefItem* it = (RefItem*)((char*)records + n * 0x24);
    while (n-- >= 0) {
        it = (RefItem*)((char*)it - 0x24);
        kf_arr_dtor((void*)&it->kf);
        it->name.~tstring();
    }
    free_mem_arr((char*)records - 4);
    records = 0;
}

// @ 0x4c6950
RefRecord* ItemPage::fill_records(RefRecord* rec)
{
    tstring a, b;
    rec->v54(&a);
    rec->v54(&b);
    rec->v4c(&field);
    rec->v4c((void*)((char*)this + 0x1a0));
    retire();
    records = 0;
    int n = rec_count;
    RefItem* p = (RefItem*)alloc_bytes(n * 0x24 + 4);
    if (p) {
        p = (RefItem*)((char*)p + 4);
        ((int*)p)[-1] = n;
        RefItem* it = p;
        for (int i = 0; i < n; i++, it = (RefItem*)((char*)it + 0x24)) {
            tstring t;
            it->name = t;
            it->refs = 0;
            it->kf = 0;
            it->f18 = 0;
        }
    }
    records = p;
    for (int i = 0; i < n; i++)
        ref_item_ctor((char*)records + i * 0x24, i, rec, i);
    return rec;
}

// @ 0x4c6aa0
void ItemPage::clear()
{
    if (*(void**)((char*)this + 0x4)) {
        free_mem_arr(*(void**)((char*)this + 0x4));
        *(void**)((char*)this + 0x4) = 0;
    }
    Pad98* p = *(Pad98**)((char*)this + 0xc);
    if (p) {
        int n = ((int*)p)[-1];
        Pad98* it = (Pad98*)((char*)p + n * 0x98);
        for (int i = n; i >= 0; i--) {
            it = (Pad98*)((char*)it - 0x98);
            it->name.~tstring();
        }
        free_mem_arr((void*)((char*)p - 4));
        *(void**)((char*)this + 0xc) = 0;
    }
    if (extra)
        free_mem(extra);
    extra = 0;
    extra_end = 0;
}

// @ 0x4c6b30
void ItemPage::place(int e)
{
    void* hdr = *(void**)((char*)records + 0x38);
    char* frames = *(char**)((char*)hdr + 0x38);
    char* v74 = *(char**)frames;
    unsigned n = *(unsigned*)(v74 + 0x74);
    if (n && 0u < (unsigned)(*(int*)(v74 + 0x78) - (int)n) >> 5) {
        for (unsigned i = 0; i < (unsigned)(*(int*)(v74 + 0x78) - (int)n) >> 5; i++) {
            float* dst = (float*)((*(char**)((char*)records + 0x40) + e * 0x34 + 0x30) + i * 0x10);
            dst[0] = 0.0f;
            dst[1] = 0.0f;
            dst[2] = 0.0f;
        }
    }
}
