// Slice s005093d0: character-sheet / party-window helpers.
//   0x5093d0  ini_file::get_int        - find the key, atoi() its value
//   0x509410  char_window::init        - clear the strings and install the eight arguments
//   0x5094a0  char_window::load_troop  - copy the troop-record display fields
//   0x509730  char_window::fill_skills - gather the visible skill ids
//   0x509770  make_button              - build the numbered "buttonN_*" image button
// Names and layouts are provisional; every offset comes from this slice's disassembly and the
// verified neighbours (ini_file: s004bd170, widget slots: s00502300 / s004dfd50).
#include <windows.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---------------------------------------------------------------------------------------------------------
// tstring (16 bytes): read cursor / heap buffer / capacity / length.
#include "../tstring.h"

// ---------------------------------------------------------------------------------------------------------
// ini file: a fixed 0x1000-entry table, each entry 0x2c bytes (key, 12 unknown, value).
struct ini_entry {
    tstring key;                                        // +0
    char    pad10[0xc];                                 // +0x10
    tstring value;                                      // +0x1c
};

struct ini_file {
    ini_entry entries[0x1000];                          // +0
    int       count;                                    // +0x2c000
    int  find(const tstring& key);                      // 0x4bd5a0
    int  get_int(const tstring& key);
};

extern "C" int atoi(const char*);                       // 0x53734d (atoi: jmp atol)

// @ 0x5093d0
int ini_file::get_int(const tstring& key)
{
    int ret = 0;
    int idx = find(key);
    if (idx >= 0)
        ret = atoi(entries[idx].value.c_str());
    return ret;
}

// ---------------------------------------------------------------------------------------------------------
// the game data object at 0x5e2930; troops are 0x508-byte records.
struct TroopView {
    char pad0[0x58];
    int  f58, f5c, f60, f64, f68, f6c, f70, f74, f78, f7c;
    int  f80, f84, f88, f8c, f90, f94, f98;
    char pad9c[0xa0 - 0x9c];
    int  fa0, fa4, fa8, fac;
};

#include "../../include/game_data.h"

#define TROOP_FIELD(i, off) (*(int*)((char*)g_game->troops + (i) * 0x508 + (off)))

// the character-sheet window the rest of the slice belongs to
struct char_window {
    char    pad0[0x14];
    int     troop;                                      // +0x14 (troop record index)
    char    pad18[0x1c - 0x18];
    tstring name;                                       // +0x1c
    char    pad2c[0x50 - 0x2c];
    int     f50;                                        // +0x50 (-1)
    int     f54;                                        // +0x54
    int     f58;                                        // +0x58
    tstring s5c;                                        // +0x5c
    tstring s6c;                                        // +0x6c
    tstring s7c;                                        // +0x7c
    tstring s8c;                                        // +0x8c
    tstring s9c;                                        // +0x9c
    tstring sac;                                        // +0xac
    tstring sbc;                                        // +0xbc
    char    padcc[0x228 - 0xcc];
    int     skill_ids[42];                              // +0x228
    int     skill_count;                                // +0x2d0
    char    pad2d4[0x4d8 - 0x2d4];
    int     f4d8, f4dc, f4e0, f4e4;                     // +0x4d8 (troop 0x58..)
    int     f4e8, f4ec, f4f0, f4f4, f4f8, f4fc, f500;   // +0x4e8 (troop 0x80..)
    int     f504, f508, f50c, f510;                     // +0x504 (troop 0xa0..)
    int     f514, f518, f51c, f520, f524, f528;         // +0x514 (troop 0x68..)
    void init(const tstring& n, int a, const char* b, const char* c,
              const char* d, const char* e, const char* f, const char* g);
    void load_troop();
    void fill_skills();
};

// @ 0x509410
void char_window::init(const tstring& n, int a, const char* b, const char* c,
                       const char* d, const char* e, const char* f, const char* g)
{
    s5c.clear();
    name = n;
    f54 = a;
    s6c.assign(b);
    s7c.assign(c);
    s8c.assign(d);
    s9c.assign(e);
    sac.assign(f);
    sbc.assign(g);
    f50 = -1;
}

