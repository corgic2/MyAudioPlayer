#pragma once
#include "FFmpegAudioBaseStrcutDefine.h"
#include "libavformat/avformat.h"
#include "SDL3/SDL_init.h"

struct ST_OpenAudioDevice
{
    ST_AVInputFormat m_pInputFormatCtx; // 输入设备格式
    ST_AVFormatContext m_pFormatCtx; // 输入格式上下文
    ST_OpenAudioDevice() = default;
    ~ST_OpenAudioDevice() = default;

    ST_OpenAudioDevice(ST_OpenAudioDevice&& obj)
    {
        m_pInputFormatCtx = std::move(obj.m_pInputFormatCtx);
        m_pFormatCtx = std::move(obj.m_pFormatCtx);
        obj.m_pInputFormatCtx.m_pInputFormatCtx = nullptr;
        obj.m_pFormatCtx.m_pFormatCtx = nullptr;
    }

    // 移动赋值运算符
    ST_OpenAudioDevice& operator=(ST_OpenAudioDevice&& obj)
    {
        if (this != &obj)
        {
            m_pInputFormatCtx = std::move(obj.m_pInputFormatCtx);
            m_pFormatCtx = std::move(obj.m_pFormatCtx);
            obj.m_pInputFormatCtx.m_pInputFormatCtx = nullptr;
            obj.m_pFormatCtx.m_pFormatCtx = nullptr;
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
    void InitAudioSpec(bool bsrc, int sampleRate, AVSampleFormat format, int channels);
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
    int GetDataIsEnd();
    /// <summary>
    /// 等待一段时间
    /// </summary>
    void Delay(Uint32 ms);

private:
    ST_SDLAudioDeviceID m_audioDeviceId;
    ST_SDLAudioStream m_audioStream;
    SDL_AudioSpec m_srcSpec;
    SDL_AudioSpec m_dstSpec;
};
