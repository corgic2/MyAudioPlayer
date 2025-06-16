#pragma once

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
#include "libavcodec/codec_par.h"
}

/// <summary>
/// 音频编解码器上下文封装类
/// </summary>
class ST_AVCodecContext
{
  public:
    ST_AVCodecContext() = default;
    explicit ST_AVCodecContext(const AVCodec *codec);
    ~ST_AVCodecContext();

    /// <summary>
    /// 绑定参数到上下文
    /// </summary>
    int BindParamToContext(const AVCodecParameters *parameters);

    /// <summary>
    /// 打开编解码器
    /// </summary>
    int OpenCodec(const AVCodec *codec, AVDictionary **options = nullptr);

    /// <summary>
    /// 获取原始编解码器上下文指针
    /// </summary>
    AVCodecContext *GetRawContext() const
    {
        return m_pCodecContext;
    }

  private:
    AVCodecContext *m_pCodecContext = nullptr;
};
