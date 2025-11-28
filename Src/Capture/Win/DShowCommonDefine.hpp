#ifndef __DSHOW_COMMON_DEFINE_HPP__
#define __DSHOW_COMMON_DEFINE_HPP__

#include "Common/CYDevicePrivDefine.hpp"

#include <initguid.h>
#include <intsafe.h>

#include <mmreg.h>
#define _IKsControl_
#include <ks.h>
#include <ksmedia.h>

CYDEVICE_NAMESPACE_BEGIN

constexpr int inputPriority[] =
{
    1,
    6,
    7,
    7,

    12,
    12,

    -1,
    -1,

    13,
    13,
    13,
    13,

    5,
    -1,

    10,
    10,
    10,

    9
};

inline void FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((LPVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }

    SafeRelease(mt.pUnk);
}

struct MediaOutputInfo
{
    ECYVideoOutputType videoType;
    AM_MEDIA_TYPE* mediaType = nullptr;
    UINT64 minFrameInterval, maxFrameInterval;
    UINT minCX, minCY;
    UINT maxCX, maxCY;
    UINT xGranularity, yGranularity;
    bool bUsingFourCC;

    inline void FreeData()
    {
        if (mediaType)
        {
            FreeMediaType(*mediaType);
            CoTaskMemFree(mediaType);
            mediaType = nullptr;
        }
    }

    ~MediaOutputInfo()
    {
        FreeData();
    }
};

constexpr GUID MEDIASUBTYPE_I420 = { 0x30323449, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };

inline void DeleteMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt != NULL)
    {
        FreeMediaType(*pmt);
        CoTaskMemFree(pmt);
    }
}

inline IBaseFilter* GetExceptionDevice(REFGUID targetGUID)
{
    IBaseFilter* filter;
    if (SUCCEEDED(CoCreateInstance(targetGUID, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&filter)))
        return filter;

    return NULL;
}

inline bool GetGUIDFromString(const wchar_t* lpGUID, GUID& targetGUID)
{
    std::wstring strGUID = lpGUID;

    if (strGUID.length() != 38)
        return false;

    strGUID = strGUID.substr(1, strGUID.length() - 1);

    std::vector<std::wstring> GUIDData;
    GetTokenList(GUIDData, strGUID.c_str(), '-', FALSE);

    if (GUIDData.size() != 5)
        return false;

    if (GUIDData[0].length() != 8 ||
        GUIDData[1].length() != 4 ||
        GUIDData[2].length() != 4 ||
        GUIDData[3].length() != 4 ||
        GUIDData[4].length() != 12)
    {
        return false;
    }
    targetGUID.Data1 = (UINT)wcstoul(GUIDData[0].c_str(), NULL, 16);
    targetGUID.Data2 = (WORD)wcstoul(GUIDData[1].c_str(), NULL, 16);
    targetGUID.Data3 = (WORD)wcstoul(GUIDData[2].c_str(), NULL, 16);
    targetGUID.Data4[0] = (BYTE)wcstoul(GUIDData[3].substr(0, 2).c_str(), NULL, 16);
    targetGUID.Data4[1] = (BYTE)wcstoul(GUIDData[3].substr(GUIDData[3].length() - 2, 2).c_str(), NULL, 16);
    targetGUID.Data4[2] = (BYTE)wcstoul(GUIDData[4].substr(0, 2).c_str(), NULL, 16);
    targetGUID.Data4[3] = (BYTE)wcstoul(GUIDData[4].substr(2, 4).c_str(), NULL, 16);
    targetGUID.Data4[4] = (BYTE)wcstoul(GUIDData[4].substr(4, 6).c_str(), NULL, 16);
    targetGUID.Data4[5] = (BYTE)wcstoul(GUIDData[4].substr(6, 8).c_str(), NULL, 16);
    targetGUID.Data4[6] = (BYTE)wcstoul(GUIDData[4].substr(8, 10).c_str(), NULL, 16);
    targetGUID.Data4[7] = (BYTE)wcstoul(GUIDData[4].substr(GUIDData[3].length() - 2, 2).c_str(), NULL, 16);

    return true;
}

