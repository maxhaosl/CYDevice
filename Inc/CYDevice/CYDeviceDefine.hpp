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

#ifndef __CYDEVICE_DEFINE_HPP__
#define __CYDEVICE_DEFINE_HPP__

#include <stdint.h>

#define CYDEVICE_NAMESPACE_BEGIN        namespace cry {
#define CYDEVICE_NAMESPACE              cry
#define CYDEVICE_NAMESPACE_END          }

#ifdef _WIN32
#ifdef CYDEVICE_USE_DLL
#ifdef CYDEVICE_EXPORTS
#define CYDEVICE_API __declspec(dllexport)
#else
#define CYDEVICE_API __declspec(dllimport)
#endif
#else
#define CYDEVICE_API
#endif
#else
#define CYDEVICE_API __attribute__ ((visibility ("default")))
#endif

CYDEVICE_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
enum ECYDeviceType
{
    TYPE_CYDEVICE_VIDEO_INPUT = 0x00,
    TYPE_CYDEVICE_AUDIO_INPUT = 0x01,
    TYPE_CYDEVICE_AUDIO_RENDER = 0x02,
};

enum ECYVideoType
{
    TYPE_CYVIDEO_I420 = 0x00,
};

enum ECYErrorCode
{
    CYERR_SUCESS = 0x00,         // 返回成功
    CYERR_FAILED = 0x01,         // 返回失败
    CYERR_CREATE_GRAPH_FAILED = 0x02,         // 创建Graph失败
    CYERR_CREATE_GRAPHBUILDER_FAILED = 0x03,         // 创建GraphBuilder2失败
    CYERR_REPEAT_START_CAPTURE = 0x04,         // 重复打开采集
    CYEER_NOT_START_CAPTURE = 0x05,         // 当前没有开启采集
    CYERR_CAPTURE_RUN_FAILED = 0x06,         // Capture Run Failed.
};

//////////////////////////////////////////////////////////////////////////
struct TDeviceInfo
{
    char szDeviceName[512];
    char szDeviceId[512];
};
//////////////////////////////////////////////////////////////////////////
class CYDEVICE_API ICYAudioDataCallBack
{
public:
    ICYAudioDataCallBack() {}
    virtual ~ICYAudioDataCallBack() {}

public:
    virtual void OnAudioData(float* pBuffer, uint32_t nNumberAudioFrames, uint32_t nChannel, uint64_t nTimeStamps) = 0;
};

class CYDEVICE_API ICYVideoDataCallBack
{
public:
    ICYVideoDataCallBack() {}
    virtual ~ICYVideoDataCallBack() {}

public:
    virtual void OnVideoData(const unsigned char* pData, int nLen, int nWidth, int nHeight, unsigned long long nTimeStampls) = 0;
};

CYDEVICE_NAMESPACE_END

#endif // __CYDEVICE_DEFINE_HPP__