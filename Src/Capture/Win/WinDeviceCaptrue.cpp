#include "Capture/Win/WinDeviceCaptrue.hpp"

#include <memory>
#include <string>
#include <vector>
#include <initguid.h>
#include <intsafe.h>

#include <mmreg.h>
#define _IKsControl_
#include <ks.h>
#include <ksmedia.h>

#include "Common/Win/IVideoCaptureFilter.h"
#include "Common/Win/CaptureFilter/CaptureFilter.h"
#include "Common/CYStringHelper.hpp"
#include "Capture/Win/ReSampleRateDefine.hpp"
#include "Capture/Win/DShowCommonDefine.hpp"

#include <xmmintrin.h>
#include <emmintrin.h>

#include "libyuv.h"

#define KSAUDIO_SPEAKER_4POINT1     (KSAUDIO_SPEAKER_QUAD|SPEAKER_LOW_FREQUENCY)
#define KSAUDIO_SPEAKER_3POINT1     (KSAUDIO_SPEAKER_STEREO|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY)
#define KSAUDIO_SPEAKER_2POINT1     (KSAUDIO_SPEAKER_STEREO|SPEAKER_LOW_FREQUENCY)

CYDEVICE_NAMESPACE_BEGIN

void CWinDeviceCaptrue::SetAudioInfo(AM_MEDIA_TYPE* audioMediaType, GUID& expectedAudioType)
{
    expectedAudioType = audioMediaType->subtype;

    if (audioMediaType->formattype == FORMAT_WaveFormatEx)
    {
        WAVEFORMATEX* pFormat = reinterpret_cast<WAVEFORMATEX*>(audioMediaType->pbFormat);
        memcpy(&audioFormat, pFormat, sizeof(audioFormat));

        //WAVE_FORMAT_PCM
        if (audioFormat.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            WAVEFORMATEXTENSIBLE* wfext = (WAVEFORMATEXTENSIBLE*)&audioFormat;
            if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
                m_bFloat = true;
        }
        else if (audioFormat.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            m_bFloat = true;

        inputBitsPerSample = audioFormat.wBitsPerSample;
        inputBlockSize = audioFormat.nBlockAlign;
        inputChannelMask = 0;
        inputChannels = audioFormat.nChannels;
        inputSamplesPerSec = audioFormat.nSamplesPerSec;

        sampleFrameCount = inputSamplesPerSec / 100;
        sampleSegmentSize = inputBlockSize * sampleFrameCount;

        outputBuffer.reserve(sampleSegmentSize);

        CY_LOG_ERROR(TEXT("Device audio info - bits per sample: %u, channels: %u, samples per sec: %u, block size: %u"),
            audioFormat.wBitsPerSample, audioFormat.nChannels, audioFormat.nSamplesPerSec, audioFormat.nBlockAlign);

        //avoid local resampling if possible
        /*if(pFormat->nSamplesPerSec != 44100)
        {
            pFormat->nSamplesPerSec = 44100;
            if(SUCCEEDED(audioConfig->SetFormat(audioMediaType)))
            {
                Log(TEXT("    also successfully set samples per sec to 44.1k"));
                audioFormat.nSamplesPerSec = 44100;
            }
        }*/

        if (m_nSampleRateHz != inputSamplesPerSec)
        {
            int errVal;

            int converterType = SRC_SINC_FASTEST;
            MoreVariables->pResampler = src_new(converterType, 2, &errVal);
            if (!MoreVariables->pResampler)
                CY_LOG_WARN(TEXT("AudioSource::InitAudioData: Could not initiate pResampler"));

            resampleRatio = double(m_nSampleRateHz) / double(inputSamplesPerSec);
            bResample = true;

            //----------------------------------------------------
            // hack to get rid of that weird first quirky resampled packet size

            SRC_DATA data;
            data.src_ratio = resampleRatio;

            std::vector<float> blankBuffer;
            blankBuffer.reserve(inputSamplesPerSec / 100 * 2);

            data.data_in = blankBuffer.data();
            data.input_frames = inputSamplesPerSec / 100;

            UINT frameAdjust = UINT((double(data.input_frames) * resampleRatio) + 1.0);
            UINT newFrameSize = frameAdjust * 2;

            tempResampleBuffer.reserve(newFrameSize);

            data.data_out = tempResampleBuffer.data();
            data.output_frames = frameAdjust;

            data.end_of_input = 0;

            int hResult = src_process(MoreVariables->pResampler, &data);

            nop();
        }
    }
    else
    {
        CY_LOG_WARN(TEXT("CYDevice: Audio format was not a normal wave format"));
        soundOutputType = 0;
    }

    DeleteMediaType(audioMediaType);
}

CWinDeviceCaptrue::CWinDeviceCaptrue()
    : IDeviceSource()
{
    m_pResampler = (void*)new TResamplerContext;
    MoreVariables->nJumpRange = 70;
    m_ptrSampleBuffer = std::make_unique<std::vector<BYTE>>();
}

CWinDeviceCaptrue::~CWinDeviceCaptrue()
{
    if (bResample)
        src_delete(MoreVariables->pResampler);

    delete (TResamplerContext*)m_pResampler;

    if (m_ptrSampleBuffer)
        m_ptrSampleBuffer->clear();
    m_ptrSampleBuffer.reset();
}

int16_t CWinDeviceCaptrue::Init(int nWidth, int nHeight, int nFPS, const wchar_t* pszDeviceName, const wchar_t* pszDeviceId, int nSampleRateHz, const wchar_t* pszAudioName, const wchar_t* pszAudioID, bool bUseRender/* = true*/)
{
    m_nFPS = nFPS;
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_nSampleRateHz = nSampleRateHz;

    bool bSucceeded = false;
    IAMStreamConfig* pStreamConfig = nullptr; SafeReleasePtr<IAMStreamConfig> ptrStreamConfig;
    IMediaControl* pControl = nullptr;

    IPin* pDevicePin = nullptr, *pAudioPin = nullptr;
    SafeReleasePtr<IPin> ptrDevicePin, ptrAudioPin;

    bool bForceCustomAudio = false;
    std::vector<SharePtr<MediaOutputInfo>> outputList;
    IElgatoVideoCaptureFilter6* elgatoFilterInterface6 = nullptr;
    bool elgatoSupportsIAMStreamConfig = false;
    bool elgatoCanRenderFromPin = false;
    GUID expectedMediaType;
    bool bAddedVideoCapture = false, bAddedAudioCapture = false, bAddedDevice = false;
    bool bConnected = false;

    VIDEOINFOHEADER* vih = nullptr;
    BITMAPINFOHEADER* bmi = nullptr;

    UINT elgatoCX = 1280;
    UINT elgatoCY = 720;
    bool elgato = wcsstr(pszDeviceName, L"elgato") != nullptr;

    float volume = 1.0f;

    m_bFloat = false;

    bufferTime = 10 * 10000;// data->GetInt(TEXT("bufferTime")) * 10000;

    SharePtr<MediaOutputInfo> ptrBestOutput;

    HRESULT hResult = S_OK;
    IGraphBuilder* pGraph = nullptr;
    hResult = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, (REFIID)IID_IFilterGraph, (void**)&pGraph);
    if (FAILED(hResult))
    {
        CY_LOG_ERROR(TEXT("CYDevice: Failed to build IGraphBuilder, result = %08lX"), hResult);
        return false;
    }

    m_ptrGraph.reset(pGraph);
    ICaptureGraphBuilder2* pGraphBuilder = nullptr;
    hResult = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, (REFIID)IID_ICaptureGraphBuilder2, (void**)&pGraphBuilder);
    if (FAILED(hResult))
    {
        CY_LOG_ERROR(TEXT("CYDevice: Failed to build ICaptureGraphBuilder2, result = %08lX"), hResult);
        return false;
    }

    m_ptrGraphBuilder.reset(pGraphBuilder);
    m_ptrGraphBuilder->SetFiltergraph(m_ptrGraph.get());

    if (pszDeviceName != nullptr && wcslen(pszDeviceName) > 0)
        m_pDeviceFilter = GetDeviceByValue(CLSID_VideoInputDeviceCategory, const_cast<wchar_t*>(L"FriendlyName"), pszDeviceName, const_cast<wchar_t*>(L"DevicePath"), pszDeviceId);

    if (!m_pDeviceFilter)
    {
        CY_LOG_ERROR(TEXT("CYDevice: Could not create device filter"));
        goto cleanFinish;
    }

    pDevicePin = GetOutputPin(m_pDeviceFilter, &MEDIATYPE_Video);
    if (!pDevicePin)
    {
        CY_LOG_ERROR(TEXT("CYDevice: Could not get device video pin"));
        goto cleanFinish;
    }

    ptrDevicePin.reset(pDevicePin);
    soundOutputType = 1;// data->GetInt(TEXT("soundOutputType")); //0 is for backward-compatibility

    if (soundOutputType != 0)
    {
        if (!bForceCustomAudio)
        {
            hResult = m_ptrGraphBuilder->FindPin(m_pDeviceFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, FALSE, 0, &pAudioPin);
            m_bDeviceHasAudio = SUCCEEDED(hResult);
        }
        else
            m_bDeviceHasAudio = false;

        if (!m_bDeviceHasAudio)
        {
            if (pszAudioName != nullptr && wcslen(pszAudioName) > 0)
            {
                m_pAudioDeviceFilter = GetDeviceByValue(CLSID_AudioInputDeviceCategory, const_cast<wchar_t*>(L"FriendlyName"), pszAudioName, const_cast<wchar_t*>(L"DevicePath"), pszAudioID);
                if (!m_pAudioDeviceFilter)
                    CY_LOG_WARN(TEXT("CYDevice: Invalid audio device: name '%s', path '%s'"), pszAudioName, pszAudioID);
            }

            if (m_pAudioDeviceFilter)
                hResult = m_ptrGraphBuilder->FindPin(m_pAudioDeviceFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, FALSE, 0, &pAudioPin);
            else
                hResult = E_FAIL;
        }

        if (FAILED(hResult) || !pAudioPin)
        {
            CY_LOG_ERROR(TEXT("CYDevice: No audio pin, result = %lX"), hResult);
        }
        else
        {
            ptrAudioPin.reset(pAudioPin);
        }
    }
    else
    {
        m_bDeviceHasAudio = bForceCustomAudio = false;
    }

    GetOutputList(ptrDevicePin, outputList);

    // initialize the basic video variables and data
    if (FAILED(hResult = ptrDevicePin->QueryInterface(IID_IAMStreamConfig, (void**)&pStreamConfig)))
    {
        CY_LOG_ERROR(TEXT("CYDevice: Could not get IAMStreamConfig for device pin, result = %08lX"), hResult);
        goto cleanFinish;
    }
    ptrStreamConfig.reset(pStreamConfig);

    if (SUCCEEDED(m_pDeviceFilter->QueryInterface(IID_IElgatoVideoCaptureFilter6, (void**)&elgatoFilterInterface6)))
        elgatoFilterInterface6->Release();
    elgatoSupportsIAMStreamConfig = (nullptr != elgatoFilterInterface6);
    elgatoCanRenderFromPin = (nullptr != elgatoFilterInterface6);

    renderCX = renderCY = newCX = newCY = 0;
    frameInterval = 0;

    if (m_bUseCustomResolution)
    {
        renderCX = newCX = 640;// data->GetInt(TEXT("resolutionWidth"));
        renderCY = newCY = 480; // data->GetInt(TEXT("resolutionHeight"));
        frameInterval = 25; // data->GetInt(TEXT("frameInterval"));
    }
    else
    {
        SIZE size;
        size.cx = 0;
        size.cy = 0;

        // blackmagic/decklink devices will display a black screen if the resolution/fps doesn't exactly match.
        // they should rename the devices to blackscreen
        if (wcsstr(pszDeviceName, L"blackmagic") != nullptr || wcsstr(pszDeviceName, L"decklink") != nullptr ||
            !GetClosestResolutionFPS(outputList, size, frameInterval, true, m_nWidth, m_nHeight, m_nFPS))
        {
            AM_MEDIA_TYPE* pmt;
            if (SUCCEEDED(ptrStreamConfig->GetFormat(&pmt)))
            {
                VIDEOINFOHEADER* pVih = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);

                // Use "preferred" format from the device
                size.cx = pVih->bmiHeader.biWidth;
                size.cy = pVih->bmiHeader.biHeight;
                frameInterval = pVih->AvgTimePerFrame;

                DeleteMediaType(pmt);
            }
            else
            {
                if (!outputList.size())
                {
                    CY_LOG_ERROR(TEXT("CYDevice: Not even an output list!  What the f***"));
                    goto cleanFinish;
                }

                /* ..........elgato */
                _ASSERTE(elgato && !elgatoSupportsIAMStreamConfig);
                size.cx = outputList[0]->maxCX;
                size.cy = outputList[0]->maxCY;
                frameInterval = outputList[0]->minFrameInterval;
            }
        }

        renderCX = newCX = size.cx;
        renderCY = newCY = size.cy;
    }

    /* elgato always starts off at 720p and changes after. */
    if (elgato && !elgatoSupportsIAMStreamConfig)
    {
        elgatoCX = renderCX;
        elgatoCY = renderCY;

        renderCX = newCX = 1280;
        renderCY = newCY = 720;
    }

    if (!renderCX || !renderCY || !frameInterval)
    {
        CY_LOG_ERROR(TEXT("CYDevice: Invalid size/fps specified"));
        goto cleanFinish;
    }

    preferredOutputType = -1;// (data->GetInt(TEXT("usePreferredType")) != 0) ? data->GetInt(TEXT("preferredType")) : -1;

    // get the closest media output for the settings used
    ptrBestOutput = GetBestMediaOutput(outputList, renderCX, renderCY, preferredOutputType, frameInterval);
    if (!ptrBestOutput)
    {
        if (!outputList.size())
        {
            CY_LOG_ERROR(TEXT("CYDevice: Could not find appropriate resolution to create device image source"));
            goto cleanFinish;
        }
        else
        {
            ptrBestOutput = outputList[0];
            renderCX = newCX = ptrBestOutput->minCX;
            renderCY = newCY = ptrBestOutput->minCY;
            frameInterval = ptrBestOutput->minFrameInterval;
        }
    }

    // set up shaders and video output data

    expectedMediaType = ptrBestOutput->mediaType->subtype;
    m_eColorType = ptrBestOutput->videoType;

    // configure video pin

    AM_MEDIA_TYPE outputMediaType;
    CopyMediaType(&outputMediaType, ptrBestOutput->mediaType);

    vih = reinterpret_cast<VIDEOINFOHEADER*>(outputMediaType.pbFormat);
    bmi = GetVideoBMIHeader(&outputMediaType);
    vih->AvgTimePerFrame = frameInterval;
    bmi->biWidth = renderCX;
    bmi->biHeight = renderCY;
    bmi->biSizeImage = renderCX * renderCY * (bmi->biBitCount >> 3);

    if (FAILED(hResult = ptrStreamConfig->SetFormat(&outputMediaType)))
    {
        if (hResult != E_NOTIMPL)
        {
            CY_LOG_ERROR(TEXT("CYDevice: SetFormat on device pin failed, result = %08lX"), hResult);
            goto cleanFinish;
        }
    }

    FreeMediaType(outputMediaType);

    // get audio pin configuration, optionally configure audio pin to 44100

    GUID expectedAudioType;

    if (soundOutputType == 1)
    {
        IAMStreamConfig* audioConfig;
        if (SUCCEEDED(ptrAudioPin->QueryInterface(IID_IAMStreamConfig, (void**)&audioConfig)))
        {
            AM_MEDIA_TYPE* audioMediaType;
            if (SUCCEEDED(hResult = audioConfig->GetFormat(&audioMediaType)))
            {
                SetAudioInfo(audioMediaType, expectedAudioType);
            }
            else if (hResult == E_NOTIMPL) //elgato probably
            {
                IEnumMediaTypes* audioMediaTypes;
                if (SUCCEEDED(hResult = ptrAudioPin->EnumMediaTypes(&audioMediaTypes)))
                {
                    ULONG i = 0;
                    if ((hResult = audioMediaTypes->Next(1, &audioMediaType, &i)) == S_OK)
                        SetAudioInfo(audioMediaType, expectedAudioType);
                    else
                    {
                        CY_LOG_ERROR(TEXT("CYDevice: audioMediaTypes->Next failed, result = %08lX"), hResult);
                        soundOutputType = 0;
                    }

                    audioMediaTypes->Release();
                }
                else
                {
                    CY_LOG_ERROR(TEXT("CYDevice: audioMediaTypes->Next failed, result = %08lX"), hResult);
                    soundOutputType = 0;
                }
            }
            else
            {
                CY_LOG_ERROR(TEXT("CYDevice: Could not get audio format, result = %08lX"), hResult);
                soundOutputType = 0;
            }

            audioConfig->Release();
        }
        else
        {
            soundOutputType = 0;
        }
    }

    // add video m_ptrGraphBuilder filter if any

    m_pCaptureFilter = new CaptureFilter(this, MEDIATYPE_Video, expectedMediaType);

    if (FAILED(hResult = m_ptrGraph->AddFilter(m_pCaptureFilter, nullptr)))
    {
        CY_LOG_ERROR(TEXT("CYDevice: Failed to add video m_ptrGraphBuilder filter to m_ptrGraph, result = %08lX"), hResult);
        goto cleanFinish;
    }

    bAddedVideoCapture = true;
    // add audio m_ptrGraphBuilder filter if any

    if (soundOutputType == 1)
    {
        m_pAudioFilter = new CaptureFilter(this, MEDIATYPE_Audio, expectedAudioType);
        if (!m_pAudioFilter)
        {
            CY_LOG_ERROR(TEXT("Failed to create audio m_ptrGraphBuilder filter"));
            soundOutputType = 0;
        }
    }
    else if (soundOutputType == 2)
    {
        if (bUseRender)
        {
            if (FAILED(hResult = CoCreateInstance(CLSID_AudioRender, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pAudioFilter)))
            {
                CY_LOG_ERROR(TEXT("CYDevice: failed to create WaveOut Audio renderer, result = %08lX"), hResult);
                soundOutputType = 0;
            }
        }
        else
        {
            if (FAILED(hResult = CoCreateInstance(CLSID_DSoundRender, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pAudioFilter)))
            {
                CY_LOG_ERROR(TEXT("CYDevice: failed to create DirectSound renderer, result = %08lX"), hResult);
                soundOutputType = 0;
            }
        }

        IBasicAudio* basicAudio;
        if (SUCCEEDED(m_pAudioFilter->QueryInterface(IID_IBasicAudio, (void**)&basicAudio)))
        {
            long lVol = long((double(volume) * NEAR_SILENTf) - NEAR_SILENTf);
            if (lVol <= -NEAR_SILENT)
                lVol = -10000;
            basicAudio->put_Volume(lVol);
            basicAudio->Release();
        }
    }

    if (soundOutputType != 0)
    {
        if (FAILED(hResult = m_ptrGraph->AddFilter(m_pAudioFilter, nullptr)))
            CY_LOG_ERROR(TEXT("CYDevice: Failed to add audio m_ptrGraphBuilder filter to m_ptrGraph, result = %08lX"), hResult);

        bAddedAudioCapture = true;
    }

    // add primary device filter

    if (FAILED(hResult = m_ptrGraph->AddFilter(m_pDeviceFilter, nullptr)))
    {
        CY_LOG_ERROR(TEXT("CYDevice: Failed to add device filter to m_ptrGraph, result = %08lX"), hResult);
        goto cleanFinish;
    }

    if (soundOutputType != 0 && !m_bDeviceHasAudio)
    {
        if (FAILED(hResult = m_ptrGraph->AddFilter(m_pAudioDeviceFilter, nullptr)))
            CY_LOG_ERROR(TEXT("CYDevice: Failed to add audio device filter to m_ptrGraph, result = %08lX"), hResult);
    }

    bAddedDevice = true;

    // change elgato resolution
    if (elgato && !elgatoSupportsIAMStreamConfig)
    {
        /* choose closest matching elgato resolution */
        if (!m_bUseCustomResolution)
        {
            UINT baseCX, baseCY;
            UINT closest = 0xFFFFFFFF;
            // API->GetBaseSize(baseCX, baseCY);
            baseCX = 640;
            baseCY = 480;

            const TResSize resolutions[] = { {480, 360}, {640, 480}, {1280, 720}, {1920, 1080} };
            for (const TResSize& res : resolutions)
            {
                UINT val = (UINT)labs((long)res.cy - (long)baseCY);
                if (val < closest)
                {
                    elgatoCX = res.cx;
                    elgatoCY = res.cy;
                    closest = val;
                }
            }
        }

        IElgatoVideoCaptureFilter3* elgatoFilter = nullptr;

        if (SUCCEEDED(m_pDeviceFilter->QueryInterface(IID_IElgatoVideoCaptureFilter3, (void**)&elgatoFilter)))
        {
            VIDEO_CAPTURE_FILTER_SETTINGS settings;
            if (SUCCEEDED(elgatoFilter->GetSettings(&settings)))
            {
                if (elgatoCY == 1080)
                    settings.profile = VIDEO_CAPTURE_FILTER_VID_ENC_PROFILE_1080;
                else if (elgatoCY == 480)
                    settings.profile = VIDEO_CAPTURE_FILTER_VID_ENC_PROFILE_480;
                else if (elgatoCY == 360)
                    settings.profile = VIDEO_CAPTURE_FILTER_VID_ENC_PROFILE_360;
                else
                    settings.profile = VIDEO_CAPTURE_FILTER_VID_ENC_PROFILE_720;

                elgatoFilter->SetSettings(&settings);
            }

            elgatoFilter->Release();
        }
    }

    lastSampleCX = renderCX;
    lastSampleCY = renderCY;

    //------------------------------------------------
    // connect all pins and set up the whole m_ptrGraphBuilder thing

    if (elgato && !elgatoCanRenderFromPin)
    {
        bConnected = SUCCEEDED(hResult = m_ptrGraph->ConnectDirect(ptrDevicePin.get(), m_pCaptureFilter->GetCapturePin(), nullptr));
        if (!bConnected)
        {
            CY_LOG_ERROR(TEXT("CYDevice: Failed to connect the video device pin to the video m_ptrGraphBuilder pin, result = %08lX"), hResult);
            goto cleanFinish;
        }
    }
    else
    {
        bConnected = SUCCEEDED(hResult = m_ptrGraphBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pDeviceFilter, nullptr, m_pCaptureFilter));
        if (!bConnected)
        {
            if (FAILED(hResult = m_ptrGraph->Connect(ptrDevicePin.get(), m_pCaptureFilter->GetCapturePin())))
            {
                CY_LOG_ERROR(TEXT("CYDevice: Failed to connect the video device pin to the video m_ptrGraphBuilder pin, result = %08lX"), hResult);
                goto cleanFinish;
            }
        }
    }

    if (soundOutputType != 0)
    {
        if (elgato && m_bDeviceHasAudio && !elgatoCanRenderFromPin)
        {
            bConnected = false;

            IPin* pAudioPin2 = GetOutputPin(m_pDeviceFilter, &MEDIATYPE_Audio);
            if (pAudioPin2)
            {
                IPin* audioRendererPin = nullptr;

                // FMB NOTE: Connect with first (= the only) pin of audio renderer
                IEnumPins* pIEnum = nullptr;
                if (SUCCEEDED(hResult = m_pAudioFilter->EnumPins(&pIEnum)))
                {
                    IPin* pIPin = nullptr;
                    pIEnum->Next(1, &audioRendererPin, nullptr);
                    SafeRelease(pIEnum);
                }

                if (audioRendererPin)
                {
                    bConnected = SUCCEEDED(hResult = m_ptrGraph->ConnectDirect(pAudioPin2, audioRendererPin, nullptr));
                    audioRendererPin->Release();
                }
                pAudioPin2->Release();
            }
        }
        else
        {
            if (!m_bDeviceHasAudio)
                bConnected = SUCCEEDED(hResult = m_ptrGraphBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, m_pAudioDeviceFilter, nullptr, m_pAudioFilter));
            else
                bConnected = SUCCEEDED(hResult = m_ptrGraphBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, m_pDeviceFilter, nullptr, m_pAudioFilter));
        }

        if (!bConnected)
        {
            CY_LOG_ERROR(TEXT("CYDevice: Failed to connect the audio device pin to the audio m_ptrGraphBuilder pin, result = %08lX"), hResult);
            soundOutputType = 0;
        }
    }

    if (FAILED(hResult = m_ptrGraph->QueryInterface(IID_IMediaControl, (void**)&pControl)))
    {
        CY_LOG_ERROR(TEXT("CYDevice: Failed to get IMediaControl, result = %08lX"), hResult);
        goto cleanFinish;
    }
    ptrMediaControl.reset(pControl);
    bSucceeded = true;