inline IBaseFilter* GetExceptionDevice(const wchar_t* lpGUID)
{
    GUID targetGUID;
    if (!GetGUIDFromString(lpGUID, targetGUID))
        return NULL;

    return GetExceptionDevice(targetGUID);
}

inline bool PinHasMajorType(IPin* pin, const GUID* majorType)
{
    HRESULT hRes;

    IAMStreamConfig* config;
    if (SUCCEEDED(pin->QueryInterface(IID_IAMStreamConfig, (void**)&config)))
    {
        int count, size;
        if (SUCCEEDED(config->GetNumberOfCapabilities(&count, &size)))
        {
            BYTE* capsData = (BYTE*)malloc(size);

            int priority = -1;
            for (int i = 0; i < count; i++)
            {
                AM_MEDIA_TYPE* pMT = nullptr;
                if (SUCCEEDED(config->GetStreamCaps(i, &pMT, capsData)))
                {
                    BOOL bDesiredMediaType = (pMT->majortype == *majorType);

                    FreeMediaType(*pMT);
                    CoTaskMemFree(pMT);

                    if (bDesiredMediaType)
                    {
                        free(capsData);
                        SafeRelease(config);

                        return true;
                    }
                }
            }

            free(capsData);
        }

        SafeRelease(config);
    }

    AM_MEDIA_TYPE* pinMediaType;

    IEnumMediaTypes* mediaTypesEnum;
    if (FAILED(pin->EnumMediaTypes(&mediaTypesEnum)))
        return false;

    ULONG curVal = 0;
    hRes = mediaTypesEnum->Next(1, &pinMediaType, &curVal);

    mediaTypesEnum->Release();

    if (hRes != S_OK)
        return false;

    BOOL bDesiredMediaType = (pinMediaType->majortype == *majorType);
    DeleteMediaType(pinMediaType);

    if (!bDesiredMediaType)
        return false;

    return true;
}

inline IPin* GetOutputPin(IBaseFilter* filter, const GUID* majorType)
{
    IPin* foundPin = NULL;
    IEnumPins* pins;

    if (!filter) return NULL;
    if (FAILED(filter->EnumPins(&pins))) return NULL;

    IPin* curPin;
    ULONG num;
    while (pins->Next(1, &curPin, &num) == S_OK)
    {
        if (majorType)
        {
            if (!PinHasMajorType(curPin, majorType))
            {
                SafeRelease(curPin);
                continue;
            }
        }

        //------------------------------

        PIN_DIRECTION pinDir;
        if (SUCCEEDED(curPin->QueryDirection(&pinDir)))
        {
            if (pinDir == PINDIR_OUTPUT)
            {
                IKsPropertySet* propertySet;
                if (SUCCEEDED(curPin->QueryInterface(IID_IKsPropertySet, (void**)&propertySet)))
                {
                    GUID pinCategory;
                    DWORD retSize;

                    PIN_INFO chi;
                    curPin->QueryPinInfo(&chi);

                    if (chi.pFilter)
                        chi.pFilter->Release();

                    if (SUCCEEDED(propertySet->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &pinCategory, sizeof(GUID), &retSize)))
                    {
                        const GUID PIN_CATEGORY_ROXIOCAPTURE = { 0x6994AD05, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96} };
                        if (pinCategory == PIN_CATEGORY_CAPTURE || pinCategory == PIN_CATEGORY_ROXIOCAPTURE)
                        {
                            SafeRelease(propertySet);
                            SafeRelease(pins);

                            return curPin;
                        }
                    }

                    SafeRelease(propertySet);
                }
            }
        }

        SafeRelease(curPin);
    }

    SafeRelease(pins);

    return foundPin;
}

