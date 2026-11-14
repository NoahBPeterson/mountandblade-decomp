/* <crtdefs.h> for cl 13.10, which predates it (VS2005 split it out of the CRT headers): the types and macros the
   corpus sources take from it. */
#ifndef CORPUS_CRTDEFS_H
#define CORPUS_CRTDEFS_H
#include <stddef.h>
#include <stdlib.h>
#ifndef _ERRCODE_DEFINED
#define _ERRCODE_DEFINED
typedef int errno_t;
#endif
#ifndef _RSIZE_T_DEFINED
#define _RSIZE_T_DEFINED
typedef size_t rsize_t;
#endif
#ifndef _CRT_INSECURE_DEPRECATE
#define _CRT_INSECURE_DEPRECATE(x)
#endif
#ifndef _CRTIMP_ALTERNATIVE
#define _CRTIMP_ALTERNATIVE
#endif
#endif
