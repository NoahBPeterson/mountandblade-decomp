// port/platform/gdi.cpp: the GDI subset the game uses (src/platform/dxframework/d3dfont.cpp's font bake, the launcher's bitmap):
// memory device contexts, DIB sections, bitmaps loaded from .bmp files, TrueType text (SDL3_ttf), BitBlt / StretchBlt.
// A window DC (GetDC / BeginPaint) has no pixel store: the window's pixels belong to the renderer, so drawing into it
// is accepted and has no visible effect (the launcher's paint of a dialog whose controls are virtual, dialog.cpp).
#include "mbp_win.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <ctype.h>
#include <dirent.h>

using namespace mbp;

namespace {
enum GdiType : uint32_t { G_BITMAP = 0x47424d50, G_FONT = 0x47464e54, G_DC = 0x47444300, G_BRUSH = 0x47425348 };
struct Gdi { uint32_t magic; };
struct Bitmap : Gdi {
    int w = 0, h = 0, bpp = 32, pitch = 0;
    std::vector<uint8_t> bits;          // top-down rows
    bool dib = false;
};
struct Font : Gdi { TTF_Font* font = nullptr; int px = 12; bool bold = false, italic = false; std::string face; };
struct Brush : Gdi { COLORREF color; };
struct Dc : Gdi {
    Bitmap* bmp = nullptr;
    Bitmap* default_bmp = nullptr;
    Font* font = nullptr;
    COLORREF text = 0, bk = 0xFFFFFF;
    UINT align = 0;
    int map_mode = MM_TEXT;
    HWND win = nullptr;
};

std::mutex g_ttf_mu;
bool g_ttf_ok = false;
Brush g_stock[20];

bool ttf_init()
{
    std::lock_guard<std::mutex> g(g_ttf_mu);
    if (!g_ttf_ok) g_ttf_ok = TTF_Init();
    return g_ttf_ok;
}
bool file_exists(const std::string& p)
{
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) return false;
    fclose(f);
    return true;
}
std::string lower(std::string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }

// Map a face name and weight to a font file: MB_FONT, then the OS / common free fonts.
std::string find_font_file(const std::string& face, bool bold, bool italic)
{
    if (const char* e = getenv("MB_FONT")) if (file_exists(e)) return e;
    std::string f = lower(face);
    std::vector<std::string> names;
    auto add = [&](const char* n) { names.push_back(n); };
    bool serif = f.find("times") != std::string::npos || f.find("serif") != std::string::npos || f.find("georgia") != std::string::npos;
    bool mono = f.find("courier") != std::string::npos || f.find("consolas") != std::string::npos || f.find("mono") != std::string::npos;
    if (mono) {
        add(bold ? "Courier New Bold.ttf" : "Courier New.ttf"); add(bold ? "LiberationMono-Bold.ttf" : "LiberationMono-Regular.ttf");
        add(bold ? "DejaVuSansMono-Bold.ttf" : "DejaVuSansMono.ttf");
    } else if (serif) {
        add(bold ? "Times New Roman Bold.ttf" : "Times New Roman.ttf"); add(bold ? "LiberationSerif-Bold.ttf" : "LiberationSerif-Regular.ttf");
        add(bold ? "DejaVuSerif-Bold.ttf" : "DejaVuSerif.ttf");
    }
    if (bold && italic) { add("Arial Bold Italic.ttf"); add("LiberationSans-BoldItalic.ttf"); add("DejaVuSans-BoldOblique.ttf"); }
    else if (bold) { add("Arial Bold.ttf"); add("LiberationSans-Bold.ttf"); add("DejaVuSans-Bold.ttf"); }
    else if (italic) { add("Arial Italic.ttf"); add("LiberationSans-Italic.ttf"); add("DejaVuSans-Oblique.ttf"); }
    add("Arial.ttf"); add("LiberationSans-Regular.ttf"); add("DejaVuSans.ttf"); add("Verdana.ttf"); add("Tahoma.ttf");
#if defined(_WIN32)
    // Windows keeps the font files in %WINDIR%\Fonts (the file system is case-insensitive, so the names above match).
    std::string windir = getenv("WINDIR") ? getenv("WINDIR") : "C:\\Windows";
    for (const std::string& n : names)
        if (file_exists(windir + "\\Fonts\\" + n)) return windir + "\\Fonts\\" + n;
    for (const char* fb : {"arial.ttf", "tahoma.ttf", "segoeui.ttf", "lucon.ttf"})      // Wine's built-in set
        if (file_exists(windir + "\\Fonts\\" + fb)) return windir + "\\Fonts\\" + fb;
    // Under Wine a prefix may have no TTFs at all (wineboot copies only bitmap .fon fonts; TTFs arrive with an
    // installed corefonts package). Wine on macOS/Linux still runs on the host, whose font directories exist —
    // the same fallback as the POSIX branch (dead paths on real Windows, where /System etc. do not exist).
    static const char* host_dirs[] = {"/System/Library/Fonts/Supplemental/", "/Library/Fonts/", "/System/Library/Fonts/",
                                      "/usr/share/fonts/truetype/msttcorefonts/", "/usr/share/fonts/truetype/liberation/",
                                      "/usr/share/fonts/truetype/liberation2/", "/usr/share/fonts/truetype/dejavu/"};
    std::string home = getenv("HOME") ? getenv("HOME") : "";
    for (const std::string& n : names)
        for (const char* d : host_dirs) if (file_exists(std::string(d) + n)) return std::string(d) + n;
    for (const char* fb : {"/System/Library/Fonts/Supplemental/Arial.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"})
        if (file_exists(fb)) return fb;
    return "";
#else
    static const char* dirs[] = {"/System/Library/Fonts/Supplemental/", "/Library/Fonts/", "/System/Library/Fonts/",
                                 "/usr/share/fonts/truetype/msttcorefonts/", "/usr/share/fonts/truetype/liberation/",
                                 "/usr/share/fonts/truetype/liberation2/", "/usr/share/fonts/truetype/dejavu/",
                                 "/usr/share/fonts/liberation/", "/usr/share/fonts/dejavu/", "/usr/share/fonts/TTF/",
                                 "/usr/share/fonts/liberation-fonts/", "/usr/share/fonts/dejavu-sans-fonts/"};
    std::string home = getenv("HOME") ? getenv("HOME") : "";
    for (const std::string& n : names) {
        for (const char* d : dirs) if (file_exists(std::string(d) + n)) return std::string(d) + n;
        if (!home.empty() && file_exists(home + "/Library/Fonts/" + n)) return home + "/Library/Fonts/" + n;
        if (!home.empty() && file_exists(home + "/.fonts/" + n)) return home + "/.fonts/" + n;
    }
    for (const char* fb : {"/System/Library/Fonts/Helvetica.ttc", "/System/Library/Fonts/SFNS.ttf", "/System/Library/Fonts/Geneva.ttf"})
        if (file_exists(fb)) return fb;
    return "";
#endif
}

