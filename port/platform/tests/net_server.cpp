// native side of the Winsock test: a one-shot HTTP-ish server on 127.0.0.1 (no Winsock shim in this TU)
#if defined(_WIN32)
// Real Winsock: on Windows this TU is built without port/include and without the prelude (its own object library in
// tests/CMakeLists.txt, the same reason mb_net_native has one), so <winsock2.h> is the SDK's own header.
#include <winsock2.h>
#include <string.h>
#include <stdio.h>

static SOCKET g_listen = INVALID_SOCKET;
static DWORD WINAPI serve(LPVOID)
{
    SOCKET c = accept(g_listen, nullptr, nullptr);
    if (c == INVALID_SOCKET) return 0;
    char buf[1024];
    int total = 0, n;
    while (total < (int)sizeof buf - 1 && (n = recv(c, buf + total, (int)sizeof buf - 1 - total, 0)) > 0) {
        total += n;
        buf[total] = 0;
        if (strstr(buf, "\r\n\r\n")) break;
    }
    const char* reply = (strncmp(buf, "GET /version ", 13) == 0) ? "HTTP/1.0 200 OK\r\n\r\n1011" : "HTTP/1.0 404 Not Found\r\n\r\n";
    send(c, reply, (int)strlen(reply), 0);
    closesocket(c);
    return 0;
}
extern "C" int test_server_start(void)       // returns the port (host order), or -1
{
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d) != 0) return -1;
    g_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen == INVALID_SOCKET) return -1;
    sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = 0;
    if (bind(g_listen, (sockaddr*)&a, sizeof a) != 0 || listen(g_listen, 1) != 0) return -1;
    int l = sizeof a;
    getsockname(g_listen, (sockaddr*)&a, &l);
    HANDLE t = CreateThread(nullptr, 0, serve, nullptr, 0, nullptr);
    if (!t) return -1;
    CloseHandle(t);
    return ntohs(a.sin_port);
}
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static int g_listen = -1;
static void* serve(void*)
{
    int c = accept(g_listen, nullptr, nullptr);
    if (c < 0) return nullptr;
    char buf[1024];
    ssize_t n = 0, total = 0;
    while (total < (ssize_t)sizeof buf - 1 && (n = read(c, buf + total, sizeof buf - 1 - (size_t)total)) > 0) {
        total += n;
        buf[total] = 0;
        if (strstr(buf, "\r\n\r\n")) break;
    }
    const char* reply = (strncmp(buf, "GET /version ", 13) == 0) ? "HTTP/1.0 200 OK\r\n\r\n1011" : "HTTP/1.0 404 Not Found\r\n\r\n";
    write(c, reply, strlen(reply));
    close(c);
    return nullptr;
}
extern "C" int test_server_start(void)       // returns the port (host order), or -1
{
    g_listen = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = 0;
    if (bind(g_listen, (sockaddr*)&a, sizeof a) != 0 || listen(g_listen, 1) != 0) return -1;
    socklen_t l = sizeof a;
    getsockname(g_listen, (sockaddr*)&a, &l);
    pthread_t t;
    pthread_create(&t, nullptr, serve, nullptr);
    pthread_detach(t);
    return ntohs(a.sin_port);
}
#endif