cleanFinish:
    ptrStreamConfig.reset();
    ptrDevicePin.reset();
    ptrAudioPin.reset();
    outputList.clear();

    if (!bSucceeded)
    {
        m_bCapturing = false;

        if (bAddedVideoCapture)
            m_ptrGraph->RemoveFilter(m_pCaptureFilter);
        if (bAddedAudioCapture)
            m_ptrGraph->RemoveFilter(m_pAudioFilter);

        if (bAddedDevice)
        {
            if (!m_bDeviceHasAudio && m_pAudioDeviceFilter)
                m_ptrGraph->RemoveFilter(m_pAudioDeviceFilter);
            m_ptrGraph->RemoveFilter(m_pDeviceFilter);
        }

        SafeRelease(m_pAudioDeviceFilter);
        SafeRelease(m_pDeviceFilter);
        SafeRelease(m_pCaptureFilter);
        SafeRelease(m_pAudioFilter);
        ptrMediaControl.reset();

        soundOutputType = 0;
    }
    else

        // Updated check to ensure that the source actually turns red instead of
        // screwing up the size when SetFormat fails.
        if (renderCX <= 0 || renderCX >= 8192)
        {
            newCX = renderCX = 32; imageCX = renderCX;
        }
    if (renderCY <= 0 || renderCY >= 8192)
    {
        newCY = renderCY = 32; imageCY = renderCY;
    }

    m_bFiltersLoaded = bSucceeded;

    //    ChangeSize(bSucceeded, true);

    return bSucceeded;
}

