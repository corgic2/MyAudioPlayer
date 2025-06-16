#pragma once

#include "BaseDataDefine/ST_AVPacket.h"
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
    bool IsPlaying() const
    {
        return m_isPlaying;
    }
    bool IsPaused() const
    {
        return m_isPaused;
    }
    bool IsRecording() const
    {
        return m_isRecording;
    }
    double GetCurrentPosition() const
    {
        return m_currentPosition;
    }
    double GetDuration() const
    {
        return m_duration;
    }
    int64_t GetStartTime() const
    {
        return m_startTime;
    }

    // Setter方法
    void SetPlaying(bool playing)
    {
        m_isPlaying = playing;
    }
    void SetPaused(bool paused)
    {
        m_isPaused = paused;
    }
    void SetRecording(bool recording)
    {
        m_isRecording = recording;
    }
    void SetCurrentPosition(double position)
    {
        m_currentPosition = position;
    }
    void SetDuration(double duration)
    {
        m_duration = duration;
    }
    void SetStartTime(int64_t time)
    {
        m_startTime = time;
    }

  private:
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_isRecording = false;
    double m_currentPosition = 0.0;
    double m_duration = 0.0;
    int64_t m_startTime = 0;
};



