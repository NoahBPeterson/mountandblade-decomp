// Slice s00415320: text stream (writer/reader of the engine's serialisation format) - write side and
// typed readers; the class is declared in ../s00414960/mbstream.h.
#include "../s00414960/mbstream.h"
#include <stdlib.h>

// 0x54a83c " ", 0x54a840 "0 ", 0x54a844 "1 ", 0x54a848 "%f ", 0x54a84c "format broken",
// 0x54a85c "Unexpected End of File while reading file: %s"
#define EXPECT_TOKEN() \
    do { if (!buf.has_token()) mb_error("Unexpected End of File while reading file: %s", name.c_str()); } while (0)

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
    { tstring t(s); buf.append(t); }
    { tstring t(" "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415410
stream_base& text_stream::w_tstr(const tstring* s)
{
    buf.append(*s);
    { tstring t(" "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415470
stream_base& text_stream::w_byte(unsigned char* v)
{
    char tmp[128];
    _itoa(*v, tmp, 10);
    { tstring t(tmp); buf.append(t); }
    { tstring t(" "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415500
stream_base& text_stream::w_int(int* v)
{
    char tmp[128];
    _itoa(*v, tmp, 10);
    { tstring t(tmp); buf.append(t); }
    { tstring t(" "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415590
stream_base& text_stream::w_i64(__int64* v)
{
    tstring s;
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
    tstring s;
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
        { tstring t("1 "); buf.append(t); }
    else
        { tstring t("0 "); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415790
stream_base& text_stream::w_float(float* v)
{
    char tmp[128];
    mb_snprintf(tmp, 128, "%f ", (double)*v);
    { tstring t(tmp); buf.append(t); }
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415800
stream_base& text_stream::w_stream(stream_base* o)
{
    int len = o->buf.len;
    w_int(&len);
    { tstring t(" "); buf.append(t); }
    buf.append(o->buf);
    if (autoflush)
        flush();
    return *this;
}

// @ 0x415870
stream_base& text_stream::slot12(void* v) { return *this; }

// @ 0x415880
stream_base& text_stream::r_tstr(tstring* v)
{
    tstring tmp;
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

// @ 0x415880 shares the check through a call

// @ 0x415900
void text_stream::expect_token() { EXPECT_TOKEN(); }

// @ 0x415930
stream_base& text_stream::r_int(int* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    *v = mb_parse_int(tok);
    return *this;
}

// @ 0x4159a0
int mb_parse_int(const char* s)
{
    bool neg = false;
    int r = 0;
    int i = 0;
    if (s[0] > 0) {
        do {
            if (i >= 15)
                break;
            if (i == 0 && s[0] == '-')
                neg = true;
            else
                r = r * 10 + s[i] - '0';
            i++;
        } while (s[i] > 0);
        if (neg)
            r = -r;
    }
    return r;
}

// @ 0x4159f0
stream_base& text_stream::r_byte(char* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    *v = (char)atol(tok);
    return *this;
}

// @ 0x415a60
stream_base& text_stream::r_i64(__int64* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    bool neg = false;
    int start = 0;
    if (tok[0] == '-') {
        neg = true;
        start = 1;
    }
    __int64 mult = 1;
    *v = 0;
    for (int i = toklen - 1; i >= start; i--) {
        *v += (__int64)(tok[i] - '0') * mult;
        mult *= 10;
    }
    if (neg)
        *v = -*v;
    return *this;
}

// @ 0x415b60
stream_base& text_stream::r_u64(unsigned __int64* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    *v = 0;
    __int64 mult = 1;
    for (int i = toklen - 1; i >= 0; i--) {
        *v += (__int64)(tok[i] - '0') * mult;
        mult *= 10;
    }
    return *this;
}

// @ 0x415c10
stream_base& text_stream::r_bool(char* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    *v = atol(tok) != 0;
    return *this;
}
