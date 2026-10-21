// Slice s0045ff30: Agent::tick (the per-frame agent update: animations, rider/mount state copy, off-map check,
// death state machine) and the dialog text machinery: ScriptState::expand_token (0x460160, replaces one
// {...} token of a string: registers, string registers, "playername", he/she and reg?a:b choices) and
// ScriptState::expand (0x4606d0, expands a whole string: '^' -> newline, {tokens}); the DialogMgr accessors
// (speaker / listener ids, line text and flags) and the block reader helper 0x4607e0. Names provisional.
#include <stdio.h>
#include <stddef.h>
#include <vector>
#include "rgl.h"
#include "tstring.h"
using rgl::vector4;

#define MB_NOTHROW throw()

void mb_snprintf(char* buf, size_t n, const char* fmt, ...);   // 0x415100

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

// ---- globals ---------------------------------------------------------------------------------------------
struct StrTable { tstring* find(const tstring* key) MB_NOTHROW; };   // 0x4c6010 (null when absent)
extern StrTable g_strtable;                                // 0x5e2a18

struct TroopRec {                                          // 0x508 bytes
    tstring name;                                          // +0
    char pad10[0x20 - 0x10];
    tstring id;                                            // +0x20 (translation key)
    unsigned f30;                                          // +0x30 (low nibble 1 = female)
    char pad34[0x508 - 0x34];
};
struct GameData {
    char pad0[0x8c];
    int player;                                            // +0x8c
    char pad90[0xc8 - 0x90];
    int fc8;                                               // +0xc8
    char padcc[0x13a00 - 0xcc];
    TroopRec* chars;                                       // +0x13a00
};
extern GameData* g_game;                                   // 0x5e2930

// ---- the string register file / expander (object at 0xa70440) ---------------------------------------------
struct ScriptState {
    char pad0[0x50];
    __int64 reg[128];                                      // +0x50
    char pad450[0x2450 - 0x450];
    tstring str[128];                                      // +0x2450
    tstring expand_token(const tstring& tok);              // 0x460160 (thiscall, ret 8: hidden return + token)
    tstring expand(const tstring& in);                     // 0x4606d0 (thiscall, ret 8)
};

// ---- the agent -------------------------------------------------------------------------------------------------
struct Agent;
struct Mission {
    char pad0[0x18];
    std::vector<Agent*> vp;                                // +0x18 (first at +0x1c; id >> 4 -> page of 16 agents)
    char pad28[0x653c - 0x28];
    float min_x, min_y, max_x, max_y;                      // +0x653c map bounds
};
extern Mission* g_mission;                                 // 0x5e292c
extern __int64 g_ticks[];                                  // 0x5e2948 (100 kHz clocks)
extern float g_blend_rate;                                 // 0x5a9c28

struct Timer {
    int clock;
    int pad4;
    __int64 start;
    void restart() { start = g_ticks[clock]; }
    float elapsed() const { return (float)(g_ticks[clock] - start) * 1e-5f; }
};
struct AgentBody;
struct Agent {
    char pad0[0x2c];
    int f2c;                                               // +0x2c (mount / rider agent id, -1 = none)
    int f30;                                               // +0x30
    char pad34[0x3c - 0x34];
    int f3c;                                               // +0x3c
    char pad40[0x50 - 0x40];
    vector4 apos;                                          // +0x50
    vector4 dirv;                                          // +0x60
    float rot0;                                            // +0x70
    char pad74[0x98 - 0x74];
    vector4 eye;                                           // +0x98
    float fa8, fac;                                        // +0xa8
    char padb0[0xbc - 0xb0];
    int fbc;                                               // +0xbc
    char padc0[0x250 - 0xc0];
    AgentBody* body;                                       // +0x250
    char pad254[0x56a4 - 0x254];
    vector4 vel;                                           // +0x56a4
    vector4 vel2;                                          // +0x56b4
    char pad56c4[0x57e0 - 0x56c4];
    Timer t57e0;                                           // +0x57e0
    char pad57f0[0x5830 - 0x57f0];
    int state;                                             // +0x5830
    char pad5834[0x5844 - 0x5834];
    int f5844;                                             // +0x5844
    char pad5848[0x5858 - 0x5848];
    int f5858;                                             // +0x5858
    char pad585c[4];
    Timer t5860;                                           // +0x5860
    float limit5870;                                       // +0x5870
    char pad5874[0x5928 - 0x5874];
    void update_anims(float rate);                         // 0x43d040 (ret 4)
    void draw_name();                                      // 0x4552c0
    void tick_death();                                     // 0x457710
    void ragdoll_update();                                 // 0x4543a0
    void tick();                                           // 0x45ff30
};

