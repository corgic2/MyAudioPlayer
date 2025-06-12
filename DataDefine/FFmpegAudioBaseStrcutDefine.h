#pragma once
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
}

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_stdinc.h>

/// <summary>
/// 音频格式上下文封装类
/// </summary>
class ST_AVFormatContext
{
public:
    ST_AVFormatContext() = default;
    ~ST_AVFormatContext();

    // 禁用拷贝操作
    ST_AVFormatContext(const ST_AVFormatContext&) = delete;
    ST_AVFormatContext& operator=(const ST_AVFormatContext&) = delete;

    // 移动操作
    ST_AVFormatContext(ST_AVFormatContext&& other) noexcept;
    ST_AVFormatContext& operator=(ST_AVFormatContext&& other) noexcept;

    /// <summary>
    /// 打开输入文件
    /// </summary>
    /// <param name="url">文件路径</param>
    /// <param name="fmt">输入格式</param>
    /// <param name="options">选项字典</param>
    /// <returns>成功返回0，失败返回负值</returns>
    int OpenInputFilePath(const char* url, const AVInputFormat* fmt = nullptr, AVDictionary** options = nullptr);

    /// <summary>
    /// 打开输出文件
    /// </summary>
    /// <param name="oformat">输出格式</param>
    /// <param name="formatName">格式名称</param>
    /// <param name="filename">文件名</param>
    /// <returns>成功返回0，失败返回负值</returns>
    int OpenOutputFilePath(const AVOutputFormat* oformat, const char* formatName, const char* filename);

    /// <summary>
    /// 查找最佳流
    /// </summary>
    /// <returns>成功返回流索引，失败返回负值</returns>
    int FindBestStream(enum AVMediaType type, int wanted_stream_nb = -1, 
                      int related_stream = -1, const AVCodec** decoder_ret = nullptr, int flags = 0);

    /// <summary>
    /// 写入文件尾
    /// </summary>
    void WriteFileTrailer();

    /// <summary>
    /// 写入文件头
    /// </summary>
    int WriteFileHeader(AVDictionary** options = nullptr);

    /// <summary>
    /// 获取流的总时长（秒）
    /// </summary>
    double GetStreamDuration(int streamIndex) const;

    /// <summary>
    /// 跳转到指定时间点
    /// </summary>
    int SeekFrame(int streamIndex, double timestamp);

    /// <summary>
    /// 获取当前时间戳
    /// </summary>
    double GetCurrentTimestamp(int streamIndex) const;

    /// <summary>
    /// 获取原始格式上下文指针
    /// </summary>
    AVFormatContext* GetRawContext() const { return m_pFormatCtx; }

private:
    AVFormatContext* m_pFormatCtx = nullptr;
};

/// <summary>
/// 音频包封装类
/// </summary>
class ST_AVPacket
{
public:
    ST_AVPacket();
    ~ST_AVPacket();

    // 禁用拷贝操作
    ST_AVPacket(const ST_AVPacket&) = delete;
    ST_AVPacket& operator=(const ST_AVPacket&) = delete;

    // 移动操作
    ST_AVPacket(ST_AVPacket&& other) noexcept;
    ST_AVPacket& operator=(ST_AVPacket&& other) noexcept;

    /// <summary>
    /// 从格式上下文读取一个数据包
    /// </summary>
    int ReadPacket(AVFormatContext* pFormatContext);

    /// <summary>
    /// 发送数据包到解码器
    /// </summary>
    int SendPacket(AVCodecContext* pCodecContext);

    /// <summary>
    /// 释放数据包引用
    /// </summary>
    void UnrefPacket();

    /// <summary>
    /// 复制数据包
    /// </summary>
    int CopyPacket(const AVPacket* src);

    /// <summary>
    /// 移动数据包
    /// </summary>
    void MovePacket(AVPacket* src);

    /// <summary>
    /// 重置数据包时间戳
    /// </summary>
    void RescaleTimestamp(const AVRational& srcTimeBase, const AVRational& dstTimeBase);

    // Getter方法
    int GetPacketSize() const;
    int64_t GetTimestamp() const;
    int64_t GetDuration() const;
    int GetStreamIndex() const;

    // Setter方法
    void SetTimestamp(int64_t pts);
    void SetDuration(int64_t duration);
    void SetStreamIndex(int index);

    /// <summary>
    /// 获取原始包指针
    /// </summary>
    AVPacket* GetRawPacket() const { return m_pkt; }

private:
    AVPacket* m_pkt = nullptr;
};

/// <summary>
/// 音频输入格式封装类
/// </summary>
class ST_AVInputFormat
{
public:
    ST_AVInputFormat() = default;
    ~ST_AVInputFormat() = default;

    // 移动操作
    ST_AVInputFormat(ST_AVInputFormat&& other) noexcept;
    ST_AVInputFormat& operator=(ST_AVInputFormat&& other) noexcept;

    /// <summary>
    /// 查找输入格式
    /// </summary>
    void FindInputFormat(const std::string& deviceFormat);

    /// <summary>
    /// 获取原始输入格式指针
    /// </summary>
    const AVInputFormat* GetRawFormat() const { return m_pInputFormatCtx; }

private:
    const AVInputFormat* m_pInputFormatCtx = nullptr;
};

/// <summary>
/// 音频编解码器上下文封装类
/// </summary>
class ST_AVCodecContext
{
public:
    ST_AVCodecContext() = default;
    explicit ST_AVCodecContext(const AVCodec* codec);
    ~ST_AVCodecContext();

