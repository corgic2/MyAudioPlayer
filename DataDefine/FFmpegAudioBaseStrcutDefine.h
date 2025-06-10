#pragma once
#include <qDebug>
#include <algorithm>

extern "C" {
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/time.h"
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

    /// <summary>
    /// 打开输入文件
    /// </summary>
    int OpenInputFilePath(const char* url, const AVInputFormat* fmt, AVDictionary** options)
    {
        return avformat_open_input(&m_pFormatCtx, url, fmt, options);
    }

    /// <summary>
    /// 打开输出文件
    /// </summary>
    int OpenOutputFilePath(const AVOutputFormat* oformat, const char* formatName, const char* filename)
    {
        return avformat_alloc_output_context2(&m_pFormatCtx, oformat, formatName, filename);
    }

    /// <summary>
    /// 查找最佳流
    /// </summary>
    int FindBestStream(enum AVMediaType type, int wanted_stream_nb, int related_stream, const struct AVCodec** decoder_ret, int flags)
    {
        return av_find_best_stream(m_pFormatCtx, type, wanted_stream_nb, related_stream, decoder_ret, flags);
    }

    /// <summary>
    /// 写入文件尾
    /// </summary>
    void WriteFileTrailer()
    {
        av_write_trailer(m_pFormatCtx);
    }

    /// <summary>
    /// 写入文件头
    /// </summary>
    int WriteFileHeader(AVDictionary** options)
    {
        return avformat_write_header(m_pFormatCtx, options);
    }

    /// <summary>
    /// 获取流的总时长（秒）
    /// </summary>
    double GetStreamDuration(int streamIndex) const
    {
        if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
        {
            return 0.0;
        }
        AVStream* stream = m_pFormatCtx->streams[streamIndex];
        if (stream->duration == AV_NOPTS_VALUE)
        {
            return 0.0;
        }
        return stream->duration * av_q2d(stream->time_base);
    }

    /// <summary>
    /// 跳转到指定时间点
    /// </summary>
    int SeekFrame(int streamIndex, double timestamp)
    {
        if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
        {
            return -1;
        }
        AVStream* stream = m_pFormatCtx->streams[streamIndex];
        int64_t ts = (int64_t)(timestamp / av_q2d(stream->time_base));
        return av_seek_frame(m_pFormatCtx, streamIndex, ts, AVSEEK_FLAG_BACKWARD);
    }

    /// <summary>
    /// 获取当前时间戳
    /// </summary>
    double GetCurrentTimestamp(int streamIndex) const
    {
        if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
        {
            return 0.0;
        }
        AVStream* stream = m_pFormatCtx->streams[streamIndex];
        return av_gettime() * av_q2d(stream->time_base);
    }
};

struct ST_AVPacket
{
    AVPacket* m_pkt = av_packet_alloc();

    ~ST_AVPacket()
    {
        qDebug() << "~ST_AVPacket()";
        av_packet_free(&m_pkt);
    }

    /// <summary>
    /// 从格式上下文读取一个数据包
    /// </summary>
    int ReadPacket(AVFormatContext* pFormatContext)
    {
        return av_read_frame(pFormatContext, m_pkt);
    }

    /// <summary>
    /// 发送数据包到解码器
    /// </summary>
    int SendPacket(AVCodecContext* pCodecContext)
    {
        return avcodec_send_packet(pCodecContext, m_pkt);
    }

    /// <summary>
    /// 释放数据包引用
    /// </summary>
    void UnrefPacket()
    {
        av_packet_unref(m_pkt);
    }

    /// <summary>
    /// 复制数据包
    /// </summary>
    int CopyPacket(const AVPacket* src)
    {
        return av_packet_copy_props(m_pkt, src);
    }

    /// <summary>
    /// 移动数据包
    /// </summary>
    void MovePacket(AVPacket* src)
    {
        av_packet_move_ref(m_pkt, src);
    }

    /// <summary>
    /// 重置数据包时间戳
    /// </summary>
    void RescaleTimestamp(const AVRational& srcTimeBase, const AVRational& dstTimeBase)
    {
        av_packet_rescale_ts(m_pkt, srcTimeBase, dstTimeBase);
    }

    /// <summary>
    /// 获取数据包大小
    /// </summary>
    int GetPacketSize() const
    {
        return m_pkt->size;
    }

    /// <summary>
    /// 获取数据包时间戳
    /// </summary>
    int64_t GetTimestamp() const
    {
        return m_pkt->pts;
    }

    /// <summary>
    /// 获取数据包持续时间
    /// </summary>
    int64_t GetDuration() const
    {
        return m_pkt->duration;
    }

    /// <summary>
    /// 获取数据包所属流索引
    /// </summary>
    int GetStreamIndex() const
    {
        return m_pkt->stream_index;
    }

    /// <summary>
    /// 设置数据包时间戳
    /// </summary>
    void SetTimestamp(int64_t pts)
    {
        m_pkt->pts = pts;
    }

    /// <summary>
    /// 设置数据包持续时间
    /// </summary>
    void SetDuration(int64_t duration)
    {
        m_pkt->duration = duration;
    }

    /// <summary>
    /// 设置数据包所属流索引
    /// </summary>
    void SetStreamIndex(int index)
    {
        m_pkt->stream_index = index;
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

/// <summary>
/// 音频播放状态结构体
/// </summary>
struct ST_AudioPlayState
{
    bool m_isPlaying = false;    /// 是否正在播放
    bool m_isPaused = false;     /// 是否已暂停
    bool m_isRecording = false;  /// 是否正在录制
    double m_currentPosition = 0.0;  /// 当前播放位置（秒）
    double m_duration = 0.0;     /// 总时长（秒）
    int64_t m_startTime = 0;     /// 开始播放的时间戳

    /// <summary>
    /// 重置播放状态
    /// </summary>
    void Reset()
    {
        m_isPlaying = false;
        m_isPaused = false;
        m_isRecording = false;
        m_currentPosition = 0.0;
        m_duration = 0.0;
        m_startTime = 0;
    }
};