int16_t CWinDeviceCaptrue::UnInit()
{
    if (m_bFiltersLoaded)
    {
        m_ptrGraph->RemoveFilter(m_pCaptureFilter);
        m_ptrGraph->RemoveFilter(m_pDeviceFilter);
        if (!m_bDeviceHasAudio) m_ptrGraph->RemoveFilter(m_pAudioDeviceFilter);

        if (m_pAudioFilter)
            m_ptrGraph->RemoveFilter(m_pAudioFilter);

        SafeReleaseLogRef(m_pCaptureFilter);
        SafeReleaseLogRef(m_pDeviceFilter);
        SafeReleaseLogRef(m_pAudioDeviceFilter);
        SafeReleaseLogRef(m_pAudioFilter);

        m_bFiltersLoaded = false;
    }

    m_ptrGraphBuilder.reset();
    m_ptrGraph.reset();
    ptrMediaControl.reset();

    return true;
}

int16_t CWinDeviceCaptrue::Start(ICYAudioDataCallBack* pAudioDataCallBack, ICYVideoDataCallBack* pVideoDataCallBack)
{
    if (m_bCapturing || !ptrMediaControl)
        return CYERR_REPEAT_START_CAPTURE;

    HRESULT hResult;
    if (FAILED(hResult = ptrMediaControl->Run()))
    {
        CY_LOG_ERROR(TEXT("CYDevice: control->Run failed, result = %08lX"), hResult);
        return CYERR_CAPTURE_RUN_FAILED;
    }

    m_pAudioDataCallBack = pAudioDataCallBack;
    m_pVideoDataCallBack = pVideoDataCallBack;

    if (m_pAudioDataCallBack)
    {
        m_audioThread = std::thread(&CWinDeviceCaptrue::OnAudioEntry, this);
    }

    if (m_pVideoDataCallBack)
    {
        m_videoThread = std::thread(&CWinDeviceCaptrue::OnVideoEntry, this);
    }

    m_bCapturing = true;
    return CYERR_SUCESS;
}

