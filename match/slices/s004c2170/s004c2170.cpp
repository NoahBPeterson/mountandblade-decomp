// Mount & Blade 1.011 - batch ds1, slice s004c2170.
// Stream readers for the trigger/game record vectors plus three large record containers.
// Class and record names are provisional.
#include <windows.h>

// tstring {pos, buf, cap, len}; aggregate with the real methods declared on it.
struct tstr {
    int pos;    // +0
    char* buf;  // +4
    int cap;    // +8
    int len;    // +0xc
    void ctor();                                  // 0x42bbd0
    void dtor();                                  // 0x42bc40
    void dtor2();                                 // 0x42bf80
};

// ---- globals ----
extern __int64 g_ticks[256];           // 0x5e2948 clock table (8 bytes per entry)
extern int   g_a70474;                 // 0xa70474
extern int   g_a70478;                 // 0xa70478
extern int   g_a7047c;                 // 0xa7047c
extern float g_5aa010;                 // 0x5aa010
extern float g_590cd0;                 // 0x590cd0
extern float g_549ca8;                 // 0x549ca8
extern float g_549cdc;                 // 0x549cdc
extern float g_54a6d0;                 // 0x54a6d0
extern float g_54a6bc;                 // 0x54a6bc
extern float g_590b50;                 // 0x590b50
extern float g_54a6f0;                 // 0x54a6f0
struct Logger { void* vt; };           // 0x5a9f60

int __cdecl dprintf(const char* fmt, ...);                 // 0x535717
void __cdecl free(void* p);                                // 0x53570d
void __cdecl free2(void* p);                               // 0x535712
int __cdecl fmt_536f8f(char* buf, const char* fmt, ...);   // 0x536f8f
int __cdecl file_open(int req, int flags);                 // 0x534318
int __cdecl file_seek(int req, int pos);                   // 0x534312
int __cdecl file_size(int req);                            // 0x53430c
int __cdecl file_pos(int req);                             // 0x534306
int __cdecl file_close(int req);                           // 0x5342e2
DWORD __stdcall GetTickCount();                            // 0x536a9f
float __cdecl time_query();                                // 0x40e400

// ---- stream interface (+0x40 span, +0x48, +0x4c int, +0x54 string) ----
struct stream_reader;
struct stream {
    stream_reader* vt;
};
struct stream_reader {
    void s0(); void s1(); void s2(); void s3(); void s4(); void s5(); void s6(); void s7();
    void s8(); void s9(); void sa(); void sb(); void sc(); void sd(); void se(); void sf();
    void s10(); void s11(); void s12(); void s13(); void s14(); void s15(); void s16(); void s17();
    void s18(); void s19(); void s1a(); void s1b(); void s1c(); void s1d(); void s1e(); void s1f();
    void s20(); void s21(); void s22(); void s23();
    void __cdecl read_int(void* dst);   // +0x4c
    void s25(); void s26(); void s27(); void s28(); void s29(); void s2a(); void s2b(void* dst);
    void __cdecl read_str(void* dst);   // +0x54
};

// Loader: the same object seen through a class whose only known slot calls are the read
// primitives; methods are invoked with the stream as `this`.
struct Loader {
    char pad2[0x10];
    void __cdecl s10(void* dst);        // +0x40
    char pad3[0xc];
    void __cdecl read_int(void* dst);   // +0x4c
    char pad4[4];
    void __cdecl s2b(void* dst);        // +0x54
};
struct VTabS40 {
    void s0(); void s1(); void s2(); void s3(); void s4(); void s5(); void s6(); void s7();
    void s8(); void s9(); void sa(); void sb(); void sc(); void sd(); void se(); void sf();
    void s10v(void* dst);
};
struct VTabS4c {
    void s0(); void s1(); void s2(); void s3(); void s4(); void s5(); void s6(); void s7();
    void s8(); void s9(); void sa(); void sb(); void sc(); void sd(); void se(); void sf();
    void s10v(); void s11(); void s12(); void s13(); void s14(); void s15(); void s16();
    void s17(); void s18(); void s19(void* dst);
};
struct VTabS54 {
    void s0(); void s1(); void s2(); void s3(); void s4(); void s5(); void s6(); void s7();
    void s8(); void s9(); void sa(); void sb(); void sc(); void sd(); void se(); void sf();
    void s10v(); void s11(); void s12(); void s13(); void s14(); void s15(); void s16();
    void s17(); void s18(); void s19(); void s1a(); void s1b(); void s1c(); void s1d();
    void s1e(); void s1f(); void s20(void* dst);
};

