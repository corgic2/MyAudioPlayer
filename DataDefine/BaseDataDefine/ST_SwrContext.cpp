#pragma once
#include "ST_SwrContext.h"

#include <vector>
#include "LogSystem/LogSystem.h"

// ST_SwrContext implementation
ST_SwrContext::~ST_SwrContext()
{
    if (m_swrCtx)
    {
        swr_free(&m_swrCtx);
    }
}

int ST_SwrContext::SwrContextInit()
{
    return m_swrCtx ? swr_init(m_swrCtx) : -1;
}

int ST_SwrContext::SwrConvert(uint8_t* const* out, int out_count,const uint8_t* const* in, int in_count)
{
    int realOutSamples = swr_convert(m_swrCtx, out, out_count, in, in_count);
    if (realOutSamples < 0)
    {
        char error_buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(realOutSamples, error_buffer, AV_ERROR_MAX_STRING_SIZE);
        LOG_ERROR("Resample conversion failed: " + std::string(error_buffer));
    }
    else if (realOutSamples == 0)
    {
        LOG_DEBUG("Resample() : No output samples generated");
    }
    return realOutSamples;
}

int ST_SwrContext::GetDelayData(int sampleRate)
{
    return swr_get_delay(m_swrCtx, sampleRate);
}