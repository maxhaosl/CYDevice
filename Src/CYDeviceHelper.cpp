#include "CYDevice/CYDeviceHelper.hpp"

#include <dshow.h>
#include <uuids.h>
#include <strmif.h>
#include <xstring>

#include <iostream>
#include <string>
#include <locale>
#include <cwchar>

#include <atlbase.h>

CYDEVICE_NAMESPACE_BEGIN

#define SafeRelease(var) if(var) {var->Release(); var = NULL;}

std::string WStringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return "";

    std::mbstate_t state{};
    const wchar_t* src = wstr.c_str();
    size_t len = wcsrtombs(nullptr, &src, 0, &state);

    if (len == static_cast<size_t>(-1)) return "";

    std::string result(len, 0);
    wcsrtombs(&result[0], &src, len, &state);
    return result;
}

int16_t GetDeviceList(ECYDeviceType eType, TDeviceInfo* pInfo, uint32_t* pCount)
{
    CComPtr<ICreateDevEnum> ptrDeviceEnum;
    CComPtr<IEnumMoniker> ptrVideoDeviceEnum;

    GUID objGUID;
    switch (eType)
    {
    case TYPE_CYDEVICE_VIDEO_INPUT:
        objGUID = CLSID_VideoInputDeviceCategory;
        break;
    case TYPE_CYDEVICE_AUDIO_INPUT:
        objGUID = CLSID_AudioInputDeviceCategory;
        break;
    case TYPE_CYDEVICE_AUDIO_RENDER:
        objGUID = CLSID_AudioRendererCategory;
        break;
    default:
        break;
    }

    *pCount = 0;

    HRESULT err;
    err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&ptrDeviceEnum);
    if (FAILED(err))
    {
        // TRACE(TEXT("FillOutListDevices: CoCreateInstance for the device enum failed, result = %08lX"), err);
        return false;
    }

    err = ptrDeviceEnum->CreateClassEnumerator(objGUID, &ptrVideoDeviceEnum, 0);
    if (FAILED(err))
    {
        //  TRACE(TEXT("FillOutListDevices: pDeviceEnum->CreateClassEnumerator failed, result = %08lX"), err);
        return false;
    }

    if (err == S_FALSE) //no devices
        return false;

    //------------------------------------------
    CComPtr<IMoniker> ptrDeviceInfo;
    DWORD dwCount = 0;

    while (ptrVideoDeviceEnum->Next(1, &ptrDeviceInfo, &dwCount) == S_OK)
    {
        CComPtr<IPropertyBag> ptrPropertyData;
        err = ptrDeviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&ptrPropertyData);
        if (SUCCEEDED(err))
        {
            VARIANT friendlyNameValue, devicePathValue;
            friendlyNameValue.vt = VT_BSTR;
            friendlyNameValue.bstrVal = NULL;
            devicePathValue.vt = VT_BSTR;
            devicePathValue.bstrVal = NULL;

            err = ptrPropertyData->Read(L"FriendlyName", &friendlyNameValue, NULL);
            ptrPropertyData->Read(L"DevicePath", &devicePathValue, NULL);

            if (SUCCEEDED(err))
            {
                IBaseFilter* filter;
                err = ptrDeviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
                if (SUCCEEDED(err))
                {
                    if (pInfo)
                    {
                        std::wstring strDeviceNameW = (WCHAR*)friendlyNameValue.bstrVal;
                        std::string strDeviceName = WStringToString(strDeviceNameW);
                        strcpy_s(pInfo[*pCount].szDeviceName, strDeviceName.c_str());

                        if (devicePathValue.bstrVal)
                        {
                            std::wstring strDeviceIDW = (WCHAR*)devicePathValue.bstrVal;
                            std::string strDeviceID = WStringToString(strDeviceIDW);
                            strcpy_s(pInfo[*pCount].szDeviceId, strDeviceID.c_str());
                        }
                        SafeRelease(filter);
                    }
                    *pCount++;
                }
            }
        }
    }
    return CYERR_SUCESS;
}

CYDEVICE_NAMESPACE_END