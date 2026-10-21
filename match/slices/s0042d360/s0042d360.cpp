// Slice s0042d360: the UI root's ctor/dtor, TiXmlString (reserve/assign/append), TiXmlNode linking helpers and
// TiXmlBase::PutString (the engine embeds TinyXML). Names provisional except TinyXML's own.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui.h"
#include "tinyxml.h"

// tstring and friends are out of line here (0x42bbd0 ctor, 0x42bc40 dtor, 0x42bcb0 assign, 0x42bb50 reserve).
void tstring_reserve(tstring*, unsigned);

// 0x5e23d0: the shared empty representation of TiXmlString
TiXmlString::Rep TiXmlString::nullrep_;

struct entity_t { const char* str; unsigned int strLength; char chr; };
extern entity_t entity[5];          // 0x5a90b0: &amp; &lt; &gt; &quot; &apos;

static inline void destroy_range(tstring* first, tstring* last)
{
    for (; first != last; ++first)
        first->~tstring();
}

// @ 0x42d360
void __stdcall destroy_tstrings(tstring* first, tstring* last)
{
    for (; first != last; ++first)
        first->~tstring();
}

// @ 0x42d390
void tstring_vec::tidy()
{
    if (first != 0) {
        destroy_range(first, last);
        ::operator delete(first);
    }
    first = 0; last = 0; end = 0;
}

// @ 0x42d3e0
gui_manager::~gui_manager()
{
    reset();
    if (o564) { delete o564; o564 = 0; }
    if (o568) { delete o568; o568 = 0; }
    if (o56c) { delete o56c; o56c = 0; }
    if (o1210) { delete o1210; o1210 = 0; }
    if (o1214) { delete o1214; o1214 = 0; }
    if (o1218) { delete o1218; o1218 = 0; }
    if (o121c) { delete o121c; o121c = 0; }
}

// vec<vector4> as laid out in the engine's std::vector (alloc, first, last, end)
struct vec4_vec {
    int alloc;
    float* first;
    float* last;
    float* end;
    bool buy(unsigned n);           // 0x42d570
    __declspec(noreturn) void xlen() const;   // 0x43e2a0
};
// @ 0x42d570
bool vec4_vec::buy(unsigned n)
{
    first = 0; last = 0; end = 0;
    if (n == 0) return false;
    if (n > 0xfffffff) xlen();
    float* p = (float*)operator new(n * 16);
    end = p + n * 4;
    first = p;
    last = p;
    return true;
}

// @ 0x42d5c0
gui_manager::gui_manager()
{
    reset();
    f16e0 = f1718 = -1;
    p24 = pair2(0, 0);
    p14 = pair2(0, 0);
    p2c = pair2(0, 0);
    f560 = 1;
    o564 = 0;
    o568 = 0;
    o56c = 0;
    fa0 = 0;
    fa4 = 0;
    fa8 = -0.03f;
    o1210 = 0;
    o1214 = 0;
    o1218 = 0;
    o121c = 0;
    f16e4 = 0xff101010;
    f1709 = 0;
    f170c = 0;
    fb0ac = 0;
    fb028 = 1.0f;
    f1710 = 0;
}

// ---- TiXmlString (non-STL TinyXML) ----
// @ 0x42d820
void TiXmlString::quit()
{
    if (rep_ != &nullrep_)
        delete[] reinterpret_cast<int*>(rep_);
}

// @ 0x42d840
void TiXmlString::reserve(size_type cap)
{
    if (cap > capacity()) {
        TiXmlString tmp;
        tmp.init(length(), cap);
        memcpy(tmp.start(), data(), length());
        swap(tmp);
    }
}

// @ 0x42d8b0
TiXmlString& TiXmlString::assign(const char* str, size_type len)
{
    size_type cap = capacity();
    if (len > cap || cap > 3 * (len + 8)) {
        TiXmlString tmp;
        tmp.init(len);
        memcpy(tmp.start(), str, len);
        swap(tmp);
    } else {
        memmove(start(), str, len);
        set_size(len);
    }
    return *this;
}

