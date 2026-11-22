// port/include/winsock2.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <windows.h>
// The version check to taleworlds.com (src/game/config/launcher_options_dialog.cpp) is dropped by ticket 007; declarations only.
typedef UINT_PTR SOCKET;
typedef unsigned short u_short;
typedef unsigned long u_long;
#undef htons            // macOS <sys/_endian.h> defines these as macros; src/ calls the Winsock functions
#undef ntohs
#undef inet_addr
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#define AF_INET 2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6
typedef struct WSAData { WORD wVersion, wHighVersion; char szDescription[257]; char szSystemStatus[129]; unsigned short iMaxSockets, iMaxUdpDg; char* lpVendorInfo; } WSADATA, *LPWSADATA;
struct in_addr { union { struct { BYTE s_b1, s_b2, s_b3, s_b4; } S_un_b; struct { WORD s_w1, s_w2; } S_un_w; ULONG S_addr; } S_un; };
#define s_addr S_un.S_addr
struct sockaddr { unsigned short sa_family; char sa_data[14]; };
struct sockaddr_in { short sin_family; unsigned short sin_port; struct in_addr sin_addr; char sin_zero[8]; };
struct hostent { char* h_name; char** h_aliases; short h_addrtype, h_length; char** h_addr_list; };
#define h_addr h_addr_list[0]
// The plain names belong to the C library here (different types: int fds, BSD sockaddr). port/platform/net.cpp implements
// the Winsock semantics under mbws_ names; src/ reaches them through these.
extern "C" {
int WINAPI mbws_WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData);
int WINAPI mbws_WSACleanup(void);
SOCKET WINAPI mbws_socket(int af, int type, int protocol);
int WINAPI mbws_connect(SOCKET s, const struct sockaddr* name, int namelen);
int WINAPI mbws_send(SOCKET s, const char* buf, int len, int flags);
int WINAPI mbws_recv(SOCKET s, char* buf, int len, int flags);
int WINAPI mbws_closesocket(SOCKET s);
struct hostent* WINAPI mbws_gethostbyname(const char* name);
unsigned short WINAPI mbws_htons(unsigned short hostshort);
unsigned long WINAPI mbws_inet_addr(const char* cp);
}
#define WSAStartup mbws_WSAStartup
#define WSACleanup mbws_WSACleanup
#define socket mbws_socket
#define connect mbws_connect
#define send mbws_send
#define recv mbws_recv
#define closesocket mbws_closesocket
#define gethostbyname mbws_gethostbyname
#define htons mbws_htons
#define inet_addr mbws_inet_addr
