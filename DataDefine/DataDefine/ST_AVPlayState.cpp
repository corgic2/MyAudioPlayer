#include "ST_AVPlayState.h"
#include <algorithm>

ST_AVPlayState::ST_AVPlayState() = default;

ST_AVPlayState::~ST_AVPlayState() = default;

StateTransitionResult ST_AVPlayState::TransitionTo(AVPlayState newState)
{
    AVPlayState oldState = m_currentState.load();

    // 检查是否允许转换
    if (!IsValidTransition(oldState, newState))
    {
        return {false, oldState, oldState, "Invalid state transition from " + GetStateName(oldState) + " to " + GetStateName(newState)};
    }

    // 原子更新状态
    m_currentState.store(newState);

    // 清除错误状态
    if (newState != AVPlayState::Error)
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError.clear();
    }

    // 通知状态变更
    NotifyStateChange(oldState, newState);

    return {true, oldState, newState, "State transitioned successfully"};
}

AVPlayState ST_AVPlayState::GetCurrentState() const
{
    return m_currentState.load();
}

bool ST_AVPlayState::CanTransitionTo(AVPlayState targetState) const
{
    return IsValidTransition(GetCurrentState(), targetState);
}

std::string ST_AVPlayState::GetStateName(AVPlayState state) const
{
    switch (state)
    {
        case AVPlayState::Idle:
            return "Idle";
        case AVPlayState::Playing:
            return "Playing";
        case AVPlayState::Paused:
            return "Paused";
        case AVPlayState::Stopped:
            return "Stopped";
        case AVPlayState::Recording:
            return "Recording";
        case AVPlayState::Error:
            return "Error";
        default:
            return "Unknown";
    }
}

bool ST_AVPlayState::IsValidTransition(AVPlayState from, AVPlayState to) const
{
    // 状态转换规则表
    switch (from)
    {
        case AVPlayState::Idle:
            return to == AVPlayState::Playing || to == AVPlayState::Error;

        case AVPlayState::Playing:
            return to == AVPlayState::Paused || to == AVPlayState::Stopped || to == AVPlayState::Error;

        case AVPlayState::Paused:
            return to == AVPlayState::Playing || to == AVPlayState::Stopped || to == AVPlayState::Error;

        case AVPlayState::Stopped:
            return to == AVPlayState::Playing || to == AVPlayState::Idle || to == AVPlayState::Error;

        case AVPlayState::Recording:
            return to == AVPlayState::Stopped || to == AVPlayState::Error;

        case AVPlayState::Error:
            return to == AVPlayState::Idle || to == AVPlayState::Stopped;

        default:
            return false;
    }
}

double ST_AVPlayState::GetCurrentPosition() const
{
    return m_currentPosition.load();
}

double ST_AVPlayState::GetDuration() const
{
    return m_duration.load();
}

int64_t ST_AVPlayState::GetStartTime() const
{
    return m_startTime.load();
}

void ST_AVPlayState::SetCurrentPosition(double position)
{
    m_currentPosition.store(position);
}

void ST_AVPlayState::SetDuration(double duration)
{
    m_duration.store(duration);
}

void ST_AVPlayState::SetStartTime(int64_t time)
{
    m_startTime.store(time);
}

StateTransitionResult ST_AVPlayState::SetError(const std::string& errorMessage)
{
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError = errorMessage;
    }

    return TransitionTo(AVPlayState::Error);
}

std::string ST_AVPlayState::GetLastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

void ST_AVPlayState::RegisterStateChangeCallback(StateChangeCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_stateChangeCallback = callback;
}

void ST_AVPlayState::UnregisterStateChangeCallback()
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_stateChangeCallback = nullptr;
}

void ST_AVPlayState::Reset()
{
    // 重置到初始状态
    m_currentState.store(AVPlayState::Idle);
    m_currentPosition.store(0.0);
    m_duration.store(0.0);
    m_startTime.store(0);

    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError.clear();
    }

    // 通知状态变更
    NotifyStateChange(AVPlayState::Error, AVPlayState::Idle);
}

void ST_AVPlayState::NotifyStateChange(AVPlayState oldState, AVPlayState newState)
{
    StateChangeCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callback = m_stateChangeCallback;
    }

    if (callback)
    {
        callback(oldState, newState);
    }
}
