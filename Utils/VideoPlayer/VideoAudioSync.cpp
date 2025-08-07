#include "VideoAudioSync.h"
#include "../AudioPlayer/AudioFFmpegPlayer.h"
#include <SDL3/SDL_timer.h>
#include <cmath>
#include <algorithm>
#include "LogSystem/LogSystem.h"

VideoAudioSync::VideoAudioSync()
    : m_audioPlayer(nullptr), m_syncThreshold(0.05) /// 默认50ms 约大于1帧 (24)同步阈值
    , m_maxWaitTime(1.0)                            /// 最大等待1秒
    , m_consecutiveDrops(0)
    , m_totalFrameCount(0)
    , m_dropFrameCount(0)
{
}

void VideoAudioSync::SetAudioPlayer(AudioFFmpegPlayer* audioPlayer)
{
    m_audioPlayer = audioPlayer;
}

int VideoAudioSync::SyncVideoFrame(double videoPTS, bool isKeyFrame)
{
    if (!m_audioPlayer || videoPTS < 0) {
        return 0; /// 无音频播放器或无效时间戳，直接显示
    }

    m_totalFrameCount++;

    /// 获取音频时钟
    double audioClock = GetAudioClock();
    if (audioClock < 0) {
        return 0; /// 无法获取音频时钟，直接显示
    }

    /// 计算时间差   
    double diff = videoPTS - audioClock;
    
    /// 只在关键帧或较大差异时记录详细日志
    static bool bFirstFrameAfterSeek = false;
    if (std::abs(diff) > 0.1 || isKeyFrame || m_totalFrameCount <= 5)
    {
        LOG_INFO("SyncVideoFrame: audioClock=" + std::to_string(audioClock) + 
                 " videoPTS=" + std::to_string(videoPTS) + 
                 " diff=" + std::to_string(diff) + 
                 " isKeyFrame=" + std::to_string(isKeyFrame) +
                 " frameCount=" + std::to_string(m_totalFrameCount));
    }
    else
    {
        LOG_DEBUG("SyncVideoFrame: audioClock=" + std::to_string(audioClock) + 
                  " videoPTS=" + std::to_string(videoPTS) + 
                  " diff=" + std::to_string(diff));
    }
    /// 同步策略
    if (diff < -m_syncThreshold)
    {
        /// 视频落后太多，丢弃当前帧
        m_dropFrameCount++;
        m_consecutiveDrops++;

        /// 防止过度丢弃，遇到关键帧时强制显示
        if (isKeyFrame || m_consecutiveDrops >= 10)
        {
            m_consecutiveDrops = 0;
            return 0; /// 显示关键帧
        }

        return 1; /// 丢弃帧
    }
    else if (diff > m_syncThreshold)
    {
        /// 视频超前太多，等待
        if (diff > m_maxWaitTime) {
            diff = m_maxWaitTime; /// 限制最大等待时间
        }

        PreciseWait(diff);
        m_consecutiveDrops = 0;
        return 2; /// 等待后显示
    }
    else {
        /// 在同步阈值范围内，正常显示
        m_consecutiveDrops = 0;

        /// 微秒级等待，确保精确同步
        if (std::abs(diff) > 0.001)
        {
            /// 1ms精度
            PreciseWait(diff);
        }

        return 0; /// 正常显示
    }
}

void VideoAudioSync::SetSyncThreshold(double threshold)
{
    m_syncThreshold = std::max(0.01, std::min(0.02, threshold)); // 限制在10ms-20ms之间
}

double VideoAudioSync::GetAudioClock() const
{
    return GetAudioPosition();
}

void VideoAudioSync::Reset()
{
    m_consecutiveDrops = 0;
    m_totalFrameCount = 0;
    m_dropFrameCount = 0;
}

void VideoAudioSync::SetMaxWaitTime(double maxWait)
{
    m_maxWaitTime = std::max(0.1, std::min(1.0, maxWait)); // 限制在100ms-1秒之间
}

double VideoAudioSync::GetAudioPosition() const
{
    if (!m_audioPlayer) {
        return -1.0;
    }

    return m_audioPlayer->GetCurrentPosition();
}

void VideoAudioSync::PreciseWait(double seconds)
{
    if (seconds <= 0) {
        return;
    }

    /// 使用高精度计时器
    auto start = std::chrono::high_resolution_clock::now();
    auto target_duration = std::chrono::duration<double>(seconds);

    /// 对于长时间等待，使用SDL_Delay
    if (seconds > 0.01)
    {
        /// 10ms以上
        Uint32 ms = static_cast<Uint32>(seconds * 1000);
        SDL_Delay(ms);
    }

    /// 对于短时间等待，使用忙等待确保精度
    else {
        auto end = start + target_duration;
        while (std::chrono::high_resolution_clock::now() < end) {
            std::this_thread::yield();
        }
    }
}