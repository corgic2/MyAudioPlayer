#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <SDL3/SDL_timer.h>

/// 前向声明
class AudioFFmpegPlayer;

/// <summary>
/// 音视频同步器
/// 实现基于音频时钟的视频同步策略
/// </summary>
class VideoAudioSync
{
public:
    VideoAudioSync();
    ~VideoAudioSync() = default;

    /// <summary>
    /// 设置音频播放器
    /// </summary>
    /// <param name="audioPlayer">音频播放器实例</param>
    void SetAudioPlayer(AudioFFmpegPlayer* audioPlayer);

    /// <summary>
    /// 同步视频帧
    /// </summary>
    /// <param name="videoPTS">视频帧的时间戳（秒）</param>
    /// <param name="isKeyFrame">是否为关键帧</param>
    /// <returns>同步结果：0-正常显示，1-丢弃帧，2-等待后显示</returns>
    int SyncVideoFrame(double videoPTS, bool isKeyFrame);

    /// <summary>
    /// 设置同步阈值
    /// </summary>
    /// <param name="threshold">同步阈值（秒）</param>
    void SetSyncThreshold(double threshold);

    /// <summary>
    /// 获取当前音频时钟
    /// </summary>
    /// <returns>音频时钟（秒）</returns>
    double GetAudioClock() const;

    /// <summary>
    /// 重置同步状态
    /// </summary>
    void Reset();

    /// <summary>
    /// 设置最大等待时间
    /// </summary>
    /// <param name="maxWait">最大等待时间（秒）</param>
    void SetMaxWaitTime(double maxWait);

private:
    /// <summary>
    /// 获取当前音频播放位置
    /// </summary>
    /// <returns>音频播放位置（秒）</returns>
    double GetAudioPosition() const;

    /// <summary>
    /// 精确等待指定时间
    /// </summary>
    /// <param name="seconds">等待时间（秒）</param>
    void PreciseWait(double seconds);

private:
    AudioFFmpegPlayer* m_audioPlayer;      /// 音频播放器
    double m_syncThreshold;                /// 同步阈值（秒）
    double m_maxWaitTime;                  /// 最大等待时间（秒）
    std::atomic<int> m_consecutiveDrops;   /// 连续丢弃帧计数
    std::atomic<int> m_totalFrameCount;    /// 总帧计数
    std::atomic<int> m_dropFrameCount;     /// 丢弃帧计数
};