// TriggerVec / GameVec (implementations in slice s004c17c0, this slice re-states the layout).
struct TriggerRec {
    char body[0x78];
};
struct GameRec {
    char body[0x30];
};
struct TriggerVec {
    int         count;     // +0
    TriggerRec* data;      // +4
    int         cur;       // +8
    int  resize(stream& s);
    void stamp(int c);
};
struct GameVec {
    int      count;       // +0
    GameRec* data;        // +4
    int      cur;         // +8
    int  resize(stream& s, int extra);
    void stamp(int c);
};

// =============================================================================================
// 0x30-byte entries of the trigger table
// =============================================================================================
struct TriggerEntry {        // 0x30 bytes
    tstr    s0;              // +0
    int     v10;             // +0x10
    tstr    s1;              // +0x14
    int     v24;             // +0x24
    int     v28;             // +0x28
    int     v2c;             // +0x2c
};

// Record handled by the entry reader: string at +0, flags at +0x10, count at +0x14,
// flags dword at +0x18 (its top byte copies into the string's pos), array pointer at +0x28.
struct TriggerE2 {
    tstr    s0;              // +0
    int     v10;             // +0x10
    int     v14;             // +0x14 count
    int     v18;             // +0x18 flags dword
    int     pad1c;           // +0x1c
    char    pad20[8];        // +0x20
    int     s1;              // +0x28 array pointer
};

// Vector of 0x30-byte entries: the single field points at the array, whose element count sits in
// the int right before it (block of 4 + n*0x30).
struct TriggerEntryVec {
    TriggerEntry* data;      // +0
    void dtor();             // 0x4c2200
    int  resize(stream& s);  // 0x4c2250
};

// Record helper entry points reached through `mov ecx,<rec>; call`.
struct TriggerRecord {
    char body[0x4c];
};
void __fastcall rec_4bdc00r(void* rec, stream& s);            // 0x4bdc00
void __fastcall rec_4bdd50(TriggerRecord& rec, stream& s);    // 0x4bdd50
void __fastcall rec_4be0e0(void* obj, int i, tstr* out);      // 0x4be0e0
void __fastcall counted_dtor(void* p, int last);              // 0x4315d0 (counted array dtor)

// =============================================================================================
// Record tables
// =============================================================================================

struct Slot1c {               // 0x1c-byte table slot; only the leading tstring matters here
    tstr    s0;              // +0
    int     v10;             // +0x10
    int     v14;             // +0x14
    int     v18;             // +0x18
};

// 0x400 x 0x1c byte slots at +0x30, 0x400 x 0x1c at +0x7030, one tstring at +0xe028, int at +4.
struct EditorTable {
    int     field0;              // +0
    int     v4;                  // +4
    int     v8;                  // +8
    char    pad0[0x24];
    Slot1c  rows0[0x400];        // +0x30
    char    rows1pad[0x6ff8];    // +0x7030 .. +0xe028
    tstr    name;                // +0xe028
    EditorTable();               // 0x4c2470
};