    /// <summary>
    /// 绑定参数到上下文
    /// </summary>
    int BindParamToContext(const AVCodecParameters* parameters);

    /// <summary>
    /// 打开编解码器
    /// </summary>
    int OpenCodec(const AVCodec* codec, AVDictionary** options = nullptr);

    /// <summary>
    /// 获取原始编解码器上下文指针
    /// </summary>
    AVCodecContext* GetRawContext() const { return m_pCodecContext; }

private:
    AVCodecContext* m_pCodecContext = nullptr;
};

/// <summary>
/// SDL音频设备ID封装类
/// </summary>
class ST_SDLAudioDeviceID
{
public:
    ST_SDLAudioDeviceID() = default;
    ST_SDLAudioDeviceID(SDL_AudioDeviceID deviceId, const SDL_AudioSpec* spec);
    ~ST_SDLAudioDeviceID();

    // 移动操作
    ST_SDLAudioDeviceID(ST_SDLAudioDeviceID&& other) noexcept;
    ST_SDLAudioDeviceID& operator=(ST_SDLAudioDeviceID&& other) noexcept;

    /// <summary>
    /// 获取原始设备ID
    /// </summary>
    SDL_AudioDeviceID GetRawDeviceID() const { return m_audioDevice; }

private:
    SDL_AudioDeviceID m_audioDevice = 0;
};

/// <summary>
/// SDL音频流封装类
/// </summary>
class ST_SDLAudioStream
{
public:
    ST_SDLAudioStream() = default;
    ST_SDLAudioStream(SDL_AudioSpec* srcSpec, SDL_AudioSpec* dstSpec);
    ~ST_SDLAudioStream();

    // 移动操作
    ST_SDLAudioStream(ST_SDLAudioStream&& other) noexcept;
    ST_SDLAudioStream& operator=(ST_SDLAudioStream&& other) noexcept;

    /// <summary>
    /// 获取原始音频流指针
    /// </summary>
    SDL_AudioStream* GetRawStream() const { return m_audioStreamSdl; }

private:
    SDL_AudioStream* m_audioStreamSdl = nullptr;
};

/// <summary>
/// 音频编解码器参数封装类
/// </summary>
class ST_AVCodecParameters
{
public:
    explicit ST_AVCodecParameters(AVCodecParameters* obj);
    ~ST_AVCodecParameters();

    AVCodecID GetCodecId() const;
    int GetSamplePerRate() const;
    AVSampleFormat GetSampleFormat() const;
    int GetBitPerSample() const;

    /// <summary>
    /// 获取原始参数指针
    /// </summary>
    AVCodecParameters* GetRawParameters() const { return m_codecParameters; }

private:
    AVCodecParameters* m_codecParameters = nullptr;
};

/// <summary>
/// 音频编解码器封装类
/// </summary>
class ST_AVCodec
{
public:
    ST_AVCodec() = default;
    explicit ST_AVCodec(enum AVCodecID id);
    explicit ST_AVCodec(const AVCodec* obj);
    ~ST_AVCodec() = default;

    /// <summary>
    /// 获取原始编解码器指针
    /// </summary>
    const AVCodec* GetRawCodec() const { return m_pAvCodec; }

private:
    const AVCodec* m_pAvCodec = nullptr;
};

/// <summary>
/// 音频采样格式封装类
/// </summary>
class ST_AVSampleFormat
{
public:
    AVSampleFormat sampleFormat = AV_SAMPLE_FMT_NONE;
};

/// <summary>
/// 音频重采样上下文封装类
/// </summary>
class ST_SwrContext
{
public:
    ST_SwrContext() = default;
    ~ST_SwrContext();

    /// <summary>
    /// 初始化重采样上下文
    /// </summary>
    int SwrContextInit();

    /// <summary>
    /// 获取原始重采样上下文指针
    /// </summary>
    SwrContext* GetRawContext() const { return m_swrCtx; }

private:
    SwrContext* m_swrCtx = nullptr;
};

/// <summary>
/// 音频通道布局封装类
/// </summary>
class ST_AVChannelLayout
{
public:
    ST_AVChannelLayout() = default;
    explicit ST_AVChannelLayout(AVChannelLayout* ptr);
    ~ST_AVChannelLayout() = default;

    /// <summary>
    /// 获取原始通道布局指针
    /// </summary>
    AVChannelLayout* GetRawLayout() const { return channel; }

private:
    AVChannelLayout* channel = nullptr;
};

/// <summary>
/// 音频播放状态封装类
/// </summary>
class ST_AudioPlayState
{
public:
    /// <summary>
    /// 重置播放状态
    /// </summary>
    void Reset();

    // Getter方法
    bool IsPlaying() const { return m_isPlaying; }
    bool IsPaused() const { return m_isPaused; }
    bool IsRecording() const { return m_isRecording; }
    double GetCurrentPosition() const { return m_currentPosition; }
    double GetDuration() const { return m_duration; }
    int64_t GetStartTime() const { return m_startTime; }

    // Setter方法
    void SetPlaying(bool playing) { m_isPlaying = playing; }
    void SetPaused(bool paused) { m_isPaused = paused; }
    void SetRecording(bool recording) { m_isRecording = recording; }
    void SetCurrentPosition(double position) { m_currentPosition = position; }
    void SetDuration(double duration) { m_duration = duration; }
    void SetStartTime(int64_t time) { m_startTime = time; }

private:
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_isRecording = false;
    double m_currentPosition = 0.0;
    double m_duration = 0.0;
    int64_t m_startTime = 0;
};