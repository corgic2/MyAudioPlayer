#pragma once
#include "FFmpegAudioBaseStrcutDefine.h"

extern "C" {
#include "libavformat/avformat.h"
}

#include "SDL3/SDL.h"
#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_timer.h"
struct ST_OpenAudioDevice
{
    ST_AVInputFormat m_pInputFormatCtx; // 输入设备格式
    ST_AVFormatContext m_pFormatCtx; // 输入格式上下文
    ST_OpenAudioDevice() = default;

    ~ST_OpenAudioDevice()
    {
        qDebug() << "~ST_OpenAudioDevice()";
    }

    ST_OpenAudioDevice(ST_OpenAudioDevice&) = delete;
    ST_OpenAudioDevice(const ST_OpenAudioDevice&) = delete;
    ST_OpenAudioDevice& operator=(ST_OpenAudioDevice&) = delete;
    ST_OpenAudioDevice& operator=(const ST_OpenAudioDevice&) = delete;

    // 移动构造函数
    ST_OpenAudioDevice(ST_OpenAudioDevice&& obj) noexcept
    {
        m_pInputFormatCtx = std::move(obj.m_pInputFormatCtx);
        m_pFormatCtx = std::move(obj.m_pFormatCtx);
        //其本身的移动语义已经置空，不再额外操作
        //obj.m_pInputFormatCtx.m_pInputFormatCtx = nullptr;
        //obj.m_pFormatCtx.m_pFormatCtx = nullptr;
    }

    // 移动赋值运算符
    ST_OpenAudioDevice& operator=(ST_OpenAudioDevice&& obj)
    {
        if (this != &obj)
        {
            m_pInputFormatCtx = std::move(obj.m_pInputFormatCtx);
            m_pFormatCtx = std::move(obj.m_pFormatCtx);
            // 其本身的移动语义已经置空，不再额外操作
            //obj.m_pInputFormatCtx.m_pInputFormatCtx = nullptr;
            //obj.m_pFormatCtx.m_pFormatCtx = nullptr;
        }
        return *this;
    }
};


class ST_AudioPlayInfo
{
public:
    ST_AudioPlayInfo() = default;
    ~ST_AudioPlayInfo();
    /// <summary>
    /// 获取流
    /// </summary>
    /// <returns></returns>
    ST_SDLAudioStream& GetAudioStream();
    /// <summary>
    /// 获取输入输出格式
    /// </summary>
    /// <param name="bsrc"></param>
    /// <returns></returns>
    SDL_AudioSpec& GetAudioSpec(bool bsrc);
    /// <summary>
    /// 获取设备
    /// </summary>
    /// <returns></returns>
    ST_SDLAudioDeviceID& GetDeviceId();
    /// <summary>
    /// 初始化输入输出格式
    /// </summary>
    /// <param name="bsrc"></param>
    /// <param name="sampleRate"></param>
    /// <param name="format"></param>
    /// <param name="channels"></param>
    void InitAudioSpec(bool bsrc, int sampleRate, SDL_AudioFormat format, int channels);
    /// <summary>
    /// 初始化流，必须在initSpec后
    /// </summary>
    void InitAudioStream();
    /// <summary>
    /// 初始化设备
    /// </summary>
    /// <param name="devid"></param>
    void InitAudioDevice(SDL_AudioDeviceID devid);
    /// <summary>
    /// 绑定流与设备
    /// </summary>
    void BindStreamAndDevice();
    /// <summary>
    /// 开始播放
    /// </summary>
    void BeginPlayAudio();
    /// <summary>
    /// 输入数据
    /// </summary>
    /// <param name="buf"></param>
    /// <param name="len"></param>
    void PutDataToStream(const void* buf, int len);
    /// <summary>
    /// 判断是否终止
    /// </summary>
    /// <returns></returns>
    bool GetDataIsEnd();
    /// <summary>
    /// 等待一段时间
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
    ST_SDLAudioStream m_audioStream;
    ST_SDLAudioDeviceID m_audioDeviceId;
    SDL_AudioSpec m_srcSpec;
    SDL_AudioSpec m_dstSpec;
    double m_duration;          /// 音频总时长（秒）
    double m_currentPosition;   /// 当前播放位置（秒）
    int64_t m_startTime;       /// 开始播放的时间戳
};
struct ST_ResampleSimpleData
{
    int m_sampleRate;
    int m_channels;
    ST_AVSampleFormat m_sampleFmt;
    ST_AVChannelLayout m_channelLayout; // 0表示自动推导
};
// 重采样参数结构体
struct ST_ResampleParams
{
    // 输入参数
    ST_ResampleSimpleData input;
    // 输出参数
    ST_ResampleSimpleData output;

    // 自动填充默认值
    ST_ResampleParams()
    {
        input.m_sampleRate = 48000;
        input.m_channels = 2;
        input.m_sampleFmt.sampleFormat = AV_SAMPLE_FMT_FLTP;

        output.m_sampleRate = 44100;
        output.m_channels = 2;
        output.m_sampleFmt.sampleFormat = AV_SAMPLE_FMT_S16;
    }
};

// 重采样结果结构体
struct ST_ResampleResult
{
    std::vector<uint8_t> data; // 重采样后的数据
    int m_samples;               // 实际采样点数
    int m_channels;              // 输出声道数
    int m_sampleRate;           // 输出采样率
    ST_AVSampleFormat m_sampleFmt; // 输出格式
};