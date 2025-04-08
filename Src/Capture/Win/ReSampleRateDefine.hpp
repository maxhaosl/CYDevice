#ifndef __RESAMPLE_RATE_DEFINE_HPP__
#define __RESAMPLE_RATE_DEFINE_HPP__

#include "Common/CYDevicePrivDefine.hpp"
#include "libsamplerate/samplerate.h"

#include<windows.h>

CYDEVICE_NAMESPACE_BEGIN

/**
 * Resample Context.
 */
struct TResamplerContext
{
    SRC_STATE* pResampler = nullptr;
    UINT64     nJumpRange = 0;
};

/**
 * Triple Structure.
 */
union UTriple
{
    LONG lVal;
    struct
    {
        WORD wVal;
        BYTE byTripleVal;
        BYTE byLastByte;
    };
};

/**
 * Audio Sample Data.
 */
struct TSampleData
{
    LPBYTE lpData = nullptr;
    long nDataLength = 0;

    int cx = 0, cy = 0;

    bool bAudio = false;;
    LONGLONG nTimestamp;
    volatile long refs = 1;

    inline TSampleData()
    {
        refs = 1;
    }
    inline ~TSampleData()
    {
        if (lpData)
        {
            free(lpData);
            lpData = nullptr;
        }
    }

    inline void AddRef()
    {
        ++refs;
    }
    inline void Release()
    {
        if (!InterlockedDecrement(&refs))
            delete this;
    }
};

/**
 * Calculate the difference.
 */
inline UINT64 GetQWDif(UINT64 nVal1, UINT64 nVal2)
{
    return (nVal1 > nVal2) ? (nVal1 - nVal2) : (nVal2 - nVal1);
}

/**
 * Find substrings.
 */
inline wchar_t* wstrstr(const wchar_t* strSrc, const wchar_t chr)
{
    if (!strSrc) return NULL;

    while (*strSrc != chr)
    {
        if (*strSrc++ == 0)
            return NULL;
    }

    return (wchar_t*)strSrc;
}

//////////////////////////////////////////////////////////////////////////
#define MoreVariables static_cast<TResamplerContext*>(m_pResampler)

CYDEVICE_NAMESPACE_END

#endif // __RESAMPLE_RATE_DEFINE_HPP__