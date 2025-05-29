#pragma once
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
#include "libavcodec/codec_par.h"
}

#include "SDL3/SDL_audio.h"

class FFmpegPublicUtils
{
public:
    FFmpegPublicUtils() = default;
    ~FFmpegPublicUtils() = default;
    // 根据输出格式自动选择编码器
    static const AVCodec* FindEncoder(const char* formatName);
    // 根据输入格式自动选择解码器
    static void ConfigureEncoderParams(AVCodecParameters* codecPar, AVCodecContext* encCtx);
    // 将 FFmpeg 的 AVSampleFormat 转换为 SDL_AudioFormat
    static SDL_AudioFormat FFmpegToSDLFormat(AVSampleFormat fmt);


};