// =============================================================================================
// Large object: 0x100 entries of 0x20 bytes at +0x18, a float array, three file monitors
// (request / FILE* / ended) and references into the global clock table.
// =============================================================================================
struct Entry20 {            // 0x20 bytes
    int   v0;               // +0
    char* name;             // +4   (inline when null)
    float t;                // +8
    int   flags;            // +0xc
    int   pad10;            // +0x10
    int   f14;              // +0x14
    int   pad18;            // +0x18
    int   pad1c;            // +0x1c
};
struct BigObj {
    int      mode;              // +0x0000
    int      count;             // +0x0004
    char     pad8[0x10];        // +0x0008
    Entry20  entries[0x100];    // +0x0018 .. +0x2018
    float    times[0xfc];       // +0x2018 .. +0x2408
    int      req0;              // +0x2408  file opened for monitor 0
    int      hdl0;              // +0x240c  FILE* of that monitor
    int      ended0;            // +0x2410
    int      ended1;            // +0x2414
    int      req1;              // +0x2418  file opened for monitor 1
    int      hdl1;              // +0x241c
    __int64  ta;                // +0x2420  clock snapshot pair
    int      req2;              // +0x2428  file opened lazily
    int      hdl2;              // +0x242c
    int      cnt1;              // +0x2430  entry index driven by monitor 0
    int      cnt2;              // +0x2434  entry index driven by monitor 1
    int      res0;              // +0x2438  monitor 0 result
    int      res1;              // +0x243c  monitor 1 result
    int      lim0;              // +0x2440
    int      lim1;              // +0x2444
    int      mask;              // +0x2448
    int      pad244c;           // +0x244c
    int      clock_idx;         // +0x2450
    int      pad2454;           // +0x2454
    __int64  t0;                // +0x2458
    int      clock_idx1;        // +0x2460
    int      pad2464;           // +0x2464
    __int64  t1;                // +0x2468
    int      started;           // +0x2470
    int      started2;          // +0x2474
    char     flag;              // +0x2478
    char     pad2479[3];
    tstr     name;              // +0x247c
    BigObj();                   // 0x4c24c0
    void update();              // 0x4c25e0
    void name_to_string(int idx, tstr* out);   // 0x4be0e0
};

// =============================================================================================
// Functions
// =============================================================================================

void __fastcall tstr_ctor(tstr* s) { s->ctor(); }                    // 0x42bbd0
void* __fastcall tstr_ctor_arr(tstr* s) { s->ctor(); return s; }     // 0x42bbd0
void __fastcall entry_ctor(tstr* s) { s->ctor(); }
void __stdcall push_wide_literal(const wchar_t* p);                  // opaque helper
void __fastcall tstr_ctor_wide(tstr* s) { push_wide_literal(L"WL r"); }   // 0x42bbd0

void __cdecl Loader::s10(void* dst) { ((VTabS40*)this)->s10v(dst); }
void __cdecl Loader::read_int(void* dst) { ((VTabS4c*)this)->s19(dst); }
void __cdecl Loader::s2b(void* dst) { ((VTabS54*)this)->s20(dst); }

// @ 0x4c2170
stream& read_entry(stream& s, TriggerE2& e)
{
    e.s0.pos = 0x64;
    Loader* L = (Loader*)&s;
    L->s2b(&e);
    L->s10(&e.v18);
    L->read_int(&e.v14);
    int n1 = e.v14;
    unsigned int* p = (unsigned int*)operator new(n1 * 0x40);
    e.s1 = (int)p;
    int off = 0;
    for (int i = 0; i < n1; i++) {
        rec_4bdc00r((char*)p + off, s);
        off += 0x40;
    }
    if ((e.v18 & 0xff000000) != 0)
        e.s0.pos = (unsigned char)(e.v18 >> 24);
    return s;
}

// @ 0x4c2200
void TriggerEntryVec::dtor()
{
    TriggerEntry* o = data;
    if (o == 0)
        return;
    char* base = (char*)o - 4;
    int n = *(int*)base;
    for (int i = n - 1; i >= 0; i--) {
        TriggerEntry* e = (TriggerEntry*)((char*)o + i * 0x30);
        operator delete(*(void**)((char*)e + 0x20));
        ((tstr*)((char*)e + 0x1c))->dtor();
        e->s0.dtor();
    }
    operator delete(base);
}

