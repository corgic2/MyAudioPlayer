#pragma once

extern "C" {
#include "libavcodec/codec.h"
}
/// <summary>
/// 音频编解码器封装类
/// </summary>
class ST_AVCodec
{
  public:
    ST_AVCodec() = default;
    explicit ST_AVCodec(enum AVCodecID id);
    explicit ST_AVCodec(const AVCodec *obj);
    ~ST_AVCodec() = default;

    /// <summary>
    /// 获取原始编解码器指针
    /// </summary>
    const AVCodec *GetRawCodec() const
    {
        return m_pAvCodec;
    }

  private:
    const AVCodec *m_pAvCodec = nullptr;
};
