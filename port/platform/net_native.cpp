// port/platform/net_native.cpp: the POSIX socket calls behind the Winsock names (net.cpp). Kept in a TU of its own because
// the Winsock shim (port/include/winsock2.h) and <sys/socket.h> declare the same names with different types.
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

extern "C" {
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
}
