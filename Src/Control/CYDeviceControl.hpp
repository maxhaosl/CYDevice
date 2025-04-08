#ifndef __CYDEVICE_CONTROL_HPP__
#define __CYDEVICE_CONTROL_HPP__

#include "CYDevice/CYDeviceDefine.hpp"
#include "Capture/IDeviceCapture.hpp"
#include <stdint.h>

CYDEVICE_NAMESPACE_BEGIN


class CYDeviceControl
{
public:
    CYDeviceControl();
    virtual ~CYDeviceControl();

public:
    /**
     * @brief Initialization and de-initialization of cry device.
    */
    virtual int16_t Init(int nWidth/* = 1024*/, int nHeight/* = 768*/, int nFPS/* = 25*/, const wchar_t* pszDeviceName, const wchar_t* pszDeviceId, int nSampleRateHz, const wchar_t* pszAudioName, const wchar_t* pszAudioID, bool bUseRender = true);
    virtual int16_t UnInit();

    /**
     * @brief Start Capture.
    */
    virtual int16_t StartCapture(ICYAudioDataCallBack* pAudioDataCallBack, ICYVideoDataCallBack* pVideoDataCallBack);
    virtual int16_t StopCapture();

    /**
     * @brief Get Audio Data.
    */
    virtual int16_t GetNextAudioBuffer(float*& pBuffer, uint32_t& nNumFrames, uint64_t& nTimestamp);

private:
    /**
     * Device Capture Object.
     */
    UniquePtr<IDeviceCapture> m_ptrDeviceCapture;
};

CYDEVICE_NAMESPACE_END

#endif //__CYDEVICE_CONTROL_HPP__