//-----------------------------------------------------------------------------
// File: D3DUtil.h
//
// Desc: Helper functions and typing shortcuts for Direct3D programming
//       (DirectX 9 SDK sample framework). Only the parts the game links are declared.
//-----------------------------------------------------------------------------
#ifndef D3DUTIL_H
#define D3DUTIL_H

#include <windows.h>
#include <tchar.h>
#include <d3d9.h>

//-----------------------------------------------------------------------------
// Name: D3DUtil_D3DFormatToString
// Desc: Returns the string for the given D3DFORMAT.
//       bWithPrefix determines whether the string should include the "D3DFMT_"
//-----------------------------------------------------------------------------
LPCTSTR D3DUtil_D3DFormatToString( D3DFORMAT format, bool bWithPrefix = true );   // 0x407250

#endif // D3DUTIL_H
