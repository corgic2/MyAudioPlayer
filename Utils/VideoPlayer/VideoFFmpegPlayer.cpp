#include "VideoFFmpegPlayer.h"
#include <QDebug>
#include <QMutexLocker>
#include <QThread>
#include "AVFileSystem.h"
#include "../BasePlayer/FFmpegPublicUtils.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"
#include "VideoWidget/PlayerVideoModuleWidget.h"

VideoFFmpegPlayer::VideoFFmpegPlayer(QObject* parent)
    : BaseFFmpegPlayer(parent)
{
}

VideoFFmpegPlayer::~VideoFFmpegPlayer()
{
    StopPlay();
    StopRecording();
}

void VideoFFmpegPlayer::SetVideoDisplayWidget(PlayerVideoModuleWidget* videoWidget)
{
    m_pVideoDisplayWidget = videoWidget;
}

void VideoFFmpegPlayer::StartPlay(const QString& videoPath, bool bStart, double startPosition, const QStringList& args)
{
    // 使用基类的文件验证功能
    if (!FFmpegPublicUtils::ValidateFilePath(videoPath))
    {
        LOG_WARN("VideoFFmpegPlayer::StartPlay() : Invalid video file path");
        emit SigError("无效的视频文件路径");
        return;
    }

    // 检查是否为支持的视频格式
    if (!av_fileSystem::AVFileSystem::IsVideoFile(videoPath.toStdString()))
    {
        LOG_WARN("VideoFFmpegPlayer::StartPlay() : Unsupported video format: " + videoPath.toStdString());
        emit SigError("不支持的视频格式");
        return;
    }
    // 创建播放线程和工作对象
    m_pPlayWorker = std::make_unique<VideoPlayWorker>();

    // 连接信号槽
    connect(m_pPlayWorker.get(), &VideoPlayWorker::SigPlayProgressUpdated, this, &VideoFFmpegPlayer::SigPlayProgressUpdated);
    connect(m_pPlayWorker.get(), &VideoPlayWorker::SigFrameDataUpdated, this, &VideoFFmpegPlayer::SlotHandleFrameUpdate, Qt::QueuedConnection);
    connect(m_pPlayWorker.get(), &VideoPlayWorker::SigError, this, &VideoFFmpegPlayer::SigError);

    connect(this, &VideoFFmpegPlayer::destroyed, m_pPlayWorker.get(), &VideoPlayWorker::deleteLater);

    // 初始化播放器 - 不传入SDL参数，仅使用Qt显示
    if (!m_pPlayWorker->InitPlayer(videoPath, nullptr, nullptr))
    {
        LOG_WARN("VideoFFmpegPlayer::StartPlay() : Failed to initialize player");
        emit SigError("播放器初始化失败");
        // 清理资源
        m_pPlayWorker.reset();
        return;
    }

    // 获取视频信息并设置到基类
    m_videoInfo = m_pPlayWorker->GetVideoInfo();
    SetCurrentFilePath(videoPath);
    SetDuration(m_videoInfo.m_duration);

    // 记录播放开始时间
    RecordPlayStartTime(startPosition);

    m_playState.TransitionTo(AVPlayState::Playing);
    m_pPlayWorker->SlotStartPlay();

    LOG_INFO("Video playback started successfully: " + videoPath.toStdString());
}

void VideoFFmpegPlayer::PausePlay()
{
    if (IsPlaying() && m_pPlayWorker)
    {
        m_pPlayWorker->SlotPausePlay();
        m_playState.TransitionTo(AVPlayState::Paused);
    }
}

void VideoFFmpegPlayer::ResumePlay()
{
    if (IsPaused() && m_pPlayWorker)
    {
        m_pPlayWorker->SlotResumePlay();
        m_playState.TransitionTo(AVPlayState::Playing);
    }
}

