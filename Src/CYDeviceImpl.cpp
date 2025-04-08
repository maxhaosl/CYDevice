#include "Src/CYDeviceImpl.hpp"
#include "Control/CYDeviceControl.hpp"
#include "Common/CYDevicePrivDefine.hpp"

CYDEVICE_NAMESPACE_BEGIN

CYDeviceImpl::CYDeviceImpl()
    : ICYDevice()
{
}

CYDeviceImpl::~CYDeviceImpl()
{
}

int16_t CYDeviceImpl::Init(int nWidth/* = 1024*/, int nHeight/* = 768*/, int nFPS/* = 25*/, const wchar_t* pszDeviceName, const wchar_t* pszDeviceId, int nSampleRateHz, const wchar_t* pszAudioName, const wchar_t* pszAudioID, bool bUseRender/* = true*/)
{
    m_ptrControl = MakeUnique<CYDeviceControl>();
    IfTrueThrow(!m_ptrControl, TEXT("Failed to create a control object!"));

    return m_ptrControl->Init(nWidth, nHeight, nFPS, pszDeviceName, pszDeviceId, nSampleRateHz, pszAudioName, pszAudioID, bUseRender);
}

int16_t CYDeviceImpl::UnInit()
{
    IfTrueThrow(!m_ptrControl, TEXT("The control object is not created!"));
    int16_t nRet = m_ptrControl->UnInit();
    m_ptrControl.reset();
    return nRet;
}

int16_t CYDeviceImpl::StartCapture(ICYAudioDataCallBack* pAudioDataCallBack, ICYVideoDataCallBack* pVideoDataCallBack)
{
    IfTrueThrow(!m_ptrControl, TEXT("The control object is not created!"));
    return m_ptrControl->StartCapture(pAudioDataCallBack, pVideoDataCallBack);
}

int16_t CYDeviceImpl::StopCapture()
{
    IfTrueThrow(!m_ptrControl, TEXT("The control object is not created!"));
    return m_ptrControl->StopCapture();
}

int16_t CYDeviceImpl::GetNextAudioBuffer(float*& pBuffer, uint32_t& nNumFrames, uint64_t& nTimestamp)
{
    IfTrueThrow(!m_ptrControl, TEXT("The control object is not created!"));
    return m_ptrControl->GetNextAudioBuffer(pBuffer, nNumFrames, nTimestamp);
}

CYDEVICE_NAMESPACE_END