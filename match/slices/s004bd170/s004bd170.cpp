// Slice s004bd170: ambient particle rendering (0x4bd170), screen->world ray (0x4bd2f0), the ini-file
// reader (0x4bd4d0..0x4bd630), the "key|value" writer (0x4bd740), the trigger block copy read
// (0x4bd7a0) and the two trigger-record readers (0x4bd830/0x4bd910).
// Names are provisional; layouts were derived from the disassembly of this slice.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <new>             // declares operator new[]: without it cl calls operator new (0x53545e) for new Elem[n]

void mb_warning(const char* fmt, ...);                      // 0x42cb80 (MessageBox "RGL WARNING")

// ---------------------------------------------------------------------------------------------------------
// tstring (16 bytes): read cursor, heap buffer, capacity, length.
struct tstring {
    int pos;                                                // +0
    char* buf;                                              // +4
    int cap;                                                // +8
    int len;                                                // +0xc
    tstring();                                              // 0x42bbd0
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
    void operator=(const tstring& o);                       // 0x42bc50
    void clear();                                           // 0x42be50
    void push_back(char c);                                 // 0x42be70
    bool equals(const tstring& o) const;                    // 0x42bdd0
    void to_lower();                                        // 0x42c020
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---------------------------------------------------------------------------------------------------------
// rgl frame (s/f/u basis rows and origin o).
struct vector4 {
    float x, y, z, w;
};
struct matrix {
    vector4 s, f, u, o;
};
matrix identity_pos();                                      // 0x40cd00 (cdecl, hidden return pointer)

extern float g_mouse_x;                                     // 0x5d7094
extern float g_mouse_y;                                     // 0x5d7098

// A particle mesh; render() is rgl::mesh::render. The per-particle colour lives at +0x3c.
struct part {
    char     pad0[0x3c];
    unsigned color;                                         // +0x3c
    char     pad40[0x260 - 0x40];
    void render(void* pass, const matrix* m);               // 0x413750
};

// World/sky record owning the 32-slot particle rings (0x260 bytes per slot).
struct weather {
    char     pad0[0x171c];
    float    fade;                                          // +0x171c
    int      cur;                                           // +0x1720
    char     pad1[0x1728 - 0x1724];
    part     first[32];                                     // +0x1728
    part     second[32];                                    // +0x6328
    float    life[32];                                      // +0xaf28
    float    delta[32];                                     // +0xafa8
    char     pad2[4];
    unsigned colors[32];                                    // +0xb02c
    void update(void* pass, const float* depth);            // 0x4bd170
};

// @ 0x4bd170
void weather::update(void* pass, const float* depth)
{
    matrix m = identity_pos();
    float alpha = 0.2f - fade;
    m.o.x = 0.002f;
    m.o.z = *depth;
    for (int i = 0; i < 32; i++) {
        int idx = (i + cur + 32) % 32;
        if (life[idx] < 10.0f) {
            alpha += delta[idx];
            m.o.y = alpha;
            unsigned col = colors[idx];
            int a = (int)((1.0f - life[idx] * 0.1f) * 256.0f);
            if (a < 0)
                a = 0;
            else if (a >= 255)
                a = 254;
            int b = (int)((unsigned)(255 - a) * 2 / 3);
            if (b < 0)
                b = 0;
            else if (b >= 255)
                b = 254;
            first[idx].color = (a << 24) | (col & 0xffffff);
            second[idx].color = ((b | 0xffffff00) << 8 | b) << 8 | b;
            first[idx].render(pass, &m);
            second[idx].render(pass, &m);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------
// Scene/camera record: viewport rectangle, ortho flag and the view matrix.
struct camera {
    char     pad0[0xe0];
    float    vx;                                            // +0xe0
    float    vy;                                            // +0xe4
    char     pad2[0xf0 - 0xe8];
    float    vw;                                            // +0xf0
    float    vh;                                            // +0xf4
    char     pad3[0x1ac - 0xf8];
    char     ortho;                                         // +0x1ac
    char     pad4[0x274 - 0x1ad];
    matrix   view;                                          // +0x274
    float    zb4;                                           // +0x2b4
    float    zb8;                                           // +0x2b8
    float    zbc;                                           // +0x2bc
    float    zc0;                                           // +0x2c0
    float    zc4;                                           // +0x2c4
    float    zc8;                                           // +0x2c8
    void screen_to_world(vector4* near_pt, vector4* far_pt); // 0x4bd2f0
};

// @ 0x4bd2f0
void camera::screen_to_world(vector4* near_pt, vector4* far_pt)
{
    float sx = (g_mouse_x - vx) / vw;
    float sy = (g_mouse_y - vy) / vh;
    float px = (zb8 - zb4) * sx + zb4;
    float py = (zbc - zc0) * sy + zc0;
    float z = -zc4;
    float ox, oy;
    if (ortho) {
        float k = 1.0f / zc4;
        ox = k * zc8 * px;
        oy = k * zc8 * py;
    }
    float z2 = -zc8;
    vector4 p;
    p.x = z * view.u.x + py * view.f.x + px * view.s.x + view.o.x;
    p.y = z * view.u.y + py * view.f.y + px * view.s.y + view.o.y;
    p.z = z * view.u.z + py * view.f.z + px * view.s.z + view.o.z;
    *near_pt = p;
    far_pt->x = z2 * view.u.x + oy * view.f.x + ox * view.s.x + view.o.x;
    far_pt->y = z2 * view.u.y + oy * view.f.y + ox * view.s.y + view.o.y;
    far_pt->z = z2 * view.u.z + oy * view.f.z + ox * view.s.z + view.o.z;
    far_pt->w = p.w;
}

// ---------------------------------------------------------------------------------------------------------
// Ini file: a fixed 0x1000-entry table, each entry 0x2c bytes (key, 12 unknown bytes, value).
struct ini_entry {
    tstring key;                                            // +0
    char    pad10[0xc];                                     // +0x10
    tstring value;                                          // +0x1c
};

struct ini_file {
    ini_entry entries[0x1000];                              // +0
    int       count;                                        // +0x2c000
    void load(FILE* f);                                     // 0x4bd630
    int  find(const tstring& key);                          // 0x4bd5a0
    float get_float(const tstring& key);                    // 0x4bd5e0
    void parse_line(const char* line, tstring& key, tstring& value);  // 0x4bd4d0
    void write(FILE* f, const tstring& key, const tstring& value);   // 0x4bd740
};

// @ 0x4bd4d0
void ini_file::parse_line(const char* line, tstring& key, tstring& value)
{
    int i = 0;
    key.clear();
    value.clear();
    int len = (int)strlen(line);
    int state = 0;
    for (; i < len; i++) {
        char c = line[i];
        if (c <= ' ' || c == '=') {
            if (state == 1) {
                state = 2;
            } else if (state == 3) {
                state = 4;
            }
        } else if (state == 0) {
            state = 1;
            key.push_back(c);
        } else if (state == 2) {
            state = 3;
            value.push_back(c);
        } else if (state == 1) {
            key.push_back(c);
        } else if (state == 3) {
            value.push_back(c);
        }
    }
}

// @ 0x4bd5a0
int ini_file::find(const tstring& key)
{
    int found = -1;
    int i = 0;
    do {
        if (i >= count)
            break;
        if (entries[i].key.equals(key))
            found = i;
        i++;
    } while (found == -1);
    return found;
}

// @ 0x4bd5e0
float ini_file::get_float(const tstring& key)
{
    float ret = 0.0f;
    int idx = find(key);
    if (idx >= 0)
        ret = (float)atof(entries[idx].value.c_str());
    return ret;
}

// @ 0x4bd630
void ini_file::load(FILE* f)
{
    tstring key;
    tstring value;
    count = 0;
    while (!feof(f)) {
        char line[256];
        int i = 0;
        do {
            char c = (char)fgetc(f);
            line[i] = c;
            i++;
            if (feof(f) || c == '\n')
                break;
        } while (i < 250);
        line[i] = '\0';
        if (line[0] != '#') {
            if (count < 0x1000) {
                parse_line(line, key, value);
                key.to_lower();
                entries[count].key = key;
                entries[count].value = value;
                count++;
            } else {
                mb_warning("Too many lines in Ini file. Ignoring...");
            }
        }
    }
}

// @ 0x4bd740
void ini_file::write(FILE* f, const tstring& key, const tstring& value)
{
    fputs(key.c_str(), f);
    fputs("|", f);
    fputs(value.c_str(), f);
    fputs("\n", f);
}

// ---------------------------------------------------------------------------------------------------------
// Stream interface: only the read slot used here (+0x38) is verified.
struct stream_base {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual stream_base& r_bool(void* p);                   // +0x38
};

struct Elem {
    char data[0x98];
    Elem();                                                 // 0x4607d0
};

struct Block {
    Elem*   data;                                           // +0
    int     count;                                          // +4
    char    loaded;                                         // +8
    char    pad9[3];
    tstring name;                                           // +0xc
    Block(const Block& o);                                  // 0x4bd7a0
    void read(stream_base& s);                              // 0x4607e0
};

// @ 0x4bd7a0
Block::Block(const Block& o) : name()
{
    count = o.count;
    data = 0;
    loaded = 0;
    if (count) {
        data = new Elem[count];
        for (int i = 0; i < count; i++)
            data[i] = o.data[i];
    }
}

// Trigger record: three flags followed by the condition/consequence blocks.
struct trigger_rec {
    char  f0;                                               // +0
    char  pad1[3];
    char  f4;                                               // +4
    char  pad5[3];
    char  f8;                                               // +8
    char  pad9[7];
    Block cond;                                             // +0x10
    Block cons;                                             // +0x2c
    void read(stream_base& s, const char* name, int idx);   // 0x4bd830
};

// @ 0x4bd830
void trigger_rec::read(stream_base& s, const char* name, int idx)
{
    s.r_bool(&f0);
    s.r_bool(&f4);
    s.r_bool(&f8);
    cond.read(s);
    cons.read(s);
    char buf[512];
    _snprintf(buf, 0x200, "%s trigger no: %d conditions", name, idx);
    {
        tstring tmp(buf);
        cond.name = tmp;
    }
    _snprintf(buf, 0x200, "%s trigger no: %d consequences", name, idx);
    {
        tstring tmp(buf);
        cons.name = tmp;
    }
}

// Trigger record with a single block.
struct trigger_entry {
    char  f0;                                               // +0
    char  pad1[3];
    Block block;                                            // +4
    void read(stream_base& s, const char* name, int idx);   // 0x4bd910
};

// @ 0x4bd910
void trigger_entry::read(stream_base& s, const char* name, int idx)
{
    s.r_bool(&f0);
    block.read(s);
    char buf[512];
    _snprintf(buf, 0x200, "%s trigger no: %d", name, idx);
    {
        tstring tmp(buf);
        block.name = tmp;
    }
}