Bitmap* new_bitmap(int w, int h, int bpp)
{
    Bitmap* b = new Bitmap;
    b->magic = G_BITMAP;
    b->w = w; b->h = h; b->bpp = bpp;
    b->pitch = ((w * bpp + 31) / 32) * 4;
    b->bits.assign((size_t)b->pitch * (size_t)std::max(h, 1), 0);
    return b;
}
Bitmap* as_bitmap(HGDIOBJ h) { Gdi* g = (Gdi*)h; return (g && g->magic == G_BITMAP) ? (Bitmap*)g : nullptr; }
Dc* as_dc(HDC h) { Gdi* g = (Gdi*)h; return (g && g->magic == G_DC) ? (Dc*)g : nullptr; }

Dc* new_dc(HWND win)
{
    Dc* d = new Dc;
    d->magic = G_DC;
    d->win = win;
    if (!win) { d->default_bmp = new_bitmap(1, 1, 32); d->bmp = d->default_bmp; }
    return d;
}
inline uint32_t rgb_to_pixel(COLORREF c) { return ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF); }   // COLORREF 0x00BBGGRR -> 0x00RRGGBB

void put_px(Bitmap* b, int x, int y, uint32_t v)
{
    if (b->bpp != 32 || x < 0 || y < 0 || x >= b->w || y >= b->h) return;
    memcpy(&b->bits[(size_t)y * b->pitch + (size_t)x * 4], &v, 4);
}
uint32_t get_px(const Bitmap* b, int x, int y)
{
    uint32_t v = 0;
    if (b->bpp == 32 && x >= 0 && y >= 0 && x < b->w && y < b->h) memcpy(&v, &b->bits[(size_t)y * b->pitch + (size_t)x * 4], 4);
    else if (b->bpp == 24 && x >= 0 && y >= 0 && x < b->w && y < b->h) {
        const uint8_t* p = &b->bits[(size_t)y * b->pitch + (size_t)x * 3];
        v = p[0] | (p[1] << 8) | (p[2] << 16);
    }
    return v;
}
void fill_rect(Bitmap* b, int x0, int y0, int x1, int y1, uint32_t v)
{
    for (int y = std::max(y0, 0); y < std::min(y1, b->h); ++y)
        for (int x = std::max(x0, 0); x < std::min(x1, b->w); ++x) put_px(b, x, y, v);
}
}   // namespace

