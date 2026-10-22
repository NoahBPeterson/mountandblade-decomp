// s004d9aa0.cpp — slice 371
// Font definition loader (TinyXML), font teardown, and the std::vector<>
// insert/fill instantiations for the E50 / E68 / E176 element types.

#include <vector>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include "tinyxml.h"

// ---------------------------------------------------------------- tstring
struct tstring {                            // 0x10
    int pos;                                // 0x00
    char* buf;                              // 0x04
    int cap;                                // 0x08
    int len;                                // 0x0c
    tstring();
    tstring(const tstring&);
    tstring& operator=(const tstring&);
    ~tstring();
    void assign(const char*);
};

// ---------------------------------------------------------------- elements
struct scalar4 { int v; };                  // 4-byte trivial element
struct R44 { int v[11]; };                  // 0x2c trivial element
struct b16 { int v[4]; };                   // 0x10 POD block

struct Block { void free_materials(); };    // 0x4c8330 (empty base)

struct M28 {                                // 0x1c member of E68
    char* p;                                // 0x00
    int n;                                  // 0x04
    char c;                                 // 0x08
    tstring s;                              // 0x0c
    M28() {}
    M28(const M28&);
    ~M28() { if (p) delete[] p; }
    M28& operator=(const M28& o)
        { p = o.p; n = o.n; c = o.c; s = o.s; return *this; }
};

struct E50 {                                // 0x50
    tstring s;                              // 0x00
    int x0, x1, x2, x3;                     // 0x10
    b16 y;                                  // 0x20
    b16 z;                                  // 0x30
    std::vector<R44> v;                     // 0x40
};

struct E68 : Block {                        // 0x68
    tstring s;                              // 0x00
    int b0, b1, b2, b3, b4;                 // 0x10
    int b5, b6, b7, b8, b9;                 // 0x24
    M28 m;                                  // 0x38
    std::vector<scalar4> v;                 // 0x54
    int t;                                  // 0x64
    ~E68() { free_materials(); }
};

struct E176 {                               // 0xb0
    tstring s;                              // 0x00
    int a;                                  // 0x10
    int b;                                  // 0x14
    int c;                                  // 0x18
    std::vector<int> v;                     // 0x1c
    b16 big[8];                             // 0x2c
    int t;                                  // 0xac
};

template class std::vector<scalar4>;
template class std::vector<E50>;
template class std::vector<E68>;
template class std::vector<E176>;

// ---------------------------------------------------------------- font object 0x4d9aa0
struct font_details {                       // 0x1c, seven floats
    float a, b, c, d, e, f, g;
    font_details();                         // 0x4bd120 (zero-fill)
};

struct Cls_map {
    char pad[8];
    font_details* find(const int& key);     // 0x4d92c0
};

struct Font {                               // font definition object
    int p00, p04;
    int width;                              // 0x08
    int height;                             // 0x0c
    int padding;                            // 0x10
    int font_size;                          // 0x14
    float x_ratio;                          // 0x18
    float y_ratio;                          // 0x1c
    float font_scale;                       // 0x20
    float line_spacing;                     // 0x24
    char pad28[0x1c];                       // 0x28
    Cls_map glyph_map;                      // 0x44 (0x28 bytes)
    font_details glyphs[0x200];             // 0x6c
    void load(const char* filename);        // 0x4d9aa0
};

void mb_error(const char* msg);             // 0x42cc80

// @ 0x4d9aa0
void Font::load(const char* filename)
{
    int font_scale_i = 100;
    int line_spacing_i = 100;

    width = 0x200;
    height = 0x200;
    padding = 0;

    TiXmlDocument doc(filename);
    if (!doc.LoadFile((TiXmlEncoding)0)) {
        mb_error("Unable To Load Font File!");
        return;
    }

    font_size = 0x4b;

    TiXmlHandle docHandle(&doc);
    TiXmlElement* root = docHandle.FirstChildElement().Element();
    if (!root)
        return;

    root->QueryIntAttribute("padding", &padding);
    root->QueryIntAttribute("width", &width);
    root->QueryIntAttribute("height", &height);
    root->QueryIntAttribute("font_scale", &font_scale_i);
    root->QueryIntAttribute("line_spacing", &line_spacing_i);
    root->QueryIntAttribute("font_size", &font_size);

    font_scale = (float)font_scale_i * 0.01f;
    line_spacing = (float)line_spacing_i * 0.01f;
    x_ratio = (float)font_size / (float)width;
    y_ratio = (float)font_size / (float)height;

    font_details fd;
    (void)fd;

    TiXmlElement* details = docHandle.FirstChild("FontDetails").FirstChild().Element();
    if (!details)
        return;

    for (TiXmlElement* e = details; e; e = e->NextSiblingElement()) {
        int code = 0;
        if (e->QueryIntAttribute("code", &code) != 0)
            continue;

        font_details g;
        int u, v, w, h, preshift, yadjust, postshift;

        g.a = g.b = g.c = g.d = g.e = g.f = g.g = 0.0f;

        if (e->QueryIntAttribute("u", &u) == 0)
            g.a = (float)(u - 4) / (float)width;
        if (e->QueryIntAttribute("v", &v) == 0)
            g.d = (float)((height - v) + 4) / (float)height;
        if (e->QueryIntAttribute("w", &w) == 0)
            g.c = (float)((w - padding) + 6) / (float)width;
        if (e->QueryIntAttribute("h", &h) == 0)
            g.b = (float)((height - h) - 6 + padding) / (float)height;
        if (e->QueryIntAttribute("preshift", &preshift) == 0)
            g.e = (float)(preshift - 4) / (float)width;
        if (e->QueryIntAttribute("yadjust", &yadjust) == 0)
            g.f = (float)(yadjust + 4) / (float)height;
        if (e->QueryIntAttribute("postshift", &postshift) == 0)
            g.g = (float)(postshift + 4) / (float)width;

        font_details* dst;
        if (code < 0x200)
            dst = &glyphs[code];
        else
            dst = glyph_map.find(code);
        *dst = g;
    }
}

// ---------------------------------------------------------------- teardown 0x4d9e80
struct TearObj {
    char pad0[0x78];
    void* f78;                              // 0x78
    void m4c54f0(void*);                    // 0x4c54f0
    void m4d4b80();                         // 0x4d4b80
    void teardown();                        // 0x4d9e80
};

// @ 0x4d9e80
void TearObj::teardown()
{
    m4c54f0(f78);
    m4d4b80();
}
