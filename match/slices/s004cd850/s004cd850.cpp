// Slice s004cd850: the skeleton/language module (continued by s004cdfd0 / s004ce960).
// 0x4cd850 copies the bones of the registered skeletons into the loaded skeleton resources, 0x4cda20
// constructs the 0xdf3-bucket pair, 0x4cda90 loads language.txt and every languages\<lang>\*.csv,
// 0x4cdf20 constructs one CSV row.
// ALL NAMES PROVISIONAL. Layouts and field offsets reproduced from the disassembly of this slice only.
//
// Match status: none of the four functions is byte-exact. What the disassembly pins down:
//  - all four are UNTAGGED members (raw `this` in ecx; 0x4cda90 uses `ret 4`).
//  - 0x4cda90 and 0x4cdf20 keep `this` in ebp via `mov dword ptr [esp + 0x2c], ebp`, i.e. they are
//    compiled with /Gh (inside a __try). cl 13.10 only emits /Gh prologues for functions that must be
//    inlined (`__forceinline`), which is not reproducible from ordinary C++ source; every other part of
//    0x4cda90's body (the 5000-byte buffer, the tstring line, the substr/assign chains, the bucket
//    inserts) does reproduce, so this row is listed as nonmatching.
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- tstring (16 bytes; s004608d0) ----------------------------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                                  // 0x42bbd0
    tstring(const char* s);                                     // 0x42bbe0
    ~tstring();                                                 // 0x42bc40
    void operator=(const tstring& o);                           // 0x42bc50
    void assign(const char* s);                                 // 0x42bcb0
    char equals(const char* s) const;                           // 0x42bd10
    tstring substr(int start, int end) const;                   // 0x42c4a0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
typedef char tstring_size_check[(sizeof(tstring) - 0x10) * 0 + 1];

// ---- skeleton records -----------------------------------------------------------------------------------
struct Bone {                                                   // 0x50 bytes: bone of a registered skeleton
    tstring name;                                               // +0x00
    int idx;                                                    // +0x10
    int parent;                                                 // +0x14
    int f18;                                                    // +0x18
    int f1c;                                                    // +0x1c
    vector4 v20;                                                // +0x20
    vector4 v30;                                                // +0x30
    float m40[4];                                               // +0x40
};
typedef char bone_size_check[(sizeof(Bone) - 0x50) * 0 + 1];
struct Skeleton {                                               // 0x20 bytes: one registered skeleton
    tstring model;                                              // +0x00
    char pad[0x14 - 0x10];
    Bone* begin;                                                // +0x14
    Bone* end;                                                  // +0x18
    char pad2[0x20 - 0x1c];
};
typedef char skel_size_check[(sizeof(Skeleton) - 0x20) * 0 + 1];
struct SkelRes {                                                // the object 0x418cf0 returns
    char pad[0x18];
    int count;                                                  // +0x18
    char* bones;                                                // +0x1c: 0xdc-byte bones
};
struct SkelBone {                                               // the resource bone, seen from the destination
    char pad[0x8c];
    tstring name;                                               // +0x8c
    int f9c;                                                    // +0x9c (source +0x10)
    int fa0;                                                    // +0xa0 (source +0x14)
    int fa4;                                                    // +0xa4 (source +0x18)
    int fa8;                                                    // +0xa8 (source +0x1c)
    vector4 vac;                                                // +0xac
    vector4 vbc;                                                // +0xbc
    vector4 vcc;                                                // +0xcc (source +0x40)
};
typedef char skel_bone_size_check[(sizeof(SkelBone) - 0xdc) * 0 + 1];
struct resmgr {                                                 // global at 0x5d6fa0
    SkelRes* get_object70(const tstring& name);                 // 0x418cf0 (ret 4)
};
extern resmgr g_resmgr;                                         // 0x5d6fa0

// ---- log stream (s004b0940) ------------------------------------------------------------------------------
struct stream_base {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual stream_base& w_cstr(const char* s);                 // 10 (+0x28)
    virtual stream_base& w_tstr(const tstring* s);              // 11 (+0x2c)
};
extern stream_base g_log;                                       // 0x5a9f60