inline IBaseFilter* GetDeviceByValue(const IID& enumType, wchar_t* lpType, const wchar_t* lpName, wchar_t* lpType2, const wchar_t* lpName2)
{
    if (_wcsicmp(lpType2, L"DevicePath") == 0 && lpName2 && *lpName2 == '{')
        return GetExceptionDevice(lpName2);

    ICreateDevEnum* deviceEnum;
    IEnumMoniker* videoDeviceEnum;

    HRESULT err;
    err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
    if (FAILED(err))
    {
        CY_LOG_ERROR(TEXT("GetDeviceByValue: CoCreateInstance for the device enum failed, result = %08lX"), err);
        return NULL;
    }

    err = deviceEnum->CreateClassEnumerator(enumType, &videoDeviceEnum, 0);
    if (FAILED(err))
    {
        CY_LOG_ERROR(TEXT("GetDeviceByValue: deviceEnum->CreateClassEnumerator failed, result = %08lX"), err);
        deviceEnum->Release();
        return NULL;
    }

    SafeRelease(deviceEnum);

    if (err == S_FALSE) //no devices, so NO ENUM FO U
        return NULL;

    //---------------------------------

    IBaseFilter* bestFilter = NULL;

    IMoniker* deviceInfo;
    DWORD count;
    while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
    {
        IPropertyBag* propertyData;
        err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
        if (SUCCEEDED(err))
        {
            VARIANT valueThingy;
            VARIANT valueThingy2;
            VariantInit(&valueThingy);
            VariantInit(&valueThingy2);
            /*valueThingy.vt  = VT_BSTR;
            valueThingy.pbstrVal = NULL;

            valueThingy2.vt = VT_BSTR;
            valueThingy2.bstrVal = NULL;*/

            if (SUCCEEDED(propertyData->Read(lpType, &valueThingy, NULL)))
            {
                if (lpType2 && lpName2)
                {
                    if (FAILED(propertyData->Read(lpType2, &valueThingy2, NULL)))
                        nop();
                }

                SafeRelease(propertyData);

                const wchar_t* strVal1 = (const wchar_t*)valueThingy.bstrVal;

                //if(strVal1  == lpName)
                if (wcsstr(strVal1, lpName) != NULL)
                {
                    IBaseFilter* filter;
                    err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
                    if (FAILED(err))
                    {
                        CY_LOG_ERROR(TEXT("GetDeviceByValue: deviceInfo->BindToObject failed, result = %08lX"), err);
                        continue;
                    }

                    if (!bestFilter)
                    {
                        bestFilter = filter;

                        if (!lpType2 || !lpName2)
                        {
                            SafeRelease(deviceInfo);
                            SafeRelease(videoDeviceEnum);

                            return bestFilter;
                        }
                    }
                    else if (lpType2 && lpName2)
                    {
                        const wchar_t* strVal2 = (const wchar_t*)valueThingy2.bstrVal;
                        if (strVal2 == lpName2)
                        {
                            bestFilter->Release();

                            bestFilter = filter;

                            SafeRelease(deviceInfo);
                            SafeRelease(videoDeviceEnum);

                            return bestFilter;
                        }
                    }
                    else
                        filter->Release();
                }
            }
        }

        SafeRelease(deviceInfo);
    }

    SafeRelease(videoDeviceEnum);

    return bestFilter;
}

