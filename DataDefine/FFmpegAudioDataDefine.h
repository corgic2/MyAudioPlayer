#pragma once
#include "FFmpegAudioBaseStrcutDefine.h"
#include <memory>
#include <QObject>
#include <QString>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_timer.h>

/// <summary>
/// 音频设备打开封装类
/// </summary>
class ST_OpenAudioDevice
{
public:
    ST_OpenAudioDevice() = default;
    ~ST_OpenAudioDevice() = default;

    // 禁用拷贝操作
    ST_OpenAudioDevice(ST_OpenAudioDevice&) = delete;
    ST_OpenAudioDevice(const ST_OpenAudioDevice&) = delete;
    ST_OpenAudioDevice& operator=(ST_OpenAudioDevice&) = delete;
    ST_OpenAudioDevice& operator=(const ST_OpenAudioDevice&) = delete;

    // 移动操作
    ST_OpenAudioDevice(ST_OpenAudioDevice&& other) noexcept;
    ST_OpenAudioDevice& operator=(ST_OpenAudioDevice&& other) noexcept;

    /// <summary>
    /// 获取输入格式
    /// </summary>
    ST_AVInputFormat& GetInputFormat() { return m_pInputFormatCtx; }

    /// <summary>
    /// 获取格式上下文
    /// </summary>
    ST_AVFormatContext& GetFormatContext() { return m_pFormatCtx; }

private:
    ST_AVInputFormat m_pInputFormatCtx;    /// 输入设备格式
    ST_AVFormatContext m_pFormatCtx;       /// 输入格式上下文
};

/// <summary>
/// 音频播放信息封装类
/// </summary>
class ST_AudioPlayInfo
{
public:
    ST_AudioPlayInfo() = default;
    ~ST_AudioPlayInfo();

    /// <summary>
    /// 获取音频流
    /// </summary>
    ST_SDLAudioStream& GetAudioStream();

    /// <summary>
    /// 获取音频规格
    /// </summary>
    /// <param name="bsrc">true表示获取源规格，false表示获取目标规格</param>
    SDL_AudioSpec& GetAudioSpec(bool bsrc);

    /// <summary>
    /// 获取音频设备ID
    /// </summary>
    ST_SDLAudioDeviceID& GetDeviceId();

    /// <summary>
    /// 初始化音频规格
    /// </summary>
    /// <param name="bsrc">true表示初始化源规格，false表示初始化目标规格</param>
    /// <param name="sampleRate">采样率</param>
    /// <param name="format">音频格式</param>
    /// <param name="channels">声道数</param>
    void InitAudioSpec(bool bsrc, int sampleRate, SDL_AudioFormat format, int channels);

    /// <summary>
    /// 初始化音频流
    /// </summary>
    void InitAudioStream();

    /// <summary>
    /// 初始化音频设备
    /// </summary>
    void InitAudioDevice(SDL_AudioDeviceID deviceId);

    /// <summary>
    /// 绑定音频流和设备
    /// </summary>
    void BindStreamAndDevice();

    /// <summary>
    /// 开始播放音频
    /// </summary>
    void BeginPlayAudio();

    /// <summary>
    /// 向音频流写入数据
    /// </summary>
    void PutDataToStream(const void* buf, int len);

    /// <summary>
    /// 检查数据是否已结束
    /// </summary>
    bool GetDataIsEnd();

    /// <summary>
    /// 延时指定毫秒数
    /// </summary>
    void Delay(Uint32 ms);

    /// <summary>
    /// 暂停音频播放
    /// </summary>
    void PauseAudio();

    /// <summary>
    /// 恢复音频播放
    /// </summary>
    void ResumeAudio();

    /// <summary>
    /// 停止音频播放
    /// </summary>
    void StopAudio();

    /// <summary>
    /// 音频跳转
    /// </summary>
    /// <param name="seconds">跳转的秒数，正数表示向前，负数表示向后</param>
    void SeekAudio(int seconds);

