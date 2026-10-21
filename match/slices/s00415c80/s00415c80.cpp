// Slice s00415c80: text-stream float/stream readers, stream (de)serialisers for rgl matrices, the noise hash,
// the rgl_config.txt writer and a skeleton writer.
#include "mbstream.h"
#include "../../include/rgl.h"
#include <stdlib.h>
#include <stdio.h>

extern unsigned char g_noise_perm[];   // 0x5a8160 permutation table

// 0x54a85c "Unexpected End of File while reading file: %s"
#define EXPECT_TOKEN() \
    do { if (!buf.has_token()) mb_error("Unexpected End of File while reading file: %s", name.c_str()); } while (0)

// @ 0x415c80
stream_base& text_stream::r_float(float* v)
{
    char tok[0x2000];
    int len;
    EXPECT_TOKEN();
    buf.get_token(tok, &len);
    *v = (float)atof(tok);
    return *this;
}

// @ 0x415cf0
// Reads a length, then that many raw bytes of this stream's buffer into o's buffer.
stream_base& text_stream::r_stream(stream_base* o)
{
    int n;
    r_int(&n);
    o->reset();
    if (n) {
        for (int i = 0; i < n; i++) {
            char c;
            buf.read(&c, 1);
            o->buf.push_back(c);
        }
    }
    return *this;
}

// @ 0x415d60
// Like open_read, but opens in text mode and takes the byte count fread returns.
bool text_stream::open_read_text(const char* path)
{
    name.assign(path);
    fp = fopen(path, "r");
    if (fp)
        failed = 0;
    else
        failed = 1;
    if (!failed) {
        size_t n = mb_file_size(fp);
        buf.read_from(fp, n);
        fclose(fp);
    }
    return !failed;
}

// three floats of one row, through a pointer
static inline void w3(stream_base& s, float* p) { s.w_float(&p[0]).w_float(&p[1]).w_float(&p[2]); }
static inline void r3(stream_base& s, float* p) { s.r_float(&p[0]).r_float(&p[1]).r_float(&p[2]); }

// @ 0x415df0
// Writes the 3x3 rotation part (rows s, f, u as 3 floats each).
stream_base& w_rotation(stream_base& s, rgl::matrix* m)
{
    w3(s, &m->s.x);
    w3(s, &m->f.x);
    w3(s, &m->u.x);
    return s;
}

// @ 0x415e70
stream_base& r_rotation(stream_base& s, rgl::matrix* m)
{
    r3(s, &m->s.x);
    r3(s, &m->f.x);
    r3(s, &m->u.x);
    return s;
}

// @ 0x415ef0
stream_base& w_matrix(stream_base& s, rgl::matrix* m)
{
    w3(w_rotation(s, m), &m->o.x);
    return s;
}

// @ 0x415f30
stream_base& r_matrix(stream_base& s, rgl::matrix* m)
{
    r3(r_rotation(s, m), &m->o.x);
    return s;
}

// @ 0x415f70
// Hash of an int via the 0x5a8160 permutation table: one lookup per byte of (x + 0x4ae3).
unsigned noise_hash(int x)
{
    unsigned a = x + 0x4ae3;
    unsigned b = g_noise_perm[(a >> 8) & 0xff];
    return (((b << 8 | b) << 8) | b) << 8 | g_noise_perm[a & 0xff];
}

// Writes one "name = value" line (0x54a904 "%s = %d\n", 0x54a900 "%s\n").
#define WRITE_OPT(f, name, val) \
    do { _snprintf(line, 0x100, "%s = %d\n", name, val); fprintf(f, "%s\n", line); } while (0)

// The engine's rgl_config.txt option block (ints; most are booleans).
struct render_config {
    int v[0x1b];
    void write(FILE* f);                       // 0x415fb0
};

// @ 0x415fb0
void render_config::write(FILE* f)
{
    char line[0x100];
    int* c = v;
    WRITE_OPT(f, "first_time", c[0]);
    WRITE_OPT(f, "texture_detail", c[1]);
    WRITE_OPT(f, "render_buffer_size", c[18]);
    WRITE_OPT(f, "max_framerate", c[3]);
    WRITE_OPT(f, "start_windowed", (c[4] != 0));
    WRITE_OPT(f, "use_pixel_shaders", (c[14] != 0));
    WRITE_OPT(f, "use_vertex_shaders", (c[13] != 0));
    WRITE_OPT(f, "fake_reflections", (c[11] != 0));
    WRITE_OPT(f, "show_framerate", (c[2] != 0));
    WRITE_OPT(f, "disable_music", (c[15] != 0));
    WRITE_OPT(f, "disable_sound", (c[16] != 0));
    WRITE_OPT(f, "disable_frequency_variation", (c[12] != 0));
    WRITE_OPT(f, "cheat_mode", (c[9] != 0));
    WRITE_OPT(f, "enable_blood", (c[10] != 0));
    WRITE_OPT(f, "enable_edit_mode", (c[6] != 0));
    WRITE_OPT(f, "force_single_threading", (c[7] != 0));
    WRITE_OPT(f, "debug_mode", (c[8] != 0));
    WRITE_OPT(f, "display_width", c[19]);
    WRITE_OPT(f, "display_height", c[20]);
    WRITE_OPT(f, "display_bits", c[21]);
    WRITE_OPT(f, "antialiasing", c[22]);
    WRITE_OPT(f, "sample_quality", c[23]);
    WRITE_OPT(f, "force_vsync", c[24]);
    WRITE_OPT(f, "shadowmap_quality", c[26]);
}

// Skeleton record: vptr, name (tstring at +4), bone count at +0x18, bone array at +0x1c.
struct bone {                      // 0xdc bytes
    int pad0;
    int parent;                    // +4
    unsigned flags;                // +8
    rgl::matrix frame;             // +0xc
    char pad4c[0x40];
    tstring name;                  // +0x8c
    char pad9c[0xdc - 0x9c];
};
struct skeleton {
    int vptr;
    tstring name;                  // +4
    int pad14;
    int nbones;                    // +0x18
    bone* bones;                   // +0x1c
};

// @ 0x416490
stream_base& w_skeleton(stream_base& s, skeleton* k)
{
    s.w_tstr(&k->name);
    s.w_int(&k->nbones);
    for (int i = 0; i < k->nbones; i++) {
        bone* b = &k->bones[i];
        s.w_int(&b->parent);
        s.w_tstr(&b->name);
        s.w_uint(&b->flags);
        w_matrix(s, &b->frame);
    }
    return s;
}