inline ECYVideoOutputType GetVideoOutputType(const AM_MEDIA_TYPE& media_type)
{
    ECYVideoOutputType type = TYPE_VIDEO_OUTPUT_NONE;

    if (media_type.majortype == MEDIATYPE_Video)
    {
        // Packed RGB formats
        if (media_type.subtype == MEDIASUBTYPE_RGB24)
            type = TYPE_VIDEO_OUTPUT_RGB24;
        else if (media_type.subtype == MEDIASUBTYPE_RGB32)
            type = TYPE_VIDEO_OUTPUT_RGB32;
        else if (media_type.subtype == MEDIASUBTYPE_ARGB32)
            type = TYPE_VIDEO_OUTPUT_ARGB32;
        else if (media_type.subtype == MEDIASUBTYPE_RGB565)
            type = TYPE_VIDEO_OUTPUT_RGB565;

        // Planar YUV formats
        else if (media_type.subtype == MEDIASUBTYPE_I420)
            type = TYPE_VIDEO_OUTPUT_I420;
        else if (media_type.subtype == MEDIASUBTYPE_IYUV)
            type = TYPE_VIDEO_OUTPUT_I420;
        else if (media_type.subtype == MEDIASUBTYPE_YV12)
            type = TYPE_VIDEO_OUTPUT_YV12;

        else if (media_type.subtype == MEDIASUBTYPE_Y41P)
            type = TYPE_VIDEO_OUTPUT_Y41P;
        else if (media_type.subtype == MEDIASUBTYPE_YVU9)
            type = TYPE_VIDEO_OUTPUT_YVU9;

        // Packed YUV formats
        else if (media_type.subtype == MEDIASUBTYPE_YVYU)
            type = TYPE_VIDEO_OUTPUT_YVYU;
        else if (media_type.subtype == MEDIASUBTYPE_YUY2)
            type = TYPE_VIDEO_OUTPUT_YUY2;
        else if (media_type.subtype == MEDIASUBTYPE_UYVY)
            type = TYPE_VIDEO_OUTPUT_UYVY;

        else if (media_type.subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            type = TYPE_VIDEO_OUTPUT_MPEG2_VIDEO;

        else if (media_type.subtype == MEDIASUBTYPE_H264)
            type = TYPE_VIDEO_OUTPUT_H264;

        else if (media_type.subtype == MEDIASUBTYPE_dvsl)
            type = TYPE_VIDEO_OUTPUT_DVSL;
        else if (media_type.subtype == MEDIASUBTYPE_dvsd)
            type = TYPE_VIDEO_OUTPUT_DVSD;
        else if (media_type.subtype == MEDIASUBTYPE_dvhd)
            type = TYPE_VIDEO_OUTPUT_DVHD;

        else if (media_type.subtype == MEDIASUBTYPE_MJPG)
            type = TYPE_VIDEO_OUTPUT_MJPG;

        else
            nop();
    }

    return type;
}

inline BITMAPINFOHEADER* GetVideoBMIHeader(const AM_MEDIA_TYPE* pMT)
{
    return (pMT->formattype == FORMAT_VideoInfo) ?
        &reinterpret_cast<VIDEOINFOHEADER*>(pMT->pbFormat)->bmiHeader :
        &reinterpret_cast<VIDEOINFOHEADER2*>(pMT->pbFormat)->bmiHeader;
}

inline ECYVideoOutputType GetVideoOutputTypeFromFourCC(DWORD fourCC)
{
    ECYVideoOutputType type = TYPE_VIDEO_OUTPUT_NONE;

    // Packed RGB formats
    if (fourCC == '2BGR')
        type = TYPE_VIDEO_OUTPUT_RGB32;
    else if (fourCC == '4BGR')
        type = TYPE_VIDEO_OUTPUT_RGB24;
    else if (fourCC == 'ABGR')
        type = TYPE_VIDEO_OUTPUT_ARGB32;

    // Planar YUV formats
    else if (fourCC == '024I' || fourCC == 'VUYI')
        type = TYPE_VIDEO_OUTPUT_I420;
    else if (fourCC == '21VY')
        type = TYPE_VIDEO_OUTPUT_YV12;

    // Packed YUV formats
    else if (fourCC == 'UYVY')
        type = TYPE_VIDEO_OUTPUT_YVYU;
    else if (fourCC == '2YUY')
        type = TYPE_VIDEO_OUTPUT_YUY2;
    else if (fourCC == 'YVYU')
        type = TYPE_VIDEO_OUTPUT_UYVY;
    else if (fourCC == 'CYDH')
        type = TYPE_VIDEO_OUTPUT_HDYC;

    else if (fourCC == 'V4PM' || fourCC == '2S4M')
        type = TYPE_VIDEO_OUTPUT_MPEG2_VIDEO;

    else if (fourCC == '462H')
        type = TYPE_VIDEO_OUTPUT_H264;

    else if (fourCC == 'GPJM')
        type = TYPE_VIDEO_OUTPUT_MJPG;

    return type;
}

inline HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE* pmtTarget, const AM_MEDIA_TYPE* pmtSource)
{
    if (!pmtSource || !pmtTarget) return S_FALSE;

    *pmtTarget = *pmtSource;

    if (pmtSource->cbFormat && pmtSource->pbFormat)
    {
        pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc(pmtSource->cbFormat);
        if (pmtTarget->pbFormat == NULL)
        {
            pmtTarget->cbFormat = 0;
            return E_OUTOFMEMORY;
        }
        else
            memcpy(pmtTarget->pbFormat, pmtSource->pbFormat, pmtTarget->cbFormat);
    }

    if (pmtTarget->pUnk != NULL)
        pmtTarget->pUnk->AddRef();

    return S_OK;
}

