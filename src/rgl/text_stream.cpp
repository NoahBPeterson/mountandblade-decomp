// text_stream.cpp (0x4150f0..0x4158ff): the out-of-line named_object destructor, small file helpers, the
// stream root's own slots and the text stream's open/close and write slots. The text stream's read slots
// follow in the next object (0x415900.., not migrated): text_stream::r_tstr (0x415880) calls 0x415900 forward.
// Migrated from slices s00414960 (0x4150f0..0x415290) and s00415320 (0x415320..0x415880).
#include <windows.h>
#include <shlobj.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#define RGL_NAMED_OBJECT_DTOR_OUT_OF_LINE   // this object holds the out-of-line copy of ~named_object
#include "./rgl_string.h"
#include "./rgl_math.h"
#include "./stream.h"
#include "./resource.h"
#include "./text_stream.h"

// @ 0x4150f0
named_object::~named_object() {}

// @ 0x415100
void mb_snprintf(char* buf, size_t n, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, n, fmt, ap);
}

// @ 0x415120
bool mb_file_exists(const char* path)
{
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}

// @ 0x415160
FILE* mb_open_user_file(const char* name, const char* mode)
{
    char path[260];
    char folder[260];
    SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, folder);
    sprintf(path, "%s\\Mount&Blade", folder);
    CreateDirectoryA(path, 0);
    sprintf(path, "%s\\Mount&Blade\\%s", folder, name);
    return fopen(path, mode);
}

// @ 0x4151e0
void rgl::string::read_from(FILE* f, size_t n)
{
    reserve(n + 1);
    size_t got = fread(buf, 1, n, f);
    buf[got] = 0;
    len = got;
}

// @ 0x415220
void stream_base::reset()
{
    buf.clear();
    buf.rewind();
}

// @ 0x415240
void stream_base::rewind() { buf.rewind(); }

// @ 0x415250
bool stream_base::at_end() { return buf.pos >= buf.len; }

// @ 0x415270
void stream_base::set_autoflush(char f) { autoflush = f; }

// @ 0x415280
char stream_base::get_autoflush() { return autoflush; }

// 0x408020 (shared by ICF with the other empty `return 0` functions)
int stream_base::flush() { return 0; }

// @ 0x415290
bool text_stream::open_read(const char* path)
{
    name.assign(path);
    fp = fopen(path, "rb");
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

// 0x54a83c " ", 0x54a840 "0 ", 0x54a844 "1 ", 0x54a848 "%f ", 0x54a84c "format broken"

// @ 0x415320
int text_stream::open_write(const char* path)
{
    name.assign(path);
    is_open = 0;
    fp = fopen(path, "wb");
    if (fp) {
        failed = 0;
        is_open = 1;
    } else
        failed = 1;
    return failed ? -1 : 0;
}

// @ 0x415370
int text_stream::close()
{
    int r = 1;
    if (is_open) {
        r = flush();
        if (r == 0)
            r = fclose(fp);
        failed = 1;
        is_open = 0;
    }
    return r;
}

// @ 0x4153a0
stream_base& text_stream::w_cstr(const char* s)
{
    { rgl::string t(s); buf.append(t); }
    { rgl::string t(" "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415410
stream_base& text_stream::w_tstr(const rgl::string* s)
{
    buf.append(*s);
    { rgl::string t(" "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415470
stream_base& text_stream::w_byte(unsigned char* v)
{
    char tmp[128];
    _itoa(*v, tmp, 10);
    { rgl::string t(tmp); buf.append(t); }
    { rgl::string t(" "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415500
// (also slot 7, w_uint: ICF)
stream_base& text_stream::w_int(int* v)
{
    char tmp[128];
    _itoa(*v, tmp, 10);
    { rgl::string t(tmp); buf.append(t); }
    { rgl::string t(" "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415590
stream_base& text_stream::w_i64(__int64* v)
{
    rgl::string s;
    int digits[32];
    bool neg = false;
    __int64 x = *v;
    if (x < 0) {
        x = -x;
        neg = true;
    }
    int n = 0;
    if (x == 0) {
        digits[0] = 0;
        n = 1;
    } else {
        unsigned __int64 u = x;
        while (u > 0) {
            digits[n++] = (int)(u % 10);
            u /= 10;
        }
    }
    if (neg)
        s.push_back('-');
    while (n > 0) {
        s.push_back((char)(digits[n - 1] + '0'));
        n--;
    }
    w_tstr(&s);
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415670
stream_base& text_stream::w_u64(unsigned __int64* v)
{
    rgl::string s;
    int digits[32];
    int n = 0;
    unsigned __int64 u = *v;
    if (u == 0) {
        digits[0] = 0;
        n = 1;
    }
    while (u > 0) {
        digits[n++] = (int)(u % 10);
        u /= 10;
    }
    while (n > 0) {
        s.push_back((char)(digits[n - 1] + '0'));
        n--;
    }
    w_tstr(&s);
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415730
stream_base& text_stream::w_bool(char* v)
{
    if (*v)
        { rgl::string t("1 "); buf.append(t); }
    else
        { rgl::string t("0 "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415790
stream_base& text_stream::w_float(float* v)
{
    char tmp[128];
    mb_snprintf(tmp, 128, "%f ", (double)*v);
    { rgl::string t(tmp); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415800
stream_base& text_stream::w_stream(stream_base* o)
{
    int len = o->buf.len;
    w_int(&len);
    { rgl::string t(" "); buf.append(t); }
    buf.append(o->buf);
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415870
// (also slot 22, and every "return *this" slot of the other stream classes: ICF)
stream_base& text_stream::slot12(void* v) { return *this; }

// @ 0x415880
stream_base& text_stream::r_tstr(rgl::string* v)
{
    rgl::string tmp;
    expect_token();
    if (state == 1) {
        buf.get_string(tmp);
        if (!tmp.equals(*v))
            mb_error("format broken");
        state = 0;
        return *this;
    }
    buf.get_string(*v);
    return *this;
}
