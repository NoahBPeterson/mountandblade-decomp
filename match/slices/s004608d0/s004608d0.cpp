// Slice s004608d0: the dialog manager of the conversation screen. DialogRec::read (one dialog record from the
// module file: id, text, speaker/flags, condition and consequence script blocks), the vector deleting destructor
// of DialogRec, DialogMgr::read, the availability test, the per-line refresh, line collection (up to 0x400 lines
// for the player to pick from), the "advance through automatic lines" loops, the party iteration helpers and a few
// leaf functions. Names provisional (Module System: dialogs, dlga_/dlgb_ ids, anyone/party_tpl speaker flags).
#include <stdio.h>
#include <stddef.h>
#include "tstring.h"

#define MB_NOTHROW throw()

struct stream_base {
    virtual void reset();                                  // 0
    virtual void rewind();                                 // 1
    virtual stream_base& w_stream(stream_base* o) = 0;     // 2
    virtual stream_base& w_float(float* v) = 0;            // 3
    virtual stream_base& w_bool(char* v) = 0;              // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5
    virtual stream_base& w_i64(__int64* v) = 0;            // 6
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7
    virtual stream_base& w_int(int* v) = 0;                // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9
    virtual stream_base& w_cstr(const char* s) = 0;        // 10
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 11
    virtual stream_base& slot12(void* v) = 0;              // 12
    virtual stream_base& r_stream(stream_base* o) = 0;     // 13
    virtual stream_base& r_float(float* v) = 0;            // 14
    virtual stream_base& r_bool(char* v) = 0;              // 15
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17
    virtual stream_base& r_i64(__int64* v) = 0;            // 18
    virtual stream_base& r_int(int* v) = 0;                // 19
    virtual stream_base& r_byte(char* v) = 0;              // 20
    virtual stream_base& r_tstr(tstring* v) = 0;           // 21
};

// ---- script text expansion (slice s0045ff30) ----------------------------------------------------------
struct ScriptState {                                       // object at 0xa70440
    tstring expand(const tstring& in);                     // 0x4606d0 (thiscall, ret 8: hidden return + in)
};
extern ScriptState g_script;                               // 0xa70440

struct StrTable { tstring* find(const tstring* key) MB_NOTHROW; };   // 0x4c6010 (null when absent)
extern StrTable g_strtable;                                // 0x5e2a18

// ---- parties -------------------------------------------------------------------------------------------
struct Party {                                             // 0x5ac8 bytes
    int f0;
    int type;                                              // +4 (1 = ordinary party)
    int id;                                                // +8
    char pad[0x8c - 0xc];
    int owner;                                             // +0x8c
    char pad90[0x5ac8 - 0x90];
};
struct PartyMgr {                                          // embedded in GameData at +0xf8
    char pad0[0x138b8];
    int count;                                             // +0x138b8
    char pad138bc[0x138d0 - 0x138bc];
    Party** pages;                                         // +0x138d0 (index >> 4 -> page of 16)
    char pad138d4[0x138dc - 0x138d4];
    int deflt;                                             // +0x138dc
    int index_of(int id);                                  // 0x48ad00 (thiscall, ret 4)
    Party* get(int id);                                    // 0x51ee50 (thiscall, ret 4)
    int first_party_id();                                  // 0x460d20
    void next_party_id(int* id);                           // 0x460ca0
};

struct GameData {
    char pad0[0xb0];
    int fb0;                                               // +0xb0
    char padb4[0xc8 - 0xb4];
    int fc8;                                               // +0xc8
    char padcc[0x139d4 - 0xcc];
    int f139d4;                                            // +0x139d4
    char pad139d8[0x13a04 - 0x139d8];
    int f13a04;                                            // +0x13a04
    char pad13a08[0x13a1c - 0x13a08];
    int f13a1c;                                            // +0x13a1c (current repeat index)
    char pad13a20[0x13a4c - 0x13a20];
    int f13a4c;                                            // +0x13a4c
    char pad13a50[0x1a23c - 0x13a50];
    int f1a23c;                                            // +0x1a23c
};
extern GameData* g_game;                                   // 0x5e2930
#define g_parties ((PartyMgr*)((char*)g_game + 0xf8))