#define ELGATO_WORKAROUND 1

#if ELGATO_WORKAROUND
inline void AddElgatoRes(AM_MEDIA_TYPE* pMT, int cx, int cy, ECYVideoOutputType type, std::vector<SharePtr<MediaOutputInfo>>& outputInfoList)
{
    SharePtr<MediaOutputInfo> ptrOutputInfo = SharePtr<MediaOutputInfo>(new MediaOutputInfo());
    BITMAPINFOHEADER* bmiHeader = GetVideoBMIHeader(pMT);

    ptrOutputInfo->minCX = ptrOutputInfo->maxCX = cx;
    ptrOutputInfo->minCY = ptrOutputInfo->maxCY = cy;
    ptrOutputInfo->minFrameInterval = ptrOutputInfo->maxFrameInterval = 10010000000LL / 60000LL;

    ptrOutputInfo->xGranularity = ptrOutputInfo->yGranularity = 1;

    ptrOutputInfo->mediaType = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    memset(ptrOutputInfo->mediaType, 0, sizeof(AM_MEDIA_TYPE));
    CopyMediaType(ptrOutputInfo->mediaType, pMT);

    ptrOutputInfo->videoType = type;
    ptrOutputInfo->bUsingFourCC = false;

    outputInfoList.push_back(ptrOutputInfo);
}
#endif

inline void AddOutput(AM_MEDIA_TYPE* pMT, BYTE* capsData, bool bAllowV2, std::vector<SharePtr<MediaOutputInfo>>& outputInfoList)
{
    ECYVideoOutputType type = GetVideoOutputType(*pMT);

    if (pMT->formattype == FORMAT_VideoInfo || (bAllowV2 && pMT->formattype == FORMAT_VideoInfo2))
    {
        VIDEO_STREAM_CONFIG_CAPS* pVSCC = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(capsData);
        VIDEOINFOHEADER* pVih = reinterpret_cast<VIDEOINFOHEADER*>(pMT->pbFormat);
        BITMAPINFOHEADER* bmiHeader = GetVideoBMIHeader(pMT);

        bool bUsingFourCC = false;
        if (type == TYPE_VIDEO_OUTPUT_NONE)
        {
            type = GetVideoOutputTypeFromFourCC(bmiHeader->biCompression);
            bUsingFourCC = true;
        }

        if (type != TYPE_VIDEO_OUTPUT_NONE)
        {
#if ELGATO_WORKAROUND // FMB NOTE 18-Feb-14: Not necessary anymore since Elgato Game Capture v.2.20 which implements IAMStreamConfig
            if (!pVSCC && bAllowV2)
            {
                AddElgatoRes(pMT, 480, 360, type, outputInfoList);
                AddElgatoRes(pMT, 640, 480, type, outputInfoList);
                AddElgatoRes(pMT, 1280, 720, type, outputInfoList);
                AddElgatoRes(pMT, 1920, 1080, type, outputInfoList);
                DeleteMediaType(pMT);
                return;
            }
#endif

            SharePtr<MediaOutputInfo> ptrOutputInfo = MakeShared<MediaOutputInfo>();

            if (pVSCC)
            {
                ptrOutputInfo->minFrameInterval = pVSCC->MinFrameInterval;
                ptrOutputInfo->maxFrameInterval = pVSCC->MaxFrameInterval;
                ptrOutputInfo->minCX = pVSCC->MinOutputSize.cx;
                ptrOutputInfo->maxCX = pVSCC->MaxOutputSize.cx;
                ptrOutputInfo->minCY = pVSCC->MinOutputSize.cy;
                ptrOutputInfo->maxCY = pVSCC->MaxOutputSize.cy;

                if (!ptrOutputInfo->minCX || !ptrOutputInfo->minCY || !ptrOutputInfo->maxCX || !ptrOutputInfo->maxCY)
                {
                    ptrOutputInfo->minCX = ptrOutputInfo->maxCX = bmiHeader->biWidth;
                    ptrOutputInfo->minCY = ptrOutputInfo->maxCY = bmiHeader->biHeight;
                }

                //actually due to the other code in GetResolutionFPSInfo, we can have this granularity
                // back to the way it was.  now, even if it's corrupted, it will always work
                ptrOutputInfo->xGranularity = max(pVSCC->OutputGranularityX, 1);
                ptrOutputInfo->yGranularity = max(pVSCC->OutputGranularityY, 1);
            }
            else
            {
                ptrOutputInfo->minCX = ptrOutputInfo->maxCX = bmiHeader->biWidth;
                ptrOutputInfo->minCY = ptrOutputInfo->maxCY = bmiHeader->biHeight;
                if (pVih->AvgTimePerFrame != 0)
                    ptrOutputInfo->minFrameInterval = ptrOutputInfo->maxFrameInterval = pVih->AvgTimePerFrame;
                else
                    ptrOutputInfo->minFrameInterval = ptrOutputInfo->maxFrameInterval = 10000000 / 30; //elgato hack // FMB NOTE 18-Feb-14: Not necessary anymore since Elgato Game Capture v.2.20 which implements IAMStreamConfig

                ptrOutputInfo->xGranularity = ptrOutputInfo->yGranularity = 1;
            }

            ptrOutputInfo->mediaType = pMT;
            ptrOutputInfo->videoType = type;
            ptrOutputInfo->bUsingFourCC = bUsingFourCC;

            outputInfoList.push_back(ptrOutputInfo);

            return;
        }
    }

    DeleteMediaType(pMT);
}