int16_t CWinDeviceCaptrue::Stop()
{
    if (!m_bCapturing)
        return CYEER_NOT_START_CAPTURE;

    m_bCapturing = false;
    ptrMediaControl->Stop();
    FlushSamples();

    if (m_videoThread.joinable())
    {
        m_videoThread.join();
    }

    if (m_audioThread.joinable())
    {
        m_audioThread.join();
    }

    return CYERR_SUCESS;
}

int16_t CWinDeviceCaptrue::GetNextAudioBuffer(float** bufferOut, uint32_t* numFramesOut, uint64_t* timestampOut)
{
    LPVOID buffer;
    UINT numAudioFrames;
    ULONG64 newTimestamp;

    if (m_ptrSampleBuffer->size() >= sampleSegmentSize)
    {
        {
            std::unique_lock<std::mutex> m_locker(m_audioMutex);

            memcpy(&outputBuffer[0], &m_ptrSampleBuffer.get()[0], sampleSegmentSize);
            m_ptrSampleBuffer->erase(m_ptrSampleBuffer->begin(), m_ptrSampleBuffer->begin() + sampleSegmentSize);
        }

        buffer = outputBuffer.data();
        numAudioFrames = sampleFrameCount;

        latestAudioTime += 10;
        newTimestamp = latestAudioTime;
    }
    else
    {
        return false;
    }

    float* captureBuffer;

    if (!m_bFloat)
    {
        UINT totalSamples = numAudioFrames * inputChannels;
        if (convertBuffer.size() < totalSamples)
            convertBuffer.reserve(totalSamples);

        if (inputBitsPerSample == 8)
        {
            float* tempConvert = convertBuffer.data();
            char* tempSByte = (char*)buffer;

            while (totalSamples--)
            {
                *(tempConvert++) = float(*(tempSByte++)) / 127.0f;
            }
        }
        else if (inputBitsPerSample == 16)
        {
            float* tempConvert = convertBuffer.data();
            short* tempShort = (short*)buffer;

            while (totalSamples--)
            {
                *(tempConvert++) = float(*(tempShort++)) / 32767.0f;
            }
        }
        else if (inputBitsPerSample == 24)
        {
            float* tempConvert = convertBuffer.data();
            BYTE* tempTriple = (BYTE*)buffer;
            UTriple valOut;

            while (totalSamples--)
            {
                UTriple& valIn = (UTriple&)tempTriple;

                valOut.wVal = valIn.wVal;
                valOut.byTripleVal = valIn.byTripleVal;
                if (valOut.byTripleVal > 0x7F)
                    valOut.byLastByte = 0xFF;

                *(tempConvert++) = float(double(valOut.lVal) / 8388607.0);
                tempTriple += 3;
            }
        }
        else if (inputBitsPerSample == 32)
        {
            float* tempConvert = convertBuffer.data();
            long* tempShort = (long*)buffer;

            while (totalSamples--)
            {
                *(tempConvert++) = float(double(*(tempShort++)) / 2147483647.0);
            }
        }

        captureBuffer = convertBuffer.data();
    }
    else
        captureBuffer = (float*)buffer;

    //------------------------------------------------------------
    // channel upmix/downmix

    if (tempBuffer.size() < numAudioFrames * 2)
        tempBuffer.reserve(numAudioFrames * 2);

    float* dataOutputBuffer = tempBuffer.data();
    float* tempOut = dataOutputBuffer;

    if (inputChannels == 1)
    {
        UINT  numFloats = numAudioFrames;
        float* inputTemp = (float*)captureBuffer;
        float* outputTemp = dataOutputBuffer;

        if ((UPARAM(inputTemp) & 0xF) == 0 && (UPARAM(outputTemp) & 0xF) == 0)
        {
            UINT alignedFloats = numFloats & 0xFFFFFFFC;
            for (UINT i = 0; i < alignedFloats; i += 4)
            {
                __m128 inVal = _mm_load_ps(inputTemp + i);

                __m128 outVal1 = _mm_unpacklo_ps(inVal, inVal);
                __m128 outVal2 = _mm_unpackhi_ps(inVal, inVal);

                _mm_store_ps(outputTemp + (i * 2), outVal1);
                _mm_store_ps(outputTemp + (i * 2) + 4, outVal2);
            }

            numFloats -= alignedFloats;
            inputTemp += alignedFloats;
            outputTemp += alignedFloats * 2;
        }

        while (numFloats--)
        {
            float inputVal = *inputTemp;
            *(outputTemp++) = inputVal;
            *(outputTemp++) = inputVal;

            inputTemp++;
        }
    }
    else if (inputChannels == 2) //straight up copy
    {
        memcpy(dataOutputBuffer, captureBuffer, numAudioFrames * 2 * sizeof(float));
    }
    else
    {
        //todo: downmix optimization, also support for other speaker configurations than ones I can merely "think" of.  ugh.
        float* inputTemp = (float*)captureBuffer;
        float* outputTemp = dataOutputBuffer;

        if (inputChannelMask == KSAUDIO_SPEAKER_QUAD)
        {
            UINT numFloats = numAudioFrames * 4;
            float* endTemp = inputTemp + numFloats;

            while (inputTemp < endTemp)
            {
                float left = inputTemp[0];
                float right = inputTemp[1];
                float rearLeft = inputTemp[2] * g_fSurroundMix4;
                float rearRight = inputTemp[3] * g_fSurroundMix4;

                // When in doubt, use only left and right .... and rear left and rear right :)
                // Same idea as with 5.1 downmix

                *(outputTemp++) = (left + rearLeft) * g_fAttn4dotX;
                *(outputTemp++) = (right + rearRight) * g_fAttn4dotX;

                inputTemp += 4;
            }
        }
        else if (inputChannelMask == KSAUDIO_SPEAKER_2POINT1)
        {
            UINT numFloats = numAudioFrames * 3;
            float* endTemp = inputTemp + numFloats;

            while (inputTemp < endTemp)
            {
                float left = inputTemp[0];
                float right = inputTemp[1];

                // Drop LFE since we don't need it
                //float lfe       = inputTemp[2]*g_fLowFreqMix;

                *(outputTemp++) = left;
                *(outputTemp++) = right;

                inputTemp += 3;
            }
        }
        else if (inputChannelMask == KSAUDIO_SPEAKER_3POINT1)
        {
            UINT numFloats = numAudioFrames * 4;
            float* endTemp = inputTemp + numFloats;

            while (inputTemp < endTemp)
            {
                float left = inputTemp[0];
                float right = inputTemp[1];
                float frontCenter = inputTemp[2];
                float lowFreq = inputTemp[3];

                *(outputTemp++) = left;
                *(outputTemp++) = right;

                inputTemp += 4;
            }
        }
        else if (inputChannelMask == KSAUDIO_SPEAKER_4POINT1)
        {
            UINT numFloats = numAudioFrames * 5;
            float* endTemp = inputTemp + numFloats;

            while (inputTemp < endTemp)
            {
                float left = inputTemp[0];
                float right = inputTemp[1];

                // Skip LFE , we don't really need it.
                //float lfe       = inputTemp[2];

                float rearLeft = inputTemp[3] * g_fSurroundMix4;
                float rearRight = inputTemp[4] * g_fSurroundMix4;

                // Same idea as with 5.1 downmix

                *(outputTemp++) = (left + rearLeft) * g_fAttn4dotX;
                *(outputTemp++) = (right + rearRight) * g_fAttn4dotX;

                inputTemp += 5;
            }
        }
        else if (inputChannelMask == KSAUDIO_SPEAKER_SURROUND)
        {
            UINT numFloats = numAudioFrames * 4;
            float* endTemp = inputTemp + numFloats;

            while (inputTemp < endTemp)
            {
                float left = inputTemp[0];
                float right = inputTemp[1];
                float frontCenter = inputTemp[2];
                float rearCenter = inputTemp[3];

                // When in doubt, use only left and right :) Seriously.
                // THIS NEEDS TO BE PROPERLY IMPLEMENTED!

                *(outputTemp++) = left;
                *(outputTemp++) = right;

                inputTemp += 4;
            }
        }
        // Both speakers configs share the same format, the difference is in rear speakers position
        // See: http://msdn.microsoft.com/en-us/library/windows/hardware/ff537083(v=vs.85).aspx
        // Probably for KSAUDIO_SPEAKER_5POINT1_SURROUND we will need a different coefficient for rear left/right
        else if (inputChannelMask == KSAUDIO_SPEAKER_5POINT1 || inputChannelMask == KSAUDIO_SPEAKER_5POINT1_SURROUND)
        {
            UINT numFloats = numAudioFrames * 6;
            float* endTemp = inputTemp + numFloats;

            while (inputTemp < endTemp)
            {
                float left = inputTemp[0];
                float right = inputTemp[1];
                float center = inputTemp[2] * g_fCenterMix;

                //We don't need LFE channel so skip it (see below)
                //float lowFreq   = inputTemp[3]*g_fLowFreqMix;

                float rearLeft = inputTemp[4] * g_fSurroundMix;
                float rearRight = inputTemp[5] * g_fSurroundMix;

                // According to ITU-R  BS.775-1 recommendation, the downmix from a 3/2 source to stereo
                // is the following:
                // L = FL + k0*C + k1*RL
                // R = FR + k0*C + k1*RR
                // FL = front left
                // FR = front right
                // C  = center
                // RL = rear left
                // RR = rear right
                // k0 = g_fCenterMix   = g_fDbMinus3 = 0.7071067811865476 [for k0 we can use g_fDbMinus6 = 0.5 too, probably it's better]
                // k1 = g_fSurroundMix = g_fDbMinus3 = 0.7071067811865476

                // The output (L,R) can be out of (-1,1) domain so we attenuate it [ g_fAttn5dot1 = 1/(1 + g_fCenterMix + g_fSurroundMix) ]
                // Note: this method of downmixing is far from "perfect" (pretty sure it's not the correct way) but the resulting downmix is "okayish", at least no more bleeding ears.
                // (maybe have a look at http://forum.doom9.org/archive/index.php/t-148228.html too [ 5.1 -> stereo ] the approach seems almost the same [but different coefficients])

                // http://acousticsfreq.com/blog/wp-content/uploads/2012/01/ITU-R-BS775-1.pdf
                // http://ir.lib.nctu.edu.tw/bitstream/987654321/22934/1/030104001.pdf

                *(outputTemp++) = (left + center + rearLeft) * g_fAttn5dot1;
                *(outputTemp++) = (right + center + rearRight) * g_fAttn5dot1;

                inputTemp += 6;
            }
        }

        // According to http://msdn.microsoft.com/en-us/library/windows/hardware/ff537083(v=vs.85).aspx
        // KSAUDIO_SPEAKER_7POINT1 is obsolete and no longer supported in Windows Vista and later versions of Windows
        // Not sure what to do about it, meh , drop front left of center/front right of center -> 5.1 -> stereo;

        else if (inputChannelMask == KSAUDIO_SPEAKER_7POINT1)
        {
            UINT numFloats = numAudioFrames * 8;
            float* endTemp = inputTemp + numFloats;

            while (inputTemp < endTemp)
            {
                float left = inputTemp[0];
                float right = inputTemp[1];

                float center = inputTemp[2] * g_fCenterMix;

                // Drop LFE since we don't need it
                //float lowFreq       = inputTemp[3]*g_fLowFreqMix;

                float rearLeft = inputTemp[4] * g_fSurroundMix;
                float rearRight = inputTemp[5] * g_fSurroundMix;

                // Drop SPEAKER_FRONT_LEFT_OF_CENTER , SPEAKER_FRONT_RIGHT_OF_CENTER
                //float centerLeft    = inputTemp[6];
                //float centerRight   = inputTemp[7];

                // Downmix from 5.1 to stereo
                *(outputTemp++) = (left + center + rearLeft) * g_fAttn5dot1;
                *(outputTemp++) = (right + center + rearRight) * g_fAttn5dot1;

                inputTemp += 8;
            }
        }

        // Downmix to 5.1 (easy stuff) then downmix to stereo as done in KSAUDIO_SPEAKER_5POINT1
        else if (inputChannelMask == KSAUDIO_SPEAKER_7POINT1_SURROUND)
        {
            UINT numFloats = numAudioFrames * 8;
            float* endTemp = inputTemp + numFloats;

            while (inputTemp < endTemp)
            {
                float left = inputTemp[0];
                float right = inputTemp[1];
                float center = inputTemp[2] * g_fCenterMix;

                // Skip LFE we don't need it
                //float lowFreq   = inputTemp[3]*g_fLowFreqMix;

                float rearLeft = inputTemp[4];
                float rearRight = inputTemp[5];
                float sideLeft = inputTemp[6];
                float sideRight = inputTemp[7];

                // combine the rear/side channels first , baaam! 5.1
                rearLeft = (rearLeft + sideLeft) * 0.5f;
                rearRight = (rearRight + sideRight) * 0.5f;

                // downmix to stereo as in 5.1 case
                *(outputTemp++) = (left + center + rearLeft * g_fSurroundMix) * g_fAttn5dot1;
                *(outputTemp++) = (right + center + rearRight * g_fSurroundMix) * g_fAttn5dot1;

                inputTemp += 8;
            }
        }
    }

    ReleaseAudioBuffer();

    //------------------------------------------------------------
     // resample

    if (bResample)
    {
        UINT frameAdjust = UINT((double(numAudioFrames) * resampleRatio) + 1.0);
        UINT newFrameSize = frameAdjust * 2;

        if (tempResampleBuffer.size() < newFrameSize)
            tempResampleBuffer.reserve(newFrameSize);

        SRC_DATA data;
        data.src_ratio = resampleRatio;

        data.data_in = tempBuffer.data();
        data.input_frames = numAudioFrames;

        data.data_out = tempResampleBuffer.data();
        data.output_frames = frameAdjust;

        data.end_of_input = 0;

        int hResult = src_process(MoreVariables->pResampler, &data);
        if (hResult)
        {
            CY_LOG_ERROR(TEXT("AudioSource::QueryAudio: Was unable to resample audio for device"));
            return false;
        }

        if (data.input_frames_used != numAudioFrames)
        {
            CY_LOG_ERROR(TEXT("AudioSource::QueryAudio: Failed to downsample buffer completely, which shouldn't actually happen because it should be using 10ms of samples"));
            return false;
        }

        numAudioFrames = data.output_frames_gen;
    }

    //------------------------------------------------------
    // timestamp smoothing (keep audio within 70ms of target time)

    if (!lastUsedTimestamp)
        lastUsedTimestamp = newTimestamp;
    else
        lastUsedTimestamp += 10;

    UINT64 difVal = GetQWDif(newTimestamp, lastUsedTimestamp);

    if (difVal > MoreVariables->nJumpRange /*|| (bCanBurstHack && newTimestamp < lastUsedTimestamp)*/)
    {
        /*QWORD curTimeMS = App->GetVideoTime()-App->GetSceneTimestamp();
        UINT curTimeTotalSec = (UINT)(curTimeMS/1000);
        UINT curTimeTotalMin = curTimeTotalSec/60;
        UINT curTimeHr  = curTimeTotalMin/60;
        UINT curTimeMin = curTimeTotalMin-(curTimeHr*60);
        UINT curTimeSec = curTimeTotalSec-(curTimeTotalMin*60);

        Log(TEXT("A timestamp adjustment was encountered for device %s, approximate stream time is: %u:%u:%u, prev value: %llu, new value: %llu"), GetDeviceName(), curTimeHr, curTimeMin, curTimeSec, lastUsedTimestamp, newTimestamp);*/
        //if (difVal >= 70)
        //    Log(TEXT("A timestamp adjustment was encountered for device %s, diffVal: %llu, nJumpRange: %llu, newTimestamp: %llu, lastUsedTimestamp: %llu"),
        //    GetDeviceName(), difVal, MoreVariables->nJumpRange, newTimestamp, lastUsedTimestamp);
        lastUsedTimestamp = newTimestamp;
    }

    //if (sstri(GetDeviceName(), L"avermedia") != nullptr)
    //    Log(L"newTimestamp: %llu, lastUsedTimestamp: %llu", newTimestamp, lastUsedTimestamp);

    //-----------------------------------------------------------------------------

    float* newBuffer = (bResample) ? tempResampleBuffer.data() : tempBuffer.data();

    bool bCanBurstHack = true;
    bool overshotAudio = (lastUsedTimestamp < lastSentTimestamp + 10);
    if (bCanBurstHack || !overshotAudio)
    {
        lastSentTimestamp = lastUsedTimestamp;
    }

    *bufferOut = newBuffer;
    *numFramesOut = numAudioFrames;
    *timestampOut = lastUsedTimestamp;

    return true;
}

