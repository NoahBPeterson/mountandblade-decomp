// Slice s0051e2b0 (batch ds1): the game-state loaders that mirror s004e8f10's save side -- factions,
// party templates, item kinds, troops and tracks -- together with the record readers they dispatch to,
// the track-vector resize helper and the sprite-table reader/rebuilder at the end of the range.
//
//   this = the Game object for the loaders (the layout recovered in s004e8f10 / s0051d920); the stream
//   is the save-file reader: slot 0x38 reads one int, 0x3c/0x40/0x44 read 8-byte ints, 0x48 a count,
//   0x4c raw bytes and 0x54 a nested structure.  Every reader returns a reference to the stream, which
//   is why the chained reads below reuse EAX as the next `this`.
//
// The sprite table (0x51e840 / 0x51ea10) reuses s0049e780's pool layout: count at +0, the free index
// vector at +4, the page array at +0x18 and 0x98-byte elements inside the pages.
#pragma warning(disable:4530)
#include <new>
#include "rgl.h"

// ---- the binary reader (mirror of s004e8f10's OutStream) -----------------------------------------
struct LoadStream {
    virtual void s00() = 0;                              // +0x00
    virtual void s01() = 0;                              // +0x04
    virtual void s02() = 0;                              // +0x08
    virtual void s03() = 0;                              // +0x0c
    virtual void s04() = 0;                              // +0x10
    virtual void s05() = 0;                              // +0x14
    virtual void s06() = 0;                              // +0x18
    virtual void s07() = 0;                              // +0x1c
    virtual void s08() = 0;                              // +0x20
    virtual void s09() = 0;                              // +0x24
    virtual void s10() = 0;                              // +0x28
    virtual void s11() = 0;                              // +0x2c
    virtual void s12() = 0;                              // +0x30
    virtual void s13() = 0;                              // +0x34
    virtual LoadStream* r_int(int* v) = 0;               // +0x38  one int
    virtual void r_q1(void* v) = 0;                      // +0x3c  8-byte int (the "int4" counterpart)
    virtual void r_q2(void* v) = 0;                      // +0x40  8-byte int
    virtual LoadStream* r_q3(void* v) = 0;               // +0x44  8-byte int
    virtual void r_cnt(int* v) = 0;                      // +0x48  int count
    virtual LoadStream* r_raw(void* v) = 0;              // +0x4c  raw bytes
    virtual void s14() = 0;                              // +0x50
    virtual void r_obj(void* v) = 0;                     // +0x54  nested structure
};

// ---- engine services that live in other slices ---------------------------------------------------
void __cdecl mb_error(const char* fmt, ...);                        // 0x42cc80
void __cdecl read_soa(LoadStream* s, char* base);                   // 0x51e1f0  (s0051d920)

extern char* g_party_templates;                                     // 0x5e29f8  0xb0-byte records
extern int   g_n_party_templates;                                   // 0x5e29fc
extern char* g_items;                                               // 0x5e29d0  0x238-byte records
extern int   g_n_items;                                             // 0x5e29d4
extern char  g_troop_extra[];                                       // 0x8ee3138  42 x 0x44 up to 0x8ee3c60

struct Logger {                                                     // 0x5a9f60
    virtual void l00();                                  // +0x00
    virtual void l01();
    virtual void l02();
    virtual void l03();
    virtual void l04();
    virtual void l05();
    virtual void l06();
    virtual void l07();
    virtual void l08();
    virtual void l09();
    virtual void log(const char* msg);                   // +0x28
};
extern Logger g_log;                                                // 0x5a9f60

// ---- containers (leading pad dword, then _Myfirst/_Mylast/_Myend) --------------------------------
struct Vec8 {                                           // 8-byte element vector (s004e8f10's List8)
    int    pad0;                                        // +0x0
    void*  _Myfirst;                                    // +0x4
    void*  _Mylast;                                     // +0x8
    void*  _Myend;                                      // +0xc
    void  read(LoadStream* s);                          // 0x51e0b0  (s0051d920)
};

struct Rec5c {                                          // stride 0x5c: the game tracks
    char data[0x5c];
    __declspec(noinline) void read(LoadStream* s);      // 0x51e750
};