// ---- translation tables ---------------------------------------------------------------------------------
// the 0x18-byte alternative-text record: its implicit ctor only builds the leading tstring
struct alt_rec {
    tstring s;                                                  // +0x00
    int a;                                                      // +0x10
    int b;                                                      // +0x14
};
// the tstring vector a csv row carries: 0xc bytes (three raw pointers, no leading pad -- the original's
// ctor writes exactly these three dwords and its clear() frees the buffer and re-zeroes them)
struct str_vec {
    tstring* first;                                             // +0x00
    tstring* last;                                              // +0x04
    tstring* end;                                               // +0x08
    str_vec() : first(0), last(0), end(0) { }
    void clear() { if (first) operator delete(first); first = 0; last = 0; end = 0; }
};
struct csv_row {                                                // 0x238 bytes (0x4cdf20)
    tstring key;                                                // +0x00
    tstring value;                                              // +0x10
    tstring f20;                                                // +0x20
    char pad30[0x4];
    str_vec v34;                                                // +0x34
    float f40;                                                  // +0x40 (1.0f)
    int f44;                                                    // +0x44 (0)
    alt_rec alts[16];                                           // +0x48
    int f1c8;                                                   // +0x1c8 (0)
    char pad1cc[0x1d8 - 0x1cc];
    int f1d8;                                                   // +0x1d8 (10)
    char pad1dc[0x4];
    int f1e0;                                                   // +0x1e0 (0)
    int f1e4;                                                   // +0x1e4 (0)
    char pad1e8[0x21c - 0x1e8];
    float f21c;                                                 // +0x21c (1.0f)
    int f220;                                                   // +0x220 (100)
    float f224;                                                 // +0x224 (1.0f)
    str_vec v228;                                               // +0x228
    char pad234[0x4];
    csv_row();                                                  // 0x4cdf20
};
typedef char csv_row_size_check[(sizeof(csv_row) - 0x238) * 0 + 1];

// One language hash bucket. The bucket is a 16-byte record whose FIRST dword the original never touches:
// every access (0x4c6010, the insert paths, the zero loops of 0x4cda20) addresses a std::vector<tstring>
// at +4..+0xf, so the leading dword is kept as padding and the vector is described by hand.
struct lang_bucket {
    int pad;                                                    // +0x00
    tstring* myfirst;                                           // +0x04
    tstring* mylast;                                            // +0x08
    tstring* myend;                                             // +0x0c
    unsigned size() const { return myfirst ? (unsigned)(mylast - myfirst) : 0u; }
};
typedef char lang_bucket_size_check[(sizeof(lang_bucket) - 0x10) * 0 + 1];

// The two hash tables. 0x4cda20 emits the 0x42bbd0 calls for the two trailing strings *after* the bucket
// zero loops, but a ctor's member construction always precedes its body -- so the tables live in a base
// whose inlined ctor clears them first, and the strings stay members of the derived class.
struct LangTablesBase {
    lang_bucket keys[0xdf3];                                    // +0x0000
    lang_bucket vals[0xdf3];                                    // +0xdf30
    __forceinline LangTablesBase()
    {
        int* k = (int*)keys + 2;
        int n = 0xdf3;
        do {
            k[-1] = 0; k[0] = 0; k[1] = 0;
            k += 4;
            n--;
        } while (n != 0);
        int* v = (int*)vals + 2;
        n = 0xdf3;
        do {
            v[-1] = 0; v[0] = 0; v[1] = 0;
            v += 4;
            n--;
        } while (n != 0);
    }
};

struct LangTables : LangTablesBase {
    tstring notfound;                                           // +0x1be60
    tstring language;                                           // +0x1be70
    LangTables();                                               // 0x4cda20
    tstring* lookup(const tstring& key);                        // 0x4c6010
    void load(const char* root);                                // 0x4cda90
};
typedef char lang_check1[(sizeof(LangTablesBase) - 0xdf30) * 0 + 1];
typedef char lang_check2[(offsetof(LangTables, notfound) - 0x1be60) * 0 + 1];
typedef char lang_check3[(offsetof(LangTables, language) - 0x1be70) * 0 + 1];
extern LangTables g_lang;                                       // 0x5a8e68 (provisional symbol)
extern std::vector<Skeleton*> g_skeletons;                      // 0x5d6fa0 + 0x70 (provisional)
void copy16(void* dst, const void* src);                        // 0x4ccd80 (tstring::operator=)
void vec_insert_n(lang_bucket* vec, tstring* pos, unsigned n, const tstring& v);   // 0x438f00
void vec_insert_n2(lang_bucket* vec, tstring* pos, unsigned n, const tstring& v);  // 0x43e310

