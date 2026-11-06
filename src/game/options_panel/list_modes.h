// list_modes.h: the display-mode list (0x4cf100), a header inline. The callers' codegen needs the body in the TU: cl proves
// the hidden return pointer does not escape, so a caller's local vector stays unclobbered across later calls and its base stays in
// a register (Recf0::init_screen 0x4e05c0 matches only with this body visible; a bare declaration leaves it 7 bytes off).
// Include it after any explicit specialization of vector<tstring> members (cls_map.cpp has one).
#pragma once
#include <vector>
#include <d3d9.h>
// Temporary sorted result, copied into the return value at the end (as the original does).
struct ModeList {
    std::vector<tstring> names;
    ModeList() {}
    ModeList(const ModeList& o) : names(o.names) {}   // 0x4cc680 (copy with EH frame)
};

// 0x4cf100
inline std::vector<tstring> list_modes()
{
    std::vector<tstring> mode_names;
    std::vector<int> widths;
    std::vector<int> heights;
    std::vector<int> bpps;

    unsigned adapters = g_app_obj.m_pD3D->GetAdapterCount();
    for (unsigned a = 0; a < adapters; a++) {
        // 0x4cf194: GetAdapterIdentifier into a 0x448-byte frame buffer ([esp+0x14c]) that is never read again (a
        // D3DDISPLAYMODE here let d3d9 write 1.1 KB over the frame: the in-game Video Options crash)
        D3DADAPTER_IDENTIFIER9 id;
        g_app_obj.m_pD3D->GetAdapterIdentifier(a, 0, &id);
        D3DFORMAT formats[2];
        formats[0] = D3DFMT_R5G6B5;      // 16-bit
        formats[1] = D3DFMT_X8R8G8B8;    // 32-bit
        for (int f = 0; f < 2; f++) {
            int modes = g_app_obj.m_pD3D->GetAdapterModeCount(a, formats[f]);
            for (int m = 0; m < modes; m++) {
                D3DDISPLAYMODE mode;
                g_app_obj.m_pD3D->EnumAdapterModes(a, formats[f], m, &mode);
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
            sorted.names.push_back(tstring(mode_names[best].buf));
        cur_w = best_w;
        cur_h = best_h;
        cur_bpp = best_bpp;
    }

    return std::vector<tstring>(sorted.names);
}