inline void GetOutputList(SafeReleasePtr<IPin>& ptrCurPin, std::vector<SharePtr<MediaOutputInfo>>& outputInfoList)
{
    HRESULT hRes;

    IAMStreamConfig* config;
    if (SUCCEEDED(ptrCurPin->QueryInterface(IID_IAMStreamConfig, (void**)&config)))
    {
        int count, size;
        if (SUCCEEDED(hRes = config->GetNumberOfCapabilities(&count, &size)))
        {
            BYTE* capsData = (BYTE*)malloc(size);

            int priority = -1;
            for (int i = 0; i < count; i++)
            {
                AM_MEDIA_TYPE* pMT = nullptr;
                if (SUCCEEDED(config->GetStreamCaps(i, &pMT, capsData)))
                    AddOutput(pMT, capsData, false, outputInfoList);
            }

            free(capsData);
        }
        else if (hRes == E_NOTIMPL) //...usually elgato.
        {
            IEnumMediaTypes* mediaTypes;
            if (SUCCEEDED(ptrCurPin->EnumMediaTypes(&mediaTypes)))
            {
                ULONG i;

                AM_MEDIA_TYPE* pMT;
                if (mediaTypes->Next(1, &pMT, &i) == S_OK)
                    AddOutput(pMT, NULL, true, outputInfoList);

                mediaTypes->Release();
            }
        }

        SafeRelease(config);
    }
}

inline UINT64 GetFrameIntervalDist(UINT64 minInterval, UINT64 maxInterval, UINT64 desiredInterval)
{
    INT64 minDist = INT64(minInterval) - INT64(desiredInterval);
    INT64 maxDist = INT64(desiredInterval) - INT64(maxInterval);

    if (minDist < 0) minDist = 0;
    if (maxDist < 0) maxDist = 0;

    return UINT64(MAX(minDist, maxDist));
}

