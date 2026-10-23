// Slice s0049b630: six Mission methods from one translation unit.
//   0x49b630  rebuild the mission's UI "font mesh" (material lookup + layout_text)
//   0x49b720  add "value" to an army slot and, for slot 0, log the gained amount
//   0x49b820  subtract "value" from an army slot (clamped at 0) and log the lost amount
//   0x49b930  mark a quest record as taken (write +0x118/+0x11c, log ui_new_quest_taken)
//   0x49baa0  layout one text row into a mesh with the non-wrap/scale preset
//   0x49bbb0  spawn the battle icon for one 0x98-byte army record
// Names and field names are provisional; every offset is read off the disassembly.
// The class layout is shared with the neighbouring slices s0049bea0 / s0049c2c0 (Mission).
#include <vector>
#include "rgl.h"
#include "tstring.h"

namespace rgl {
struct mesh {                                       // 0x260 bytes (see s004520d0)
    char pad[0x260];
    void clear_geometry();                          // 0x428e80
    void compute_normals();                         // 0x451c50
    mesh* clone();                                  // 0x41c6d0
};
}

using rgl::vector4;
using rgl::matrix;
using rgl::mesh;

// ---- engine tables / globals ---------------------------------------------------------------------
struct StrTable { tstring* lookup(const tstring& key); };   // 0x43a1e0
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };        // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

matrix* identity_pos(matrix* out);                  // 0x40cd00 (cdecl; returns out)
extern int g_sound_holder[];                        // 0x5aa128 (sound device)
extern float g_sound_origin[];                      // 0x5aa168 (position vector for play)
extern int g_snd_got_value;                         // 0x5aa1a0 (sound id)
extern int g_snd_lost_value;                        // 0x5aa19c
extern int g_snd_new_quest;                         // 0x5aa180
struct SoundMgr { int play(int idx, int flags, const float* pos, float freq, int* out); };  // 0x442650

struct ResMgr {
    void*  get_material(const tstring& name);       // 0x418a70
    mesh*  get_object(tstring* name);               // 0x419540 (ret 4)
};
extern ResMgr g_resmgr;                             // 0x5d6fa0

struct Font {
    void layout_text(float* out, const tstring* text, const vector4* pos_in, const float* scale_in,
                     mesh* m, int arg, const float* xmin, const float* xmax, unsigned flags);   // 0x4520d0 ret 0x24
};
extern Font* g_font;                                // 0x5e2934

struct TickPair {                                   // g_ticks[i].lo + g_ticks[i].hi * 2^32
    int lo, hi;
    operator __int64() const { return ((long long)hi << 32) | (unsigned)lo; }
};
extern TickPair g_ticks[];                          // 0x5e2948

extern "C" int __cdecl sprintf(char*, const char*, ...);        // 0x536f8f
void mb_snprintf(char* buf, unsigned n, const char* fmt, ...);  // 0x415100
void* operator new(unsigned size);                  // 0x53545e

// ---- scene objects -------------------------------------------------------------------------------
struct Shape { virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
               virtual void v4(); virtual void v5(); virtual void v6(); };
struct SphereMem { Shape* construct(vector4 pos, const float* radius); };   // 0x4332a0 (thiscall, ret 0x14, ecx = raw memory)

struct RotFrame { void rotate_sf(const float* angle); };   // 0x40e770 (thiscall): rotates the side/forward rows

// a scene entity (only the offsets this slice touches are named)
struct EntityFull {
    char pad0[0xc];
    int      f0c;                                   // +0xc   (id)
    char pad10[0x58 - 0x10];
    matrix   frame;                                 // +0x58 (frame.o at +0x88 is the position)
    Shape*   shape;                                 // +0x98
    char pad9c[0x11c - 0x9c];
    void**   f11c;                                  // +0x11c (first dword -> matrix at +0xcc)
    char pad120[0x168 - 0x120];
    int      f168;                                  // +0x168  (end fall behaviour = 3)
    void add_mesh(mesh* m);                         // 0x442110 (thiscall, ret 4)
};
struct EntityHost { EntityFull* create_entity(int type); };   // 0x428440 (ret 4)
extern EntityHost* g_entity_host;                   // 0xa732b4 (points to the host object)


