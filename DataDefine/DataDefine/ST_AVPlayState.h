#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include "BaseDataDefine/ST_AVPacket.h"

/// <summary>
/// 音视频播放状态枚举
/// </summary>
enum class AVPlayState
{
    // 空闲状态
    Idle,
    // 播放中
    Playing,
    // 暂停状态
    Paused,
    // 停止状态
    Stopped,
    // 录制状态
    Recording,
    // 错误状态
    Error
};

/// <summary>
/// 状态转换结果
/// </summary>
struct StateTransitionResult
{
    bool success;
    AVPlayState oldState;
    AVPlayState newState;
    std::string message;
};

/// <summary>
/// 状态变更回调函数类型
/// </summary>
using StateChangeCallback = std::function<void(AVPlayState oldState, AVPlayState newState)>;

/// <summary>
/// 音视频播放状态机 - 解决状态管理设计缺陷
/// </summary>
class ST_AVPlayState
{
public:
    ST_AVPlayState();
    ~ST_AVPlayState();

    // 状态机核心接口
    StateTransitionResult TransitionTo(AVPlayState newState);
    AVPlayState GetCurrentState() const;
    bool CanTransitionTo(AVPlayState targetState) const;
    std::string GetStateName(AVPlayState state) const;

    // 便捷状态检查方法
    bool IsIdle() const
    {
        return GetCurrentState() == AVPlayState::Idle;
    }
    bool IsPlaying() const
    {
        return GetCurrentState() == AVPlayState::Playing;
    }

    bool IsPaused() const
    {
        return GetCurrentState() == AVPlayState::Paused;
    }

    bool IsStopped() const
    {
        return GetCurrentState() == AVPlayState::Stopped;
    }

    bool IsRecording() const
    {
        return GetCurrentState() == AVPlayState::Recording;
    }

    bool IsError() const
    {
        return GetCurrentState() == AVPlayState::Error;
    }

    // 状态参数管理（线程安全）
    double GetCurrentPosition() const;
    double GetDuration() const;
    int64_t GetStartTime() const;
    void SetCurrentPosition(double position);
    void SetDuration(double duration);
    void SetStartTime(int64_t time);

    // 错误处理
    StateTransitionResult SetError(const std::string& errorMessage);
    std::string GetLastError() const;

    // 状态变更回调
    void RegisterStateChangeCallback(StateChangeCallback callback);
    void UnregisterStateChangeCallback();

    // 重置状态机
    void Reset();

private:
    // 状态转换规则
    bool IsValidTransition(AVPlayState from, AVPlayState to) const;
    void NotifyStateChange(AVPlayState oldState, AVPlayState newState);

    // 原子状态管理
    std::atomic<AVPlayState> m_currentState{AVPlayState::Idle};
    std::atomic<double> m_currentPosition{0.0};
    std::atomic<double> m_duration{0.0};
    std::atomic<int64_t> m_startTime{0};

    // 错误信息（使用互斥锁保护）
    mutable std::mutex m_errorMutex;
    std::string m_lastError;

    // 回调管理
    mutable std::mutex m_callbackMutex;
    StateChangeCallback m_stateChangeCallback;
};