// ---- script blocks -----------------------------------------------------------------------------------------
struct Elem {                                              // 0x98 bytes: one statement of a block
    unsigned id;                                           // +0
    int pad4;
    int n;                                                 // +8
    int padc;
    __int64 vals[16];                                      // +0x10
    int f90;                                               // +0x90 (-1)
    int pad94;
    Elem() : f90(-1) {}                                    // 0x4607d0
};
struct Block {                                             // 0x1c bytes
    Elem* items;                                           // +0
    int count;                                             // +4
    char done;                                             // +8
    tstring text;                                          // +0xc (debug text)
    Block() { count = 0; items = 0; done = 0; }
    ~Block() { if (items) delete[] items; }
    void read(stream_base* s);                             // 0x4607e0
    char exec(int mode);                                   // 0x4875e0 (thiscall, ret 4)
};

struct DialogRec {                                         // 0x64 bytes
    tstring text;                                          // +0x00 (spoken text, underscores -> spaces)
    tstring id;                                            // +0x10
    unsigned flags;                                        // +0x20
    int f24;                                               // +0x24
    Block cond;                                            // +0x28
    Block conseq;                                          // +0x44
    int f60;                                               // +0x60
    void read(stream_base* s);                             // 0x4608d0
    ~DialogRec();                                          // folded into the vector deleting dtor 0x4609d0
};

struct DlgLine {                                           // 0x1c bytes
    int dialog;
    int idx;
    unsigned flags;
    tstring text;
};

struct DialogMgr {
    int count;                                             // +0
    DialogRec* recs;                                       // +4
    int f8;                                                // +8
    int fc;                                                // +0xc
    int f10;                                               // +0x10
    int f14;                                               // +0x14
    int cur;                                               // +0x18
    int cur_idx;                                           // +0x1c
    int nres;                                              // +0x20
    DlgLine list[0x400];                                   // +0x24
    DlgLine sel[0x400];                                    // +0x7024
    char fe024;                                            // +0xe024
    tstring cur_text;                                      // +0xe028
    void read(stream_base* s);                             // 0x460ab0
    bool available(int idx, char flag);                    // 0x460b90
    void refresh_text();                                   // 0x460c10
    void select(DlgLine* l);                               // 0x460d80
    __forceinline void select_inl(DlgLine* l)
    {
        g_game->fc8 = 0;
        f8 = recs[l->dialog].f60;
        cur = l->dialog;
        cur_idx = l->idx;
        g_game->f13a1c = l->idx;
        recs[l->dialog].conseq.exec(4);
        refresh_text();
    }
    void collect(DlgLine* out, int max, int* n, char flag);   // 0x460de0
    void advance();                                        // 0x460ff0
    void start(int a, int b, int c, int d, char f);   // 0x4610b0 (ret 0x14)
    void advance_all(int* n);                              // 0x461140
};

// @ 0x4608d0
void DialogRec::read(stream_base* s)
{
    char buf[0x200];
    s->r_tstr(&id);
    s->r_int((int*)&flags).r_int(&f24);
    cond.read(s);
    _snprintf(buf, 0x200, "dialog condition: %s", id.c_str());
    {
        tstring t(buf);
        cond.text = t;
    }
    s->r_tstr(&text);
    text.underscores_to_spaces();
    s->r_int(&f60);
    conseq.read(s);
    _snprintf(buf, 0x200, "dialog consequence: %s", id.c_str());
    {
        tstring t(buf);
        conseq.text = t;
    }
}

// @ 0x4609d0
DialogRec::~DialogRec() {}
void destroy_dialogs(DialogRec* p) { delete[] p; }

// @ 0x460ab0
void DialogMgr::read(stream_base* s)
{
    s->r_int(&count);
    if (recs) {
        delete[] recs;
        recs = 0;
    }
    recs = new DialogRec[count];
    for (int i = 0; i < count; i++)
        recs[i].read(s);
}

// @ 0x460b90
bool DialogMgr::available(int idx, char flag)
{
    unsigned char bad = 0;
    DialogRec* r = &recs[idx];
    if (r->f24 == f8) {
        if (flag) {
            bad = ~(unsigned char)(r->flags >> 16) & 1;
            if (bad)
                goto done;
        }
        unsigned who = r->flags & 0xfff;
        if (who == 0xfff)
            goto done;
        if (r->flags & 0x20000) {
            if (f10 < 0)
                goto fail;
            if (who == g_parties->get(f10)->owner)
                goto done;
        } else if (who == fc) {
            goto done;
        }
    }
fail:
    bad = 1;
done:
    return bad == 0;
}

