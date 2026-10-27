//-----------------------------------------------------------------------------
// File: tstring.cpp  (0x42bac0..0x42cb70)
//
// Desc: the engine string `tstring` (16 bytes: read cursor, heap buffer, capacity, length): every out-of-line
//       member, in address order (cl emits the TU callee-first: grow/reserve before their callers). The class
//       itself is declared in ../tstring.h (shared). Names provisional (rgl engine code, string.cpp?).
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <new>     // declares operator new[]/delete[]: without it cl 13.10 calls the scalar operators (the original calls 0x535482/0x53570d)

// ../tstring.h says range_equals (0x42c160) returns bool, but it returns `char` (mangled ...QBEDABH0ABU1@@Z: D);
// the declaration is rewritten through a macro while the header is included, so the class stays defined in ONE
// header. (tstring::split 0x42c9b0 used to be injected the same way; the header declares it now.)
#define range_equals range_equals_hdr(const int& a, const int& b, const tstring& c) const; char range_equals
#include "../tstring.h"
#undef range_equals

int   g_str_grow_add;    // 0x5a8e60
float g_str_grow_mul;    // 0x5a8e64

// @ 0x42bac0
// NONMATCHING (17 bytes): same instructions, `this`/new-size registers swapped (ebx/ebp)
void tstring::grow(unsigned n)
{
    unsigned need, a, b;
    char* nb;
    if (n > 0) {
        need = n + 1;
        if ((unsigned)cap < need) {
            a = (unsigned)((int)cap * g_str_grow_mul);
            b = cap + g_str_grow_add;
            b = (a > b) ? a : b;
            b = (b > need) ? b : need;
            nb = new char[b];
            if (buf) {
                memcpy(nb, buf, len + 1);
                delete[] buf;
            }
            cap = b;
            buf = nb;
        }
    }
}
// @ 0x42bb50
// NONMATCHING (19 bytes): same instructions and size; `this`/new size are in esi/edi in the original, edi/esi here
void tstring::reserve(unsigned n)
{
    if (n > 0) {
        unsigned need = n + 1;
        int old = cap;
        if ((unsigned)old < need) {
            unsigned b = (unsigned)(old * g_str_grow_mul);
            unsigned c = old + g_str_grow_add;
            if (c > b) b = c;
            if (b <= need) b = need;
            char* nb = new char[b];
            if (buf) delete[] buf;
            cap = b;
            buf = nb;
            len = 0;
            *nb = 0;
        }
    }
}
// @ 0x42bbd0
tstring::tstring() { buf = 0; len = 0; cap = 0; pos = 0; }
// @ 0x42bbe0
tstring::tstring(const char* s)
{
    buf = 0; len = 0; cap = 0; pos = 0;
    int n = (int)strlen(s);
    if (n) {
        reserve(n);
        memcpy(buf, s, n + 1);
        len = n;
    }
}
// @ 0x42bc40
tstring::~tstring()
{
    if (buf) {
        unsigned p = (unsigned)buf;
        if (p < 0x10000u || p >= 0x80000000u || p == 0x3c6d9160u || p == 0x3c6d9166u || p == 0x3c6d9168u) {   // TEMP diagnostic
            static FILE* f = fopen("tstr_bad.txt", "a");
            if (f) {
                fprintf(f, "bad dtor this=%p buf=%08x\n", (void*)this, p);
                unsigned* sp = (unsigned*)&p;
                for (int i = 0; i < 24; i++) {
                    unsigned v = sp[i];
                    if (v >= 0x401000u && v < 0x600000u) fprintf(f, "   [%d] %08x\n", i, v);
                }
                fflush(f);
            }
            return;
        }
        delete buf;
    }
}
// @ 0x42bc50
void tstring::operator=(const tstring& o)
{
    int n = o.len;
    reserve(n);
    if (n) memcpy(buf, o.buf, n);
    len = n;
    if (buf) buf[n] = 0;
    pos = o.pos;
}
// @ 0x42bcb0
void tstring::assign(const char* s)
{
    int n = (int)strlen(s);
    reserve(n);
    if (n) memcpy(buf, s, n);
    len = n;
    if (buf) buf[n] = 0;
    pos = 0;
}
// @ 0x42bd10
char tstring::equals(const char* s) const
{
    if (len == 0) return strlen(s) == 0;
    if (strlen(s) != 0) return strcmp(buf, s) == 0 ? 1 : 0;
    return false;
}
// @ 0x42bdb0
bool tstring::differs(const char* s) const { return !equals(s); }
// @ 0x42bdd0
char tstring::equals(const tstring& o) const
{
    if (len == 0) return o.len == 0;
    if (o.len != 0) return strcmp(buf, o.buf) == 0 ? 1 : 0;
    return false;
}
// @ 0x42be50
void tstring::clear() { len = 0; if (buf) buf[0] = 0; }
// @ 0x42be70
void tstring::push_back(char c)
{
    grow(len + 1);
    buf[len] = c;
    buf[len + 1] = 0;
    len++;
}
// @ 0x42bea0
void tstring::erase(int start, int n)
{
    if (start + n > len) n = len - start;
    for (int i = start + n; i < len; i++) buf[i - n] = buf[i];
    len -= n;
    buf[len] = 0;
}
// @ 0x42bef0
void tstring::rewind() { pos = 0; }
// @ 0x42bf00
void tstring::read(void* dst, int n)
{
    memcpy(dst, buf + pos, n);
    pos += n;
}
// @ 0x42bf30
void tstring::write(const void* src, int n)
{
    int nl = len + n;
    grow(nl);
    memcpy(buf + len, src, n);
    len = nl;
    buf[nl] = 0;
}
// @ 0x42bf80
void tstring::underscores_to_spaces()
{
    for (int i = 0; i < len; i++)
        if (buf[i] == '_') buf[i] = ' ';
}
// @ 0x42bfb0
void tstring::spaces_to_underscores()
{
    for (int i = 0; i < len; i++)
        if (buf[i] == ' ') buf[i] = '_';
}
// @ 0x42bfe0
void tstring::to_upper()
{
    for (int i = 0; i < len; i++)
        if (buf[i] >= 'a' && buf[i] <= 'z') buf[i] -= 0x20;
}
// @ 0x42c020
void tstring::to_lower()
{
    for (int i = 0; i < len; i++)
        if (buf[i] >= 'A' && buf[i] <= 'Z') buf[i] += 0x20;
}
// @ 0x42c060
void tstring::capitalize()
{
    if (len > 0 && buf[0] >= 'a' && buf[0] <= 'z') buf[0] -= 0x20;
}
// @ 0x42c080
int tstring::find(const char& c, int start) const
{
    for (; start < len; start++)
        if (buf[start] == c) return start;
    return -1;
}
// @ 0x42c0b0
int tstring::rfind(const char& c, int start, int end) const
{
    if (end == -1) end = len;
    for (end--; end >= start; end--)
        if (buf[end] == c) return end;
    return -1;
}