// ---- records ---------------------------------------------------------------------------------------
// 0x98-byte army/settlement record (mission+0xe4 page table; see 0x49bbb0)
struct Rec98 {
    char    pad0[0x10];
    int     f10;                                    // +0x10  (=1 -> icon owner present)
    char    pad14[0x24 - 0x14];
    float   f24;                                    // +0x24  (x)
    float   f28;                                    // +0x28  (y)
    char    pad2c[0x3c - 0x2c];
    float   f3c;                                    // +0x3c  (z base)
    char    pad40[0x50 - 0x40];
    int     f50;                                    // +0x50  (scene entity)
    int     f54;                                    // +0x54  (agent id A)
    int     f58;                                    // +0x58  (agent id B)
    void bind_host();                               // 0x4b2770 (thiscall)
};
// 0x138-byte quest record (mission+0x13a58)
struct Rec138 {
    char    pad0[0x40];
    tstring text;                                   // +0x40
    int     f50;                                    // +0x50  (bit 1 -> "new")
    char    pad54[0x64 - 0x54];
    int     f64;                                    // +0x64  (0x100 = already announced)
    char    pad68[0x118 - 0x68];
    int     f118;                                   // +0x118 (taken flag)
    float   f11c;                                   // +0x11c (game time when taken)
};
// 0x5ac8-byte order record: the fields 0x49bbb0 stamps on the two participating agents
struct Rec5ac8 {
    char     pad0[0x88];
    int      pmid;                                  // +0x88  (order icon id)
    char     pad8c[0xa0 - 0x8c];
    unsigned __int64 flags;                         // +0xa0  (0x400 = siege)
    char     pada8[0x918 - 0xa8];
    int      icon_type;                             // +0x918 (7 = battle icon)
    char     pad91c[0x5ac8 - 0x91c];
};

struct AgentTable {
    char    pad0[0x138cc];
    std::vector<char*> pages;                       // +0x138cc, relative to the table (page pointers, 16 records per page)
    int     index_of(int id);                       // 0x48ad00 (ret 4)
    Rec5ac8* get(int id) { int a = index_of(id); return (Rec5ac8*)((a & 0xf) * 0x5ac8 + *(pages.begin() + (a >> 4))); }
};

// ---- Mission -------------------------------------------------------------------------------------
struct Mission {
    char        pad000[0xe0];
    std::vector<char*> f0e0;                        // +0xe0  page table of Rec98 records
    char        pad0f0[0xf8 - 0xf0];
    AgentTable  agents;                             // +0xf8 (0x138d4; pages at +0x139c8)
    char        pad1[0x10000 - 0x6a0];
    int         f132c;                              // +0x132c (font mesh active)
    char        pad1b[0x13a00 - 0x132c - 4];
    char*       f13a00;                             // +0x13a00  array of 0x508 records
    char        pad2[0x13a34 - 0x13a04];
    int         f13a34;                             // +0x13a34
    char        pad3[0x13a58 - 0x13a38];
    char*       f13a58;                             // +0x13a58  array of Rec138 records
    char        pad4[0x13acc - 0x13a5c];
    float       f13acc;                             // +0x13acc
    char        pad5[0x13b18 - 0x13ad0];
    int         f13b18;                             // +0x13b18
    char        pad6[0x13b20 - 0x13b1c];
    int         f13b20;                             // +0x13b20 (clock low)
    int         f13b24;                             // +0x13b24 (clock high)
    char        pad7[0xc58];
    mesh        f1480;                              // +0x1480 (0x260)
    char        pad8[0x158c - 0x16e0 + 0x10000];
    int         f158c;                              // +0x158c (font mesh visible)

    tstring string_value(int index);                // 0x48e9e0 (ret 4)
    mesh* make_label(EntityFull* e);                // 0x495b20 (ret 4)

    void set_font_mesh(const tstring* text, int arg);                       // 0x49b630
    void add_value(int amount, int group);                                  // 0x49b720
    void sub_value(int amount, int group);                                  // 0x49b820
    void quest_taken(int index, int value);                                 // 0x49b930
    void add_item(mesh* m, const tstring* text, char flag, int arg, const float* h);   // 0x49baa0
    void spawn_battle_icon(unsigned id);                                    // 0x49bbb0
};
extern Mission* g_mission;                          // 0x5e2930

// ---- shared log tail of 0x49b720 / 0x49b820 (inlined in both; the source of the original
// likely had a static helper, but cl 13.10 inlines it at /O2 regardless of `noinline`) ----------
static void value_message(Mission* m, int amount, const char* key, int sound_id)
{
    tstring s(key);
    tstring val = m->string_value(amount);
    const char* valuestr = val.buf;
    if (val.buf == 0)
        valuestr = (const char*)&val.buf;
    tstring* f = g_strtable.lookup(s);
    const char* fmt = f->buf;
    if (f->buf == 0)
        fmt = (const char*)&f->buf;
    char buf[0x200];
    buf[0] = 0;
    sprintf(buf, fmt, valuestr);
    val.~tstring();
    s.~tstring();
    g_msglog.add(tstring(buf), -1);
    ((SoundMgr*)g_sound_holder)->play(sound_id, 1, g_sound_origin, 1.0f, 0);
}

// @ 0x49b630
void Mission::set_font_mesh(const tstring* text, int arg)
{
    tstring name("ui_font");
    *(void**)((char*)&f1480 + 0x2c) = g_resmgr.get_material(name);
    name.~tstring();
    f1480.clear_geometry();
    if (text->len <= 0) {
        *(int*)((char*)&f1480 + 0x10c) = 0;
        f132c = 0;
        return;
    }
    float   out[2];
    vector4 pos;
    float   scale[2];
    float   xmin = -0.35f;
    float   xmax = 0.35f;
    arg = arg;
    pos.x = -0.35f; pos.y = 0.0f; pos.z = 0.0f; pos.w = 0.0f;
    scale[0] = 0.017f; scale[1] = 0.017f;
    g_font->layout_text(out, text, &pos, scale, &f1480, (int)&arg, &xmin, &xmax, 0x100);
    *(int*)((char*)&f1480 + 0x10c) = 1;
    f132c = 1;
}

