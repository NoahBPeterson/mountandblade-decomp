// native side of the Winsock test: a one-shot HTTP-ish server on 127.0.0.1 (no Winsock shim in this TU)
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