int16_t CWinDeviceCaptrue::ReleaseAudioBuffer()
{
    std::unique_lock<std::mutex> m_locker(m_audioMutex);
    m_ptrSampleBuffer->clear();
    return CYERR_SUCESS;
}

void CWinDeviceCaptrue::FlushSamples()
{
    std::unique_lock<std::mutex> m_locker(m_audioMutex);
    m_ptrSampleBuffer->clear();
}

void CWinDeviceCaptrue::ReceiveMediaSample(IMediaSample* sample, bool bAudio)
{
    if (!sample)
        return;

    BYTE* pointer;

    long nLength = sample->GetActualDataLength();

    if (!nLength)
        return;

    if (SUCCEEDED(sample->GetPointer(&pointer)))
    {
        if (bAudio)
        {
            std::unique_lock<std::mutex> m_locker(m_audioMutex);
            m_ptrSampleBuffer->insert(m_ptrSampleBuffer->end(), pointer, pointer + nLength);
            CY_LOG_TRACE(TEXT("Audio Length=%d Time=%d"), nLength, GetTickCount() - m_dwStartTime);
            m_dwStartTime = GetTickCount();
            m_audioCV.notify_one();
        }
        else
        {
            std::unique_ptr<TSampleData> ptrdata;

            AM_MEDIA_TYPE* mt = nullptr;
            if (sample->GetMediaType(&mt) == S_OK)
            {
                BITMAPINFOHEADER* bih = GetVideoBMIHeader(mt);
                lastSampleCX = bih->biWidth;
                lastSampleCY = bih->biHeight;
                DeleteMediaType(mt);
            }

            ptrdata = std::make_unique<TSampleData>();
            ptrdata->bAudio = bAudio;
            ptrdata->nDataLength = nLength;
            ptrdata->lpData = (LPBYTE)malloc(ptrdata->nDataLength);//pointer; //
            ptrdata->cx = lastSampleCX;
            ptrdata->cy = lastSampleCY;
            /*data->sample = sample;
            sample->AddRef();*/

            memcpy(ptrdata->lpData, pointer, ptrdata->nDataLength);

            LONGLONG stopTime;
            sample->GetTime(&stopTime, &ptrdata->nTimestamp);

            {
                UniqueLock locker(m_videoMutex);
                latestVideoSample = std::move(ptrdata);
            }
            m_videoCV.notify_one();
        }
    }
}