// @ 0x4cd850
void fix_skeletons(std::vector<Skeleton*>* list)
{
    for (unsigned i = 0; i < (unsigned)(list->_Mylast - list->_Myfirst) && i < list->_Mylast - list->_Myfirst; i++) {
        Skeleton* sk = list->_Myfirst[i];
        SkelRes* res = g_resmgr.get_object70(sk->model);
        if (res == 0) {
            g_log.w_cstr("Fixing skeletons: Unable to find skeleton model:").w_tstr(&sk->model);
            continue;
        }
        int n = (int)((sk->end - sk->begin) / 0x50);
        int j = 0;
        if (n) {
            while (j < n && j < res->count) {
                SkelBone* d = (SkelBone*)(res->bones + j * 0xdc);
                Bone* b = sk->begin + j;
                d->name = b->name;
                copy16(&d->vcc, b->m40);
                d->fa0 = b->parent;
                d->fa4 = b->f18;
                d->f9c = b->idx;
                d->fa8 = b->f1c;
                d->vac = b->v20;
                d->vbc = b->v30;
                j++;
            }
        }
    }
}

// @ 0x4cda20
LangTables::LangTables()
{
    notfound.assign("STR NOT FOUND");
}

// @ 0x4cda90
void LangTables::load(const char* root)
{
    FILE* f = fopen("language.txt", "r");
    tstring line;
    char buf[5000];
    if (fgets(buf, 5000, f)) {
        line.assign(buf);
        if (line.buf[line.len - 1] == '\n')
            language = line.substr(0, line.len - 1);
        else
            language = line;
    } else {
        language.assign("en");
    }
    fclose(f);

    char pattern[0x2b8];
    _finddata_t* fd = (_finddata_t*)&pattern[0x80];
    sprintf(pattern, "%slanguages\\%s\\*", root, language.c_str());
    long h = _findfirst(pattern, fd);
    if (h == -1) {
        _findclose(h);
        return;
    }
    do {
        if ((fd->attrib & 0x10) != 0)
            continue;
        char path[0x100];
        sprintf(path, "%slanguages\\%s\\%s", root, language.c_str(), fd->name);
        FILE* g = fopen(path, "r");
        tstring key;
        tstring value;
        tstring text;
        bool first = true;
        while (fgets(buf, 5000, g)) {
            if (first)
                first = false;
            text.assign(buf);
            if (text.len > 2 && text.buf[0] == '\xef' && text.buf[1] == '\xbb' && text.buf[2] == '\xbf')
                text = text.substr(3, text.len);
            int i = 0;
            int len = text.len;
            while (i < len) {
                if (text.buf[i] == '|') {
                    int a = 0;
                    int stop = i;
                    int kstart = i + 1;
                    int end = len - 1;
                    if (text.buf[0] == '"') a = 1;
                    if (text.buf[i - 1] == '"') stop = i - 1;
                    if (text.buf[kstart] == '"') kstart++;
                    if (text.buf[end] == '"') end--;
                    key = text.substr(a, stop);
                    value = text.substr(kstart, end);
                    unsigned hash = 0;
                    for (int c = 0; c < key.len; c++)
                        hash = (hash * 0x3b + (unsigned)(signed char)key.buf[c]) % 0xdf3;
                    tstring* found = lookup(key);
                    if (found != 0) {
                        *found = value;
                    } else {
                        lang_bucket* kb = &keys[hash];
                        if (kb->size() < (unsigned)(kb->myend - kb->myfirst))
                            vec_insert_n(kb, kb->mylast, 1, key);
                        else
                            vec_insert_n2(kb, kb->mylast, 1, key);
                        kb->mylast++;
                        lang_bucket* vb = &vals[hash];
                        if (vb->size() < (unsigned)(vb->myend - vb->myfirst))
                            vec_insert_n(vb, vb->mylast, 1, value);
                        else
                            vec_insert_n2(vb, vb->mylast, 1, value);
                        vb->mylast++;
                    }
                    len = text.len;
                    i = -1;
                }
                i++;
            }
        }
        fclose(g);
    } while (_findnext(h, fd) == 0);
    _findclose(h);
}

// @ 0x4cdf20
csv_row::csv_row()
{
    // the scalar fields are assigned in the body, not in an init list: the original emits their constant
    // stores only after the alts loop (i.e. after every implicit member construction) and shares the 1.0f
    // constant in eax across +0x40/+0x21c/+0x224, which only happens when the three stores stay adjacent.
    f40 = 1.0f;
    f1c8 = 0;
    f1d8 = 10;
    f1e0 = 0;
    f1e4 = 0;
    f44 = 0;
    f21c = 1.0f;
    f224 = 1.0f;
    f220 = 100;
    v34.clear();
}
