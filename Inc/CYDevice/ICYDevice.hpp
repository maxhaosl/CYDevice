/*
* CYDevice License
* -----------
*
* CYDevice is licensed under the terms of the MIT license reproduced below.
* This means that CYDevice is free software and can be used for both academic
* and commercial purposes at absolutely no cost.
*
*
* ===============================================================================
*
* Copyright (C) 2023-2024 ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
* ===============================================================================
*/
/*
* AUTHORS:  ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
* VERSION:  1.0.0
* PURPOSE:  A cross platform audio and video collection library.
* CREATION: 2025.04.08
* LCHANGE:  2025.04.08
* LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
*/

#ifndef __I_CYDEVICE_HPP__
#define __I_CYDEVICE_HPP__

#include <stdint.h>
#include "CYDevice/CYDeviceDefine.hpp"

CYDEVICE_NAMESPACE_BEGIN

class CYDEVICE_API ICYDevice
{
public:
    ICYDevice()
    {
    }
    virtual ~ICYDevice()
    {
    }

public:
    /**
     * @brief Initialization and de-initialization of cry device.
    */
    virtual int16_t Init(int nWidth/* = 1024*/, int nHeight/* = 768*/, int nFPS/* = 25*/, const wchar_t* pszDeviceName, const wchar_t* pszDeviceId, int nSampleRateHz, const wchar_t* pszAudioName, const wchar_t* pszAudioID, bool bUseRender = true) = 0;
    virtual int16_t UnInit() = 0;

    /**
     * @brief Start Capture.
    */
    virtual int16_t StartCapture(ICYAudioDataCallBack* pAudioDataCallBack = nullptr, ICYVideoDataCallBack* pVideoDataCallBack = nullptr) = 0;
    virtual int16_t StopCapture() = 0;

    /**
     * @brief Get Audio Data.
    */
    virtual int16_t GetNextAudioBuffer(float*& pBuffer, uint32_t& nNumFrames, uint64_t& nTimestamp) = 0;
};

CYDEVICE_NAMESPACE_END

#endif //__I_CYDEVICE_HPP__