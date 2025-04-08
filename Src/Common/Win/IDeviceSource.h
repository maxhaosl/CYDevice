#pragma once

#include <strmif.h>

class IDeviceSource
{
public:
    virtual void FlushSamples() = 0;
    virtual void ReceiveMediaSample(IMediaSample* pSample, bool bAudio) = 0;
};