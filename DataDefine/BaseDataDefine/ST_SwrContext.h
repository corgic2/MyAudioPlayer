#pragma once

extern "C" {
#include "libswresample/swresample.h"
}

/// <summary>
/// 音频重采样上下文封装类
/// </summary>
class ST_SwrContext
{
  public:
    ST_SwrContext() = default;
    ~ST_SwrContext();

    /// <summary>
    /// 初始化重采样上下文
    /// </summary>
    int SwrContextInit();

    void SetRawContext(SwrContext *p)
    {
        m_swrCtx = p;
    }
    /// <summary>
    /// 获取原始重采样上下文指针
    /// </summary>
    SwrContext *GetRawContext() const
    {
        return m_swrCtx;
    }

  private:
    SwrContext *m_swrCtx = nullptr;
};
