// port/platform/net_native.cpp: the native socket calls behind the Winsock names (net.cpp). Kept in a TU of its own because
// the Winsock shim (port/include/winsock2.h) and the native socket headers declare the same names with different types; it
// is built without port/include in its path (port/platform/CMakeLists.txt, its own object library), so <winsock2.h> here
// is the SDK's own header, and no portable shim or prelude meets the SDK declarations.
#if defined(_WIN32)
#include <winsock2.h>
#include <string.h>
#include <stdint.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#endif

extern "C" {
#if defined(_WIN32)
// The socket interface is int like POSIX fds; a Winsock SOCKET is a 32-bit kernel handle widened to UINT_PTR.
int mbn_socket(void)
{
    static bool started = false;
    if (!started) {
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d) != 0) return -1;      // the launcher's version check is only user of sockets
        started = true;
    }
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    return s == INVALID_SOCKET ? -1 : (int)s;
}
int mbn_connect_v4(int fd, uint32_t addr_be, uint16_t port_be)
{
    sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = port_be;
    a.sin_addr.s_addr = addr_be;
    return connect((SOCKET)(uintptr_t)fd, (sockaddr*)&a, sizeof a) == 0 ? 0 : -1;
}
int mbn_send(int fd, const char* b, int n, int) { return send((SOCKET)(uintptr_t)fd, b, n, 0); }
int mbn_recv(int fd, char* b, int n, int) { return recv((SOCKET)(uintptr_t)fd, b, n, 0); }
int mbn_close(int fd) { return closesocket((SOCKET)(uintptr_t)fd) == 0 ? 0 : -1; }
// 0 on success; addr in network order, canonical name copied to `name_out`
int mbn_resolve_v4(const char* name, uint32_t* addr_be, char* name_out, int cap)
{
    hostent* he = gethostbyname(name);
    if (!he || he->h_addrtype != AF_INET || !he->h_addr_list || !he->h_addr_list[0]) return -1;
    memcpy(addr_be, he->h_addr_list[0], 4);
    strncpy(name_out, he->h_name ? he->h_name : name, (size_t)cap - 1);
    name_out[cap - 1] = 0;
    return 0;
}
uint32_t mbn_inet_addr(const char* s) { return (uint32_t)inet_addr(s); }   // INADDR_NONE for anything it cannot parse
#else
int mbn_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
#if defined(SO_NOSIGPIPE)
    if (fd >= 0) { int on = 1; setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof on); }
#endif
    return fd;
}
int mbn_connect_v4(int fd, uint32_t addr_be, uint16_t port_be)
{
    sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = port_be;
    a.sin_addr.s_addr = addr_be;
    return connect(fd, (sockaddr*)&a, sizeof a);
}
int mbn_send(int fd, const char* b, int n, int)
{
#if defined(MSG_NOSIGNAL)
    return (int)send(fd, b, (size_t)n, MSG_NOSIGNAL);
#else
    return (int)send(fd, b, (size_t)n, 0);
#endif
}
int mbn_recv(int fd, char* b, int n, int) { return (int)recv(fd, b, (size_t)n, 0); }
int mbn_close(int fd) { return close(fd); }
// 0 on success; addr in network order, canonical name copied to `name_out`
int mbn_resolve_v4(const char* name, uint32_t* addr_be, char* name_out, int cap)
{
    addrinfo hints, *res = nullptr;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    if (getaddrinfo(name, nullptr, &hints, &res) != 0 || !res) return -1;
    *addr_be = ((sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
    strncpy(name_out, res->ai_canonname ? res->ai_canonname : name, (size_t)cap - 1);
    name_out[cap - 1] = 0;
    freeaddrinfo(res);
    return 0;
}
uint32_t mbn_inet_addr(const char* s)
{
    in_addr a;
    return inet_aton(s, &a) ? a.s_addr : 0xFFFFFFFFu;     // INADDR_NONE
}
#endif
}
