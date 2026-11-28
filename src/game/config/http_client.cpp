//-----------------------------------------------------------------------------
// File: http_client.cpp  (0x4b8f40..0x4b96d0)
//
// Desc: the update checker's HTTP/1.0 client: a growable byte buffer, a socket string send, URL split and the
//       request / reply (headers and body). Split out of launcher_options_dialog.cpp (2026-11-27): in the original
//       http_fetch was not visible to get_latest_version 0x4b9aa0 (the caller keeps the url / headers zero stores
//       that cl deletes when the callee's body is in the same unit: c2scope docs/c2/ipa.md, vs2005-cpp 60d3f76),
//       so these functions sat in their own object. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "platform/image_addrs.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "game/config/launcher_options_dialog.h"
#include "engine/core/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "game/config/core.h"
#include "game/config/config.h"
#include "game/ui/gui_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/render/dev_buffer.h"
#include "engine/scene/frustum.h"
#include "engine/math/mat4_include.h"
#include "engine/render/render_batch.h"      // (listed directly: the resolution audit scans only the source's own includes)

extern "C" int __cdecl isalpha(int c);                                 // 0x537f65 (CRT; the compiler's isalpha)
extern "C" int __cdecl isdigit(int c);                                 // 0x537fa4

// request text fragments of the update checker (the narrow literals live in .rdata at 0x593668..0x593794)
#define kUserAgent "User-Agent: Mozilla/4.0\r\n"                          // 0x593694
#define kAcceptEnc "Accept-Encoding: gzip, deflate\r\n"                   // 0x5936b0
#define kAcceptLang "Accept-Language: en-us\r\n"                          // 0x5936d4
#define kAccept "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, " \
    "application/vnd.ms-excel, application/msword, application/vnd.ms-powerpoint, */*\r\n"  // 0x5936f0

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
    char* p = strchr(s, ':');
    if (p) {
        *p = '\0';
        p++;
        lstrcpynA(scheme, s, scheme_n);
    } else {
        lstrcpynA(scheme, "HTTP", scheme_n);
        p = s;
    }
    if (*p == '/' && p[1] == '/')
        p += 2;
    char* start = p;
    for (;;) {
        char c = *start;
        if (!(isalpha(c) || isdigit(c) || c == '-' || c == '.' || c == ':'))
            break;
        if (*start == 0)
            break;
        start++;
    }
    *start = '\0';
    lstrcpynA(host, p, host_n);
    lstrcpynA(path, url + (start - s), path_n);
    char* ph = strchr(host, ':');
    if (ph) {
        *ph = '\0';
        *port = atoi(ph + 1);
    }
    free(s);
}

// @ 0x4b9110
int __cdecl http_fetch(const char* url, const char* extra, const char* post, int post_len, http_result* out)
{
    struct { char scheme[20]; char req[1024]; char host[256]; char buf[512]; char path[1024]; } b;
    long port;
    parse_url(url, b.scheme, 20, b.host, 256, b.path, 1024, &port);
    // strcmp with a literal of four characters: cl folds it to the same 5-byte repe cmpsb as the original
    // (memcmp with a 5 count puts the length in ecx before the buffer address; strcmp matches the original)
    if (strcmp(b.scheme, "HTTP") != 0)
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
    hostent* he = gethostbyname(b.host);
    DWORD host_addr = (DWORD)he;            // the IPv4 address: 4 bytes (a long is 8 at LP64)
    if (he)
        host_addr = *(DWORD*)he->h_addr_list[0];
    addr.sin_addr.s_addr = host_addr;
    if (connect(sock, (sockaddr*)&addr, 16) != 0)
        return 1;                           // (the original leaks the socket here: no closesocket)
    if (b.path[0] == '\0')
        lstrcpynA(b.path, "/", 1024);
    if (post == 0) {
        sock_send_str(sock, "GET ");
        strcpy(b.req, "GET ");
    } else {
        sock_send_str(sock, "POST ");
        strcpy(b.req, "POST ");
    }
    sock_send_str(sock, b.path);
    strcat(b.req, b.path);
    sock_send_str(sock, " HTTP/1.0\r\n");
    strcat(b.req, " HTTP/1.0\r\n");
    sock_send_str(sock, kAccept);
    strcat(b.req, kAccept);
    sock_send_str(sock, kAcceptLang);
    strcat(b.req, kAcceptLang);
    sock_send_str(sock, kAcceptEnc);
    strcat(b.req, kAcceptEnc);
    sock_send_str(sock, kUserAgent);
    strcat(b.req, kUserAgent);
    if (post_len != 0) {
        wsprintfA(b.buf, "Content-Length: %ld\r\n", post_len);
        sock_send_str(sock, b.buf);
        strcat(b.req, b.buf);
    }
    sock_send_str(sock, "Host: ");
    strcat(b.req, "Host: ");
    sock_send_str(sock, b.host);
    strcat(b.req, b.host);
    sock_send_str(sock, "\r\n");
    strcat(b.req, "\r\n");
    if (extra != 0 && extra[0] != '\0') {
        sock_send_str(sock, extra);
        strcat(b.req, extra);
    }
    sock_send_str(sock, "\r\n");
    strcat(b.req, "\r\n");
    if (post != 0 && post_len != 0) {
        send(sock, post, post_len, 0);
        ((char*)post)[post_len] = '\0';
        strcat(b.req, post);
    }
    char* urlcopy = (char*)malloc(strlen(b.req) * 4);
    out->url = urlcopy;
    { char* s = b.req; char* d = urlcopy; char c; do { c = *s++; *d++ = c; } while (c); }
    // header lines, one recv'd byte at a time
    byte_buf hdr;
    hdr.cap = 10;
    hdr.data = (char*)malloc(hdr.cap);
    hdr.pos = hdr.data;
    int blank = 0;
    int flag = 0;
    do {
        int r = recv(sock, b.buf, 1, 0);
        if (r < 0)
            flag = 1;
        // (inline in the original: grow once, by doubling, when the buffer is full)
        if (b.buf[0] != '\n') {
            if (b.buf[0] != '\r')
                blank++;
        } else {
            if (blank == 0)
                flag = 1;
            blank = 0;
        }
        char c = b.buf[0];
        unsigned used = (unsigned)(hdr.pos - hdr.data);
        if (used >= (unsigned)hdr.cap) {
            hdr.cap *= 2;
            hdr.data = (char*)realloc(hdr.data, hdr.cap);
            hdr.pos = hdr.data + used;
        }
        *hdr.pos++ = c;
    } while (!flag);
    out->headers = hdr.data;
    *hdr.pos = '\0';
    // body, in 0x1ff-byte chunks
    byte_buf bd;
    bd.cap = 10;
    bd.data = (char*)malloc(bd.cap);
    bd.pos = bd.data;
    int n;
    do {
        n = recv(sock, b.buf, 0x1ff, 0);
        if (n < 0)
            break;
        b.buf[n] = '\0';
        bytes_append(&bd, b.buf, n);
    } while (n > 0);
    *bd.pos = '\0';
    out->body = bd.data;
    out->body_len = bd.pos - bd.data;
    closesocket(sock);
    return 0;
}