inline bool GetClosestResolutionFPS(std::vector<SharePtr<MediaOutputInfo>>& outputList, SIZE& resolution, UINT64& frameInterval, bool bPrioritizeFPS, int nWidth, int nHeight, int nFPS)
{
    UINT64 internalFrameInterval = 10000000 / nFPS;
    LONG width = nWidth;
    LONG height = nHeight;

    LONG bestDistance = 0x7FFFFFFF;
    SIZE bestSize;
    UINT64 maxFrameInterval = 0;
    UINT64 minFrameInterval = 0;
    UINT64 bestFrameIntervalDist = 0xFFFFFFFFFFFFFFFFLL;

    for (UINT i = 0; i < outputList.size(); i++)
    {
        SharePtr<MediaOutputInfo>& ptrOutputInfo = outputList[i];

        LONG outputWidth = ptrOutputInfo->minCX;
        do
        {
            LONG distWidth = width - outputWidth;
            if (distWidth < 0)
                break;

            if (distWidth > bestDistance)
            {
                outputWidth += ptrOutputInfo->xGranularity;
                continue;
            }

            LONG outputHeight = ptrOutputInfo->minCY;
            do
            {
                LONG distHeight = height - outputHeight;
                if (distHeight < 0)
                    break;

                LONG totalDist = distHeight + distWidth;

                UINT64 frameIntervalDist = GetFrameIntervalDist(ptrOutputInfo->minFrameInterval,
                    ptrOutputInfo->maxFrameInterval, internalFrameInterval);

                bool bBetter;
                if (bPrioritizeFPS)
                    bBetter = (frameIntervalDist != bestFrameIntervalDist) ?
                    (frameIntervalDist < bestFrameIntervalDist) :
                    (totalDist < bestDistance);
                else
                    bBetter = (totalDist != bestDistance) ?
                    (totalDist < bestDistance) :
                    (frameIntervalDist < bestFrameIntervalDist);

                if (bBetter)
                {
                    bestDistance = totalDist;
                    bestSize.cx = outputWidth;
                    bestSize.cy = outputHeight;
                    maxFrameInterval = ptrOutputInfo->maxFrameInterval;
                    minFrameInterval = ptrOutputInfo->minFrameInterval;
                    bestFrameIntervalDist = frameIntervalDist;
                }

                outputHeight += ptrOutputInfo->yGranularity;
            } while ((UINT)outputHeight <= ptrOutputInfo->maxCY);

            outputWidth += ptrOutputInfo->xGranularity;
        } while ((UINT)outputWidth <= ptrOutputInfo->maxCX);
    }

    if (bestDistance != 0x7FFFFFFF)
    {
        resolution.cx = bestSize.cx;
        resolution.cy = bestSize.cy;

        if (internalFrameInterval > maxFrameInterval)
            frameInterval = maxFrameInterval;
        else if (internalFrameInterval < minFrameInterval)
            frameInterval = minFrameInterval;
        else
            frameInterval = internalFrameInterval;
        return true;
    }

    return false;
}

inline SharePtr<MediaOutputInfo> GetBestMediaOutput(std::vector<SharePtr<MediaOutputInfo>>& outputList, UINT width, UINT height, UINT preferredType, UINT64& frameInterval)
{
    SharePtr<MediaOutputInfo> ptrBestMediaOutput;
    int bestPriority = -1;
    UINT64 closestIntervalDifference = 0xFFFFFFFFFFFFFFFFLL;
    UINT64 bestFrameInterval = 0;

    bool bUsePreferredType = preferredType != -1;

    for (UINT i = 0; i < outputList.size(); i++)
    {
        SharePtr<MediaOutputInfo>& ptrOutputInfo= outputList[i];
        //VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(ptrOutputInfo->mediaType->pbFormat);

        if (ptrOutputInfo->minCX <= width && ptrOutputInfo->maxCX >= width &&
            ptrOutputInfo->minCY <= height && ptrOutputInfo->maxCY >= height)
        {
            int priority = inputPriority[(UINT)ptrOutputInfo->videoType];
            if (priority == -1)
                continue;

            UINT64 curInterval;
            if (frameInterval > ptrOutputInfo->maxFrameInterval)
                curInterval = ptrOutputInfo->maxFrameInterval;
            else if (frameInterval < ptrOutputInfo->minFrameInterval)
                curInterval = ptrOutputInfo->minFrameInterval;
            else
                curInterval = frameInterval;

            UINT64 intervalDifference = (UINT64)_abs64(INT64(curInterval) - INT64(frameInterval));

            if (intervalDifference > closestIntervalDifference)
                continue;

            bool better;
            if (!bUsePreferredType)
                better = priority > bestPriority || !ptrBestMediaOutput || intervalDifference < closestIntervalDifference;
            else
                better = (UINT)ptrOutputInfo->videoType == preferredType && intervalDifference <= closestIntervalDifference;

            if (better)
            {
                closestIntervalDifference = intervalDifference;
                bestFrameInterval = curInterval;
                ptrBestMediaOutput = ptrOutputInfo;
                bestPriority = priority;
            }
        }
    }

    frameInterval = bestFrameInterval;
    return ptrBestMediaOutput;
}

CYDEVICE_NAMESPACE_END

#endif // __DSHOW_COMMON_DEFINE_HPP__