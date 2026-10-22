// Options screen helpers: builds the sorted resolution list for the config dialog.
#include <windows.h>
#include <d3d9.h>
#include <stdlib.h>
#include <vector>
#include <new>

// rgl::string: 16 bytes, verified in match/include/rgl.h (ctor 0x42bbe0).
struct tstring {
    char* p; int size, cap, extra;   // 0x0 / 0x4 / 0x8 / 0xc (extra unverified)
    tstring(const char* s);          // 0x42bbe0
    tstring(const tstring& o);       // 0x42c110
    ~tstring();                      // 0x42bc40
    void push_back(char c);          // 0x42be70
    void append(const tstring& o);   // 0x42c1e0
};

// 0x43a1e0: string-table lookup on 0x8ee3c48.
tstring& strtab_lookup(tstring& key);

// 0x08ee3d3c: renderer display-mode enumerator. Verified call offsets: +0x10 adapter count,
// +0x14 adapter display mode, +0x18 mode count, +0x1c enumerate mode.
struct IModeEnum {
    virtual void v0();
    virtual int v1();
    virtual int v2();
    virtual int v3();
    virtual int count();                                  // +0x10
    virtual void display_mode(int adapter, int fmt, D3DDISPLAYMODE* mode);   // +0x14
    virtual int mode_count(int adapter, int fmt);                            // +0x18
    virtual void enum_mode(int adapter, int fmt, int index, D3DDISPLAYMODE* m); // +0x1c
};

// 0x08ee3d3c: renderer singleton pointer.
#define g_renderer (*(IModeEnum**)0x8ee3d3c)

// Temporary sorted result, copied into the return value at the end (as the original does).
struct ModeList {
    std::vector<tstring> names;
    ModeList() {}
    ModeList(const ModeList& o) : names(o.names) {}   // 0x4cc680 (copy with EH frame)
};

// @ 0x4cf100
std::vector<tstring> FUN_004cf100()
{
    std::vector<tstring> mode_names;
    std::vector<int> widths;
    std::vector<int> heights;
    std::vector<int> bpps;

    unsigned adapters = g_renderer->count();
    int adapter_geometry[32];
    for (unsigned a = 0; a < adapters; a++) {
        D3DDISPLAYMODE adapter_mode;
        g_renderer->display_mode(a, 0, &adapter_mode);
        adapter_geometry[a] = adapter_mode.Height;
        D3DFORMAT formats[2];
        formats[0] = D3DFMT_R5G6B5;      // 16-bit
        formats[1] = D3DFMT_X8R8G8B8;    // 32-bit
        for (int f = 0; f < 2; f++) {
            int modes = g_renderer->mode_count(a, formats[f]);
            for (int m = 0; m < modes; m++) {
                D3DDISPLAYMODE mode;
                g_renderer->enum_mode(a, formats[f], m, &mode);
                if (mode.Width < 640 || mode.Height < 480)
                    continue;
                char buf[128];
                tstring name(_itoa(mode.Width, buf, 10));
                name.push_back('x');
                name.append(tstring(_itoa(mode.Height, buf, 10)));
                name.append(tstring(f ? " 32 bits" : " 16 bits"));
                int bpp = f ? 32 : 16;
                bool dup = false;
                for (unsigned i = 0; i < widths.size(); i++) {
                    if (widths[i] == (int)mode.Width && heights[i] == (int)mode.Height && bpps[i] == bpp)
                        dup = true;
                }
                if (!dup) {
                    mode_names.push_back(name);
                    widths.push_back(mode.Width);
                    heights.push_back(mode.Height);
                    bpps.push_back(bpp);
                }
            }
        }
    }

    ModeList sorted;
    unsigned n = (unsigned)widths.size();
    int cur_w = 0, cur_h = 0, cur_bpp = 0;
    for (unsigned k = 0; k < n; k++) {
        int best = -1;
        int best_w = 1000000, best_h = 1000000, best_bpp = 1000000;
        for (unsigned i = 0; i < n; i++) {
            int w = widths[i], h = heights[i], bpp = bpps[i];
            if (w == cur_w) {
                if (h == cur_h) {
                    if (bpp > cur_bpp) {
                        best = i; best_w = w; best_h = h; best_bpp = bpp;
                    }
                } else if (h > cur_h) {
                    if (best_h == h) {
                        if (best_bpp >= bpp) {
                            best = i; best_w = w; best_h = h; best_bpp = bpp;
                        }
                    } else if (best_h > h) {
                        best = i; best_w = w; best_h = h; best_bpp = bpp;
                    }
                }
            } else if (w > cur_w) {
                if (best_w == w) {
                    if (best_h == h) {
                        if (best_bpp >= bpp) {
                            best = i; best_w = w; best_h = h; best_bpp = bpp;
                        }
                    } else if (best_h > h) {
                        best = i; best_w = w; best_h = h; best_bpp = bpp;
                    }
                } else if (best_w > w) {
                    best = i; best_w = w; best_h = h; best_bpp = bpp;
                }
            }
        }
        if (best >= 0)
            sorted.names.push_back(tstring(mode_names[best].p));
        cur_w = best_w;
        cur_h = best_h;
        cur_bpp = best_bpp;
    }

    std::vector<tstring> result(sorted.names);
    return result;
}