// @ 0x45ff30
void Agent::tick()
{
    if (body) {
        update_anims(g_blend_rate);
        if (f2c >= 0) {
            unsigned id = f2c;
            Agent* o = g_mission->vp[id >> 4] + (id & 0xf);
            apos = o->apos;
            dirv = o->dirv;
            rot0 = o->rot0;
            vel2 = o->vel2;
            vel = o->vel;
            fbc = o->fbc;
            fa8 = o->fa8;
            fac = o->fac;
            f3c = o->f3c;
            eye = o->eye;
        }
        if (f5844 && state == 2 && f30 == -1 &&
            (apos.x - g_mission->min_x < 1.0f || apos.y - g_mission->min_y < 1.0f ||
             g_mission->max_x - apos.x < 1.0f || g_mission->max_y - apos.y < 1.0f)) {
            state = 3;
            t57e0.restart();
            draw_name();
        }
        tick_death();
        ragdoll_update();
        if (f5858 >= 0 && limit5870 < t5860.elapsed())
            f5858 = -1;
    }
}

// @ 0x460160
tstring ScriptState::expand_token(const tstring& tok_)
{
    char buf[0x10000];
    tstring& tok = (tstring&)tok_;
    buf[0] = 0;
    if (tok.len == 3 && tok.equals("***"))
        return tstring("");
    for (int i = 1; i < tok.len; i++) {
        if (tok.buf[i] == '{') {
            tstring inner("");
            int depth = 0;
            int j = i + 1;
            for (; j < tok.len; j++) {
                if (tok.buf[j] == '}' && depth <= 0)
                    break;
                if (tok.buf[j] == '{')
                    depth++;
                else if (tok.buf[j] == '}')
                    depth--;
                inner.push_back(tok.buf[j]);
            }
            if (j > i && j < tok.len) {
                tok.erase_range(i, j + 1);
                tok.insert(i, expand_token(inner));
            }
        }
    }
    if ((tok.len == 4 || tok.len == 5) && tok.buf[0] == 'r' && tok.buf[1] == 'e' && tok.buf[2] == 'g' &&
        tok.buf[3] >= '0' && tok.buf[3] < ':') {
        int n = tok.buf[3] - '0';
        if (tok.len > 4)
            n = tok.buf[4] - '0' + n * 10;
        if (n >= 0 && n < 0x80)
            mb_snprintf(buf, 0x10000, "%d", reg[n]);
        else
            mb_snprintf(buf, 0x10000, "TOKEN EXCEEDS MAX REGISTERS: <%s>", tok.c_str());
    } else if (tok.len > 5 && tok.buf[0] == 'r' && tok.buf[1] == 'e' && tok.buf[2] == 'g' && tok.buf[3] >= '0' &&
               tok.buf[3] < ':' && (tok.buf[4] == '?' || tok.buf[5] == '?')) {
        int n = tok.buf[3] - '0';
        int start = 5;
        if (tok.buf[4] >= '0' && tok.buf[4] < ':') {
            n = tok.buf[4] - '0' + n * 10;
            start = 6;
        }
        if (n >= 0 && n < 0x80) {
            int colon = -1;
            for (int k = 0; k < tok.len; k++)
                if (tok.buf[k] == ':')
                    colon = k;
            if (colon >= 0) {
                if (reg[n] == 0)
                    mb_snprintf(buf, 0x10000, "%s", tok.substr(colon + 1, tok.len).c_str());
                else
                    mb_snprintf(buf, 0x10000, "%s", tok.substr(start, colon).c_str());
            } else {
                mb_snprintf(buf, 0x10000, "UNRECOGNIZED TOKEN <%s>", tok.c_str());
            }
        } else {
            mb_snprintf(buf, 0x10000, "TOKEN EXCEEDS MAX REGISTERS: <%s>", tok.c_str());
        }
    } else if ((tok.len == 2 || tok.len == 3) && tok.buf[0] == 's' && tok.buf[1] >= '0' && tok.buf[1] < ':') {
        int n = tok.buf[1] - '0';
        if (tok.len > 2)
            n = tok.buf[2] - '0' + n * 10;
        if (n >= 0 && n < 0x80)
            mb_snprintf(buf, 0x10000, "%s", str[n].c_str());
        else
            mb_snprintf(buf, 0x10000, "TOKEN EXCEEDS MAX REGISTERS: <%s>", tok.c_str());
    } else if (tok.equals("playername")) {
        TroopRec* p = &g_game->chars[g_game->player];
        tstring* t = g_strtable.find(&p->id);
        if (!t)
            t = (tstring*)p;
        mb_snprintf(buf, 0x10000, "%s", t->c_str());
    } else {
        int slash = -1;
        for (int k = 0; k < tok.len; k++)
            if (tok.buf[k] == '/')
                slash = k;
        if (slash >= 0) {
            if ((g_game->chars[g_game->player].f30 & 0xf) != 1)
                mb_snprintf(buf, 0x10000, "%s", tok.substr(0, slash).c_str());
            else
                mb_snprintf(buf, 0x10000, "%s", tok.substr(slash + 1, tok.len).c_str());
        } else {
            mb_snprintf(buf, 0x10000, "UNRECOGNIZED TOKEN <%s>", tok.c_str());
        }
    }
post:
    if (buf[0]) {
        char* p = buf;
        do {
            if (*p == '^')
                *p = '\n';
        } while (*++p);
    }
    tstring res(buf);
    return res;
}