    /// <summary>
    /// 获取当前播放位置（秒）
    /// </summary>
    double GetCurrentPosition() const;

    /// <summary>
    /// 获取音频总时长（秒）
    /// </summary>
    double GetDuration() const;

    /// <summary>
    /// 清空音频流
    /// </summary>
    void FlushAudioStream();

    /// <summary>
    /// 获取音频流缓冲区大小（字节）
    /// </summary>
    int GetAudioStreamAvailable() const;

private:
    ST_SDLAudioStream m_audioStream;       /// 音频流
    ST_SDLAudioDeviceID m_audioDeviceId;   /// 音频设备ID
    SDL_AudioSpec m_srcSpec;               /// 源音频规格
    SDL_AudioSpec m_dstSpec;               /// 目标音频规格
    double m_duration;                     /// 音频总时长（秒）
    double m_currentPosition;              /// 当前播放位置（秒）
    int64_t m_startTime;                  /// 开始播放的时间戳
};

/// <summary>
/// 重采样简单数据封装类
/// </summary>
class ST_ResampleSimpleData
{
public:
    ST_ResampleSimpleData& operator=(const ST_ResampleSimpleData& obj)
    {
        if (this != &obj)
        {
            m_sampleRate = obj.m_sampleRate;
            m_sampleFmt = obj.m_sampleFmt;
            m_channelLayout = obj.m_channelLayout;
        }
        return *this;
    }
    int GetSampleRate() const { return m_sampleRate; }

    int GetChannels() const { return m_channelLayout.GetRawLayout()->nb_channels; }
    const ST_AVSampleFormat& GetSampleFormat() const { return m_sampleFmt; }
    const ST_AVChannelLayout& GetChannelLayout() const { return m_channelLayout; }

    void SetSampleRate(int rate) { m_sampleRate = rate; }

    void SetChannels(int channels) { m_channelLayout.GetRawLayout()->nb_channels = channels; }
    void SetSampleFormat(const ST_AVSampleFormat& fmt) { m_sampleFmt = fmt; }
    void SetChannelLayout(const ST_AVChannelLayout& layout) { m_channelLayout = layout; }

private:
    int m_sampleRate = 0;                  /// 采样率
    ST_AVSampleFormat m_sampleFmt; /// 采样格式
    ST_AVChannelLayout m_channelLayout;    /// 通道布局
};

/// <summary>
/// 重采样参数封装类
/// </summary>
class ST_ResampleParams
{
public:
    ST_ResampleParams() = default;

    ST_ResampleSimpleData& GetInput() { return input; }

    ST_ResampleSimpleData& GetOutput() { return output; }
    void SetInput(const ST_ResampleSimpleData& in) { input = in; }
    void SetOutput(const ST_ResampleSimpleData& out) { output = out; }

private:
    ST_ResampleSimpleData input;           /// 输入参数
    ST_ResampleSimpleData output;          /// 输出参数
};

/// <summary>
/// 重采样结果封装类
/// </summary>
class ST_ResampleResult
{
public:
    const std::vector<uint8_t>& GetData() const { return data; }
    int GetSamples() const { return m_samples; }
    int GetChannels() const { return m_channels; }
    int GetSampleRate() const { return m_sampleRate; }
    const ST_AVSampleFormat& GetSampleFormat() const { return m_sampleFmt; }

    void SetData(const std::vector<uint8_t>& newData) { data = newData; }
    void SetSamples(int samples) { m_samples = samples; }
    void SetChannels(int channels) { m_channels = channels; }
    void SetSampleRate(int rate) { m_sampleRate = rate; }
    void SetSampleFormat(const ST_AVSampleFormat& fmt) { m_sampleFmt = fmt; }

private:
    std::vector<uint8_t> data;            /// 重采样后的数据
    int m_samples = 0;                    /// 实际采样点数
    int m_channels = 0;                   /// 输出声道数
    int m_sampleRate = 0;                 /// 输出采样率
    ST_AVSampleFormat m_sampleFmt;        /// 输出格式
};