namespace mbp {
// a Bitmap loaded from a .bmp on disk (LoadImage LR_LOADFROMFILE)
HBITMAP gdi_load_bmp(const char* path)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    SDL_Surface* s = SDL_LoadBMP(host);
    if (!s) return nullptr;
    SDL_Surface* c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_BGRA32);
    SDL_DestroySurface(s);
    if (!c) return nullptr;
    Bitmap* b = new_bitmap(c->w, c->h, 32);
    for (int y = 0; y < c->h; ++y) memcpy(&b->bits[(size_t)y * b->pitch], (uint8_t*)c->pixels + (size_t)y * c->pitch, (size_t)c->w * 4);
    SDL_DestroySurface(c);
    return (HBITMAP)b;
}
}

extern "C" {
HDC WINAPI GetDC(HWND h) { return (HDC)new_dc(h ? h : (HWND)1); }
int WINAPI ReleaseDC(HWND, HDC dc) { Dc* d = as_dc(dc); if (!d) return 0; d->magic = 0; delete d; return 1; }
HDC WINAPI BeginPaint(HWND h, LPPAINTSTRUCT ps)
{
    memset(ps, 0, sizeof *ps);
    Win* w = win_of(h);
    if (!w) return nullptr;
    w->invalid = false;
    ps->hdc = (HDC)new_dc(h);
    ps->fErase = TRUE;
    ps->rcPaint.right = w->w; ps->rcPaint.bottom = w->h;
    return ps->hdc;
}
BOOL WINAPI EndPaint(HWND, const PAINTSTRUCT* ps)
{
    if (ps && ps->hdc) ReleaseDC(nullptr, ps->hdc);
    return TRUE;
}
HDC WINAPI CreateCompatibleDC(HDC) { return (HDC)new_dc(nullptr); }
BOOL WINAPI DeleteDC(HDC dc)
{
    Dc* d = as_dc(dc);
    if (!d) return FALSE;
    if (d->default_bmp) { d->default_bmp->magic = 0; delete d->default_bmp; }
    d->magic = 0;
    delete d;
    return TRUE;
}
HBITMAP WINAPI CreateCompatibleBitmap(HDC, int cx, int cy) { return (HBITMAP)new_bitmap(std::max(cx, 1), std::max(cy, 1), 32); }
HBITMAP WINAPI CreateDIBSection(HDC, const BITMAPINFO* bmi, UINT, void** ppv, HANDLE, DWORD)
{
    if (!bmi) return nullptr;
    int bpp = bmi->bmiHeader.biBitCount;
    if (bpp != 32 && bpp != 24) { set_last_error(87); if (ppv) *ppv = nullptr; return nullptr; }
    int w = bmi->bmiHeader.biWidth, h = bmi->bmiHeader.biHeight;
    bool topdown = h < 0;
    if (h < 0) h = -h;
    if (w <= 0 || h <= 0) { set_last_error(87); return nullptr; }
    Bitmap* b = new_bitmap(w, h, bpp);
    b->dib = true;
    // The caller owns the pixel pointer's layout: rows run in the direction biHeight's sign asks. Rows are stored top-down
    // here; a bottom-up request (positive height) is served by the same memory read bottom-up by callers that flip it.
    (void)topdown;
    if (ppv) *ppv = b->bits.data();
    return (HBITMAP)b;
}
HGDIOBJ WINAPI GetStockObject(int i)
{
    Brush* b = &g_stock[(i >= 0 && i < 20) ? i : 0];
    b->magic = G_BRUSH;
    b->color = (i == BLACK_BRUSH) ? 0 : 0xFFFFFF;
    return (HGDIOBJ)b;
}
HGDIOBJ WINAPI SelectObject(HDC dc, HGDIOBJ obj)
{
    Dc* d = as_dc(dc);
    Gdi* g = (Gdi*)obj;
    if (!d || !g) return nullptr;
    if (g->magic == G_BITMAP) { Bitmap* prev = d->bmp; d->bmp = (Bitmap*)g; return (HGDIOBJ)prev; }
    if (g->magic == G_FONT) { Font* prev = d->font; d->font = (Font*)g; return (HGDIOBJ)prev; }
    if (g->magic == G_BRUSH) return (HGDIOBJ)GetStockObject(WHITE_BRUSH);
    return nullptr;
}
BOOL WINAPI DeleteObject(HGDIOBJ obj)
{
    Gdi* g = (Gdi*)obj;
    if (!g) return FALSE;
    if (g->magic == G_BITMAP) { g->magic = 0; delete (Bitmap*)g; return TRUE; }
    if (g->magic == G_FONT) {
        Font* f = (Font*)g;
        if (f->font) TTF_CloseFont(f->font);
        g->magic = 0;
        delete f;
        return TRUE;
    }
    if (g->magic == G_BRUSH) return TRUE;           // stock objects are never freed
    return FALSE;
}
int WINAPI GetObjectA(HANDLE h, int cb, LPVOID pv)
{
    Bitmap* b = as_bitmap(h);
    if (!b || !pv || cb < (int)sizeof(BITMAP)) return 0;
    BITMAP* bm = (BITMAP*)pv;
    bm->bmType = 0; bm->bmWidth = b->w; bm->bmHeight = b->h; bm->bmWidthBytes = b->pitch; bm->bmPlanes = 1; bm->bmBitsPixel = (WORD)b->bpp;
    bm->bmBits = b->dib ? b->bits.data() : nullptr;
    return sizeof(BITMAP);
}
HFONT WINAPI CreateFontA(int h, int, int, int, int weight, DWORD italic, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR face)
{
    if (!ttf_init()) return nullptr;
    Font* f = new Font;
    f->magic = G_FONT;
    f->face = face ? face : "";
    f->bold = weight >= 600;
    f->italic = italic != 0;
    int px = h < 0 ? -h : (h > 0 ? (int)(h * 0.87f + 0.5f) : 12);        // negative = em height; positive = cell height
    f->px = std::max(px, 4);
    std::string file = find_font_file(f->face, f->bold, f->italic);
    if (file.empty()) { fprintf(stderr, "mb: CreateFont: no TrueType font found for \"%s\" (set MB_FONT=<file.ttf>)\n", f->face.c_str()); delete f; set_last_error(2); return nullptr; }
    f->font = TTF_OpenFont(file.c_str(), (float)f->px);
    if (!f->font) { fprintf(stderr, "mb: CreateFont: %s: %s\n", file.c_str(), SDL_GetError()); delete f; set_last_error(2); return nullptr; }
    return (HFONT)f;
}
int WINAPI GetDeviceCaps(HDC, int idx)
{
    switch (idx) {
    case 88: case 90: return 96;                       // LOGPIXELSX / LOGPIXELSY
    case 8: return GetSystemMetrics(SM_CXSCREEN);      // HORZRES
    case 10: return GetSystemMetrics(SM_CYSCREEN);     // VERTRES
    case 12: return 32;                                // BITSPIXEL
    case 14: return 1;                                 // PLANES
    case 2: return 1;                                  // TECHNOLOGY: DT_RASDISPLAY
    default: return 0;
    }
}
int WINAPI SetMapMode(HDC dc, int m) { Dc* d = as_dc(dc); if (!d) return 0; int p = d->map_mode; d->map_mode = m; return p; }
COLORREF WINAPI SetTextColor(HDC dc, COLORREF c) { Dc* d = as_dc(dc); if (!d) return 0xFFFFFFFF; COLORREF p = d->text; d->text = c; return p; }
COLORREF WINAPI SetBkColor(HDC dc, COLORREF c) { Dc* d = as_dc(dc); if (!d) return 0xFFFFFFFF; COLORREF p = d->bk; d->bk = c; return p; }
UINT WINAPI SetTextAlign(HDC dc, UINT a) { Dc* d = as_dc(dc); if (!d) return 0xFFFFFFFF; UINT p = d->align; d->align = a; return p; }

static void text_extent(Dc* d, const std::string& ansi, int* w, int* h)
{
    *w = 0; *h = 0;
    if (!d->font || !d->font->font) return;
    *h = TTF_GetFontHeight(d->font->font);
    if (ansi.empty()) return;
    std::string u = ansi_to_utf8(ansi.c_str());
    TTF_GetStringSize(d->font->font, u.c_str(), 0, w, h);
    *h = TTF_GetFontHeight(d->font->font);
}
BOOL WINAPI GetTextExtentPoint32A(HDC dc, LPCSTR s, int n, LPSIZE sz)
{
    Dc* d = as_dc(dc);
    if (!d || !sz || !d->font) { if (sz) sz->cx = sz->cy = 0; return FALSE; }
    int w, h;
    text_extent(d, std::string(s, (size_t)(n < 0 ? 0 : n)), &w, &h);
    sz->cx = w; sz->cy = h;
    return TRUE;
}
BOOL WINAPI ExtTextOutA(HDC dc, int x, int y, UINT opts, const RECT* rc, LPCSTR s, UINT n, const INT*)
{
    Dc* d = as_dc(dc);
    if (!d) return FALSE;
    Bitmap* b = d->bmp;
    std::string ansi(s ? s : "", s ? n : 0);
    int tw = 0, th = 0;
    if (d->font) text_extent(d, ansi, &tw, &th);
    int ox = x, oy = y;
    if ((d->align & 6) == 6) ox -= tw / 2; else if (d->align & 2) ox -= tw;       // TA_CENTER / TA_RIGHT
    if ((d->align & 24) == 24) oy -= d->font && d->font->font ? TTF_GetFontAscent(d->font->font) : 0;   // TA_BASELINE
    else if (d->align & 8) oy -= th;                                              // TA_BOTTOM
    if (!b || d->win) return TRUE;                // window DCs have no pixel store
    uint32_t bk = rgb_to_pixel(d->bk);
    if ((opts & ETO_OPAQUE) && rc) fill_rect(b, rc->left, rc->top, rc->right, rc->bottom, bk);
    else fill_rect(b, ox, oy, ox + tw, oy + th, bk);       // GDI's default background mode is OPAQUE: the cell gets the back color
    if (ansi.empty() || !d->font || !d->font->font) return TRUE;
    std::string u = ansi_to_utf8(ansi.c_str());
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* raw = TTF_RenderText_Blended(d->font->font, u.c_str(), 0, white);
    if (!raw) return TRUE;                          // nothing visible (a lone space)
    SDL_Surface* sf = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ARGB8888);
    SDL_DestroySurface(raw);
    if (!sf) return TRUE;
    uint32_t fg = rgb_to_pixel(d->text);
    for (int sy = 0; sy < sf->h; ++sy) {
        const uint32_t* row = (const uint32_t*)((const uint8_t*)sf->pixels + (size_t)sy * sf->pitch);
        for (int sx = 0; sx < sf->w; ++sx) {
            uint32_t a = row[sx] >> 24;
            if (!a) continue;
            int px = ox + sx, py = oy + sy;
            uint32_t dst = get_px(b, px, py), out = 0;
            for (int sh = 0; sh <= 16; sh += 8) {
                uint32_t dc_ = (dst >> sh) & 0xFF, fc = (fg >> sh) & 0xFF;
                out |= ((dc_ * (255 - a) + fc * a + 127) / 255) << sh;
            }
            put_px(b, px, py, out);
        }
    }
    SDL_DestroySurface(sf);
    return TRUE;
}
BOOL WINAPI TextOutA(HDC dc, int x, int y, LPCSTR s, int n) { return ExtTextOutA(dc, x, y, 0, nullptr, s, (UINT)n, nullptr); }
BOOL WINAPI StretchBlt(HDC dst, int dx, int dy, int dw, int dh, HDC src, int sx, int sy, int sw, int sh, DWORD rop)
{
    Dc* d = as_dc(dst); Dc* s = as_dc(src);
    if (!d || !s || rop != SRCCOPY) return FALSE;
    if (!d->bmp || d->win) return TRUE;                // window DC: no pixel store
    if (!s->bmp) return FALSE;
    if (dw == 0 || dh == 0 || sw == 0 || sh == 0) return TRUE;
    for (int y = 0; y < std::abs(dh); ++y)
        for (int x = 0; x < std::abs(dw); ++x) {
            int sxp = sx + (int)((long long)x * sw / dw), syp = sy + (int)((long long)y * sh / dh);
            put_px(d->bmp, dx + x, dy + y, get_px(s->bmp, sxp, syp));
        }
    return TRUE;
}
BOOL WINAPI BitBlt(HDC dst, int x, int y, int cx, int cy, HDC src, int x1, int y1, DWORD rop) { return StretchBlt(dst, x, y, cx, cy, src, x1, y1, cx, cy, rop); }

HANDLE WINAPI LoadImageA(HINSTANCE, LPCSTR name, UINT type, int, int, UINT flags)
{
    if ((uintptr_t)name < 0x10000 || !(flags & LR_LOADFROMFILE)) { set_last_error(1814); return nullptr; }   // no embedded resources
    if (type == IMAGE_BITMAP) {
        HBITMAP b = gdi_load_bmp(name);
        if (!b) set_last_error(2);
        return (HANDLE)b;
    }
    return (HANDLE)LoadIconA(nullptr, name);
}
}   // extern "C"
