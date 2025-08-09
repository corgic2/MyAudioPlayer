#pragma once

#include <atomic>
#include "BaseDataDefine/ST_AVPacket.h"
#include "BaseDataDefine/ST_SDLAudioDeviceID.h"
#include "BaseDataDefine/ST_SDLAudioStream.h"


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
    ST_SDLAudioStream &GetAudioStream();

    /// <summary>
    /// 获取音频规格
    /// </summary>
    /// <param name="bsrc">true表示获取源规格，false表示获取目标规格</param>
    SDL_AudioSpec &GetAudioSpec(bool bsrc);

    /// <summary>
    /// 获取音频设备ID
    /// </summary>
    ST_SDLAudioDeviceID &GetDeviceId();

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
    void PutDataToStream(const void *buf, int len);

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
    /// 清空音频设备缓冲
    /// </summary>
    void ClearAudioDeviceBuffer();

    /// <summary>
    /// 获取音频流缓冲区大小（字节）
    /// </summary>
    int GetAudioStreamAvailable() const;

    /// <summary>
    /// 获取当前音频设备ID
    /// </summary>
    /// <returns>音频设备ID</returns>
    SDL_AudioDeviceID GetAudioDevice() const
    {
        return m_audioDeviceId.GetRawDeviceID();
    }

    /// <summary>
    /// 解绑音频流和设备
    /// </summary>
    void UnbindStreamAndDevice()
    {
        if (m_audioDeviceId.GetRawDeviceID() && m_audioStream.GetRawStream())
        {
            SDL_UnbindAudioStream(m_audioStream.GetRawStream());
        }
    }

    /// <summary>
    /// 设置seek状态
    /// </summary>
    void SetSeeking(bool seeking)
    {
        m_isSeeking = seeking;
    }
    
    /// <summary>
    /// 是否正在执行seek操作
    /// </summary>
    bool IsSeeking() const
    {
        return m_isSeeking.load();
    }
  private:
    ST_SDLAudioStream m_audioStream;     /// 音频流
    ST_SDLAudioDeviceID m_audioDeviceId; /// 音频设备ID
    SDL_AudioSpec m_srcSpec;             /// 源音频规格
    SDL_AudioSpec m_dstSpec;             /// 目标音频规格
    double m_duration;                   /// 音频总时长（秒）
    double m_currentPosition;            /// 当前播放位置（秒）
    int64_t m_startTime;                 /// 开始播放的时间戳
    std::atomic<bool> m_isSeeking{false};  /// 是否正在执行seek操作
};
