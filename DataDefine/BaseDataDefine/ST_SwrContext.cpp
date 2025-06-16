#pragma once
#include "ST_SwrContext.h"

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
