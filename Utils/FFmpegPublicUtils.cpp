#include "FFmpegPublicUtils.h"
#include <qDebug>
#include <string>
#include <unordered_map>
#include "LogSystem/LogSystem.h"

const AVCodec* FFmpegPublicUtils::FindEncoder(const char* formatName)
{
    if (!formatName)
    {
        LOG_WARN("FindEncoder() : formatName is nullptr");
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
        LOG_WARN("ConfigureEncoderParams() :: codecPar is nullptr and encCtx is nullptr");
        return;
    }

    switch (codecPar->codec_id)
    {
        case AV_CODEC_ID_PCM_S16LE: // WAV/PCM
            codecPar->format = AV_SAMPLE_FMT_S16;
            codecPar->sample_rate = 44100;
            codecPar->bit_rate = 1411200; // 44100Hz * 16bit * 2channels
            break;
        case AV_CODEC_ID_MP3:                        // MP3
            encCtx->bit_rate = 192000;               // 典型比特率
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
        case AV_SAMPLE_FMT_S16P:
            return SDL_AUDIO_S16;
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
            return SDL_AUDIO_S32;
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            return SDL_AUDIO_F32;
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
            return SDL_AUDIO_F32; // 转换为float32，避免精度损失
        default:
            return SDL_AUDIO_S16; // 默认使用S16格式，确保最大兼容性
    }
}

int FFmpegPublicUtils::GetAudioFrameSize(const AVFrame* frame)
{
    if (!frame)
    {
        LOG_WARN("GetAudioFrameSize() : frame is nullptr");
        return 0;
    }

    return av_samples_get_buffer_size(nullptr, frame->ch_layout.nb_channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 1);
}

ST_AudioDecodeResult FFmpegPublicUtils::DecodeAudioPacket(const AVPacket* packet, AVCodecContext* codecCtx)
{
    ST_AudioDecodeResult result;
    if (!packet || !codecCtx)
    {
        LOG_WARN("DecodeAudioPacket() : Invalid parameters");
        return result;
    }

    // 发送数据包到解码器
    int ret = avcodec_send_packet(codecCtx, packet);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Error sending packet to decoder:" + std::string(errbuf));
        return result;
    }

    // 分配音频帧
    AVFrame* frame = av_frame_alloc();
    if (!frame)
    {
        LOG_WARN("Failed to allocate frame");
        return result;
    }

    // 接收解码后的音频帧
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(codecCtx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }
        else if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Error receiving frame from decoder:" + std::string(errbuf));
            break;
        }

        // 获取音频帧大小
        int frameSize = GetAudioFrameSize(frame);
        if (frameSize <= 0)
        {
            continue;
        }

        // 更新解码结果信息
        result.samplesCount = frame->nb_samples;
        result.channels = frame->ch_layout.nb_channels;
        result.sampleRate = frame->sample_rate;
        result.format = static_cast<AVSampleFormat>(frame->format);

        // 分配对齐的缓冲区
        size_t currentSize = result.audioData.size();
        size_t newSize = currentSize + frameSize;
        result.audioData.resize(newSize + AV_INPUT_BUFFER_PADDING_SIZE);

        // 对于平面格式（FLTP等），需要交错复制数据
        if (av_sample_fmt_is_planar(result.format))
        {
            int bytesPerSample = av_get_bytes_per_sample(result.format);
            uint8_t* dst = result.audioData.data() + currentSize;

            // 优化的平面格式到交错格式的转换
            for (int s = 0; s < frame->nb_samples; s++)
            {
                for (int ch = 0; ch < result.channels; ch++)
                {
                    const uint8_t* src = frame->data[ch] + s * bytesPerSample;
                    memcpy(dst, src, bytesPerSample);
                    dst += bytesPerSample;
                }
            }
        }
        else
        {
            // 对于已经交错的格式，直接复制
            memcpy(result.audioData.data() + currentSize, frame->data[0], frameSize);
        }

        // 清零padding区域
        memset(result.audioData.data() + newSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    }

    // 释放资源
    av_frame_free(&frame);
    return result;
}