void VideoFFmpegPlayer::StopPlay()
{
    if (!IsPlaying() && !IsPaused())
    {
        return;
    }

    if (m_pPlayWorker)
    {
        m_pPlayWorker->SlotStopPlay();
        m_pPlayWorker.reset();
        SDL_Delay(20);
    }

    m_playState.TransitionTo(AVPlayState::Stopped);
    LOG_INFO("Video playback stopped");
}

void VideoFFmpegPlayer::StartRecording(const QString& outputPath)
{
    if (outputPath.isEmpty())
    {
        LOG_WARN("VideoFFmpegPlayer::StartRecording() : Invalid output file path");
        emit SigError("无效的输出文件路径");
        return;
    }

    if (IsRecording())
    {
        StopRecording();
    }

    // 创建录制线程和工作对象
    m_pRecordWorker = std::make_unique<VideoRecordWorker>();

    // 连接录制信号槽
    connect(m_pRecordWorker.get(), &VideoRecordWorker::SigError, this, &VideoFFmpegPlayer::SigError);
    connect(this, &VideoFFmpegPlayer::destroyed, m_pRecordWorker.get(), &VideoRecordWorker::deleteLater);

    // 启动录制线程
    m_pRecordWorker->SlotStartRecord(outputPath);
    m_playState.TransitionTo(AVPlayState::Recording);

    LOG_INFO("Video recording started: " + outputPath.toStdString());
}

void VideoFFmpegPlayer::StopRecording()
{
    if (!IsRecording())
    {
        return;
    }

    if (m_pRecordWorker)
    {
        m_pRecordWorker->SlotStopRecord();
        m_pRecordWorker.reset();
    }

    m_playState.TransitionTo(AVPlayState::Stopped);

    LOG_INFO("Video recording stopped");
}

void VideoFFmpegPlayer::SeekPlay(double seconds)
{
    if (m_pPlayWorker && (IsPlaying() || IsPaused()))
    {
        RecordPlayStartTime(seconds);
        m_pPlayWorker->SlotSeekPlay(seconds);
        LOG_INFO("Video seek to: " + std::to_string(seconds) + " seconds");
    }
}

double VideoFFmpegPlayer::GetCurrentPosition()
{
    // 使用基类的计算方法
    return CalculateCurrentPosition();
}

ST_VideoFrameInfo VideoFFmpegPlayer::GetVideoInfo() const
{
    return m_videoInfo;
}

void VideoFFmpegPlayer::SlotHandleFrameUpdate(std::vector<uint8_t> frameData, int width, int height)
{
    // 将帧数据传递给显示控件
    if (m_pVideoDisplayWidget)
    {
        m_pVideoDisplayWidget->SetVideoFrame(frameData.data(), width, height);
    }

    // 同时发送信号供其他模块使用
    emit SigFrameUpdated(frameData.data(), width, height);
}

void VideoFFmpegPlayer::ResetPlayerState()
{
    LOG_INFO("Resetting VideoFFmpegPlayer player state");

    // 强制停止播放和录制
    ForceStop();

    // 清理播放工作对象
    if (m_pPlayWorker)
    {
        m_pPlayWorker->Cleanup();
        m_pPlayWorker.reset();
    }

    // 清理录制工作对象
    if (m_pRecordWorker)
    {
        m_pRecordWorker.reset();
    }

    // 重置视频信息
    m_videoInfo = ST_VideoFrameInfo();

    // 调用基类的重置方法
    BaseFFmpegPlayer::ResetPlayerState();

    LOG_INFO("VideoFFmpegPlayer player state reset completed");
}

void VideoFFmpegPlayer::ForceStop()
{
    LOG_INFO("Force stopping VideoFFmpegPlayer");

    // 强制停止播放工作对象
    if (m_pPlayWorker)
    {
        m_pPlayWorker->SlotStopPlay();
    }

    // 强制停止录制工作对象
    if (m_pRecordWorker)
    {
        m_pRecordWorker->SlotStopRecord();
    }

    // 调用基类的强制停止
    BaseFFmpegPlayer::ForceStop();

    LOG_INFO("VideoFFmpegPlayer force stop completed");
}
