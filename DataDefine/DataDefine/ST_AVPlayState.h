#pragma once

#include <mutex>
#include "BaseDataDefine/ST_AVPacket.h"
/// <summary>
/// 音频播放状态封装类
/// </summary>
class ST_AVPlayState
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
        std::unique_lock<std::recursive_mutex>(m_mutex);
        m_isPlaying = playing;
        if (playing)
        {
            m_isPaused = false; // 播放时自动取消暂停状态
            m_isRecording = false;
        }
    }
    void SetPaused(bool paused)
    {
        std::unique_lock<std::recursive_mutex>(m_mutex);
        m_isPaused = paused;
        if (paused)
        {
            m_isPlaying = false; // 暂停时自动取消播放状态
            m_isRecording = false;
        }
    }
    void SetRecording(bool recording)
    {
        std::unique_lock<std::recursive_mutex>(m_mutex);
        m_isRecording = recording;
        if (recording)
        {
            m_isPlaying = false; // 录制时自动取消播放状态
            m_isPaused = false;  // 录制时自动取消暂停状态
        }
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
    std::recursive_mutex m_mutex;
};



