// port/include/winsock2.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <windows.h>
// The version check to taleworlds.com (src/game/config/options_dlg.cpp) is dropped by ticket 007; declarations only.
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
extern "C" {
int WINAPI WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData);
int WINAPI WSACleanup(void);
SOCKET WINAPI socket(int af, int type, int protocol);
int WINAPI connect(SOCKET s, const struct sockaddr* name, int namelen);
int WINAPI send(SOCKET s, const char* buf, int len, int flags);
int WINAPI recv(SOCKET s, char* buf, int len, int flags);
int WINAPI closesocket(SOCKET s);
struct hostent* WINAPI gethostbyname(const char* name);
unsigned short WINAPI htons(unsigned short hostshort);
unsigned long WINAPI inet_addr(const char* cp);
}