// @ 0x42d950
TiXmlString& TiXmlString::append(const char* str, size_type len)
{
    size_type newsize = length() + len;
    if (newsize > capacity())
        reserve(newsize + capacity());
    memmove(finish(), str, len);
    set_size(newsize);
    return *this;
}

// ---- TiXmlNode / TiXmlAttributeSet ----
// @ 0x42d9a0
TiXmlNode* TiXmlNode::LinkEndChild(TiXmlNode* node)
{
    node->parent = this;
    node->prev = lastChild;
    node->next = 0;
    if (lastChild)
        lastChild->next = node;
    else
        firstChild = node;
    lastChild = node;
    return node;
}

// @ 0x42d9d0
const TiXmlDocument* TiXmlNode::GetDocument() const
{
    const TiXmlNode* node;
    for (node = this; node; node = node->parent) {
        if (node->ToDocument())
            return node->ToDocument();
    }
    return 0;
}

// @ 0x42da00
void TiXmlAttributeSet::Add(TiXmlAttribute* addMe)
{
    addMe->next = &sentinel;
    addMe->prev = sentinel.prev;
    sentinel.prev->next = addMe;
    sentinel.prev = addMe;
}

// @ 0x42da20
TiXmlString& TiXmlString::operator=(const char* copy)
{
    return assign(copy, (size_type)strlen(copy));
}

// @ 0x42da50
TiXmlString& TiXmlString::operator+=(const char* suffix)
{
    return append(suffix, (size_type)strlen(suffix));
}

// @ 0x42da80
// The scalar deleting destructor ??_GTiXmlBase@@UAEPAXI@Z is compiler-generated with the vtable (0x54b8f8); defining
// the (empty) destructor here makes cl emit both.
TiXmlBase::~TiXmlBase() {}

// @ 0x42daa0
void TiXmlAttribute::SetValue(const char* _value)
{
    value = _value;
}

// @ 0x42dad0
void TiXmlBase::PutString(const TiXmlString& str, TiXmlString* outString)
{
    int i = 0;
    while (i < (int)str.length()) {
        unsigned char c = (unsigned char)str[i];
        if (c == '&' && i < ((int)str.length() - 2) && str[i + 1] == '#' && str[i + 2] == 'x') {
            while (i < (int)str.length() - 1) {
                outString->append(str.c_str() + i, 1);
                ++i;
                if (str[i] == ';')
                    break;
            }
        } else if (c == '&') {
            outString->append(entity[0].str, entity[0].strLength);
            ++i;
        } else if (c == '<') {
            outString->append(entity[1].str, entity[1].strLength);
            ++i;
        } else if (c == '>') {
            outString->append(entity[2].str, entity[2].strLength);
            ++i;
        } else if (c == '\"') {
            outString->append(entity[3].str, entity[3].strLength);
            ++i;
        } else if (c == '\'') {
            outString->append(entity[4].str, entity[4].strLength);
            ++i;
        } else if (c < 32) {
            char buf[32];
            _snprintf(buf, sizeof(buf), "&#x%02X;", (unsigned)(c & 0xff));
            outString->append(buf, (TiXmlString::size_type)strlen(buf));
            ++i;
        } else {
            *outString += (char)c;
            ++i;
        }
    }
}

// @ 0x42dc10
TiXmlNode* TiXmlNode::FirstChild(const char* _value) const
{
    TiXmlNode* node;
    for (node = firstChild; node; node = node->next) {
        if (strcmp(node->Value(), _value) == 0)
            return node;
    }
    return 0;
}

// @ 0x42dc80
TiXmlElement* TiXmlNode::NextSiblingElement() const
{
    const TiXmlNode* node;
    for (node = next; node; node = node->next) {
        if (node->ToElement())
            return const_cast<TiXmlElement*>(node->ToElement());
    }
    return 0;
}