// @ 0x460c10
void DialogMgr::refresh_text()
{
    tstring name;
    if (cur >= 0) {
        DialogRec* r = &recs[cur];
        tstring* t = g_strtable.find(&r->id);
        if (!t)
            t = &r->text;
        name = *t;
    } else {
        name.assign("Error");
    }
    cur_text = g_script.expand(name);
}

// @ 0x460ca0
void PartyMgr::next_party_id(int* id)
{
    unsigned i = index_of(*id) + 1;
    while ((int)i < count && pages[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        *id = deflt;
    else
        *id = pages[i >> 4][i & 0xf].id;
}

// @ 0x460d20
int PartyMgr::first_party_id()
{
    unsigned i = 0;
    while ((int)i < count && pages[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        return deflt;
    return pages[i >> 4][i & 0xf].id;
}

// @ 0x460d80
void DialogMgr::select(DlgLine* l)
{
    select_inl(l);
}

// @ 0x460de0
void DialogMgr::collect(DlgLine* out, int max, int* n, char flag)
{
    *n = 0;
    if (max > 0x400)
        max = 0x400;
    for (int i = 0; i < count; i++) {
        if (*n >= max)
            return;
        if (available(i, flag)) {
            int start = 0;
            int reps = 1;
            unsigned mode = recs[i].flags & 0xf000;
            if (mode != 0) {
                if (mode == 0x1000) {
                    reps = g_game->f13a4c;
                } else if (mode == 0x3000) {
                    reps = g_game->f13a04;
                } else if (mode == 0x2000) {
                    GameData* g = g_game;
                    start = g_parties->first_party_id();
                    reps = g->f139d4;
                } else if (mode == 0x4000) {
                    reps = 100;
                } else if (mode == 0x5000) {
                    reps = 1000;
                }
            }
            int j = start;
            while (j < reps && *n < max) {
                g_game->f13a1c = j;
                if (recs[i].cond.exec(2)) {
                    out[*n].dialog = i;
                    out[*n].idx = j;
                    DialogRec* r = &recs[i];
                    tstring* t = g_strtable.find(&r->id);
                    if (!t)
                        t = &r->text;
                    out[*n].text = g_script.expand(*t);
                    out[*n].flags = recs[i].flags;
                    (*n)++;
                }
                if ((recs[i].flags & 0xf000) == 0x2000)
                    g_parties->next_party_id(&j);
                else
                    j++;
            }
        }
    }
}

// @ 0x460ff0
void DialogMgr::advance()
{
    int n;
    collect(sel, 1, &n, 0);
    while (n > 0) {
        if (!(((unsigned char*)&recs[sel[0].dialog].flags)[2] & 4)) {
            if (n > 0)
                select(sel);
            return;
        }
        select_inl(sel);
        collect(sel, 1, &n, 0);
    }
}

// @ 0x4610b0
// Starts a conversation: stores the parties / troop, resets the dialog globals and runs the automatic lines.
// The body (0x4610c2) is bracketed by two 18-byte inline tags of the original: `jmp +0x10; "WL  ", id, 0, "WL  "`
// with id 0x0c at the start (the 0x4610b0 "thunk") and 0x0d at the end (before the epilogue).
// The tags cannot be written in C++ (the inline assembler here accepts only _emit for data, which the rules
// forbid), so this source omits them and the row is nonmatching.
void DialogMgr::start(int a, int b, int c, int d, char f)
{
    fc = a;
    f10 = c;
    fe024 = f;
    g_game->fb0 = 0;
    g_game->fc8 = 0;
    g_game->f1a23c = 0;
    f8 = d;
    f14 = b;
    cur = -1;
    cur_idx = 0;
    advance();
}

// @ 0x461140
void DialogMgr::advance_all(int* n)
{
    collect(list, 0x400, n, 1);
    while (*n > 0 && (recs[list[0].dialog].flags & 0x40000)) {
        select_inl(list);
        collect(list, 0x400, n, 1);
    }
    nres = *n;
}

struct Agent {
    char pad0[0x1bc];
    float hp;                                              // +0x1bc
    float max_hp;                                          // +0x1c0
    void init_max_hp();                                    // 0x461200
};

// @ 0x461200
void Agent::init_max_hp()
{
    if (hp > 0.0f)
        max_hp = hp;
    else
        max_hp = 100.0f;
}

// @ 0x461230
struct E24 { int a, b, c, d, e, f; };
E24* copy_range(E24* first, E24* last, E24* dest)
{
    for (; first != last; ++first, ++dest)
        *dest = *first;
    return dest;
}
