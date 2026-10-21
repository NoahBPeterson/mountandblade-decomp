//-----------------------------------------------------------------------------
// File: DXUtil.cpp
//
// Desc: Shortcut macros and functions for using DX objects
//       (DirectX 9 SDK sample framework). Region 0x407660-0x407c0f; only
//       DXUtil_Timer, DXUtil_ConvertAnsiStringToGenericCch and CArrayList
//       are linked.
//-----------------------------------------------------------------------------
#define _MBCS   // project setting: multi-byte character set (TCHAR = char)
#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#include <string.h>
#include "./dxutil.h"

//-----------------------------------------------------------------------------
// Name: DXUtil_Timer()
// Desc: Performs timer opertations. Use the following commands:
//          TIMER_RESET           - to reset the timer
//          TIMER_START           - to start the timer
//          TIMER_STOP            - to stop (or pause) the timer
//          TIMER_ADVANCE         - to advance the timer by 0.1 seconds
//          TIMER_GETABSOLUTETIME - to get the absolute system time
//          TIMER_GETAPPTIME      - to get the current time
//          TIMER_GETELAPSEDTIME  - to get the time that elapsed between
//                                  TIMER_GETELAPSEDTIME calls
//-----------------------------------------------------------------------------
// @ 0x407660
FLOAT __stdcall DXUtil_Timer( TIMER_COMMAND command )
{
    static BOOL     m_bTimerInitialized = FALSE;    // 0x5d6f94
    static BOOL     m_bUsingQPF         = FALSE;    // 0x5d6f90
    static BOOL     m_bTimerStopped     = TRUE;     // 0x5a8100
    static LONGLONG m_llQPFTicksPerSec  = 0;        // 0x5d6f88

    // Initialize the timer
    if( FALSE == m_bTimerInitialized )
    {
        m_bTimerInitialized = TRUE;

        // Use QueryPerformanceFrequency() to get frequency of timer.  If QPF is
        // not supported, we will timeGetTime() which returns milliseconds.
        LARGE_INTEGER qwTicksPerSec;
        m_bUsingQPF = QueryPerformanceFrequency( &qwTicksPerSec );
        if( m_bUsingQPF )
            m_llQPFTicksPerSec = qwTicksPerSec.QuadPart;
    }

    if( m_bUsingQPF )
    {
        static LONGLONG m_llStopTime        = 0;    // 0x5d6f80
        static LONGLONG m_llLastElapsedTime = 0;    // 0x5d6f78
        static LONGLONG m_llBaseTime        = 0;    // 0x5d6f70
        double fTime;
        double fElapsedTime;
        LARGE_INTEGER qwTime;

        // Get either the current time or the stop time, depending
        // on whether we're stopped and what command was sent
        if( m_llStopTime != 0 && command != TIMER_START && command != TIMER_GETABSOLUTETIME)
            qwTime.QuadPart = m_llStopTime;
        else
            QueryPerformanceCounter( &qwTime );

        // Return the elapsed time
        if( command == TIMER_GETELAPSEDTIME )
        {
            fElapsedTime = (double) ( qwTime.QuadPart - m_llLastElapsedTime ) / (double) m_llQPFTicksPerSec;
            m_llLastElapsedTime = qwTime.QuadPart;
            return (FLOAT) fElapsedTime;
        }

        // Return the current time
        if( command == TIMER_GETAPPTIME )
        {
            double fAppTime = (double) ( qwTime.QuadPart - m_llBaseTime ) / (double) m_llQPFTicksPerSec;
            return (FLOAT) fAppTime;
        }

        // Reset the timer
        if( command == TIMER_RESET )
        {
            m_llBaseTime        = qwTime.QuadPart;
            m_llLastElapsedTime = qwTime.QuadPart;
            m_llStopTime        = 0;
            m_bTimerStopped     = FALSE;
            return 0.0f;
        }

        // Start the timer
        if( command == TIMER_START )
        {
            if( m_bTimerStopped )
                m_llBaseTime += qwTime.QuadPart - m_llStopTime;
            m_llStopTime = 0;
            m_llLastElapsedTime = qwTime.QuadPart;
            m_bTimerStopped = FALSE;
            return 0.0f;
        }

        // Stop the timer
        if( command == TIMER_STOP )
        {
            if( !m_bTimerStopped )
            {
                m_llStopTime = qwTime.QuadPart;
                m_llLastElapsedTime = qwTime.QuadPart;
                m_bTimerStopped = TRUE;
            }
            return 0.0f;
        }

        // Advance the timer by 1/10th second
        if( command == TIMER_ADVANCE )
        {
            m_llStopTime += m_llQPFTicksPerSec/10;
            return 0.0f;
        }

        if( command == TIMER_GETABSOLUTETIME )
        {
            fTime = qwTime.QuadPart / (double) m_llQPFTicksPerSec;
            return (FLOAT) fTime;
        }

        return -1.0f; // Invalid command specified
    }
    else
    {
        // Get the time using timeGetTime()
        static double m_fLastElapsedTime  = 0.0;    // 0x5d6f60
        static double m_fBaseTime         = 0.0;    // 0x5d6f58
        static double m_fStopTime         = 0.0;    // 0x5d6f68
        double fTime;
        double fElapsedTime;

        // Get either the current time or the stop time, depending
        // on whether we're stopped and what command was sent
        if( m_fStopTime != 0.0 && command != TIMER_START && command != TIMER_GETABSOLUTETIME)
            fTime = m_fStopTime;
        else
            fTime = timeGetTime() * 0.001;

        // Return the elapsed time
        if( command == TIMER_GETELAPSEDTIME )
        {
            fElapsedTime = (double) (fTime - m_fLastElapsedTime);
            m_fLastElapsedTime = fTime;
            return (FLOAT) fElapsedTime;
        }

        // Return the current time
        if( command == TIMER_GETAPPTIME )
        {
            return (FLOAT) (fTime - m_fBaseTime);
        }

        // Reset the timer
        if( command == TIMER_RESET )
        {
            m_fBaseTime         = fTime;
            m_fLastElapsedTime  = fTime;
            m_fStopTime         = 0;
            m_bTimerStopped     = FALSE;
            return 0.0f;
        }

        // Start the timer
        if( command == TIMER_START )
        {
            if( m_bTimerStopped )
                m_fBaseTime += fTime - m_fStopTime;
            m_fStopTime = 0.0f;
            m_fLastElapsedTime  = fTime;
            m_bTimerStopped = FALSE;
            return 0.0f;
        }

        // Stop the timer
        if( command == TIMER_STOP )
        {
            if( !m_bTimerStopped )
            {
                m_fStopTime = fTime;
                m_fLastElapsedTime  = fTime;
                m_bTimerStopped = TRUE;
            }
            return 0.0f;
        }

        // Advance the timer by 1/10th second
        if( command == TIMER_ADVANCE )
        {
            m_fStopTime += 0.1f;
            return 0.0f;
        }

        if( command == TIMER_GETABSOLUTETIME )
        {
            return (FLOAT) fTime;
        }

        return -1.0f; // Invalid command specified
    }
}