// the smaller of two ints, by reference (inlined by its callers)
// @ 0x42c0f0
int min_int(const int& a, const int& b) { return a < b ? a : b; }

// @ 0x42c110
tstring::tstring(const tstring& o)
{
    buf = 0; len = 0; cap = 0; pos = 0;
    unsigned n = o.len;
    if (n > 0) {
        reserve(n);
        memcpy(buf, o.buf, n + 1);
        len = n;
    }
}
// @ 0x42c160
char tstring::range_equals(const int& start, const int& end, const tstring& o) const
{
    char r = 1;
    int e = min_int(end, len);
    if (len != 0) {
        int s = start;
        int n = e - s;
        if (n > 0) {
            if (n == o.len && o.len != 0) {
                do {
                    if (s >= e) break;
                    r = buf[s] == o.buf[s];
                    s++;
                } while (r);
                return r;
            }
            return 0;
        }
    }
    return o.len == 0;
}
// @ 0x42c1e0
void tstring::append(const tstring& o)
{
    if (o.buf) {
        int n = len + o.len;
        grow(n);
        memcpy(buf + len, o.buf, o.len + 1);
        len = n;
    }
}
// @ 0x42c230
bool tstring::has_token()
{
    while (pos < len && (unsigned char)buf[pos] < 0x21) pos++;
    return pos < len && (unsigned char)buf[pos] >= 0x21;
}
// @ 0x42c270
void tstring::get_token(char* out, int* n)
{
    *n = 0;
    int l = len;
    while (pos < len && (unsigned char)buf[pos] < 0x21) pos++;
    int i = pos;
    if (i >= l) {
        out[0] = 0;
        return;
    }
    char q = buf[i];
    if (q == '"' || q == '\'') {
        pos++; i = pos; if (i < l) { do { if (buf[pos] == q || *n >= 0x2000) break; out[*n] = buf[pos]; (*n)++; pos++; } while (pos < len); } pos++;
    } else {
        do { if ((unsigned char)buf[pos] < 0x21 || *n >= 0x2000) break; out[*n] = buf[pos]; (*n)++; pos++; } while (pos < len);
    }
    int k = *n;
    if (k >= 0x1fff) k = 0x1fff;
    out[k] = 0;
}
// @ 0x42c350
void tstring::get_string(tstring& out)
{
    out.len = 0;
    if (out.buf) out.buf[0] = 0;
    int l = len;
    if (pos < l) {
        do {
            if ((unsigned char)buf[pos] >= 0x21) break;
            pos++;
        } while (pos < len);
    }
    int i = pos;
    if (i < l) {
        char q = buf[i];
        if (q == '"' || q == '\'') {
            i++;
            pos = i;
            if (i < l) {
                do {
                    char c = buf[i];
                    if (c == q) break;
                    out.push_back(c);
                    i = pos + 1;
                    pos = i;
                } while (i < len);
            }
            pos = i + 1;
        } else {
            do {
                unsigned char c = buf[pos];
                if (c < 0x21) return;
                out.push_back(c);
                pos++;
            } while (pos < len);
        }
    }
}
// @ 0x42c440
int tstring::find(const tstring& s, int start) const
{
    int n = s.len;
    int k = start;
    int last = len - n + 1;
    for (; k < last; k++) {
        int i = 0;
        for (; i < n; i++)
            if (buf[k + i] != s.buf[i]) break;
        if (i == n) return k;
    }
    return -1;
}
// @ 0x42c4a0
tstring tstring::substr(int start, int end) const
{
    tstring t;
    for (; start < end && start < len; start++)
        t.push_back(buf[start]);
    return t;
}
// @ 0x42c560
void tstring::erase_range(int a, int b)
{
    tstring tail = substr(b, len);
    len = a;
    append(tail);
}
// @ 0x42c5d0
tstring tstring::strip_tags() const
{
    tstring r;
    int lt = find('<', 0);
    int start = 0;
    while (lt >= 0 && start >= 0) {
        r.append(substr(start, lt));
        start = find('>', lt) + 1;
        if (start >= 0) lt = find('<', start);
    }
    r.append(substr(start < 0 ? lt : start, len));
    return r;
}
// @ 0x42c7a0
void tstring::trim()
{
    int a = 0, b = len;
    for (int i = 0; i < len && buf[i] == ' '; i++) a++;
    for (int i = len - 1; i >= 0 && buf[i] == ' '; i--) b--;
    tstring t = substr(a, b);
    len = 0;
    append(t);
}
// @ 0x42c840
tstring tstring::operator+(const tstring& o) const
{
    tstring t;
    t = *this;
    t.append(o);
    return t;
}
// @ 0x42c900
void tstring::insert(int at, const tstring& s)
{
    tstring tail = substr(at, len);
    len = at;
    append(s);
    append(tail);
}

// @ 0x42c9b0
tstring* tstring::split(const char& sep, int& count) const
{
    int n = 0;
    for (int i = 0; i < len; i++)
        if (buf[i] == sep) n++;
    count = n + 1;
    tstring* out = new tstring[n + 1];
    int k = 0;
    int start = 0;
    for (int i = 0; i < len; i++) {
        if (buf[i] == sep) {
            out[k] = substr(start, i);
            k++;
            start = i + 1;
        }
    }
    out[k] = substr(start, len);
    return out;
}
