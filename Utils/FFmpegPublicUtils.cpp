#include "FFmpegPublicUtils.h"

#include <qDebug>

// 根据输出格式自动选择编码器
const AVCodec* FFmpegPublicUtils::FindEncoder(const char* formatName)
{
    if (!formatName)
    {
        qWarning() << "FindEncoder() : formatName is nullptr";
        return nullptr;
    }
    if (strcmp(formatName, "wav") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    }
    else if (strcmp(formatName, "mp3") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_MP3);
    }
    // 其他格式处理...
    return nullptr;
}

void FFmpegPublicUtils::ConfigureEncoderParams(AVCodecParameters* codecPar, AVCodecContext* encCtx)
{
    if (!codecPar || !encCtx)
    {
        qWarning() << "ConfigureEncoderParams() :: codecPar is nullptr and encCtx is nullptr";
        return;
    }

    switch (codecPar->codec_id)
    {
        case AV_CODEC_ID_PCM_S16LE: // WAV/PCM
            codecPar->format = AV_SAMPLE_FMT_S16;
            codecPar->sample_rate = 44100;
            codecPar->bit_rate = 1411200; // 44100Hz * 16bit * 2channels
            break;
        case AV_CODEC_ID_MP3: // MP3
            encCtx->bit_rate = 192000; // 典型比特率
            encCtx->sample_fmt = AV_SAMPLE_FMT_FLTP; // MP3编码器要求的输入格式
            break;
    }
}

SDL_AudioFormat FFmpegPublicUtils::FFmpegToSDLFormat(AVSampleFormat fmt)
{
    switch (fmt)
    {
        case AV_SAMPLE_FMT_U8:
            return SDL_AUDIO_U8;
        case AV_SAMPLE_FMT_S16:
            return SDL_AUDIO_S16;
        case AV_SAMPLE_FMT_S32:
            return SDL_AUDIO_S32;
        case AV_SAMPLE_FMT_FLT:
            return SDL_AUDIO_F32;
        case AV_SAMPLE_FMT_DBL:
            return SDL_AUDIO_S32LE;
        default:
            return SDL_AUDIO_S16; // 默认兼容格式
    }
}
