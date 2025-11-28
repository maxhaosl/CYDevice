#pragma once

#include <dshow.h>
#include <Amaudio.h>
#include <Dvdmedia.h>
#include "Common/Win/CaptureFilter/CaptureFilter.h"
#include "Common/Win/IDeviceSource.h"
#include "Common/CYDevicePrivDefine.hpp"
#include "Capture/IDeviceCapture.hpp"

#include <vector>
#include <mutex>

class CaptureFilter;

CYDEVICE_NAMESPACE_BEGIN

template<typename T>
[[maybe_unused]] inline void CYSafeRelease(T* t)
{
    if (t)
    {
        SafeReleaseLogRef(t);
    }
}
template<typename T, void (*Fn)(T*) = CYSafeRelease>
using SafeReleasePtr = std::unique_ptr<typename std::remove_pointer_t<T>, PointerDel<Fn>>;

struct TSampleData;
class CWinDeviceCaptrue : public IDeviceSource, public IDeviceCapture
{
public:
    CWinDeviceCaptrue();
    virtual ~CWinDeviceCaptrue();

    int16_t Init(int nWidth/* = 1024*/, int nHeight/* = 768*/, int nFPS/* = 25*/, const wchar_t* pszDeviceName, const wchar_t* pszDeviceId, int nSampleRateHz, const wchar_t* pszAudioName, const wchar_t* pszAudioID, bool bUseRender = true) override;
    int16_t UnInit() override;

    int16_t Start(ICYAudioDataCallBack* pAudioDataCallBack, ICYVideoDataCallBack* pVideoDataCallBack) override;
    int16_t Stop() override;

    int16_t GetNextAudioBuffer(float** buffer, uint32_t* numFrames, uint64_t* timestamp) override;
    int16_t ReleaseAudioBuffer() override;

protected:
    void SetAudioInfo(AM_MEDIA_TYPE* audioMediaType, GUID& expectedAudioType);

    virtual void FlushSamples() override;
    virtual void ReceiveMediaSample(IMediaSample* sample, bool bAudio) override;

    void OnAudioEntry();
    void OnVideoEntry();

private:
    SafeReleasePtr<IGraphBuilder> m_ptrGraph;
    SafeReleasePtr<ICaptureGraphBuilder2> m_ptrGraphBuilder;

    SafeReleasePtr<IMediaControl> ptrMediaControl;

    IBaseFilter* m_pDeviceFilter = nullptr;
    IBaseFilter* m_pAudioDeviceFilter = nullptr;
    CaptureFilter* m_pCaptureFilter;
    IBaseFilter* m_pAudioFilter; // Audio renderer filter

    bool m_bDeviceHasAudio = true;
    bool m_bUseCustomResolution = false;

    bool m_bCapturing = false;
    bool m_bFiltersLoaded = false;

    bool  m_bFloat = false;
    UINT  inputChannels;
    UINT  inputSamplesPerSec;
    UINT  inputBitsPerSample;
    UINT  inputBlockSize;
    DWORD inputChannelMask;

    UINT sampleSegmentSize, sampleFrameCount;

    UINT imageCX, imageCY;

    UINT bufferTime;				// 100-nsec units (same as REFERENCE_TIME)
    UINT lastSampleCX, lastSampleCY;

    int  soundOutputType;
    WAVEFORMATEX            audioFormat;

    ECYVideoOutputType m_eColorType;
    UINT64          frameInterval;
    UINT            renderCX, renderCY;
    UINT            newCX, newCY;
    UINT            preferredOutputType = -1;

    UINT64          lastUsedTimestamp = 0;
    UINT64          lastSentTimestamp = 0;
    double          resampleRatio;

    LPVOID          m_pResampler = nullptr;
    bool            bResample = false;



    std::mutex m_audioMutex;
    std::condition_variable m_audioCV;
    std::unique_ptr<std::vector<BYTE>> m_ptrSampleBuffer;
    std::unique_ptr<TSampleData> latestVideoSample;

    ULONG64 latestAudioTime = 0;
    std::vector<float> outputBuffer;

    std::vector<float> convertBuffer;

    std::vector<float> tempBuffer;
    std::vector<float> tempResampleBuffer;

    std::mutex m_videoMutex;
    std::condition_variable m_videoCV;

    std::thread m_audioThread;
    std::thread m_videoThread;

    DWORD   m_dwStartTime = GetTickCount();
    ICYAudioDataCallBack* m_pAudioDataCallBack = nullptr;
    ICYVideoDataCallBack* m_pVideoDataCallBack = nullptr;

    int     m_nFPS = 0;
    int     m_nWidth = 0;
    int     m_nHeight = 0;

    int    m_nSampleRateHz = 0;
};

CYDEVICE_NAMESPACE_END