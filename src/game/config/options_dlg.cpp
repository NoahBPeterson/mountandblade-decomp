//-----------------------------------------------------------------------------
// File: options_dlg.cpp  (0x4b8f40..0x4b9e70)
//
// Desc: the update checker's HTTP/1.0 client (URL split, request, reply split into headers and body with a
//       growable byte buffer), two registry helpers for the key file, the launcher's options dialog (tab
//       switching, the DXDiag video-memory probe, paint), the start-up display probe and the version check.
//       Names provisional. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: the map puts 0x4b9b90 between this object and the next (class change OptionsDlg|Core, low). The
// function at 0x4b9b90 is itself an OptionsDlg method (choose_dx_settings), so the object runs on to 0x4b9e70
// (CreateGameOptionsPage::refresh, the first function of the next class); see MIGRATION.md.
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "./options_dlg.h"
#include "./core.h"
#include "./config.h"
#include "../strat/gui_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../dev_buffer.h"
#include "../../include/frustum.h"
#include "../../include/mat4.h"

extern "C" int __cdecl _isalpha(int c);                                 // 0x537f65 (CRT; the compiler's isalpha)
extern "C" int __cdecl _isdigit(int c);                                 // 0x537fa4

// request text fragments of the update checker (the narrow literals live in .rdata at 0x593668..0x593794)
static const char kUserAgent[] = "User-Agent: Mozilla/4.0\r\n";          // 0x593694
static const char kAcceptEnc[] = "Accept-Encoding: gzip, deflate\r\n";   // 0x5936b0
static const char kAcceptLang[] = "Accept-Language: en-us\r\n";          // 0x5936d4
static const char kAccept[] = "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, "
    "application/vnd.ms-excel, application/msword, application/vnd.ms-powerpoint, */*\r\n";  // 0x5936f0

// @ 0x4b8f40
// append n bytes from src to the buffer, doubling the capacity (realloc) until it fits
void __cdecl bytes_append(byte_buf* v, const char* src, unsigned n)
{
    while ((unsigned)((v->pos - v->data) + n) >= (unsigned)v->cap) {
        char* d = v->data;
        unsigned used = (unsigned)(v->pos - d);
        v->cap *= 2;
        v->data = (char*)realloc(d, v->cap);
        v->pos = v->data + used;
    }
    memcpy(v->pos, src, n);
    v->pos += n;
}

// @ 0x4b8fc0
void __cdecl sock_send_str(int sock, const char* s)
{
    int n = strlen(s);
    send(sock, s, n, 0);
}

// @ 0x4b8ff0
void __cdecl parse_url(const char* url, LPSTR scheme, int scheme_n, LPSTR host, int host_n,
                       LPSTR path, int path_n, long* port)
{
    *path = '\0';
    *host = '\0';
    *scheme = '\0';
    *port = 80;
    char* s = _strdup(url);
    _strupr(s);
    char* colon = strchr(s, ':');
    char* p;
    if (colon) {
        *colon = '\0';
        lstrcpynA(scheme, s, scheme_n);
        p = colon + 1;
    } else {
        lstrcpynA(scheme, "HTTP", scheme_n);
        p = s;
    }
    if (*p == '/' && p[1] == '/')
        p += 2;
    char* start = p;
    while (_isalpha((unsigned char)*p) || _isdigit((unsigned char)*p) || *p == '-' || *p == '.' ||
           *p == ':')
        p++;
    *p = '\0';
    lstrcpynA(host, start, host_n);
    lstrcpynA(path, url + (p - s), path_n);
    char* ph = strchr(host, ':');
    if (ph) {
        *ph = '\0';
        *port = atoi(ph + 1);
    }
    free(s);
}