void CWinDeviceCaptrue::OnAudioEntry()
{
    while (m_bCapturing)
    {
        UniqueLock locker(m_audioMutex);
        m_audioCV.wait_for(locker, std::chrono::milliseconds(10), [this]() { return m_ptrSampleBuffer->size() >= sampleSegmentSize; });

        if (!m_bCapturing) break;

        float* pBuffer = nullptr;
        uint32_t numAudioFrames = 0;
        uint64_t newTimestamp = 0;
        if (GetNextAudioBuffer((float**)&pBuffer, &numAudioFrames, &newTimestamp))
        {
            if (m_pAudioDataCallBack) m_pAudioDataCallBack->OnAudioData(pBuffer, numAudioFrames, 2, newTimestamp);
        }
    }
}

int ConvertVideoType(ECYVideoOutputType eVideoType)
{
    switch (eVideoType)
    {
    case TYPE_VIDEO_OUTPUT_NONE:
        return libyuv::FOURCC_ANY;
    case TYPE_VIDEO_OUTPUT_I420:
        return libyuv::FOURCC_I420;
    case TYPE_VIDEO_OUTPUT_YV12:
        return libyuv::FOURCC_YV12;
    case TYPE_VIDEO_OUTPUT_RGB24:
        return libyuv::FOURCC_24BG;
    case TYPE_VIDEO_OUTPUT_RGB565:
        return libyuv::FOURCC_RGBP;
    case TYPE_VIDEO_OUTPUT_YUY2:
        return libyuv::FOURCC_YUY2;
    case TYPE_VIDEO_OUTPUT_UYVY:
        return libyuv::FOURCC_UYVY;
    case TYPE_VIDEO_OUTPUT_MJPG:
        return libyuv::FOURCC_MJPG;
    case TYPE_VIDEO_OUTPUT_ARGB32:
        return libyuv::FOURCC_ARGB;
    case TYPE_VIDEO_OUTPUT_RGB32:
        return libyuv::FOURCC_ARGB;
    case TYPE_VIDEO_OUTPUT_Y41P:
    case TYPE_VIDEO_OUTPUT_YVU9:
        return libyuv::FOURCC_ANY; ///???
    case TYPE_VIDEO_OUTPUT_YVYU:
        return libyuv::FOURCC_YUYV;
    case TYPE_VIDEO_OUTPUT_HDYC:
        return libyuv::FOURCC_HDYC;
    case TYPE_VIDEO_OUTPUT_MPEG2_VIDEO:
        return libyuv::FOURCC_ANY; ///????
    case TYPE_VIDEO_OUTPUT_H264:
        return libyuv::FOURCC_ANY; ///????
    case TYPE_VIDEO_OUTPUT_DVSL:
    case TYPE_VIDEO_OUTPUT_DVSD:
    case TYPE_VIDEO_OUTPUT_DVHD:
        return libyuv::FOURCC_ANY; ///????
    }
    return libyuv::FOURCC_ANY;
}

