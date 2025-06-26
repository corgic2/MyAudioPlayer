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
    bool BindParamToContext(const AVCodecParameters* parameters);

    /// <summary>
    /// 打开编解码器
    /// </summary>
    bool OpenCodec(const AVCodec* codec, AVDictionary** options = nullptr);
    /// <summary>
    /// 获取剩余数据
    /// </summary>
    void FlushBuffer();

    /// <summary>
    /// 获取原始编解码器上下文指针
    /// </summary>
    AVCodecContext *GetRawContext() const
    {
        return m_pCodecContext;
    }
    bool CopyParamtersFromContext(AVCodecParameters* parameters);
  private:
    AVCodecContext *m_pCodecContext = nullptr;
};
