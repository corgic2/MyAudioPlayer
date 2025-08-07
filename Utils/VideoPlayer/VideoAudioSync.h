#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <SDL3/SDL_timer.h>

// 前向声明
class AudioFFmpegPlayer;

/**
 * @brief 音视频同步器
 * 实现基于音频时钟的视频同步策略
 */
class VideoAudioSync
{
public:
    VideoAudioSync();
    ~VideoAudioSync() = default;

    /**
     * @brief 设置音频播放器
     * @param audioPlayer 音频播放器实例
     */
    void SetAudioPlayer(AudioFFmpegPlayer* audioPlayer);

    /**
     * @brief 同步视频帧
     * @param videoPTS 视频帧的时间戳（秒）
     * @param isKeyFrame 是否为关键帧
     * @return 同步结果：0-正常显示，1-丢弃帧，2-等待后显示
     */
    int SyncVideoFrame(double videoPTS, bool isKeyFrame);

    /**
     * @brief 设置同步阈值
     * @param threshold 同步阈值（秒）
     */
    void SetSyncThreshold(double threshold);

    /**
     * @brief 获取当前音频时钟
     * @return 音频时钟（秒）
     */
    double GetAudioClock() const;

    /**
     * @brief 重置同步状态
     */
    void Reset();

    /**
     * @brief 设置最大等待时间
     * @param maxWait 最大等待时间（秒）
     */
    void SetMaxWaitTime(double maxWait);

private:
    /**
     * @brief 获取当前音频播放位置
     * @return 音频播放位置（秒）
     */
    double GetAudioPosition() const;

    /**
     * @brief 精确等待指定时间
     * @param seconds 等待时间（秒）
     */
    void PreciseWait(double seconds);

private:
    AudioFFmpegPlayer* m_audioPlayer;      ///< 音频播放器
    double m_syncThreshold;                ///< 同步阈值（秒）
    double m_maxWaitTime;                ///< 最大等待时间（秒）
    std::atomic<int> m_consecutiveDrops; ///< 连续丢弃帧计数
    std::atomic<int> m_totalFrameCount;  ///< 总帧计数
    std::atomic<int> m_dropFrameCount;   ///< 丢弃帧计数
};