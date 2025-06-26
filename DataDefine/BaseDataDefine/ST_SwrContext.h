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
    /// <summary>
    /// 重采样转换
    /// </summary>
    /// <param name="out"></param>
    /// <param name="out_count"></param>
    /// <param name="in"></param>
    /// <param name="in_count"></param>
    /// <returns></returns>
    int SwrConvert(uint8_t* const* out, int out_count, const uint8_t* const* in, int in_count);
    /// <summary>
    /// 获取剩余数据
    /// </summary>
    /// <param name="sampleRate"></param>
    /// <returns></returns>
    int GetDelayData(int sampleRate);

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
