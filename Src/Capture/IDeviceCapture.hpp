#ifndef __I_DEVICE_CAPTURE_HPP__
#define __I_DEVICE_CAPTURE_HPP__

#include "Common/CYDevicePrivDefine.hpp"

CYDEVICE_NAMESPACE_BEGIN

class IDeviceCapture
{
public:
    IDeviceCapture(){ }
    virtual ~IDeviceCapture(){ }

public:
    virtual int16_t Init(int nWidth/* = 1024*/, int nHeight/* = 768*/, int nFPS/* = 25*/, const wchar_t* pszDeviceName, const wchar_t* pszDeviceId, int nSampleRateHz, const wchar_t* pszAudioName, const wchar_t* pszAudioID, bool bUseRender = true) = 0;
    virtual int16_t UnInit() = 0;

    virtual int16_t Start(ICYAudioDataCallBack* pAudioDataCallBack, ICYVideoDataCallBack* pVideoDataCallBack) = 0;
    virtual int16_t Stop() = 0;

    virtual int16_t GetNextAudioBuffer(float** buffer, uint32_t* numFrames, uint64_t* timestamp) = 0;
    virtual int16_t ReleaseAudioBuffer() = 0;
};

CYDEVICE_NAMESPACE_END

#endif // __I_DEVICE_CAPTURE_HPP__