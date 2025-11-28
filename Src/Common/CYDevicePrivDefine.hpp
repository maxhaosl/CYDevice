#ifndef __CYDEVICE_PRIVATE_DEFINE_HPP__
#define __CYDEVICE_PRIVATE_DEFINE_HPP__

#include "CYDevice/CYDeviceDefine.hpp"
#include "Inc/ICYLoggerDefine.hpp"
#include "CYCoroutine/CYTypeDefine.hpp"
#include "CYCoroutine/Common/Exception/CYException.hpp"
#include "Inc/ICYLogger.hpp"

CYDEVICE_NAMESPACE_BEGIN

/**
 * Video output format type.
 */
enum ECYVideoOutputType
{
    TYPE_VIDEO_OUTPUT_NONE,
    TYPE_VIDEO_OUTPUT_RGB24,
    TYPE_VIDEO_OUTPUT_RGB32,
    TYPE_VIDEO_OUTPUT_ARGB32,
    TYPE_VIDEO_OUTPUT_RGB565,

    TYPE_VIDEO_OUTPUT_I420,
    TYPE_VIDEO_OUTPUT_YV12,

    TYPE_VIDEO_OUTPUT_Y41P,
    TYPE_VIDEO_OUTPUT_YVU9,

    TYPE_VIDEO_OUTPUT_YVYU,
    TYPE_VIDEO_OUTPUT_YUY2,
    TYPE_VIDEO_OUTPUT_UYVY,
    TYPE_VIDEO_OUTPUT_HDYC,

    TYPE_VIDEO_OUTPUT_MPEG2_VIDEO,
    TYPE_VIDEO_OUTPUT_H264,

    TYPE_VIDEO_OUTPUT_DVSL,
    TYPE_VIDEO_OUTPUT_DVSD,
    TYPE_VIDEO_OUTPUT_DVHD,

    TYPE_VIDEO_OUTPUT_MJPG
};

/**
 * Resolution size.
 */
struct TResSize
{
    uint32_t cx;
    uint32_t cy;
};

#define SafeRelease(var) if(var) {var->Release(); var = nullptr;}
#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))
#define SafeReleaseLogRef(var) if(var) {ULONG chi = var->Release(); CY_LOG_TRACE(TEXT("releasing %s, %d refs were left\r\n"), L#var, chi); var = nullptr;}
#define SafeRelease(var) if(var) {var->Release(); var = nullptr;}

#define NEAR_SILENT  3000
#define NEAR_SILENTf 3000.0

#if defined(_WIN64)
typedef long long           PARAM;
typedef unsigned long long  UPARAM;
#else
typedef long                PARAM;
typedef unsigned long       UPARAM;
#endif

constexpr float g_fDbMinus3 = 0.7071067811865476f;
constexpr float g_fDbMinus6 = 0.5f;
constexpr float g_fDbMinus9 = 0.3535533905932738f;

//not entirely sure if these are the correct coefficients for downmixing,
//I'm fairly new to the whole multi speaker thing
constexpr float g_fSurroundMix = g_fDbMinus3;
constexpr float g_fCenterMix = g_fDbMinus6;
constexpr float g_fLowFreqMix = g_fDbMinus3;

constexpr float g_fSurroundMix4 = g_fDbMinus6;

constexpr float g_fAttn5dot1 = 1.0f / (1.0f + g_fCenterMix + g_fSurroundMix);
constexpr float g_fAttn4dotX = 1.0f / (1.0f + g_fSurroundMix4);

CYDEVICE_NAMESPACE_END

#define ExceptionLog(e)		CY_LOG_ERROR(CYCOROUTINE_NAMESPACE::AtoT(e))

#define EXCEPTION_BEGIN 	UniquePtr<CYCOROUTINE_NAMESPACE::CYBaseException> excp;  try 
#define EXCEPTION_END		catch (CYCOROUTINE_NAMESPACE::CYBaseException* e) {	excp.reset(e); ExceptionLog(excp->what()); } \
							catch (...) { ExceptionLog("Unknown exception!"); }


/**
 * nop empty function.
 */
inline void nop()
{
}

#endif //__CYDEVICE_PRIVATE_DEFINE_HPP__