struct Vec5c {                                          // vector of Rec5c
    int    pad0;                                        // +0x0
    Rec5c* _Myfirst;                                    // +0x4
    Rec5c* _Mylast;                                     // +0x8
    Rec5c* _Myend;                                      // +0xc
    int size() const { if (_Myfirst == 0) return 0; return (int)(_Mylast - _Myfirst); }
    __declspec(noinline) void resize(unsigned n, Rec5c val);     // 0x51e7a0
    void insert_n(Rec5c* where, unsigned count, const Rec5c* val);      // 0x4b54a0
    void shift_over(Rec5c** out, Rec5c* first, Rec5c* last);            // 0x518990
};

struct IntVec {                                         // vector<int>
    int  pad0;                                          // +0x0
    int* _Myfirst;                                      // +0x4
    int* _Mylast;                                       // +0x8
    int* _Myend;                                        // +0xc
    void resize(unsigned n, int v);                     // 0x43f310
    void insert_n(int* where, unsigned count, const int* val);          // 0x43e7b0
    void push_back(int v)
    {
        unsigned size;
        if (_Myfirst == 0)
            size = 0;
        else
            size = (unsigned)(_Mylast - _Myfirst);
        if (_Myfirst && size < (unsigned)(_Myend - _Myfirst)) {
            int* last = _Mylast;
            *last = v;
            _Mylast = last + 1;
        } else {
            insert_n(_Mylast, 1, &v);
        }
    }
};

// ---- record types --------------------------------------------------------------------------------
struct Rec300 {                                         // stride 0x300: factions (Game::factions)
    char data[0x300];
    __declspec(noinline) void read(LoadStream* s, int n);      // 0x51e320
};

struct Rec0B0 {                                         // stride 0xb0: party templates (0x5e29f8)
    char data[0xb0];
    __declspec(noinline) void read(LoadStream* s);      // 0x51e410
};

struct Rec508 {                                         // stride 0x508: troop/character records
    char data[0x508];
    __declspec(noinline) void read(LoadStream* s);      // 0x51e540
};

struct Rec60 {                                          // 0x300 bytes: 0x60 pairs (Rec508 +0xb4)
    char data[0x300];
    __declspec(noinline) void read(LoadStream* s);      // 0x51e690
};

struct Rec98 {                                          // stride 0x98: the sprite-table elements
    char data[0x98];
    __declspec(noinline) void read(LoadStream* s, int arg);    // 0x51e970
};

struct Table {                                          // sprite/object table (s0049e780's pool)
    int     count;                                      // +0x0
    IntVec  free_ids;                                   // +0x4   (pad +4, _Myfirst +8 .. _Myend +0x10)
    char    pad14[4];                                   // +0x14  (the page vector's pad dword)
    char**  pages;                                      // +0x18
    int     alloc_id();                                 // 0x493700
    __declspec(noinline) void clear();                  // 0x51ea10
    void    read(LoadStream* s, int arg);               // 0x51e840
};

struct Scene {                                          // the scene embedded in Game at +0xf8
    char    pad0[0x138d0];
    char**  pages;                                      // +0x138d0
    void    read(LoadStream* s, int flag);              // 0x51eb90  (s0051eb90)
    int     party_index(int id);                        // 0x48ad00
};

struct Game {
    char  pad0[0x78];
    int   f78;                                          // +0x78  party id used by the scene loader
    char  pad7c[0x13a00 - 0x7c];
    char* records;                                      // +0x13a00  0x508-byte records
    int   nrecords;                                     // +0x13a04
    char  pad13a08[0x13a48 - 0x13a08];
    char* factions;                                     // +0x13a48  0x300-byte records
    int   nfactions;                                    // +0x13a4c
    char  pad13a50[0x13a60 - 0x13a50];
    Vec5c tracks;                                       // +0x13a60
    void  load_factions(LoadStream* s);                 // 0x51e2b0
    void  load_party_templates(LoadStream* s);          // 0x51e3a0
    void  load_items(LoadStream* s);                    // 0x51e450
    void  load_troops(LoadStream* s);                   // 0x51e4c0
    void  load_tracks(LoadStream* s);                   // 0x51e6c0
    void  load_scene_parties(LoadStream* s);            // 0x51eaa0
};

// -------------------------------------------------------------------------------------------------
// @ 0x51e2b0    void Game::load_factions(LoadStream* s)                       (ret 4)
void Game::load_factions(LoadStream* s)
{
    int n;
    s->r_raw(&n);
    if (n > this->nfactions)
        mb_error("Load error: Number of factions in file: %d, exceeds %d", n, this->nfactions);
    for (int i = 0; i < n; i++)
        ((Rec300*)(this->factions + i * 0x300))->read(s, n);
}

