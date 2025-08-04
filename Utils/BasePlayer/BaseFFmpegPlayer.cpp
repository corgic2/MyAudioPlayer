#include "BaseFFmpegPlayer.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"

BaseFFmpegPlayer::BaseFFmpegPlayer(QObject* parent)
    : QObject(parent)
{
    ResetPlayerState();
}

BaseFFmpegPlayer::~BaseFFmpegPlayer()
{
}

bool BaseFFmpegPlayer::IsPlaying()
{
    return m_playState.GetCurrentState() == AVPlayState::Playing;
}

bool BaseFFmpegPlayer::IsPaused()
{
    return m_playState.GetCurrentState() == AVPlayState::Paused;
}

bool BaseFFmpegPlayer::IsRecording()
{
    return m_playState.GetCurrentState() == AVPlayState::Recording;
}

double BaseFFmpegPlayer::GetDuration()
{
    return m_duration;
}

void BaseFFmpegPlayer::PreparePlay(const QString& filePath, double startPosition, const QStringList& args)
{
}

std::unique_ptr<ST_OpenFileResult> BaseFFmpegPlayer::OpenMediaFile(const QString& filePath)
{
    if (filePath.isEmpty())
    {
        LOG_WARN("BaseFFmpegPlayer::OpenMediaFile() : Empty file path");
        return nullptr;
    }

    if (!my_sdk::FileSystem::Exists(filePath.toStdString()))
    {
        LOG_WARN("BaseFFmpegPlayer::OpenMediaFile() : File does not exist: " + filePath.toStdString());
        return nullptr;
    }

    auto openFileResult = std::make_unique<ST_OpenFileResult>();
    openFileResult->OpenFilePath(filePath);

    if (!openFileResult->m_formatCtx || !openFileResult->m_formatCtx->GetRawContext())
    {
        LOG_WARN("BaseFFmpegPlayer::OpenMediaFile() : Failed to open file: " + filePath.toStdString());
        return nullptr;
    }

    // 设置文件路径和时长
    SetCurrentFilePath(filePath);
    double duration = static_cast<double>(openFileResult->m_formatCtx->GetRawContext()->duration) / AV_TIME_BASE;
    SetDuration(duration);

    LOG_INFO("Successfully opened media file: " + filePath.toStdString() + ", duration: " + std::to_string(duration) + " seconds");
    return openFileResult;
}

void BaseFFmpegPlayer::SetCurrentFilePath(const QString& filePath)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_currentFilePath = filePath;
}

QString BaseFFmpegPlayer::GetCurrentFilePath() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_currentFilePath;
}

void BaseFFmpegPlayer::SetDuration(double duration)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_duration = duration;
}

void BaseFFmpegPlayer::RecordPlayStartTime(double startPosition)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_startTime = startPosition;
    m_playStartTimePoint = std::chrono::steady_clock::now();
}

double BaseFFmpegPlayer::CalculateCurrentPosition() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (!m_playState.IsPlaying())
    {
        return m_startTime;
    }

    // 计算当前播放位置：开始位置 + 已播放时间
    auto now = std::chrono::steady_clock::now();
    auto elapsedSeconds = std::chrono::duration<double>(now - m_playStartTimePoint).count();
    double currentPos = m_startTime + elapsedSeconds;

    // 确保不超过总时长
    return std::min(currentPos, m_duration);
}

void BaseFFmpegPlayer::ResetPlayerState()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 强制停止当前活动
    ForceStop();

    // 重置所有状态
    m_playState.Reset();
    m_startTime = 0.0;

    // 清空文件路径和时长
    m_currentFilePath.clear();
    m_duration = 0.0;
    m_startTime = 0.0;

    LOG_INFO("Player state reset completed");
}

void BaseFFmpegPlayer::ForceStop()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 强制设置状态为停止
    m_playState.TransitionTo(AVPlayState::Stopped);

    LOG_INFO("Force stop completed");
}
