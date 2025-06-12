#pragma once

#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_par.h>
}

#include <SDL3/SDL_audio.h>

/// <summary>
/// FFmpeg公共工具类
/// </summary>
class FFmpegPublicUtils
{
public:
    FFmpegPublicUtils() = default;
    ~FFmpegPublicUtils() = default;

    /// <summary>
    /// 根据输出格式自动选择编码器
    /// </summary>
    /// <param name="formatName">格式名称</param>
    /// <returns>编码器指针</returns>
    static const AVCodec* FindEncoder(const char* formatName);

    /// <summary>
    /// 配置编码器参数
    /// </summary>
    /// <param name="codecPar">编解码器参数</param>
    /// <param name="encCtx">编码器上下文</param>
    static void ConfigureEncoderParams(AVCodecParameters* codecPar, AVCodecContext* encCtx);

    /// <summary>
    /// 将FFmpeg的AVSampleFormat转换为SDL_AudioFormat
    /// </summary>
    /// <param name="fmt">FFmpeg采样格式</param>
    /// <returns>SDL音频格式</returns>
    static SDL_AudioFormat FFmpegToSDLFormat(AVSampleFormat fmt);
};
