#include "FFmpegPublicUtils.h"
#include <qDebug>
#include <string>
#include <unordered_map>

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

int FFmpegPublicUtils::GetAudioFrameSize(const AVFrame* frame)
{
    if (!frame)
    {
        qWarning() << "GetAudioFrameSize() : frame is nullptr";
        return 0;
    }

    return av_samples_get_buffer_size(nullptr, frame->ch_layout.nb_channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 1);
}

ST_AudioDecodeResult FFmpegPublicUtils::DecodeAudioPacket(const AVPacket* packet, AVCodecContext* codecCtx)
{
    ST_AudioDecodeResult result;
    if (!packet || !codecCtx)
    {
        qWarning() << "DecodeAudioPacket() : Invalid parameters";
        return result;
    }

    // 发送数据包到解码器
    int ret = avcodec_send_packet(codecCtx, packet);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "Error sending packet to decoder:" << errbuf;
        return result;
    }

    // 分配音频帧
    AVFrame* frame = av_frame_alloc();
    if (!frame)
    {
        qWarning() << "Failed to allocate frame";
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
            qWarning() << "Error receiving frame from decoder:" << errbuf;
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

        // 将音频数据复制到结果中
        size_t currentSize = result.audioData.size();
        result.audioData.resize(currentSize + frameSize);

        // 对于平面格式（FLTP等），需要交错复制数据
        if (av_sample_fmt_is_planar(result.format))
        {
            int bytesPerSample = av_get_bytes_per_sample(result.format);
            uint8_t* dst = result.audioData.data() + currentSize;

            for (int s = 0; s < frame->nb_samples; s++)
            {
                for (int ch = 0; ch < result.channels; ch++)
                {
                    memcpy(dst, frame->data[ch] + s * bytesPerSample, bytesPerSample);
                    dst += bytesPerSample;
                }
            }
        }
        else
        {
            // 对于已经交错的格式，直接复制
            memcpy(result.audioData.data() + currentSize, frame->data[0], frameSize);
        }
    }

    // 释放资源
    av_frame_free(&frame);
    return result;
}