//-----------------------------------------------------------------------------
// Name: DXUtil_ConvertAnsiStringToGenericCch()
// Desc: This is a UNICODE conversion utility to convert a CHAR string into a
//       TCHAR string.
//       cchDestChar is the size in TCHARs of tstrDestination.  Be careful not to
//       pass in sizeof(strDest) on UNICODE builds
//-----------------------------------------------------------------------------
// @ 0x407a30
HRESULT DXUtil_ConvertAnsiStringToGenericCch( TCHAR* tstrDestination, const CHAR* strSource,
                                              int cchDestChar )
{
    if( tstrDestination==NULL || strSource==NULL || cchDestChar < 1 )
        return E_INVALIDARG;

#ifdef _UNICODE
    return DXUtil_ConvertAnsiStringToWideCch( tstrDestination, strSource, cchDestChar );
#else
    strncpy( tstrDestination, strSource, cchDestChar );
    tstrDestination[cchDestChar-1] = '\0';
    return S_OK;
#endif
}

//-----------------------------------------------------------------------------
// Name: CArrayList constructor
// Desc:
//-----------------------------------------------------------------------------
// @ 0x407a70
CArrayList::CArrayList( ArrayListType Type, UINT BytesPerEntry )
{
    if( Type == AL_REFERENCE )
        BytesPerEntry = sizeof(void*);
    m_ArrayListType = Type;
    m_pData = NULL;
    m_BytesPerEntry = BytesPerEntry;
    m_NumEntries = 0;
    m_NumEntriesAllocated = 0;
}

//-----------------------------------------------------------------------------
// Name: CArrayList destructor
// Desc:
//-----------------------------------------------------------------------------
// @ 0x407aa0
CArrayList::~CArrayList( void )
{
    if( m_pData != NULL )
        delete[] m_pData;
}

//-----------------------------------------------------------------------------
// Name: CArrayList::Add
// Desc: Adds pEntry to the list.
//-----------------------------------------------------------------------------
// @ 0x407ab0
HRESULT CArrayList::Add( void* pEntry )
{
    if( m_BytesPerEntry == 0 )
        return E_FAIL;
    if( m_pData == NULL || m_NumEntries + 1 > m_NumEntriesAllocated )
    {
        void* pDataNew;
        UINT NumEntriesAllocatedNew;
        if( m_NumEntriesAllocated == 0 )
            NumEntriesAllocatedNew = 16;
        else
            NumEntriesAllocatedNew = m_NumEntriesAllocated * 2;
        pDataNew = new BYTE[NumEntriesAllocatedNew * m_BytesPerEntry];
        if( pDataNew == NULL )
            return E_OUTOFMEMORY;
        if( m_pData != NULL )
        {
            CopyMemory( pDataNew, m_pData, m_NumEntries * m_BytesPerEntry );
            delete[] m_pData;
        }
        m_pData = pDataNew;
        m_NumEntriesAllocated = NumEntriesAllocatedNew;
    }

    if( m_ArrayListType == AL_VALUE )
        CopyMemory( (BYTE*)m_pData + (m_NumEntries * m_BytesPerEntry), pEntry, m_BytesPerEntry );
    else
        *(((void**)m_pData) + m_NumEntries) = pEntry;
    m_NumEntries++;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CArrayList::GetPtr
// Desc: Returns a pointer to the Entry'th entry in the list.
//-----------------------------------------------------------------------------
// @ 0x407ba0
void* CArrayList::GetPtr( UINT Entry )
{
    if( m_ArrayListType == AL_VALUE )
        return (BYTE*)m_pData + (Entry * m_BytesPerEntry);
    else
        return *(((void**)m_pData) + Entry);
}

//-----------------------------------------------------------------------------
// Name: CArrayList::Contains
// Desc: Returns whether the list contains an entry identical to the
//       specified entry data.
//-----------------------------------------------------------------------------
// @ 0x407bc0
bool CArrayList::Contains( void* pEntryData )
{
    for( UINT iEntry = 0; iEntry < m_NumEntries; iEntry++ )
    {
        if( m_ArrayListType == AL_VALUE )
        {
            if( memcmp( GetPtr(iEntry), pEntryData, m_BytesPerEntry ) == 0 )
                return true;
        }
        else
        {
            if( GetPtr(iEntry) == pEntryData )
                return true;
        }
    }
    return false;
}
