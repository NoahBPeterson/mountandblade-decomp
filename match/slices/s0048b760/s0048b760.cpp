// Slice s0048b760: the remaining mission-UI widgets and a character-skill panel.  Three more text/slider
// widgets derive from the MissionUi base of s0048add0 (vptrs 0x592078 / 0x592140 / 0x592208), followed by
// the skill allocation panel whose packed 4-bit ranks live at +0x68.  Names are provisional; offsets are
// read off this slice.
#include <windows.h>
#include <math.h>

// ---- shared helpers / globals (layouts from the neighbouring verified slices) -------------------------
struct tstring {                                    // 0x10 bytes, ctor 0x42bbd0
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    ~tstring();                                     // 0x42bc40
    void push_back(char c);                         // 0x42be70
    void erase(int pos, int n);                     // 0x42bea0
};

// 0x487b00: UTF-8 decoder.  The original passes its arguments in a nonstandard way (tstring* in EDI,
// int* out_len in EDX, buffer and capacity on the stack); here it is a local normal-ABI copy so the callers
// below stay readable and semantically correct.
static __declspec(noinline) void decode_utf8(int* out_len, const tstring* s, int* buf, const int* maxn)
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            if (*out_len >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((((q[0] & 7) << 6) | (q[1] & 0x3f)) << 6 | (q[2] & 0x3f)) << 6) | (q[3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = ((((q[0] & 0xf) << 6) | (q[1] & 0x3f)) << 6) | (q[2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = ((q[0] & 0x1f) << 6) | (q[1] & 0x3f);
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[*out_len] = ch;
            (*out_len)++;
        } while (i < n);
    }
}

extern int g_ticks[];                               // 0x5e2948 (two dwords per clock)
extern int g_5e2930;                                // 0x5e2930 (game data pointer, +0xc8 = screen mode)
extern int g_5d8798;                                // 0x5d8798 (last keyboard character)
extern float g_ui_target[2];                        // 0x5d7094 (slider target positions)
extern void* g_ui_active;                           // 0x5d7090 (widget being dragged)
extern int g_skin_cap;                              // 0x8ee3ac4 (max attribute/skill rank; symbol g_skin_cap)
#define g_skill_max ((int*)0x8ee3134)               // 0x8ee3134 (0x44-byte records, +0 = max rank)

struct SoundMgr {                                   // 0x5d7080
    char pad[0x170c];
    int handle;                                     // +0x170c
};
extern SoundMgr g_sound;                            // 0x5d7080
void __fastcall play_sound(SoundMgr* m);            // 0x48b2d0

// ---- virtual interface seen through the object's vptr ------------------------------------------------
struct UiVt {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17();
    virtual void set_key_flags(int f);              // +0x48
    virtual void d19(); virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23();
    virtual void d24();
    virtual void set_xy(float* v);                  // +0x64
    virtual void d26();
    virtual void set_size(float* v);                // +0x6c
    virtual void set_pos(float* v);                 // +0x70
    virtual void d29(); virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33();
    virtual void d34(); virtual void d35();
    virtual void refresh();                         // +0x90
    virtual void get_xy(float* v);                  // +0x94
    virtual void set_color(unsigned* c);            // +0x98
    virtual void d39(); virtual void d40();
    virtual void get_text_size(tstring* s, float* out);  // +0xa4
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48();
    virtual void update_children();                 // +0xc4
};

struct MissionUi {                                  // 0x178 bytes (see s0048add0)
    int vptr;                                       // 0x000
    int f_4;                                        // 0x004
    int f_8;                                        // 0x008
    int f_c;                                        // 0x00c
    struct Slot { int lo, hi, zero, pad; };
    Slot t[5];                                      // 0x010..0x05f
    int t5lo, t5hi;                                 // 0x060
    float neg[6];                                   // 0x068
    char pad80[0xc0 - 0x80];                        // 0x080
    char f_c0, f_c1, f_c2, f_c3, f_c4, f_c5;        // 0x0c0..0x0c5
    char padc6[2];                                  // 0x0c6
    int f_c8, f_cc, f_d0, f_d4;                     // 0x0c8
    float f_d8, f_dc;                               // 0x0d8
    float f_e0[4];                                  // 0x0e0
    float screen[4];                                // 0x0f0
    int f_100;                                      // 0x100
    unsigned flags104;                              // 0x104
    int f_108, f_10c;                               // 0x108
    int f_110;                                      // 0x110 (child array / target buffer)
    int f_114, f_118;                               // 0x114
    char f_11c, f_11d, f_11e, f_11f, f_120;         // 0x11c..0x120
    char f_121, f_122;                              // 0x121
    char pad123[0x124 - 0x123];                     // 0x123
    int f_124;                                      // 0x124
    tstring name;                                   // 0x128
    float f_138, f_13c, f_140, f_144;               // 0x138
    unsigned f_148;                                 // 0x148
    unsigned f_14c, f_150;                          // 0x14c
    int f_154;                                      // 0x154
    int f_158, f_15c, f_160;                        // 0x158
    int f_164;                                      // 0x164
    float f_168, f_16c, f_170;                      // 0x168
    int f_174;                                      // 0x174

    MissionUi();
    void bounds(int a, int b, int c, int d);        // 0x488720
};

// ---- 0x592078: label/icon widget with a four-float vector and an int parameter -----------------------
struct UiC : MissionUi {
    int f_178;                                      // 0x178
    float f_17c;                                    // 0x17c
    int f_180;                                      // 0x180
    int f_184;                                      // 0x184
    int f_188;                                      // 0x188
    int f_18c;                                      // 0x18c
    char f_190;                                     // 0x190
    char pad191[3];
    float f_194;                                    // 0x194
    float f_198, f_19c, f_1a0, f_1a4;               // 0x198
    int f_1a8;                                      // 0x1a8

    UiC();
    void show(int a, int b, int c, int d);          // 0x48b7f0
    int type();                                     // 0x48b880
};

// ---- 0x592140: progress bar (value in +0x168, tolerance in +0x17c) -----------------------------------
struct UiD : MissionUi {
    int f_178;                                      // 0x178 (index)
    float f_17c;                                    // 0x17c (tolerance)
    int f_180;                                      // 0x180 (dragging)
    float f_184;                                    // 0x184 (grab offset)

    UiD();
    void show(int a, int b, int c, int d);          // 0x48b8d0
    float value();                                  // 0x48b930
    char key(unsigned p);                           // 0x48b960
    void set_value(float x);                        // 0x48baa0
};

// Stand-in for UiD::value as seen from a different translation unit (the real 0x48b930 is out of line and
// its register usage is not visible to these callers in the original build).
struct UiDv { float value(); };
struct UiDsv { void set_value(float x); };

// ---- 0x592208: single-line text editor ---------------------------------------------------------------
struct UiE : MissionUi {
    int f_178;                                      // 0x178
    char pad17c[4];
    char f_180;                                     // 0x180 (editing)
    char f_181;                                     // 0x181 (has selection)
    char pad182[2];
    int f_184;                                      // 0x184 (capacity)

    UiE();
    char key(unsigned p);                           // 0x48bb50
    void update(int a, int b, int c, int d);        // 0x48bda0
};

// ---- skill panel: 8-byte entries at +0, four ranks at +0x58, packed 4-bit ranks at +0x68 ---------------
struct StatEntry {                                  // 0x8 bytes
    int id;
    int val;
    void sort(int n);                               // 0x489090
};

struct StatPanel {
    StatEntry entries[11];                          // 0x000..0x057
    int stats[4];                                   // 0x058
    int packed[4];                                  // 0x068
    char pad78[2];                                  // 0x078
    unsigned char f_7a;                             // 0x07a
    char pad7b[0xa0 - 0x7b];
    float f_a0;                                     // 0x0a0
    int f_a4;                                       // 0x0a4

    void compact(int n);                            // 0x48be00
    void set_stat(int idx, int val);                // 0x48be70
    void add_stat(int idx, int delta);              // 0x48bef0
    void set_skill(int idx, int val);               // 0x48bf90
    int get_packed(int idx);                        // 0x4351a0
    void add_skill(int id, int delta);              // 0x48c090
};

// Stand-ins for the same object's out-of-line methods as seen from these callers in the original build.
struct StatPanelX {
    void set_stat(int idx, int val);
    void set_skill(int idx, int val);
};

// @ 0x48b760 -------------------------------------------------------------------------------
UiC::UiC()
{
    float v[4];
    float* p = v;
    f_198 = p[0] = 1.0f;
    f_19c = p[1] = 1.0f;
    f_1a0 = p[2] = 1.0f;
    f_1a4 = p[3];
    int z = 0;
    f_178 = z;
    f_180 = z;
    f_184 = z;
    f_18c = z;
    f_17c = 0.008f;
    f_168 = 1.0f;
    *(int*)this = 0x592078;
    f_190 = 1;
    f_1a8 = 0xff000000;
    f_194 = 1.0f;
}

// @ 0x48b7f0 -------------------------------------------------------------------------------
void UiC::show(int a, int b, int c, int d)
{
    f_11c = f_11d = f_11e = 1;
    ((UiVt*)this)->update_children();
    if (f_110) {
        int i = f_178;
        float v = (screen[i] - (f_17c + f_17c)) * ((f_168 - f_16c) / (f_170 - f_16c)) + f_17c;
        *(float*)(f_110 + 0xfc + i * 4) = v;
    }
    bounds(a, b, c, d);
}

// @ 0x48b880 -------------------------------------------------------------------------------
int UiC::type() { return 3; }

// @ 0x48b890 -------------------------------------------------------------------------------
UiD::UiD()
{
    f_178 = 0;
    f_180 = 0;
    f_184 = 0;
    *(int*)this = 0x592140;
    f_17c = 0.1f;
    f_168 = 1.0f;
}

// @ 0x48b8d0 -------------------------------------------------------------------------------
void UiD::show(int a, int b, int c, int d)
{
    f_11c = f_11e = 1;
    if (f_110)
        *(float*)(f_110 + 0xfc + f_178 * 4) = ((UiDv*)this)->value();
    bounds(a, b, c, d);
}

// @ 0x48b960 -------------------------------------------------------------------------------
char UiD::key(unsigned p)
{
    char b = 0;
    if (f_180 == 0) {
        if (p != 0x10)
            goto tail;
        b = 1;
        ::play_sound(&g_sound);
        int i = f_178;
        float t = g_ui_target[i];
        float v = ((UiDv*)this)->value() + f_e0[i];
        if (v - 0.01f < t && t < (f_17c + 0.01f) + v) {
            f_180 = 1;
            g_ui_active = this;
            f_184 = t - v;
            goto tail;
        }
        f_184 = 0;
        ((UiDsv*)this)->set_value(t - f_e0[i]);
    } else {
        if (p == 8) {
            g_ui_active = 0;
            b = 1;
            f_180 = 0;
            ((UiVt*)this)->set_key_flags(0x10000);
            goto tail;
        }
        ((UiDsv*)this)->set_value((g_ui_target[f_178] - f_184) - f_e0[f_178]);
    }
    ((UiVt*)this)->set_key_flags(0x10000);
tail:
    ((UiVt*)this)->set_key_flags(p);
    ((UiVt*)this)->refresh();
    return b;
}

// @ 0x48baa0 -------------------------------------------------------------------------------
void UiD::set_value(float x)
{
    float v = (f_170 - f_16c) * (x / (screen[f_178] - f_17c)) + f_16c;
    f_168 = v;
    if (v < f_16c) {
        f_168 = f_16c;
        return;
    }
    if (f_170 < v)
        f_168 = f_170;
}

// @ 0x48b930 -------------------------------------------------------------------------------
float UiD::value()
{
    return (screen[f_178] - f_17c) * ((f_168 - f_16c) / (f_170 - f_16c));
}

// @ 0x48bb10 -------------------------------------------------------------------------------
UiE::UiE()
{
    f_178 = 0;
    f_121 = 0;
    f_180 = 0;
    f_181 = 0;
    *(int*)this = 0x592208;
    f_184 = 0x1c;
}

// @ 0x48bb50 -------------------------------------------------------------------------------
char UiE::key(unsigned p)
{
    char b = 0;
    if (f_180) {
        if (p == 0x1000) {
            char buf[0x2000];
            int out_len;
            int maxn = 0x800;
            b = 1;
            decode_utf8(&out_len, &name, (int*)buf, &maxn);
            int ch = g_5d8798;
            if (ch >= 0x20 && out_len < f_184) {
                char u[4];
                int n;
                if (ch < 0x80) {
                    u[0] = (char)ch;
                    n = 1;
                } else if (ch < 0x7ff) {
                    u[0] = (char)((ch >> 6) | 0xc0);
                    u[1] = (char)((ch & 0x3f) | 0x80);
                    n = 2;
                } else {
                    u[0] = (char)((ch >> 12) | 0xe0);
                    u[1] = (char)(((ch >> 6) & 0x3f) | 0x80);
                    u[2] = (char)((ch & 0x3f) | 0x80);
                    n = 3;
                }
                for (int i = 0; i < n; i++)
                    name.push_back(u[i]);
            }
            ((UiVt*)this)->get_xy((float*)&out_len);
            if (g_5d8798 == 0xd) {
                f_180 = 0;
                b = 1;
                g_ui_active = 0;
                ::play_sound(&g_sound);
                ((UiVt*)this)->set_key_flags(0x10000);
            } else if (g_5d8798 == 8) {
                if (name.len > 0) {
                    int i = name.len - 1;
                    int n = 1;
                    while (i > 0 && (name.buf[i] & 0xc0) == 0x80 && n < name.len) {
                        i--;
                        n++;
                    }
                    name.erase(name.len - n, n);
                }
                ((UiVt*)this)->get_xy((float*)&out_len);
            }
        } else if (p == 0x10) {
            f_180 = 0;
            b = 1;
            g_ui_active = 0;
            ::play_sound(&g_sound);
            ((UiVt*)this)->set_key_flags(0x10000);
        }
    } else if (p == 0x10) {
        f_180 = 1;
        b = 1;
        g_ui_active = this;
        ::play_sound(&g_sound);
        if (f_181) {
            tstring tmp("");
            float sz[2];
            ((UiVt*)this)->get_text_size(&tmp, sz);
            tmp.~tstring();
            unsigned c = 0xff000000;
            ((UiVt*)this)->set_color(&c);
            f_181 = 0;
        }
    }
    ((UiVt*)this)->set_key_flags(p);
    ((UiVt*)this)->refresh();
    return b;
}

// @ 0x48bda0 -------------------------------------------------------------------------------
void UiE::update(int a, int b, int c, int d)
{
    if (f_180 != 0)
        f_178 = 0;
    else
        f_178 = 1;
    f_11f = 0;
    if (f_178 != 0) {
        f_11e = 1;
        f_11d = 0;
        bounds(a, b, c, d);
        return;
    }
    f_11e = 0;
    f_11d = 1;
    bounds(a, b, c, d);
}

// @ 0x48be00 -------------------------------------------------------------------------------
void StatPanel::compact(int n)
{
    if (n <= 0)
        n = 0x60;
    int i = 0;
    while (i < n) {
        if (((StatEntry*)this)[i].id == -1)
            break;
        i++;
    }
    int j = i + 1;
    if (j < n) {
        StatEntry* d = &((StatEntry*)this)[i];
        while (j < n) {
            if (((StatEntry*)this)[j].id != -1) {
                if (j >= n)
                    break;
                d->id = ((StatEntry*)this)[j].id;
                d->val = ((StatEntry*)this)[j].val;
                i++;
                ((StatEntry*)this)[j].id = -1;
                d++;
            }
            j++;
        }
    }
    ((StatEntry*)this)->sort(i);
}

// @ 0x48be70 -------------------------------------------------------------------------------
void StatPanel::set_stat(int idx, int val)
{
    if (val < 0)
        val = 0;
    else if (val > 100)
        val = 100;
    stats[idx] = val;
    int v = f_7a & 0xf;
    int x = v;
    int base = stats[0];
    int m = g_skin_cap;
    if (v < 0)
        x = 0;
    else if (v > m)
        x = m;
    if ((float)(base + 0x23 + x * 2) < f_a0) {
        if (v < 0)
            v = 0;
        else if (v > m)
            v = m;
        f_a0 = (float)(base + 0x23 + v * 2);
    }
}

// @ 0x48bef0 -------------------------------------------------------------------------------
void StatPanel::add_stat(int idx, int delta)
{
    int step = 1;
    if (delta < 0) {
        delta = -delta;
        step = -1;
    }
    if (idx < 0 || idx >= 4 || delta <= 0)
        return;
    int n = delta;
    do {
        int v = stats[idx] + step;
        if (v >= 0 && v < 0x40) {
            if (idx == 2) {
                f_a4 += step;
            } else if (idx == 0) {
                if (step == 1)
                    f_a0 += 1.0f;
                else if (step == -1)
                    f_a0 -= 1.0f;
            }
            int sv = stats[idx] + step;
            ((StatPanelX*)this)->set_stat(idx, sv);
        }
    } while (--n);
}

// @ 0x48bf90 -------------------------------------------------------------------------------
void StatPanel::set_skill(int idx, int val)
{
    if (val < 0)
        val = 0;
    else if (val > g_skill_max[idx * 0x11])
        val = g_skill_max[idx * 0x11];
    int word = idx / 8;
    int shift = (idx % 8) * 4;
    int cur = ((unsigned)packed[word] >> shift) & 0xf;
    if (cur < 0)
        cur = 0;
    else if (cur > g_skill_max[idx * 0x11])
        cur = g_skill_max[idx * 0x11];
    if (val > cur && idx == 0x24)
        f_a0 += (float)((val - cur) * 2);
    packed[word] = (packed[word] & ~(0xf << shift)) | ((val & 0xf) << shift);
    int v = f_7a & 0xf;
    int m;
    int base = stats[0];
    m = g_skin_cap;
    int x = v;
    if (v < 0)
        x = 0;
    else if (v > m)
        x = m;
    if ((float)(base + 0x23 + x * 2) < f_a0) {
        if (v < 0)
            v = 0;
        else if (v > m)
            v = m;
        f_a0 = (float)(base + 0x23 + v * 2);
    }
}

// @ 0x48c090 -------------------------------------------------------------------------------
void StatPanel::add_skill(int id, int delta)
{
    if (id < 0 || id >= 0x2a)
        return;
    ((StatPanelX*)this)->set_skill(id, get_packed(id) + delta);
}