size_t CalcBufferSize(ECYVideoOutputType eType, int width, int height)
{
    size_t buffer_size = 0;
    switch (eType)
    {
    case TYPE_VIDEO_OUTPUT_I420:
    case TYPE_VIDEO_OUTPUT_YV12:
    {
        int half_width = (width + 1) >> 1;
        int half_height = (height + 1) >> 1;
        buffer_size = width * height + half_width * half_height * 2;
        break;
    }
    case TYPE_VIDEO_OUTPUT_RGB565:
    case TYPE_VIDEO_OUTPUT_YUY2:
    case TYPE_VIDEO_OUTPUT_YVYU:
    case TYPE_VIDEO_OUTPUT_UYVY:
        buffer_size = width * height * 2;
        break;
    case TYPE_VIDEO_OUTPUT_RGB24:
        buffer_size = width * height * 3;
        break;
    case TYPE_VIDEO_OUTPUT_ARGB32:
    case TYPE_VIDEO_OUTPUT_RGB32:
        buffer_size = width * height * 4;
        break;
    default:
        assert(0);
        break;
    }
    return buffer_size;
}

void CWinDeviceCaptrue::OnVideoEntry()
{
    while (m_bCapturing)
    {
        UniqueLock locker(m_videoMutex);
        m_videoCV.wait_for(locker, std::chrono::milliseconds(10), [this]() { return latestVideoSample != nullptr; });

        if (!m_bCapturing) break;

        std::unique_ptr<TSampleData> lastSample;
        lastSample = std::move(latestVideoSample);
        latestVideoSample.reset();

        if (lastSample)
        {
            newCX = lastSample->cx;
            newCY = lastSample->cy;

            const int32_t width = lastSample->cx;
            const int32_t height = lastSample->cy;

            
            // Not encoded, convert to I420.
            if (m_eColorType != TYPE_VIDEO_OUTPUT_MJPG &&
                CalcBufferSize(m_eColorType, width, abs(height)) !=
                lastSample->nDataLength)
            {
                CY_LOG_ERROR("Wrong incoming frame length.");
                continue;
            }

            int stride_y = width;
            int stride_uv = (width + 1) / 2;
            int target_width = width;
            int target_height = abs(height);

            libyuv::RotationMode rotation_mode = libyuv::kRotate0;

            std::unique_ptr<char[]> ptrYUVBuffer = std::make_unique<char[]>(width * height * 3 / 2);

            uint8_t* dst_y = (uint8_t*)ptrYUVBuffer.get();
            uint8_t* dst_u = dst_y + width * height;
            uint8_t* dst_v = dst_u + width * height / 4;

            const int conversionResult = libyuv::ConvertToI420(
                lastSample->lpData, lastSample->nDataLength, dst_y,
                stride_y, dst_u,
                stride_uv, dst_v,
                stride_uv, 0, 0,  // No Cropping
                width, height, target_width, target_height, rotation_mode,
                ConvertVideoType(m_eColorType));
            if (conversionResult < 0)
            {
                CY_LOG_ERROR("Failed to convert capture frame from type %d to I420.", (int)m_eColorType);
                continue;
            }

            if (m_pVideoDataCallBack)
            {
                m_pVideoDataCallBack->OnVideoData(dst_y, width * height * 3 / 2, width, height, lastSample->nTimestamp);
            }

            lastSample.reset();
        }
    }
}

