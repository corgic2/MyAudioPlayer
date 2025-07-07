#include "BaseFFmpegUtils.h"
#include "LogSystem/LogSystem.h"
#include "FileSystem/FileSystem.h"

BaseFFmpegUtils::BaseFFmpegUtils(QObject* parent)
    : QObject(parent)
{
    ResetPlayState();
    ResetRecordState();
}

BaseFFmpegUtils::~BaseFFmpegUtils()
{
}

std::unique_ptr<ST_OpenFileResult> BaseFFmpegUtils::OpenMediaFile(const QString& filePath)
{
    if (filePath.isEmpty())
    {
        LOG_WARN("BaseFFmpegUtils::OpenMediaFile() : Empty file path");
        return nullptr;
    }

    if (!my_sdk::FileSystem::Exists(filePath.toStdString()))
    {
        LOG_WARN("BaseFFmpegUtils::OpenMediaFile() : File does not exist: " + filePath.toStdString());
        return nullptr;
    }

    auto openFileResult = std::make_unique<ST_OpenFileResult>();
    openFileResult->OpenFilePath(filePath);
    
    if (!openFileResult->m_formatCtx || !openFileResult->m_formatCtx->GetRawContext())
    {
        LOG_WARN("BaseFFmpegUtils::OpenMediaFile() : Failed to open file: " + filePath.toStdString());
        return nullptr;
    }

    // 设置文件路径和时长
    SetCurrentFilePath(filePath);
    double duration = static_cast<double>(openFileResult->m_formatCtx->GetRawContext()->duration) / AV_TIME_BASE;
    SetDuration(duration);

    LOG_INFO("Successfully opened media file: " + filePath.toStdString() + ", duration: " + std::to_string(duration) + " seconds");
    return openFileResult;
}

void BaseFFmpegUtils::SetPlayState(EM_PlayState state)
{
    EM_PlayState oldState = m_playState.load();
    m_playState.store(state);
    
    if (oldState != state)
    {
        bool isPlaying = (state == EM_PlayState::Playing);
        emit SigPlayStateChanged(isPlaying);
        LOG_INFO("Play state changed from " + std::to_string(static_cast<int>(oldState)) + " to " + std::to_string(static_cast<int>(state)));
    }
}

void BaseFFmpegUtils::SetRecordState(EM_RecordState state)
{
    EM_RecordState oldState = m_recordState.load();
    m_recordState.store(state);
    
    if (oldState != state)
    {
        bool isRecording = (state == EM_RecordState::Recording);
        emit SigRecordStateChanged(isRecording);
        LOG_INFO("Record state changed from " + std::to_string(static_cast<int>(oldState)) + " to " + std::to_string(static_cast<int>(state)));
    }
}

void BaseFFmpegUtils::SetCurrentFilePath(const QString& filePath)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_currentFilePath = filePath;
}

QString BaseFFmpegUtils::GetCurrentFilePath() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_currentFilePath;
}

void BaseFFmpegUtils::SetDuration(double duration)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_duration = duration;
}

void BaseFFmpegUtils::ResetPlayState()
{
    SetPlayState(EM_PlayState::Stopped);
    m_startTime = 0.0;
}

void BaseFFmpegUtils::ResetRecordState()
{
    SetRecordState(EM_RecordState::Stopped);
}

void BaseFFmpegUtils::RecordPlayStartTime(double startPosition)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_startTime = startPosition;
    m_playStartTimePoint = std::chrono::steady_clock::now();
}

double BaseFFmpegUtils::CalculateCurrentPosition() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (m_playState.load() != EM_PlayState::Playing)
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

void BaseFFmpegUtils::ResetPlayerState()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // 强制停止当前活动
    ForceStop();
    
    // 重置所有状态
    ResetPlayState();
    ResetRecordState();
    
    // 清空文件路径和时长
    m_currentFilePath.clear();
    m_duration = 0.0;
    m_startTime = 0.0;
    
    LOG_INFO("Player state reset completed");
}

bool BaseFFmpegUtils::CanReusePlayer() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // 如果没有正在进行的播放或录制，则可以复用
    return m_playState.load() != EM_PlayState::Playing && 
           m_recordState.load() != EM_RecordState::Recording;
}

void BaseFFmpegUtils::ForceStop()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // 强制设置状态为停止
    m_playState.store(EM_PlayState::Stopped);
    m_recordState.store(EM_RecordState::Stopped);
    
    LOG_INFO("Force stop completed");
}