// @ 0x49b720
void Mission::add_value(int amount, int group)
{
    char* rec = f13a00 + group * 0x508;
    *(int*)(rec + 0x3b4) += amount;
    if (group == 0 && f13a34 != 0 && amount != 0)
        value_message(this, amount, "ui_you_got_value", g_snd_got_value);
}

// @ 0x49b820
void Mission::sub_value(int amount, int group)
{
    char* rec = f13a00 + group * 0x508;
    int v = *(int*)(rec + 0x3b4) - amount;
    *(int*)(rec + 0x3b4) = v;
    if (v < 0)
        *(int*)(rec + 0x3b4) = 0;
    if (group == 0 && f13a34 != 0 && amount != 0)
        value_message(this, amount, "ui_you_lost_value", g_snd_lost_value);
}

// @ 0x49b930
void Mission::quest_taken(int index, int value)
{
    Rec138* rec = (Rec138*)(f13a58 + index * 0x138);
    rec->f118 = value;
    const TickPair& t = g_ticks[f13b18];
    rec->f11c = (float)((long long)t - ((long long)f13b24 << 32 | (unsigned)f13b20)) * 1e-5f;
    if ((rec->f50 & 2) != 0 || (rec->f64 & 0x100) == 0) {
        tstring name(rec->text);
        const char* n = name.buf;
        if (name.buf == 0)
            n = (const char*)&name.buf;
        tstring* f = g_strtable.lookup(tstring("ui_new_quest_taken"));
        const char* fmt = f->buf;
        if (f->buf == 0)
            fmt = (const char*)&f->buf;
        char buf[0x200];
        buf[0] = 0;
        mb_snprintf(buf, 0x200, fmt, n);
        name.~tstring();
        g_msglog.add(tstring(buf), 0xffd0f0c8);
        ((SoundMgr*)g_sound_holder)->play(g_snd_new_quest, 1, g_sound_origin, 1.0f, 0);
        rec->f64 = 0x100;
    }
}

// @ 0x49baa0
void Mission::add_item(mesh* m, const tstring* text, char flag, int arg, const float* h)
{
    unsigned flags = 0;
    vector4 pos;
    pos.x = 0.05f; pos.y = 0.1f; pos.z = 0.2f; pos.w = 0.3f;
    float   scale[2];
    scale[0] = 0.2f; scale[1] = 0.2f;
    float   xmin = 0.0f;
    float   xmax = 1.0f;

    if (flag) {
        xmax = f13acc * *h;
        xmin = -xmax;
        scale[0] = xmax * 0.6f;
        scale[1] = xmax * 0.6f;
        pos.x = 0.0f; pos.y = 0.1f; pos.z = 0.2f; pos.w = 0.3f;
        flags = 0x10;
    }
    float   out[2];
    arg = arg;
    m->clear_geometry();
    g_font->layout_text(out, text, &pos, scale, m, (int)&arg, &xmin, &xmax, flags);
}

// @ 0x49bbb0
void Mission::spawn_battle_icon(unsigned id)
{
    Rec98* rec = (Rec98*)((id & 0xf) * 0x98 + *(f0e0.begin() + (id >> 4)));
    EntityFull* e = g_entity_host->create_entity(9);
    rec->f50 = (int)e;

    Shape* shape;
    void* mem = operator new(0x44);
    if (mem != 0) {
        float radius = 0.3f;
        vector4 zero;
        zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        shape = ((SphereMem*)mem)->construct(zero, &radius);
    } else
        shape = 0;
    e->shape = shape;

    {
        matrix tmp;
        e->frame = *identity_pos(&tmp);
    }
    rec->bind_host();

    if (rec->f10 == 1) {
        agents.get(rec->f54)->icon_type = 7;
        agents.get(rec->f58)->icon_type = 7;
        agents.get(rec->f54)->pmid = id;
        agents.get(rec->f58)->pmid = id;
    }

    if (rec->f10 == 1) {
        {
            tstring nm("battle_icon");
            e->add_mesh(g_resmgr.get_object(&nm)->clone());
        }
        {
            float angle = 1.5707963f;
            ((RotFrame*)((char*)e->f11c[0] + 0xcc))->rotate_sf(&angle);
        }

        mesh* label = make_label(e);

        {
            float h = 1.0f;
            tstring kn((agents.get(rec->f58)->flags & 0x400) ? "ui_siege" : "ui_battle");
            add_item(label, g_strtable.lookup(kn), 1, 1, &h);
        }

        e->frame.o = vector4(rec->f24, rec->f28, rec->f3c + 0.4f);
    }
    e->f0c = id;
    e->f168 = 3;
}
