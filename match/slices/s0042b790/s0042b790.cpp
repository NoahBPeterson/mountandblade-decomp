// Slice s0042b790: the engine's tstring (0x42bac0..0x42c0b0) and the shadow-render helper at 0x42b790.
#include <string.h>
#include "tstring.h"
#include "shadow.h"

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
tstring::~tstring() { if (buf) delete buf; }
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

// @ 0x42b790
// Renders the shadow-map view of the entity into its 128x128 shadow render target (created on first use):
// an orthographic camera along the main light, then the entity and its linked entity are drawn into it.
void StratEnt::shadow_tick()
{
    if (p288 == 0) {
        p288 = g_resmgr.get_shadow_skin(0x80, 0x80, 0x1c, 0);
        if (p288 == 0) return;
        p288->file.assign("shadow_shader");
        skin* sk = p288;
        sk->f30 = g_resmgr.lookup_file(&sk->file);
        p288->id = 0x108;
        p288->refs[0]->f24 = 0x300000;
    }
    if (sc28c == 0) {
        sc28c = new scene;
        sc28c->mask = 0x2000;
        sc28c->f0 = 1;
        sc28c->f4 = 1;
        sc28c->f8 = 0x7e;
        sc28c->fc = 0x7e;
        sc28c->f10 = 0;
        sc28c->f14 = 1.0f;
        sc28c->f18 = 0;
        sc28c->f1c = 0;
        sc28c->f20 = 0x80;
        sc28c->f24 = 0x80;
        sc28c->f28 = 0;
        sc28c->f2c = 1.0f;
        sc28c->f1a5 = 1;
        sc28c->f164 = 1;
        sc28c->f18c = 0x2101;
        sc28c->f190 = 0;
    }
    if (p288 != 0 && p288->refs[0]->loaded != 0) {
        frustum f;
        render_info ri;
        matrix m;
        vector4 amb;
        f.farz = radius * 5.0f;
        f.perspective = 0;
        f.nearz = 0.01f;
        f.right = radius;
        f.left = -radius;
        f.top = radius;
        f.bottom = -radius;
        f.build_planes();
        build_shadow_frame(&m, &amb);
        f.m = m;
        f.build_planes();
        sc28c->fr = f;
        sc28c->target = p288->refs[0];
        if (fx) fx->draw(sc28c, &pos, &f144);
        MeshEnt::render(sc28c, &ri);
        if (f298 >= 0) {
            StratEnt* e = ctx->find_entity(f298);
            if (e != 0 && e->type() == 9 && e->fx != 0)
                e->fx->draw(sc28c, &e->pos, &f144);
        }
    }
}

// @ 0x42ba50
// Looks an entity up by id: the first list is searched until a hit (the first match wins), the second list
// is searched to the end (the last match wins).
StratEnt* Ctx::find_entity(int id)
{
    StratEnt** p = ents_a._Myfirst;
    StratEnt* r = 0;
    bool found = false;
    if (p < ents_a._Mylast) {
        do {
            if (found) return r;
            if ((*p)->id == id) {
                r = *p;
                found = true;
            }
            p++;
        } while (p < ents_a._Mylast);
        if (found) return r;
    }
    for (p = ents_b._Myfirst; p < ents_b._Mylast; p++)
        if ((*p)->id == id) r = *p;
    return r;
}
