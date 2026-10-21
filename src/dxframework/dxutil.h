//-----------------------------------------------------------------------------
// File: DXUtil.h
//
// Desc: Helper functions and typing shortcuts for DirectX programming
//       (DirectX 9 SDK sample framework, as linked into Mount & Blade 1.011).
//       Only the parts the game links are declared.
//-----------------------------------------------------------------------------
#ifndef DXUTIL_H
#define DXUTIL_H

#include <windows.h>
#include <tchar.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

//-----------------------------------------------------------------------------
// Name: DXUtil_Timer()
// Desc: Performs timer operations. Use the following commands:
//          TIMER_RESET           - to reset the timer
//          TIMER_START           - to start the timer
//          TIMER_STOP            - to stop (or pause) the timer
//          TIMER_ADVANCE         - to advance the timer by 0.1 seconds
//          TIMER_GETABSOLUTETIME - to get the absolute system time
//          TIMER_GETAPPTIME      - to get the current time
//          TIMER_GETELAPSEDTIME  - to get the time that elapsed between
//                                  TIMER_GETELAPSEDTIME calls
//-----------------------------------------------------------------------------
enum TIMER_COMMAND { TIMER_RESET, TIMER_START, TIMER_STOP, TIMER_ADVANCE,
                     TIMER_GETABSOLUTETIME, TIMER_GETAPPTIME, TIMER_GETELAPSEDTIME };
FLOAT __stdcall DXUtil_Timer( TIMER_COMMAND command );                         // 0x407660

//-----------------------------------------------------------------------------
// UNICODE support for converting between CHAR, TCHAR, and WCHAR strings
//-----------------------------------------------------------------------------
HRESULT DXUtil_ConvertAnsiStringToGenericCch( TCHAR* tstrDestination, const CHAR* strSource,
                                              int cchDestChar );               // 0x407a30

//-----------------------------------------------------------------------------
// Name: ArrayListType
// Desc: Indicates how data should be stored in a CArrayList
//-----------------------------------------------------------------------------
enum ArrayListType
{
    AL_VALUE,       // entry data is copied into the list
    AL_REFERENCE,   // entry pointers are copied into the list
};

//-----------------------------------------------------------------------------
// Name: CArrayList
// Desc: A growable array (0x14 bytes)
//-----------------------------------------------------------------------------
class CArrayList
{
protected:
    ArrayListType m_ArrayListType;      // +0x00
    void* m_pData;                      // +0x04
    UINT m_BytesPerEntry;               // +0x08
    UINT m_NumEntries;                  // +0x0c
    UINT m_NumEntriesAllocated;         // +0x10

public:
    CArrayList( ArrayListType Type, UINT BytesPerEntry = 0 );   // 0x407a70
    ~CArrayList( void );                                        // 0x407aa0
    HRESULT Add( void* pEntry );                                // 0x407ab0
    void* GetPtr( UINT Entry );                                 // 0x407ba0
    bool Contains( void* pEntryData );                          // 0x407bc0
    UINT Count( void ) { return m_NumEntries; }
    void Clear( void ) { m_NumEntries = 0; }
};

#endif // DXUTIL_H