// @ 0x51e320    void Rec300::read(LoadStream* s, int n)                       (ret 8)
void Rec300::read(LoadStream* s, int n)
{
    ((Vec8*)((char*)this + 0x24c))->read(s);
    for (int i = 0; i < n; i++)
        s->r_int((int*)((char*)this + 0x2c) + i);
    s->r_obj(this);
    s->r_q1((char*)this + 0x10);
    s->r_q2((char*)this + 0x28);
    s->r_raw((char*)this + 0x230);
    read_soa(s, (char*)this + 0x25c);
}

// @ 0x51e3a0    void Game::load_party_templates(LoadStream* s)                (ret 4, `this` unused)
void Game::load_party_templates(LoadStream* s)
{
    int n;
    s->r_raw(&n);
    if (n > g_n_party_templates)
        mb_error("Load error: Number of party_templates in file: %d, exceeds %d", n, g_n_party_templates);
    for (int i = 0; i < n; i++)
        ((Rec0B0*)(g_party_templates + i * 0xb0))->read(s);
}

// @ 0x51e410    void Rec0B0::read(LoadStream* s)                              (ret 4)
void Rec0B0::read(LoadStream* s)
{
    s->r_raw((char*)this + 0x34);
    s->r_raw((char*)this + 0x38);
    s->r_raw((char*)this + 0x3c);
    ((Vec8*)((char*)this + 0xa0))->read(s);
}

// @ 0x51e450    void Game::load_items(LoadStream* s)                          (ret 4)
void Game::load_items(LoadStream* s)
{
    int n;
    s->r_raw(&n);
    if (n < 0 || n >= g_n_items + 1)
        mb_error("Load error: Invalid number of item kinds: %d", n);
    for (int i = 0; i < n; i++)
        ((Vec8*)(g_items + i * 0x238 + 0x30))->read(s);
}

// @ 0x51e4c0    void Game::load_troops(LoadStream* s)                         (ret 4)
void Game::load_troops(LoadStream* s)
{
    int n;
    s->r_raw(&n);
    if (n < 0 || n >= this->nrecords + 1)
        mb_error("Load error: Invalid number of troops: %d", n);
    for (int i = 0; i < n; i++)
        ((Rec508*)(this->records + i * 0x508))->read(s);
    // the fixed 42-entry table at 0x8ee3138 (0x44 bytes each, ending at 0x8ee3c60)
    for (char* p = (char*)0x8ee3138; (int)p < (int)0x8ee3c60; p += 0x44)
        s->r_raw(p);
}

// @ 0x51e540    void Rec508::read(LoadStream* s)                              (ret 4)
void Rec508::read(LoadStream* s)
{
    ((Vec8*)((char*)this + 0x4f4))->read(s);
    for (int i = 0; i < 4; i++)
        s->r_raw((char*)this + 0x58 + i * 4);
    for (int i = 0; i < 7; i++)
        s->r_int((int*)((char*)this + 0x80) + i);
    for (int i = 0; i < 6; i++)
        s->r_q2((char*)this + 0x68 + i * 4);
    s->r_q3((char*)this + 0x30)->r_raw((char*)this + 0x3b8);
    s->r_raw((char*)this + 0x3b4)->r_raw((char*)this + 0x9c);
    s->r_int((int*)((char*)this + 0xa0));
    s->r_raw((char*)this + 0x4c);
    s->r_raw((char*)this + 0xa4)->r_raw((char*)this + 0xa8)->r_raw((char*)this + 0xac);
    s->r_raw((char*)this + 0xb0);
    ((Rec60*)((char*)this + 0xb4))->read(s);
    int* p = (int*)((char*)this + 0x4a4);
    for (int i = 0; i < 10; i++) {
        s->r_raw(p);
        s->r_raw(p + 1);
        p += 2;
    }
    for (int i = 0; i < 4; i++)
        s->r_q3((char*)this + 0x3c0 + i * 8);
    read_soa(s, (char*)this + 0x400);
}

// @ 0x51e690    void Rec60::read(LoadStream* s)                               (ret 4)
void Rec60::read(LoadStream* s)
{
    char* p = (char*)this;
    for (int i = 0; i < 0x60; i++) {
        s->r_raw(p);
        s->r_raw(p + 4);
        p += 8;
    }
}

