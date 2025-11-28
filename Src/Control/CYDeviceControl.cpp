#include "Control/CYDeviceControl.hpp"
#include "Common/CYDevicePrivDefine.hpp"

#ifdef WIN32
#include "Capture/Win/WinDeviceCaptrue.hpp"
#endif

CYDEVICE_NAMESPACE_BEGIN

CYDeviceControl::CYDeviceControl()
{
}

CYDeviceControl::~CYDeviceControl()
{
}

int16_t CYDeviceControl::Init(int nWidth/* = 1024*/, int nHeight/* = 768*/, int nFPS/* = 25*/, const wchar_t* pszDeviceName, const wchar_t* pszDeviceId, int nSampleRateHz, const wchar_t* pszAudioName, const wchar_t* pszAudioID, bool bUseRender/* = true*/)
{
    EXCEPTION_BEGIN
    {
#ifdef WIN32
        m_ptrDeviceCapture = MakeUnique<CWinDeviceCaptrue>();
#else

#endif
        IfTrueThrow(!m_ptrDeviceCapture, TEXT("Failed to create a device capture object!"));
    }
    EXCEPTION_END
    return m_ptrDeviceCapture->Init(nWidth, nHeight, nFPS, pszDeviceName, pszDeviceId, nSampleRateHz, pszAudioName, pszAudioID, bUseRender);
}

int16_t CYDeviceControl::UnInit()
{
    int nRet = CYERR_FAILED;
    EXCEPTION_BEGIN
    {
        IfTrueThrow(!m_ptrDeviceCapture, TEXT("The device capture object is not created!"));
        nRet = m_ptrDeviceCapture->UnInit();
    }
    EXCEPTION_END
    return CYERR_SUCESS;
}

int16_t CYDeviceControl::StartCapture(ICYAudioDataCallBack* pAudioDataCallBack, ICYVideoDataCallBack* pVideoDataCallBack)
{
    int nRet = CYERR_FAILED;
    EXCEPTION_BEGIN
    {
        IfTrueThrow(!m_ptrDeviceCapture, TEXT("The device capture object is not created!"));
        nRet = m_ptrDeviceCapture->Start(pAudioDataCallBack, pVideoDataCallBack);
    }
    EXCEPTION_END
    return nRet;
}

int16_t CYDeviceControl::StopCapture()
{
    int nRet = CYERR_FAILED;
    EXCEPTION_BEGIN
    {
        IfTrueThrow(!m_ptrDeviceCapture, TEXT("The device capture object is not created!"));
        nRet = m_ptrDeviceCapture->Stop();
    }
    EXCEPTION_END
    return nRet;
}

int16_t CYDeviceControl::GetNextAudioBuffer(float*& pBuffer, uint32_t& nNumFrames, uint64_t& nTimestamp)
{
    int nRet = CYERR_FAILED;
    EXCEPTION_BEGIN
    {
        IfTrueThrow(!m_ptrDeviceCapture, TEXT("The device capture object is not created!"));
        nRet = m_ptrDeviceCapture->GetNextAudioBuffer(&pBuffer, &nNumFrames, &nTimestamp);
    }
    EXCEPTION_END
    return nRet;
}

CYDEVICE_NAMESPACE_END