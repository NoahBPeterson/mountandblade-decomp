// port/platform/net.cpp: Winsock over POSIX sockets, for the launcher's version check (src/game/config/launcher_options_dialog.cpp:
// socket / connect / send / recv / gethostbyname / htons / inet_addr). The functions are exported as mbws_<name>:
// winsock2.h maps the game's names onto them (the plain names belong to the C library, with different types).
#include "mbp_internal.h"
#include <winsock2.h>
#undef htons
#undef inet_addr

extern "C" {
int mbn_socket(void);
int mbn_connect_v4(int fd, uint32_t addr_be, uint16_t port_be);
int mbn_send(int fd, const char* b, int n, int flags);
int mbn_recv(int fd, char* b, int n, int flags);
int mbn_close(int fd);
int mbn_resolve_v4(const char* name, uint32_t* addr_be, char* name_out, int cap);
uint32_t mbn_inet_addr(const char* s);

int WINAPI mbws_WSAStartup(WORD wVersionRequested, LPWSADATA d)
{
    if (!d) return 10014;                                                  // WSAEFAULT
    memset(d, 0, sizeof *d);
    d->wVersion = wVersionRequested;
    d->wHighVersion = 0x0202;
    strcpy(d->szDescription, "POSIX sockets");
    strcpy(d->szSystemStatus, "Running");
    d->iMaxSockets = 0x7FFF;
    return 0;
}
int WINAPI mbws_WSACleanup(void) { return 0; }
SOCKET WINAPI mbws_socket(int af, int type, int proto)
{
    if (af != AF_INET || type != SOCK_STREAM) return INVALID_SOCKET;
    (void)proto;
    int fd = mbn_socket();
    return fd < 0 ? INVALID_SOCKET : (SOCKET)fd;
}
int WINAPI mbws_connect(SOCKET s, const struct sockaddr* name, int len)
{
    // the game's sockaddr_in is Winsock's layout: 16-bit family, port and address in network order
    if (!name || len < 16 || name->sa_family != AF_INET) return SOCKET_ERROR;
    const sockaddr_in* a = (const sockaddr_in*)name;
    return mbn_connect_v4((int)s, (uint32_t)a->sin_addr.s_addr, a->sin_port) == 0 ? 0 : SOCKET_ERROR;
}
int WINAPI mbws_send(SOCKET s, const char* b, int n, int f) { int r = mbn_send((int)s, b, n, f); return r < 0 ? SOCKET_ERROR : r; }
int WINAPI mbws_recv(SOCKET s, char* b, int n, int f) { int r = mbn_recv((int)s, b, n, f); return r < 0 ? SOCKET_ERROR : r; }
int WINAPI mbws_closesocket(SOCKET s) { return mbn_close((int)s) == 0 ? 0 : SOCKET_ERROR; }
struct hostent* WINAPI mbws_gethostbyname(const char* name)
{
    // Winsock returns a per-thread hostent; h_addr_list[0] points at the 4-byte address
    static thread_local struct hostent he;
    static thread_local char canon[256];
    static thread_local uint32_t addr;
    static thread_local char* list[2];
    static thread_local char* aliases[1];
    if (!name || mbn_resolve_v4(name, &addr, canon, sizeof canon) != 0) return nullptr;
    list[0] = (char*)&addr; list[1] = nullptr; aliases[0] = nullptr;
    he.h_name = canon; he.h_aliases = aliases; he.h_addrtype = AF_INET; he.h_length = 4; he.h_addr_list = list;
    return &he;
}
unsigned short WINAPI mbws_htons(unsigned short v) { return (unsigned short)((v >> 8) | (v << 8)); }
unsigned long WINAPI mbws_inet_addr(const char* s) { return mbn_inet_addr(s); }
}