// @ 0x4c2250
int TriggerEntryVec::resize(stream& s)
{
    if (data != 0)
        counted_dtor(data, 3);
    s.vt->read_int(&data);
    int n = *(int*)&data;
    TriggerEntry* p = (TriggerEntry*)operator new(n * 0x30 + 4);
    if (p != 0) {
        p = (TriggerEntry*)((char*)p + 4);
        *(int*)((char*)p - 4) = n;
        for (int i = 0; i < n; i++) {
            p[i].s0.ctor();
            ((tstr*)((char*)&p[i] + 0x1c))->ctor();
            p[i].v10 = 0;
        }
    } else {
        p = 0;
    }
    data = p;
    for (int i = 0; i < n; i++)
        read_entry(s, *(TriggerE2*)((char*)data + i * 0x30));
    return (int)s.vt;
}

// @ 0x4c2300
int __fastcall read_trigger_block(void* self, stream& s)
{
    char buf[0x200];
    s.vt->read_str(self);
    s.vt->read_str((char*)self + 0x10);
    s.vt->read_int((char*)self + 0x20);
    s.vt->read_int((char*)self + 0x3c);
    s.vt->read_str((char*)self + 0x24);
    s.vt->read_int((char*)self + 0x38);
    int n = *(int*)((char*)self + 0x38);
    TriggerRecord* recs = (TriggerRecord*)operator new(n * 0x4c);
    *(TriggerRecord**)((char*)self + 0x34) = recs;
    int off = 0;
    for (int i = 0; i < n; i++) {
        rec_4bdd50(*(TriggerRecord*)((char*)recs + off), s);
        off += 0x4c;
    }
    char* nm;
    if (*(char**)((char*)self + 4) != 0)
        nm = *(char**)((char*)self + 4);
    else
        nm = (char*)self + 4;
    dprintf(buf, 0x200, "???%s", nm);
    (void)buf;
    return 0;
}

// @ 0x4c23f0
int __fastcall read_game_block(void* self, stream& s)
{
    char buf[0x200];
    s.vt->read_str(self);
    s.vt->read_int((char*)self + 0x10);
    s.vt->read_int((char*)self + 0x20);
    char* nm;
    if (*(char**)((char*)self + 4) != 0)
        nm = *(char**)((char*)self + 4);
    else
        nm = (char*)self + 4;
    dprintf(buf, 0x200, "???%s", nm);
    (void)buf;
    GameVec* v = (GameVec*)((char*)self + 0x14);
    v->resize(s, 0);
    v->stamp(2);
    return 0;
}

// @ 0x4c2470
EditorTable::EditorTable()
{
    for (int i = 0; i < 0x400; i++)
        rows0[i].s0.ctor();
    for (int i = 0; i < 0x400; i++)
        ((Slot1c*)((char*)this + 0x7030))[i].s0.ctor();
    name.ctor();
    v4 = 0;
}
// @ 0x4c24c0
BigObj::BigObj()
{
    for (int i = 0; i < 0x100; i++)
        entry_ctor((tstr*)((char*)entries + i * 0x20));
    req1 = 0;
    ta = g_ticks[0];
    clock_idx = 0;
    t0 = g_ticks[0];
    clock_idx1 = 0;
    t1 = g_ticks[0];
    name.ctor();
    count = 0;
    req2 = 0;
    req0 = 0;
    res0 = -1;
    ended0 = 0;
    cnt1 = -1;
    lim0 = 1000;
    started = 0;
    hdl2 = 0;
    hdl0 = 0;
    res1 = -1;
    ended1 = 0;
    cnt2 = -1;
    lim1 = 1000;
    started2 = 0;
    mode = 0;
    mask = 0;
    flag = 0;
}