// @ 0x4b9110
int __cdecl http_fetch(const char* url, const char* post, int post_len, int reserved, int out_)
{
    http_result* out = (http_result*)out_;
    char scheme[20];
    char host[256];
    char path[1024];
    long port;
    parse_url(url, scheme, 20, host, 256, path, 1024, &port);
    char req[4096];
    char buf[4096];
    if (memcmp(scheme, "HTTP", 5) != 0)
        return 1;
    WSADATA wsa;
    if (WSAStartup(0x101, &wsa) != 0)
        return 1;
    int sock = socket(2, 1, 0);
    if (sock == -1)
        return 1;
    sockaddr_in addr;
    addr.sin_family = 2;
    addr.sin_port = htons((u_short)port);
    hostent* he = gethostbyname(host);
    if (he)
        addr.sin_addr.s_addr = *(unsigned long*)he->h_addr_list[0];
    if (connect(sock, (sockaddr*)&addr, 16) != 0) {
        closesocket(sock);
        return 1;
    }
    if (path[0] == '\0')
        lstrcpynA(path, "/", 1024);
    if (post == 0) {
        sock_send_str(sock, "GET ");
        strcpy(req, "GET ");
    } else {
        sock_send_str(sock, "POST ");
        strcpy(req, "POST ");
    }
    sock_send_str(sock, path);
    strcat(req, path);
    sock_send_str(sock, " HTTP/1.0\r\n");
    strcat(req, " HTTP/1.0\r\n");
    sock_send_str(sock, kAccept);
    strcat(req, kAccept);
    sock_send_str(sock, kAcceptLang);
    strcat(req, kAcceptLang);
    sock_send_str(sock, kAcceptEnc);
    strcat(req, kAcceptEnc);
    sock_send_str(sock, kUserAgent);
    strcat(req, kUserAgent);
    if (post_len != 0) {
        char clen[256];
        wsprintfA(clen, "Content-Length: %ld\r\n", post_len);
        sock_send_str(sock, clen);
        strcat(req, clen);
    }
    sock_send_str(sock, "Host: ");
    strcat(req, "Host: ");
    sock_send_str(sock, host);
    strcat(req, host);
    sock_send_str(sock, "\r\n");
    strcat(req, "\r\n");
    if (post != 0 && post[0] != '\0') {
        sock_send_str(sock, post);
        strcat(req, post);
    }
    sock_send_str(sock, "\r\n");
    strcat(req, "\r\n");
    if (post != 0 && post_len != 0) {
        send(sock, post, post_len, 0);
        ((char*)post)[post_len] = '\0';
        strcat(req, post);
    }
    char* urlcopy = (char*)malloc(strlen(req + 1) * 4);
    out->url = urlcopy;
    strcpy(urlcopy, req);
    // header lines, one recv'd byte at a time
    byte_buf hdr;
    hdr.data = (char*)malloc(10);
    hdr.pos = hdr.data;
    hdr.cap = 10;
    int blank = 0;
    int flag = 0;
    do {
        int r = recv(sock, buf, 1, 0);
        if (r < 0)
            flag = 1;
        if (buf[0] == '\n') {
            if (blank == 0)
                flag = 1;
            blank = 0;
        } else if (buf[0] != '\r') {
            blank++;
        }
        bytes_append(&hdr, buf, 1);
    } while (!flag);
    *hdr.pos = '\0';
    out->headers = hdr.data;
    // body, in 0x1ff-byte chunks
    hdr.data = (char*)malloc(10);
    hdr.pos = hdr.data;
    hdr.cap = 10;
    for (;;) {
        int n = recv(sock, buf, 0x1ff, 0);
        if (n < 0)
            break;
        buf[n] = '\0';
        bytes_append(&hdr, buf, n);
        if (n <= 0)
            break;
    }
    *hdr.pos = '\0';
    out->body = hdr.data;
    out->body_len = hdr.pos - hdr.data;
    closesocket(sock);
    return 0;
}

// @ 0x4b96d0
// (the log object is reached through its address, not g_log: cl would devirtualise a call on the extern object)
void __cdecl open_keys_key(PHKEY out)
{
    DWORD disp;
    RegCreateKeyExA((HKEY)0x80000001, "Software\\MountAndBladeKeys", 0, 0, 0, 0x20007, 0, out, &disp);
    if (disp != 1) {
        return;
    }
    ((log_stream*)0x5a9f60)->w_cstr("Creating new Registry Key");
}

// @ 0x4b9720
void __cdecl read_key_string(HKEY* key, LPCSTR name, tstring* out)
{
    DWORD type;
    DWORD size = 0x100;
    char buf[256];
    buf[255] = '\0';
    out->assign("");
    if (*key) {
        if (RegQueryValueExA(*key, name, 0, &type, (LPBYTE)buf, &size) == 0)
            out->assign(buf);
    }
}

