// port/include/mbstring.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <string.h>
#define _mbsstr(a, b) ((unsigned char*)strstr((const char*)(a), (const char*)(b)))
#define _mbsnbcpy(a, b, n) ((unsigned char*)strncpy((char*)(a), (const char*)(b), (n)))
#define _mbscpy(a, b) ((unsigned char*)strcpy((char*)(a), (const char*)(b)))
#define _mbslen(a) strlen((const char*)(a))
