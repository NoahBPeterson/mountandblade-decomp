// port/include/tchar.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <string.h>
#include <stdio.h>
// TCHAR = char: the game and the DX SDK framework are built _MBCS. _tcs* map to the byte-string functions (the _mbs*
// forms are byte-identical for the single-byte code pages the game ships).
#define _tcscpy strcpy
#define _tcsncpy strncpy
#define _tcscat strcat
#define _tcsncat strncat
#define _tcslen strlen
#define _tcscmp strcmp
#define _tcsicmp strcasecmp
#define _tcsstr strstr
#define _tcschr strchr
#define _tcsrchr strrchr
#define _stprintf sprintf
#define _sntprintf snprintf
#define _vsntprintf vsnprintf
#define _tcstok strtok
#define _ttoi atoi
#define _tstof atof