// ---- DirectX diagnostic COM interfaces (dxdiag.h is not on the toolchain) --------------------------------------
// DXDiag bases its objects directly on IUnknown; only the slots this object calls are named
// (Initialize +0xc, GetRootContainer +0x10; GetChildContainer +0x14, EnumChildContainerName +0x10, GetProp +0x20).
struct DXDIAG_INIT_PARAMS {
    DWORD dwSize;
    DWORD dwDxDiagHeaderVersion;
    BOOL bAllowWHQLChecks;
    void* pReserved;
};
struct IDxDiagContainer : public IUnknown {
    virtual HRESULT __stdcall GetNumberOfChildContainers(DWORD* n) = 0;
    virtual HRESULT __stdcall EnumChildContainerName(DWORD idx, WCHAR* name, DWORD cap) = 0;
    virtual HRESULT __stdcall GetChildContainer(const WCHAR* name, IDxDiagContainer** c) = 0;
    virtual HRESULT __stdcall GetNumberOfProps(DWORD* n) = 0;
    virtual HRESULT __stdcall EnumPropName(DWORD idx, WCHAR* name, DWORD cap) = 0;
    virtual HRESULT __stdcall GetProp(WCHAR* name, VARIANT* v) = 0;
    virtual HRESULT __stdcall GetPropByName(const WCHAR* prop, const WCHAR* name, VARIANT* v) = 0;
    virtual HRESULT __stdcall GetContainerByIndex(DWORD idx, IDxDiagContainer** c) = 0;
};
struct IDxDiagProvider : public IUnknown {
    virtual HRESULT __stdcall Initialize(void* params) = 0;
    virtual HRESULT __stdcall GetRootContainer(IDxDiagContainer** root) = 0;
};
extern const GUID CLSID_DxDiagProvider;     // 0x59c8f4
extern const GUID IID_IDxDiagProvider;      // 0x59c8e4

// 0x4b9a30: reads a DXDiag container property (as a wide string) into a char buffer. The callee itself passes the
// property name (0x593a94, "szDisplayMemoryEnglish") to GetProp; the original also carries a fourth (unused)
// stack argument, which the mangled symbol records as int.
long __stdcall container_prop_string_impl(IDxDiagContainer* obj, WCHAR* name, char* buf, unsigned cap);


long __cdecl _atol(const char*);                                        // 0x53734d (atoi/atol)


// @ 0x4b9aa0
long __cdecl get_latest_version()
{
    int body[4];
    body[0] = 0;
    body[1] = 0;
    body[2] = 0;
    if (http_fetch("http://www.taleworlds.com/mb_version.txt", 0, 0, 0, (int)body) > 0)
        return -1;
    return _atol((const char*)body[2]);
}

// @ 0x4b9790
void OptionsDlg::set_active_tab(int tab)
{
    int show = (tab != 0) - 1 & 5;
    HWND h;
    ShowWindow(GetDlgItem(hwnd, 0x43f), show);
    ShowWindow(GetDlgItem(hwnd, 0x449), show);
    ShowWindow(GetDlgItem(hwnd, 0x447), show);
    ShowWindow(GetDlgItem(hwnd, 0x450), show);
    ShowWindow(GetDlgItem(hwnd, 0x43e), show);
    ShowWindow(GetDlgItem(hwnd, 0x441), show);
    ShowWindow(GetDlgItem(hwnd, 0x493), show);
    ShowWindow(GetDlgItem(hwnd, 0x468), show);
    ShowWindow(GetDlgItem(hwnd, 0x444), show);
    ShowWindow(GetDlgItem(hwnd, 0x448), show);
    ShowWindow(GetDlgItem(hwnd, 0x445), show);
    ShowWindow(GetDlgItem(hwnd, 0x487), show);
    ShowWindow(GetDlgItem(hwnd, 0x460), show);
    ShowWindow(GetDlgItem(hwnd, 0x45f), show);
    ShowWindow(GetDlgItem(hwnd, 0x462), show);
    ShowWindow(GetDlgItem(hwnd, 0x465), show);
    ShowWindow(GetDlgItem(hwnd, 0x492), show);
    ShowWindow(GetDlgItem(hwnd, 0x467), show);
    show = (tab != 1) - 1 & 5;
    ShowWindow(GetDlgItem(hwnd, 0x452), show);
    ShowWindow(GetDlgItem(hwnd, 0x446), show);
    ShowWindow(GetDlgItem(hwnd, 0x44d), show);
    show = (tab != 2) - 1 & 5;
    ShowWindow(GetDlgItem(hwnd, 0x443), show);
    ShowWindow(GetDlgItem(hwnd, 0x442), show);
    show = (tab != 3) - 1 & 5;
    ShowWindow(GetDlgItem(hwnd, 0x44c), show);
    ShowWindow(GetDlgItem(hwnd, 0x44a), show);
}

// @ 0x4b99c0
void run_settings_probe()
{
    if (g_app_obj.m_bWindowed)
        return;
    g_app_obj.Pause(true);
    g_app_obj.m_bWindowed = 1;
    if (g_app_obj.ChooseInitialD3DSettings() < 0)
        g_app_obj.DisplayErrorMsg(0x8200000c, (DWORD)1);
    if (g_app_obj.ToggleFullscreen() < 0)
        g_app_obj.DisplayErrorMsg(0x8200000c, (DWORD)1);
    g_app_obj.Pause(false);
}