// 枚举摄像头和音频设备列表
//////////////////////////////////////////////////////////////////////////

bool FillOutListOfDevices(GUID matchGUID, std::vector<std::wstring>& deviceList, std::vector<std::wstring>& deviceIDList)
{
    deviceIDList.clear();
    deviceList.clear();

    ICreateDevEnum* deviceEnum;
    IEnumMoniker* videoDeviceEnum;

    HRESULT hResult;
    hResult = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
    if (FAILED(hResult))
    {
        CY_LOG_ERROR(TEXT("FillOutListDevices: CoCreateInstance for the device enum failed, result = %08lX"), hResult);
        return false;
    }

    hResult = deviceEnum->CreateClassEnumerator(matchGUID, &videoDeviceEnum, 0);
    if (FAILED(hResult))
    {
        CY_LOG_ERROR(TEXT("FillOutListDevices: deviceEnum->CreateClassEnumerator failed, result = %08lX"), hResult);
        deviceEnum->Release();
        return false;
    }

    SafeRelease(deviceEnum);

    if (hResult == S_FALSE) //no devices
        return false;

    //------------------------------------------

    IMoniker* deviceInfo;
    DWORD count;

    while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
    {
        IPropertyBag* propertyData;
        hResult = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
        if (SUCCEEDED(hResult))
        {
            VARIANT friendlyNameValue, devicePathValue;
            friendlyNameValue.vt = VT_BSTR;
            friendlyNameValue.bstrVal = nullptr;
            devicePathValue.vt = VT_BSTR;
            devicePathValue.bstrVal = nullptr;

            hResult = propertyData->Read(L"FriendlyName", &friendlyNameValue, nullptr);
            propertyData->Read(L"DevicePath", &devicePathValue, nullptr);

            if (SUCCEEDED(hResult))
            {
                IBaseFilter* filter;
                hResult = deviceInfo->BindToObject(nullptr, 0, IID_IBaseFilter, (void**)&filter);
                if (SUCCEEDED(hResult))
                {
                    std::wstring strDeviceName = (WCHAR*)friendlyNameValue.bstrVal;
                    deviceList.push_back(strDeviceName);

                    if (devicePathValue.bstrVal)
                    {
                        std::wstring strDeviceID = (WCHAR*)devicePathValue.bstrVal;
                        deviceIDList.push_back(strDeviceID);
                    }
                    SafeRelease(filter);
                }
            }

            SafeRelease(propertyData);
        }

        SafeRelease(deviceInfo);
    }

    SafeRelease(videoDeviceEnum);

    return true;
}

CYDEVICE_NAMESPACE_END