// @ 0x5094a0
void char_window::load_troop()
{
    f508 = TROOP_FIELD(troop, 0xa4);
    f50c = TROOP_FIELD(troop, 0xa8);
    f510 = TROOP_FIELD(troop, 0xac);
    f504 = TROOP_FIELD(troop, 0xa0);
    f4d8 = TROOP_FIELD(troop, 0x58);
    f4dc = TROOP_FIELD(troop, 0x5c);
    f4e0 = TROOP_FIELD(troop, 0x60);
    f4e4 = TROOP_FIELD(troop, 0x64);
    f514 = TROOP_FIELD(troop, 0x68);
    f518 = TROOP_FIELD(troop, 0x6c);
    f51c = TROOP_FIELD(troop, 0x70);
    f520 = TROOP_FIELD(troop, 0x74);
    f524 = TROOP_FIELD(troop, 0x78);
    f528 = TROOP_FIELD(troop, 0x7c);
    f4e8 = TROOP_FIELD(troop, 0x80);
    f4ec = TROOP_FIELD(troop, 0x84);
    f4f0 = TROOP_FIELD(troop, 0x88);
    f4f4 = TROOP_FIELD(troop, 0x8c);
    f4f8 = TROOP_FIELD(troop, 0x90);
    f4fc = TROOP_FIELD(troop, 0x94);
    f500 = TROOP_FIELD(troop, 0x98);
}

// ---------------------------------------------------------------------------------------------------------
// the skill attribute table at 0x8ee3140, 0x44-byte records.
struct skill_entry {
    unsigned flags;                                     // +0 (bit 8: hidden)
    char     pad4[0x44 - 4];
};

// @ 0x509730
void char_window::fill_skills()
{
    int n = 0;
    int i = 0;
    skill_entry* se = (skill_entry*)0x8ee3140;
    int* out = skill_ids;
    do {
        unsigned char c = (unsigned char)(se->flags >> 8);
        c = ~c;
        if (c & 1) {
            *out = i;
            n++;
            out++;
        }
        se++;
        i++;
    } while ((int)se < 0x8ee3c68);
    skill_count = n;
}

// ---------------------------------------------------------------------------------------------------------
// widgets (slot numbers from the call sites; see s00502300 / s004dfd50).
struct UiWidget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21();
    virtual void set_xy(const float* v);                // +0x58
    virtual void d23(); virtual void d24();
    virtual void set_scale(const float* v);             // +0x64
    virtual void d26();
    virtual void set_size(const float* v);              // +0x6c
    virtual void d28(); virtual void d29(); virtual void d30();
    virtual void add_image(int idx, void* m);           // +0x7c
    virtual void d32(); virtual void d33(); virtual void d34(); virtual void d35(); virtual void d36();
    virtual void d37(); virtual void d38(); virtual void d39(); virtual void d40();
    virtual void set_text(tstring* out, const tstring* s);// +0xa4
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48(); virtual void d49();
    virtual void add_child(void* child);                // +0xc8
};

// the 0x188-byte image button (base ctor 0x48ae20, vptr 0x591d58)
struct UiButton : UiWidget {
    char pad[0x188 - 4];
};

// engine mesh + resource manager
namespace rgl { struct mesh { rgl::mesh* clone(); }; } // 0x41c6d0
using rgl::mesh;
struct resmgr {
    mesh* get_object(const tstring& name);              // 0x419540
};
extern resmgr g_resmgr;                                 // 0x5d6fa0

void* alloc_53545e(unsigned size);                      // 0x53545e  operator new
struct ctor_obj { void ctor(); };                       // 0x48ae20  base ctor (thiscall)
inline void ctor_48ae20(void* p) { ((ctor_obj*)p)->ctor(); }
extern char g_vt_591d58[];                              // 0x591d58
extern float g_590d94;                                  // 0x590d94 (0.6f)

// @ 0x509770
UiButton* make_button(UiWidget* parent, const tstring* text, float x, float y, int n, float s)
{
    UiButton* b = (UiButton*)alloc_53545e(0x188);
    if (b != 0) {
        ctor_48ae20(b);
        *(int*)((char*)b + 0x178) = 0;
        *((char*)b + 0x121) = 0;
        *(void**)b = (void*)g_vt_591d58;
        *((char*)b + 0x180) = 0;
        *(int*)((char*)b + 0x184) = 0;
    } else
        b = 0;

    {
        float v[2];
        v[0] = x;
        v[1] = y;
        b->set_xy(v);

        v[0] = n * s;
        v[1] = s;
        b->set_scale(v);
    }

    tstring up("button");
    up.push_back((char)('0' + n));
    {
        tstring sfx("_up");
        up.append(sfx);
    }

    tstring down("button");
    down.push_back((char)('0' + n));
    {
        tstring sfx("_down");
        down.append(sfx);
    }

    tstring hl("button");
    hl.push_back((char)('0' + n));
    {
        tstring sfx("_hl");
        hl.append(sfx);
    }

    b->add_image(2, g_resmgr.get_object(up)->clone());
    b->add_image(1, g_resmgr.get_object(down)->clone());
    b->add_image(3, g_resmgr.get_object(hl)->clone());

    {
        float sz = s * 0.6f;
        float v[4];
        v[0] = sz;
        v[1] = sz;
        b->set_size(v);

        b->set_text((tstring*)v, text);
    }

    parent->add_child(b);
    return b;
}