// @ 0x4b9a30
// Reads a DXDiag container property (a BSTR) into a char buffer; stdcall with four stack arguments (ret 0x10), ecx
// is not used. A failed GetProp returns its code, a non-BSTR value E_INVALIDARG (neither clears the variant).
long __stdcall container_prop_string_impl(IDxDiagContainer* obj, WCHAR* name, char* buf, unsigned cap)
{
    VARIANT result;
    VariantInit(&result);
    HRESULT hr = obj->GetProp(name, &result);
    if (hr < 0)
        return hr;
    if (result.vt != VT_BSTR)
        return 0x80070057;
    wcstombs(buf, result.bstrVal, cap);
    buf[cap - 1] = 0;
    VariantClear(&result);
    return 0;
}

// @ 0x4b9af0
void OptionsDlg::paint()
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(paint_hwnd, &ps);
    HDC mem = CreateCompatibleDC(dc);
    HGDIOBJ old = SelectObject(mem, back_bitmap);
    BITMAP bm;
    GetObjectA(back_bitmap, sizeof(BITMAP), &bm);
    BitBlt(dc, 0x108, 0x3f, 0x154, 0x113, mem, 0, 0, 0xcc0020);
    SelectObject(mem, old);
    DeleteDC(mem);
    EndPaint(paint_hwnd, &ps);
}

// @ 0x4b9b90
// Picks the texture detail / shader defaults from the video memory DXDiag reports ("32.0 MB" ...); the options block
// (a Config copy on the caller's stack) is only written when `overwrite` is set, except for the low-memory case.
void OptionsDlg::choose_dx_settings(Config* cfg, bool overwrite)
{
    MEMORYSTATUS mem;                           // zeroed field by field (dwLength stays 0); the result is never read
    mem.dwLength = 0;
    mem.dwMemoryLoad = 0;
    mem.dwTotalPhys = 0;
    mem.dwAvailPhys = 0;
    mem.dwTotalPageFile = 0;
    mem.dwAvailPageFile = 0;
    mem.dwTotalVirtual = 0;
    mem.dwAvailVirtual = 0;
    GlobalMemoryStatus(&mem);

    IDxDiagContainer* devices = 0;
    IDxDiagContainer* display = 0;
    IDxDiagContainer* root = 0;
    IDxDiagProvider* provider = 0;
    DXDIAG_INIT_PARAMS params;
    WCHAR name[0x100];
    char mem_text[0x100];

    CoInitialize(0);
    do {
        if (CoCreateInstance(CLSID_DxDiagProvider, 0, 1, IID_IDxDiagProvider, (void**)&provider) < 0)
            break;
        if (!provider)
            break;
        params.dwSize = 0x10;
        params.dwDxDiagHeaderVersion = 0x6f;
        params.bAllowWHQLChecks = 0;
        params.pReserved = 0;
        if (provider->Initialize(&params) < 0)
            break;
        if (provider->GetRootContainer(&root) < 0)
            break;
        if (root->GetChildContainer(L"DxDiag_DisplayDevices", &devices) < 0)
            break;
        if (devices->EnumChildContainerName(0, name, 0x100) < 0)
            break;
        if (devices->GetChildContainer(name, &display) < 0)
            break;
        if (container_prop_string_impl(display, L"szDisplayMemoryEnglish", mem_text, 0x100) < 0)
            break;
        tstring s(mem_text);
        s.to_upper();
        if (s.equals("4.0 MB") || s.equals("8.0 MB") || s.equals("16.0 MB")) {
            cfg->texture_detail = 0;
            cfg->antialiasing = 0;
            cfg->use_vertex_shaders = 0;
            cfg->shadowmap_quality = 0;
        } else if (s.equals("32.0 MB")) {
            if (overwrite) {
                cfg->texture_detail = 0x14;
                cfg->use_vertex_shaders = 0;
                cfg->shadowmap_quality = 0;
            }
        } else if (s.equals("64.0 MB")) {
            if (overwrite) {
                cfg->texture_detail = 0x3c;
                cfg->use_vertex_shaders = 0;
                cfg->shadowmap_quality = 0;
            }
        } else if (s.equals("128.0 MB")) {
            if (overwrite) {
                cfg->texture_detail = 0x50;
                cfg->shadowmap_quality = 1;
            }
        } else {
            s.equals("256.0 MB");
            if (overwrite) {
                cfg->texture_detail = 0x64;
                cfg->shadowmap_quality = 1;
            }
        }
    } while (0);
    if (display) {
        display->Release();
        display = 0;
    }
    if (devices) {
        devices->Release();
        devices = 0;
    }
    if (root) {
        root->Release();
        root = 0;
    }
    if (provider)
        provider->Release();
}