// @ 0x4606d0
tstring ScriptState::expand(const tstring& in)
{
    int i = 0;
    tstring out;
    tstring inner;
    for (; i < in.len; i++) {
        if (in.buf[i] != '{') {
            if (in.buf[i] == '^')
                out.push_back('\n');
            else
                out.push_back(in.buf[i]);
        } else {
            inner.assign("");
            i++;
            int depth = 0;
            for (; i < in.len; i++) {
                if (in.buf[i] == '}' && depth <= 0)
                    break;
                if (in.buf[i] == '{')
                    depth++;
                else if (in.buf[i] == '}')
                    depth--;
                inner.push_back(in.buf[i]);
            }
            out.append(expand_token(inner));
        }
    }
    return out;
}

// ---- dialog manager accessors --------------------------------------------------------------------------------
struct DialogRec {
    char pad0[0x20];
    unsigned flags;                                        // +0x20
    char pad24[0x64 - 0x24];
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
    char pad10[0x18 - 0x10];
    int cur;                                               // +0x18
    char pad1c[0x24 - 0x1c];
    DlgLine list[0x400];                                   // +0x24
    int get_c8();                                          // 0x460620
    int speaker();                                         // 0x460640
    int listener();                                        // 0x460670
    tstring line_text(int i);                              // 0x4606a0 (thiscall, ret 8)
    int line_flags(int i);                                 // 0x4606c0
};

// @ 0x460620
int DialogMgr::get_c8()
{
    if (f8 == 6)
        return 1;
    return g_game->fc8;
}

// @ 0x460640
int DialogMgr::speaker()
{
    unsigned fl = recs[cur].flags;
    if (fl & 0x10000)
        return g_game->player;
    if (fl & 0xfff00000)
        return (int)fl >> 20;
    return fc;
}

// @ 0x460670
int DialogMgr::listener()
{
    if (recs[cur].flags & 0x10000)
        return fc;
    return g_game->player;
}

// @ 0x4606a0
tstring DialogMgr::line_text(int i)
{
    return list[i].text;
}

// @ 0x4606c0
int DialogMgr::line_flags(int i)
{
    return list[i].flags;
}

// @ 0x4607d0
// Elem constructor (a block statement: 0x98 bytes, last field -1); shared by Block::read below.
struct Elem {
    unsigned id;
    int pad4;
    int n;
    int padc;
    __int64 vals[16];
    int f90;
    int pad94;
    Elem() : f90(-1) {}
};
struct Block {
    Elem* items;
    int count;
    char done;
    tstring text;
    void read(stream_base* s);                             // 0x4607e0
};

// @ 0x4607e0
void Block::read(stream_base* s)
{
    done = 0;
    if (items) {
        delete[] items;
        items = 0;
    }
    s->r_int(&count);
    if (count)
        items = new Elem[count];
    for (int i = 0; i < count; i++) {
        Elem* e = &items[i];
        s->r_uint(&e->id);
        s->r_int(&e->n);
        for (int j = 0; j < e->n; j++)
            s->r_i64(&e->vals[j]);
    }
}
