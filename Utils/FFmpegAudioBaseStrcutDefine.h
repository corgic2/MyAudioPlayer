#pragma once
#include <qDebug>
#include <algorithm>
#include <qtextstream.h>
#include "FFmpegPublicUtils.h"

extern "C" {
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

#include "SDL3/SDL_audio.h"

struct ST_AVFormatContext
{
    AVFormatContext* m_pFormatCtx = nullptr;
    ST_AVFormatContext() = default;
    ~ST_AVFormatContext()
    {
        qDebug() << "~ST_AVFormatContext()";
        if (m_pFormatCtx)
        {
            avformat_close_input(&m_pFormatCtx);
            m_pFormatCtx = nullptr;
        }
    }

    // 禁用拷贝操作
    ST_AVFormatContext(const ST_AVFormatContext&) = delete;
    ST_AVFormatContext& operator=(const ST_AVFormatContext&) = delete;

    ST_AVFormatContext(ST_AVFormatContext&& other) noexcept
        : m_pFormatCtx(other.m_pFormatCtx)
    {
        other.m_pFormatCtx = nullptr;
    }
    ST_AVFormatContext& operator=(ST_AVFormatContext&& obj)
    {
        if (this != &obj)
        {
            // 释放当前资源
            if (m_pFormatCtx)
            {
                avformat_close_input(&m_pFormatCtx);
                m_pFormatCtx = nullptr;
            }
            // 转移资源
            m_pFormatCtx = obj.m_pFormatCtx;
            obj.m_pFormatCtx = nullptr;
        }
        return *this;
    }

    int OpenInputFilePath(const char* url, const AVInputFormat* fmt, AVDictionary** options)
    {
        return avformat_open_input(&m_pFormatCtx, url, fmt, options);
    }

    int OpenOutputFilePath(const AVOutputFormat* oformat, const char* formatName, const char* filename)
    {
        return avformat_alloc_output_context2(&m_pFormatCtx, oformat, formatName, filename); // 输出格式上下文初始化
    }

    int FindBestStream(enum AVMediaType type, int wanted_stream_nb, int related_stream, const struct AVCodec** decoder_ret, int flags)
    {
        return av_find_best_stream(m_pFormatCtx, type, wanted_stream_nb, related_stream, decoder_ret, flags);
    }

    void WriteFileTrailer()
    {
        av_write_trailer(m_pFormatCtx);
    }

    int WriteFileHeader(AVDictionary** options)
    {
        return avformat_write_header(m_pFormatCtx, options);
    }
};

struct ST_AVPacket
{
    AVPacket* m_pkt = av_packet_alloc();

    int ReadPacket(AVFormatContext* pFormatContext)
    {
        return av_read_frame(pFormatContext, m_pkt);
    }

    void SendPacket(AVCodecContext* pCodecContext)
    {
        avcodec_send_packet(pCodecContext, m_pkt);
    }

    void UnrefPacket()
    {
        av_packet_unref(m_pkt);
    }

    ~ST_AVPacket()
    {
        qDebug() << "~ST_AVPacket()";
        av_packet_free(&m_pkt);
    }
};

struct ST_AVInputFormat
{
    const AVInputFormat* m_pInputFormatCtx = nullptr; // 输入设备格式
    ~ST_AVInputFormat()
    {
        qDebug() << "~ST_AVInputFormat()";
    }
    
    ST_AVInputFormat& operator=(ST_AVInputFormat&& obj)
    {
        if (this != &obj)
        {
            m_pInputFormatCtx = std::move(obj.m_pInputFormatCtx);
            obj.m_pInputFormatCtx = nullptr;
        }
        return *this;
    }

    void FindInputFormat(const QString& devieceFormat)
    {
        m_pInputFormatCtx = av_find_input_format(devieceFormat.toStdString().c_str());
    }
};

struct ST_AVCodecContext
{
    AVCodecContext* m_pCodecContext = nullptr;
    ST_AVCodecContext() = default;

    ST_AVCodecContext(const AVCodec* codec)
    {
        m_pCodecContext = avcodec_alloc_context3(codec);
    }

    ~ST_AVCodecContext()
    {
        qDebug() << "~ST_AVCodecContext()";
        avcodec_free_context(&m_pCodecContext);
    }

    void BindParamToContext(const AVCodecParameters* parameters)
    {
        avcodec_parameters_to_context(m_pCodecContext, parameters);
    }

    void OpenCodec(const AVCodec* codec, AVDictionary** options)
    {
        avcodec_open2(m_pCodecContext, codec, options);
    }
};

struct ST_SDLAudioDeviceID
{
    SDL_AudioDeviceID m_audioDevice;
    ST_SDLAudioDeviceID() = default;

    ST_SDLAudioDeviceID(SDL_AudioDeviceID deviceId, const SDL_AudioSpec* spec)
    {
        // 使用设备ID打开音频设备
        m_audioDevice = SDL_OpenAudioDevice(deviceId, spec);
    }

    ST_SDLAudioDeviceID& operator=(ST_SDLAudioDeviceID&& obj)
    {
        if (this != &obj)
        {
            m_audioDevice = obj.m_audioDevice;
            obj.m_audioDevice = 0;
        }
        return *this;
    }

    ~ST_SDLAudioDeviceID()
    {
        qDebug() << "~ST_SDLAudioDeviceID()";
        SDL_CloseAudioDevice(m_audioDevice);
    }
};

struct ST_SDLAudioStream
{
    SDL_AudioStream* m_audioStreamSdl = nullptr;
    ST_SDLAudioStream() = default;

    ST_SDLAudioStream(SDL_AudioSpec* srcSpec, SDL_AudioSpec* dstSpec)
    {
        m_audioStreamSdl = SDL_CreateAudioStream(srcSpec, dstSpec);
    }

    ST_SDLAudioStream& operator=(ST_SDLAudioStream&& obj)
    {
        if (this != &obj)
        {
            m_audioStreamSdl = obj.m_audioStreamSdl;
            obj.m_audioStreamSdl = nullptr;
        }
        return *this;
    }

    ~ST_SDLAudioStream()
    {
        qDebug() << "~ST_SDLAudioStream()";
        SDL_DestroyAudioStream(m_audioStreamSdl);
    }
};

struct ST_AVCodecParameters
{
    AVCodecParameters* m_codecParameters = nullptr;

    AVCodecID GetCodecId()
    {
        return m_codecParameters->codec_id;
    }

    ST_AVCodecParameters(AVCodecParameters* obj)
    {
        m_codecParameters = obj;
    }

    ~ST_AVCodecParameters()
    {
        m_codecParameters = nullptr;
        qDebug() << "~ST_AVCodecParameters()";
    }

    int GetSamplePerRate()
    {
        return av_get_bytes_per_sample(GetSampleFormat());
    }

    AVSampleFormat GetSampleFormat()
    {
        return (AVSampleFormat)m_codecParameters->format;
    }

    int GetBitPerSample()
    {
        return av_get_bits_per_sample(m_codecParameters->codec_id);
    }
};

struct ST_AVCodec
{
    const AVCodec* m_pAvCodec = nullptr;
    ST_AVCodec() = default;

    ~ST_AVCodec()
    {
        qDebug() << "~ST_AVCodec()";
    }

    ST_AVCodec(enum AVCodecID id)
    {
        m_pAvCodec = avcodec_find_decoder(id);
    }

    ST_AVCodec(const AVCodec* obj)
    {
        m_pAvCodec = obj;
    }
};


struct ST_AVSampleFormat
{
    AVSampleFormat sampleFormat;
};


struct ST_SwrContext
{
    SwrContext *m_swrCtx = nullptr;
    ST_SwrContext()
    {

    }

    ~ST_SwrContext()
    {
        if (m_swrCtx)
        {
            swr_free(&m_swrCtx);
        }   
    }
    void SwrContextInit()
    {
        swr_init(m_swrCtx);
    }

};

struct ST_AVChannelLayout
{
    AVChannelLayout *channel = nullptr;
    ST_AVChannelLayout() = default;
    ST_AVChannelLayout(AVChannelLayout* ptr)
    {
        channel = ptr;
    }
};