// @ 0x4c25e0
void BigObj::update()
{
    char buf[0x200];
    char sel[0x200];

    // ---- monitor 0: lazily opened file, reads one entry per record -------------------------
    if (req0 != 0) {
        if (ended0 == 0) {
            int h = file_open(req2, 0);
            if (h == 0) {
                ended0 = 1;
                res0 = file_seek(req2, -1);
                return;
            } else if (h != -2) {
                Entry20* e = &entries[cnt1];
                char* nm = e->name;
                if (nm == 0)
                    nm = (char*)&e->name;
                fmt_536f8f(buf, "???%s", nm);
                (void)buf;
                req0 = 0;
                return;
            }
        }
    }

    if (req0 != 0) {
        int end = file_size(req2);
        int pos = file_pos(req2);
        if (g_a70474 >= 0 && g_a70474 == cnt1 && g_a70478 > 0)
            g_a70474 = -1;
        if (pos < end) {
            Entry20* e = &entries[cnt1];
            if ((e->flags & 0x100) != 0 || g_a70474 < 0 || g_a70478 == 2) {
                if (started == 0 && e->flags == 0) {
                    if (mode == 0 || (e->flags & mode) != 0) {
                        if (mask == 0 || (e->flags & mask) != 0) {
                            if (g_a70474 < 0 || g_a70478 == 1) {
                                t0 = g_ticks[clock_idx];
                                started = 1;
                                flag = 1;
                            }
                        }
                    }
                }
            }
        } else {
            file_close(req2);
            req2 = 0;
            req0 = 0;
            res0 = -1;
        }
    }

    // ---- bulk pass over all entries ------------------------------------------------------
    if (g_5aa010 <= g_590cd0) {
        int best = -1;
        int first = 0;
        if (g_a70474 >= 0) {
            if (g_a70474 != -1) {
                g_a70474 = -1;
                first = 1;
            }
        }
        if (mode != 0) {
            if (cnt1 >= 0) {
                Entry20* e = &entries[cnt1];
                if ((mode & e->flags) != 0 &&
                    (mask == 0 || (mask & e->flags) != 0))
                    best = cnt1;
            }
        }
        float sum = g_549ca8;
        float now = (float)(g_ticks[0] * g_54a6d0) + g_549cdc;
        if (count > 0) {
            int* fl = (int*)((char*)entries + 8);
            float* t = times;
            for (int i = 0; i < count; i++, fl += 8, t++) {
                if (i == best) {
                    sum += now - *t + g_590b50;
                } else if (mode != 0 && (*(fl - 8) & mode) != 0) {
                    if (mask == 0 || (*fl & mask) != 0)
                        sum += now - *t + g_590b50;
                }
            }
        }
        if (sum >= g_549ca8) {
            int tt = (int)(GetTickCount() % 0x3dc9);
            int dt = (int)(tt * sum * g_54a6bc);                // fild; fmul; fmul; call __ftol2 (0x535a64)
            for (int i = 0; i < count; i++) {
                Entry20* e = &entries[i];
                if (i == best)
                    continue;
                if (mode == 0 || (e->flags & mode) == 0)
                    continue;
                if (mask != 0 && (e->flags & mask) == 0)
                    continue;
                int v = (int)(dt - ((now - times[i]) + g_590b50));   // fild dt; (now - t) + g; fsubp; call __ftol2
                if (v < g_549ca8)
                    continue;
                first = 1;
                best = i;
                break;
            }
            if (best >= 0) {
                if (first == 0 && flag == 0) {
                    if (entries[best].t >= 0)
                        goto monitor1;
                }
                name_to_string(best, (tstr*)sel);
                flag = 0;
                (void)first;
                if ((entries[cnt1].flags & 0x100) != 0)
                    mode = 0;
            }
        }

    monitor1:
        if (g_a7047c >= 0) {
            if (g_5aa010 > g_590cd0) {
                name_to_string(g_a7047c, &name);
            }
            g_a7047c = -1;
        }
    }
}
