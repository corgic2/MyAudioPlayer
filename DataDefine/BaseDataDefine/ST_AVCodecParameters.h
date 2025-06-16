#pragma once

extern "C" {
#include "libavcodec/codec_par.h"
}

/// <summary>
/// 音频编解码器参数封装类
/// </summary>
class ST_AVCodecParameters
{
  public:
    explicit ST_AVCodecParameters(AVCodecParameters *obj);
    ~ST_AVCodecParameters();
    ST_AVCodecParameters() = default;
    AVCodecID GetCodecId() const;
    int GetSamplePerRate() const;
    AVSampleFormat GetSampleFormat() const;
    int GetBitPerSample() const;

    /// <summary>
    /// 获取原始参数指针
    /// </summary>
    AVCodecParameters *GetRawParameters() const
    {
        return m_codecParameters;
    }

  private:
    AVCodecParameters *m_codecParameters = nullptr;
};
