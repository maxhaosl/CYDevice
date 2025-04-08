#ifndef __CYDEVICE_HPP__
#define __CYDEVICE_HPP__

#include "CYDevice/ICYDevice.hpp"
#include "Common/CYDevicePrivDefine.hpp"

CYDEVICE_NAMESPACE_BEGIN

class CYDeviceControl;
class CYDeviceImpl : public ICYDevice
{
public:
    CYDeviceImpl();
    virtual ~CYDeviceImpl();

public:
    /**
     * @brief Initialization and de-initialization of cry device.
    */
    virtual int16_t Init(int nWidth/* = 1024*/, int nHeight/* = 768*/, int nFPS/* = 25*/, const wchar_t* pszDeviceName, const wchar_t* pszDeviceId, int nSampleRateHz, const wchar_t* pszAudioName, const wchar_t* pszAudioID, bool bUseRender = true) override;
    virtual int16_t UnInit() override;

    /**
     * @brief Start Capture.
    */
    virtual int16_t StartCapture(ICYAudioDataCallBack* pAudioDataCallBack, ICYVideoDataCallBack* pVideoDataCallBack) override;
    virtual int16_t StopCapture() override;

    /**
     * @brief Get Audio Data.
    */
    virtual int16_t GetNextAudioBuffer(float*& pBuffer, uint32_t& nNumFrames, uint64_t& nTimestamp) override;

private:
    /**
     * @brief Control Object.
    */
    UniquePtr<CYDeviceControl> m_ptrControl;
};

CYDEVICE_NAMESPACE_END

#endif // __CYDEVICE_HPP__