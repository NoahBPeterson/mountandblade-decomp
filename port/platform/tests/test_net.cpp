// Winsock subset over POSIX sockets: the launcher's version-check sequence (WSAStartup, socket, gethostbyname, connect,
// send, recv, closesocket) against a local server, with the Winsock sockaddr_in layout.
#include <winsock2.h>
#undef htons
#undef inet_addr
#include "check.h"
#include <string>
extern "C" {
int test_server_start(void);
int WINAPI mbws_WSAStartup(WORD, LPWSADATA);
SOCKET WINAPI mbws_socket(int, int, int);
int WINAPI mbws_connect(SOCKET, const struct sockaddr*, int);
int WINAPI mbws_send(SOCKET, const char*, int, int);
int WINAPI mbws_recv(SOCKET, char*, int, int);
int WINAPI mbws_closesocket(SOCKET);
struct hostent* WINAPI mbws_gethostbyname(const char*);
unsigned short WINAPI mbws_htons(unsigned short);
unsigned long WINAPI mbws_inet_addr(const char*);
}
int main()
{
    int port = test_server_start();
    CHECK(port > 0);
    WSADATA wsa;
    CHECK(mbws_WSAStartup(0x101, &wsa) == 0 && wsa.wVersion == 0x101);
    CHECK(mbws_htons(0x1234) == 0x3412);
    CHECK(mbws_inet_addr("127.0.0.1") == 0x0100007Ful);
    SOCKET s = mbws_socket(AF_INET, SOCK_STREAM, 0);
    CHECK(s != INVALID_SOCKET);
    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = mbws_htons((unsigned short)port);
    hostent* he = mbws_gethostbyname("localhost");
    CHECK(he != nullptr && he->h_length == 4);
    if (!he) return g_fails + 1;
    addr.sin_addr.s_addr = *(ULONG*)he->h_addr_list[0];
    CHECK(mbws_connect(s, (sockaddr*)&addr, 16) == 0);
    const char* req = "GET /version HTTP/1.0\r\nHost: localhost\r\n\r\n";
    CHECK(mbws_send(s, req, (int)strlen(req), 0) == (int)strlen(req));
    std::string got;
    char c;
    while (mbws_recv(s, &c, 1, 0) == 1) got += c;           // the game reads one byte at a time
    CHECK(got.find("200 OK") != std::string::npos);
    CHECK(got.size() >= 4 && got.substr(got.size() - 4) == "1011");
    CHECK(mbws_closesocket(s) == 0);
    CHECK(mbws_gethostbyname("no-such-host.invalid") == nullptr);
    printf("%d checks, %d failed\n", g_checks, g_fails);
    return g_fails;
}
