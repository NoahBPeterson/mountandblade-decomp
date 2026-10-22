// Slice s004b8f40: the tiny HTTP/1.0 client of the game's update checker. It parses "scheme://host[:port]/path"
// (0x4b8ff0), sends a fixed GET/POST request over winsock (0x4b9110, not byte-exact), splits the reply into
// headers and body with a growable byte buffer (0x4b8f40), and has two registry helpers for the key file
// (0x4b96d0 create, 0x4b9720 read a string value). Names provisional.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
int _isalpha(int c);                            // 0x537f65 (CRT; also the compiler's isalpha)
int _isdigit(int c);                            // 0x537fa4

// ---- engine text log stream (global object at 0x5a9f60, see s00418a70): slot 10 (+0x28) is w_cstr --------
struct stream_base {
    virtual void reset();
    virtual void rewind();
    virtual stream_base& w_stream(void* o) = 0;
    virtual stream_base& w_float(void* v) = 0;
    virtual stream_base& w_bool(void* v) = 0;
    virtual stream_base& w_u64(void* v) = 0;
    virtual stream_base& w_i64(void* v) = 0;
    virtual stream_base& w_uint(void* v) = 0;
    virtual stream_base& w_int(int* v) = 0;
    virtual stream_base& w_byte(void* v) = 0;
    virtual stream_base& w_cstr(const char* s) = 0;
};
struct log_stream : stream_base {
    virtual stream_base& w_stream(void* o);
    virtual stream_base& w_float(void* v);
    virtual stream_base& w_bool(void* v);
    virtual stream_base& w_u64(void* v);
    virtual stream_base& w_i64(void* v);
    virtual stream_base& w_uint(void* v);
    virtual stream_base& w_int(int* v);
    virtual stream_base& w_byte(void* v);
    virtual stream_base& w_cstr(const char* s);
};
extern log_stream g_log;                        // 0x5a9f60

// ---- engine string (16 bytes, read cursor; see s0042c0f0/tstring.h) ----------------------------------------
struct tstring {
    int pos;                                    // +0
    char* buf;                                  // +4
    int cap;                                    // +8
    int len;                                    // +0xc
    void assign(const char* s);                 // 0x42bcb0
};

// ---- growable byte buffer (12 bytes: data, pos, cap) -------------------------------------------------------
struct byte_buf {
    char* data;                                 // +0
    char* pos;                                  // +4
    int cap;                                    // +8
};

// ---- record filled by 0x4b9110: url copy, header text, body buffer, body length --------------------------
struct http_result {
    char* url;                                  // +0
    char* headers;                              // +4
    char* body;                                 // +8
    int   body_len;                             // +0xc
};

// request text fragments (written as complete C strings; the narrow literals are 8-byte aligned)
static const char kSchemeHttp[] = "HTTP";        // 0x593668
static const char kCrlf[] = "\r\n";              // 0x593670
static const char kHostHdr[] = "Host: ";         // 0x593674
static const char kHttpVer[] = " HTTP/1.0\r\n";  // 0x593680
static const char kPost[] = "POST ";             // 0x59368c
static const char kUserAgent[] = "User-Agent: Mozilla/4.0\r\n";          // 0x593694
static const char kAcceptEnc[] = "Accept-Encoding: gzip, deflate\r\n";   // 0x5936b0
static const char kAcceptLang[] = "Accept-Language: en-us\r\n";          // 0x5936d4
static const char kAccept[] = "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, "
    "application/vnd.ms-excel, application/msword, application/vnd.ms-powerpoint, */*\r\n";  // 0x5936f0
static const char kGet[] = "GET ";               // 0x593794
static const char kSlash[] = "/";                // 0x59379c
static const char kKeyPath[] = "Software\\MountAndBladeKeys";   // 0x593a0c
static const char kNewKey[] = "Creating new Registry Key";      // 0x5939f0
static const char kEmpty[] = "";                 // 0x54a480

void __cdecl sock_send_str(int sock, const char* s);                    // 0x4b8fc0
void __cdecl parse_url(const char* url, LPSTR scheme, int scheme_n, LPSTR host, int host_n,
                       LPSTR path, int path_n, long* port);             // 0x4b8ff0
int __cdecl http_fetch(const char* url, const char* post, int post_len, http_result** out);  // 0x4b9110
void __cdecl open_keys_key(PHKEY out);                                  // 0x4b96d0
void __cdecl bytes_append(byte_buf* v, const char* src, unsigned n);    // 0x4b8f40

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
        *port = atol(ph + 1);
    }
    free(s);
}

// @ 0x4b9110
int __cdecl http_fetch(const char* url, const char* post, int post_len, http_result** out)
{
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
    (*out)->url = urlcopy;
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
    (*out)->headers = hdr.data;
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
    (*out)->body = hdr.data;
    (*out)->body_len = hdr.pos - hdr.data;
    closesocket(sock);
    return 0;
}

// @ 0x4b96d0
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
