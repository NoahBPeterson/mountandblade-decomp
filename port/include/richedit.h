// port/include/richedit.h: portable-build shim (ticket 004): declares only what src/ uses (the edit boxes of the
// in-game script/config editors: selection, char format, find).
#pragma once
#include <windows.h>
typedef struct _charrange { LONG cpMin, cpMax; } CHARRANGE;
typedef struct _charformat {
    UINT cbSize; DWORD dwMask, dwEffects; LONG yHeight, yOffset; COLORREF crTextColor; BYTE bCharSet, bPitchAndFamily;
    char szFaceName[32];
} CHARFORMATA, CHARFORMAT;
typedef struct _charformat2a {
    UINT cbSize; DWORD dwMask, dwEffects; LONG yHeight, yOffset; COLORREF crTextColor; BYTE bCharSet, bPitchAndFamily;
    char szFaceName[32]; WORD wWeight; SHORT sSpacing; COLORREF crBackColor; LCID lcid; DWORD dwReserved; SHORT sStyle;
    WORD wKerning; BYTE bUnderlineType, bAnimation, bRevAuthor, bReserved1;
} CHARFORMAT2A, CHARFORMAT2;
typedef struct _findtextexa { CHARRANGE chrg; LPCSTR lpstrText; CHARRANGE chrgText; } FINDTEXTEXA, FINDTEXTEX;
#define EM_EXGETSEL (WM_USER + 52)
#define EM_EXSETSEL (WM_USER + 55)
#define EM_SETCHARFORMAT (WM_USER + 68)
#define EM_FINDTEXTEX (WM_USER + 79)
#define EM_SETBKGNDCOLOR (WM_USER + 67)
#define FR_DOWN 0x00000001
#define FR_MATCHCASE 0x00000004
#define FR_WHOLEWORD 0x00000002
#define CFM_BOLD 0x00000001
#define CFM_ITALIC 0x00000002
#define CFM_SIZE 0x80000000
#define CFM_COLOR 0x40000000
#define CFM_FACE 0x20000000
#define CFE_BOLD 0x0001
#define CFE_AUTOCOLOR 0x40000000
#define SCF_SELECTION 0x0001
#define SCF_WORD 0x0002
#define SCF_ALL 0x0004