// @ 0x51e6c0    void Game::load_tracks(LoadStream* s)                         (ret 4)
void Game::load_tracks(LoadStream* s)
{
    ((Logger*)0x5a9f60)->log("Loading tracks\n");
    int n;
    Rec5c val;
    s->r_raw(&n);
    this->tracks.resize(n, val);
    for (int i = 0; i < n; i++) {
        Rec5c* rec = this->tracks._Myfirst + i;
        rec->read(s);
    }
}

// @ 0x51e750    void Rec5c::read(LoadStream* s)                               (ret 4)
void Rec5c::read(LoadStream* s)
{
    s->r_int((int*)this)->r_int((int*)((char*)this + 4))->r_int((int*)((char*)this + 8));
    s->r_int((int*)((char*)this + 0x10));
    s->r_int((int*)((char*)this + 0x14))->r_raw((char*)this + 0x18);
}

// @ 0x51e7a0    void Vec5c::resize(unsigned n, Rec5c val)                     (ret 0x60)
void Vec5c::resize(unsigned n, Rec5c val)
{
    if ((unsigned)this->size() < n) {
        this->insert_n(_Mylast, n - this->size(), &val);
        return;
    }
    if (_Myfirst && n < (unsigned)this->size()) {
        Rec5c* it;
        this->shift_over(&it, _Myfirst + n, _Mylast);
    }
}

// @ 0x51e840    void Table::read(LoadStream* s, int arg)                      (ret 8)
void Table::read(LoadStream* s, int arg)
{
    if (this->count)
        this->clear();
    int n;
    s->r_raw(&n);
    s->r_raw((char*)this + 0x24);
    for (int i = 0; i < n; i++)
        this->alloc_id();
    this->clear();
    this->free_ids.resize(0, 0);
    for (int i = 0; i < n; i++) {
        char* e = this->pages[(unsigned)i >> 4] + (i & 0xf) * 0x98;
        int flag;
        s->r_raw(&flag);
        *(int*)(e + 4) = flag;
        if (flag) {
            s->r_raw(e + 0xc);
            ((Rec98*)e)->read(s, arg);
        } else {
            this->free_ids.push_back(i);
        }
    }
}

// @ 0x51e970    void Rec98::read(LoadStream* s, int arg)                      (ret 8)
void Rec98::read(LoadStream* s, int arg)
{
    int* p;
    LoadStream* rs;
    s->r_obj((char*)this + 0x14);
    s->r_raw((char*)this + 0x10);
    p = (int*)((char*)this + 0x24);
    rs = s->r_int(p);
    rs->r_int(++p);
    p = (int*)((char*)this + 0x2c);
    rs = s->r_int(p);
    rs->r_int(++p);
    p = (int*)((char*)this + 0x34);
    rs = s->r_int(p);
    rs->r_int(++p);
    s->r_raw((char*)this + 0x54);
    s->r_raw((char*)this + 0x58);
    s->r_cnt((int*)((char*)this + 0x68));
    s->r_int((int*)((char*)this + 0x70));
}

// @ 0x51ea10    void Table::clear()                                           (ret 0)
void Table::clear()
{
    this->free_ids.resize(0, 0);
    for (int i = 0; i < this->count; i++) {
        char* e = this->pages[(unsigned)i >> 4] + (i & 0xf) * 0x98;
        *(int*)(e + 4) = 0;
        this->free_ids.push_back(i);
    }
}

// @ 0x51eaa0    void Game::load_scene_parties(LoadStream* s)                  (ret 4)
void Game::load_scene_parties(LoadStream* s)
{
    Scene* sc = (Scene*)((char*)this + 0xf8);
    sc->read(s, 1);
    int idx = (int)sc->party_index(this->f78);
    char** pages = *(char***)((char*)sc + 0x138d0);
    char* rec = pages[idx >> 4] + (idx & 0xf) * 0x5ac8;
    int cnt = *(int*)(rec + 0x8a8);
    char* base = (char*)this + 0x1405c;
    for (int i = 0; i < cnt; i++) {
        int id = *(int*)(rec + 0xc0 + i * 0x20 - 0x14);
        int f = *(int*)(this->records + id * 0x508 + 0x30);
        if ((f & 0x10) == 0) {
            s->r_int((int*)(rec + 0xc0 + i * 0x20));
            s->r_raw(rec + 0xc0 + i * 0x20 - 4);
            for (int k = 0; k < 0x20; k++)
                s->r_raw(base + i * 0x80 + k * 4);
        }
    }
}
