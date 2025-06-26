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

    // 音频编码器
    if (strcmp(formatName, "wav") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    }
    else if (strcmp(formatName, "pcm") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    }
    else if (strcmp(formatName, "aac") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_AAC);
    }
    else if (strcmp(formatName, "mp3") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_MP3);
    }
    else if (strcmp(formatName, "flac") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_FLAC);
    }
    else if (strcmp(formatName, "opus") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_OPUS);
    }
    else if (strcmp(formatName, "ogg") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_VORBIS);
    }
    // 视频编码器
    else if (strcmp(formatName, "mp4") == 0 || strcmp(formatName, "h264") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    else if (strcmp(formatName, "h265") == 0 || strcmp(formatName, "hevc") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_H265);
    }
    else if (strcmp(formatName, "avi") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    }
    else if (strcmp(formatName, "flv") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_FLV1);
    }
    else if (strcmp(formatName, "webm") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_VP9);
    }

    return nullptr;
}

const AVCodec* FFmpegPublicUtils::FindDecoder(AVCodecID codecId)
{
    return avcodec_find_decoder(codecId);
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
        // 音频编码器配置
        case AV_CODEC_ID_PCM_S16LE: // WAV/PCM
            codecPar->format = AV_SAMPLE_FMT_S16;
            codecPar->sample_rate = 44100;
            codecPar->bit_rate = 1411200; // 44100Hz * 16bit * 2channels
            break;
        case AV_CODEC_ID_MP3:                        // MP3
            encCtx->bit_rate = 192000;               // 典型比特率
            encCtx->sample_fmt = AV_SAMPLE_FMT_FLTP; // MP3编码器要求的输入格式
            break;
        case AV_CODEC_ID_AAC: // AAC
            encCtx->bit_rate = 128000;
            encCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
            break;

        // 视频编码器配置
        case AV_CODEC_ID_H264:          // H.264
            encCtx->bit_rate = 2000000; // 2Mbps
            encCtx->pix_fmt = AV_PIX_FMT_YUV420P;
            encCtx->gop_size = 12;    // GOP大小
            encCtx->max_b_frames = 1; // B帧数量
            break;
        case AV_CODEC_ID_H265:          // H.265
            encCtx->bit_rate = 1500000; // 1.5Mbps
            encCtx->pix_fmt = AV_PIX_FMT_YUV420P;
            encCtx->gop_size = 12;
            encCtx->max_b_frames = 1;
            break;
        case AV_CODEC_ID_MPEG4: // MPEG-4
            encCtx->bit_rate = 2000000;
            encCtx->pix_fmt = AV_PIX_FMT_YUV420P;
            encCtx->gop_size = 12;
            encCtx->max_b_frames = 1;
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

SDL_PixelFormat FFmpegPublicUtils::FFmpegToSDLPixelFormat(AVPixelFormat fmt)
{
    switch (fmt)
    {
        case AV_PIX_FMT_RGB24:
            return SDL_PIXELFORMAT_RGB24;
        case AV_PIX_FMT_BGR24:
            return SDL_PIXELFORMAT_BGR24;
        case AV_PIX_FMT_RGBA:
            return SDL_PIXELFORMAT_RGBA8888;
        case AV_PIX_FMT_BGRA:
            return SDL_PIXELFORMAT_BGRA8888;
        case AV_PIX_FMT_ARGB:
            return SDL_PIXELFORMAT_ARGB8888;
        case AV_PIX_FMT_ABGR:
            return SDL_PIXELFORMAT_ABGR8888;
        case AV_PIX_FMT_RGB565:
            return SDL_PIXELFORMAT_RGB565;
        case AV_PIX_FMT_BGR565:
            return SDL_PIXELFORMAT_BGR565;
        case AV_PIX_FMT_YUV420P:
            return SDL_PIXELFORMAT_IYUV;
        default:
            return SDL_PIXELFORMAT_RGBA8888; // 默认使用RGBA格式
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

int FFmpegPublicUtils::GetVideoFrameSize(const AVFrame* frame)
{
    if (!frame)
    {
        LOG_WARN("GetVideoFrameSize() : frame is nullptr");
        return 0;
    }

    return av_image_get_buffer_size(static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);
}

bool FFmpegPublicUtils::GetVideoStreamInfo(AVFormatContext* formatCtx, int streamIndex, int& width, int& height, double& frameRate, double& duration)
{
    if (!formatCtx || streamIndex < 0 || streamIndex >= static_cast<int>(formatCtx->nb_streams))
    {
        LOG_WARN("GetVideoStreamInfo() : Invalid parameters");
        return false;
    }

    AVStream* stream = formatCtx->streams[streamIndex];
    AVCodecParameters* codecPar = stream->codecpar;

    if (codecPar->codec_type != AVMEDIA_TYPE_VIDEO)
    {
        LOG_WARN("GetVideoStreamInfo() : Stream is not a video stream");
        return false;
    }

    // 获取宽度和高度
    width = codecPar->width;
    height = codecPar->height;

    // 计算帧率
    if (stream->avg_frame_rate.num != 0 && stream->avg_frame_rate.den != 0)
    {
        frameRate = av_q2d(stream->avg_frame_rate);
    }
    else if (stream->r_frame_rate.num != 0 && stream->r_frame_rate.den != 0)
    {
        frameRate = av_q2d(stream->r_frame_rate);
    }
    else
    {
        frameRate = 25.0; // 默认25fps
    }

    // 计算时长
    if (formatCtx->duration != AV_NOPTS_VALUE)
    {
        duration = static_cast<double>(formatCtx->duration) / AV_TIME_BASE;
    }
    else if (stream->duration != AV_NOPTS_VALUE)
    {
        duration = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
    }
    else
    {
        duration = 0.0;
    }

    return true;
}

struct SwsContext* FFmpegPublicUtils::CreateVideoConverter(int srcWidth, int srcHeight, AVPixelFormat srcFormat, int dstWidth, int dstHeight, AVPixelFormat dstFormat)
{
    return sws_getContext(srcWidth, srcHeight, srcFormat, dstWidth, dstHeight, dstFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
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

        // 分配对齐的缓冲区，增加额外的padding
        size_t currentSize = result.audioData.size();
        size_t newSize = currentSize + frameSize;
        result.audioData.resize(newSize + AV_INPUT_BUFFER_PADDING_SIZE + 32);

        // 对于平面格式（FLTP等），需要交错复制数据
        if (av_sample_fmt_is_planar(result.format))
        {
            int bytesPerSample = av_get_bytes_per_sample(result.format);
            uint8_t* dst = result.audioData.data() + currentSize;

            // 优化的平面格式到交错格式的转换，减少音频失真
            for (int s = 0; s < frame->nb_samples; s++)
            {
                for (int ch = 0; ch < result.channels; ch++)
                {
                    const uint8_t* src = frame->data[ch] + s * bytesPerSample;

                    // 对于浮点格式，进行软限幅处理避免爆音
                    if (result.format == AV_SAMPLE_FMT_FLTP)
                    {
                        auto srcFloat = reinterpret_cast<float*>(const_cast<uint8_t*>(src));
                        auto dstFloat = reinterpret_cast<float*>(dst);

                        // 软限幅处理，避免音频失真
                        float sample = *srcFloat;
                        if (sample > 0.95f)
                        {
                            sample = 0.95f;
                        }
                        else if (sample < -0.95f)
                        {
                            sample = -0.95f;
                        }

                        *dstFloat = sample;
                    }
                    else
                    {
                        memcpy(dst, src, bytesPerSample);
                    }

                    dst += bytesPerSample;
                }
            }
        }
        else
        {
            // 对于已经交错的格式，直接复制，但也进行软限幅处理
            if (result.format == AV_SAMPLE_FMT_FLT)
            {
                auto src = reinterpret_cast<float*>(frame->data[0]);
                auto dst = reinterpret_cast<float*>(result.audioData.data() + currentSize);

                for (int i = 0; i < frame->nb_samples * result.channels; i++)
                {
                    float sample = src[i];
                    if (sample > 0.95f)
                    {
                        sample = 0.95f;
                    }
                    else if (sample < -0.95f)
                    {
                        sample = -0.95f;
                    }
                    dst[i] = sample;
                }
            }
            else
            {
                memcpy(result.audioData.data() + currentSize, frame->data[0], frameSize);
            }
        }

        // 清零padding区域
        memset(result.audioData.data() + newSize, 0, AV_INPUT_BUFFER_PADDING_SIZE + 32);
    }

    // 释放资源
    av_frame_free(&frame);
    return result;
}

ST_VideoDecodeResult FFmpegPublicUtils::DecodeVideoPacket(const AVPacket* packet, AVCodecContext* codecCtx)
{
    ST_VideoDecodeResult result;
    if (!packet || !codecCtx)
    {
        LOG_WARN("DecodeVideoPacket() : Invalid parameters");
        return result;
    }

    // 发送数据包到解码器
    int ret = avcodec_send_packet(codecCtx, packet);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Error sending video packet to decoder:" + std::string(errbuf));
        return result;
    }

    // 分配视频帧
    AVFrame* frame = av_frame_alloc();
    if (!frame)
    {
        LOG_WARN("Failed to allocate video frame");
        return result;
    }

    // 接收解码后的视频帧（通常每个包只有一帧）
    ret = avcodec_receive_frame(codecCtx, frame);
    if (ret == 0)
    {
        // 成功解码到帧
        result.width = frame->width;
        result.height = frame->height;
        result.format = static_cast<AVPixelFormat>(frame->format);
        result.timestamp = frame->pts;

        // 获取视频帧大小
        int frameSize = GetVideoFrameSize(frame);
        if (frameSize > 0)
        {
            result.videoData.resize(frameSize);

            // 复制视频数据
            av_image_copy_to_buffer(result.videoData.data(), frameSize, const_cast<const uint8_t**>(frame->data), frame->linesize, result.format, result.width, result.height, 1);
        }
    }
    else if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Error receiving video frame from decoder:" + std::string(errbuf));
    }

    // 释放资源
    av_frame_free(&frame);
    return result;
}
