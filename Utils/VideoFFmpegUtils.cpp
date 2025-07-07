#include "VideoFFmpegUtils.h"
#include <QDebug>
#include <QMutexLocker>
#include <QThread>
#include "AVFileSystem.h"
#include "FFmpegPublicUtils.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"
#include "VideoWidget/PlayerVideoModuleWidget.h"

VideoFFmpegUtils::VideoFFmpegUtils(QObject* parent)
    : BaseFFmpegUtils(parent), m_pPlayWorker(nullptr), m_pRecordWorker(nullptr), m_pVideoDisplayWidget(nullptr)
{
}

VideoFFmpegUtils::~VideoFFmpegUtils()
{
    StopPlay();
    StopRecording();
}

void VideoFFmpegUtils::SetVideoDisplayWidget(PlayerVideoModuleWidget* videoWidget)
{
    m_pVideoDisplayWidget = videoWidget;
}

void VideoFFmpegUtils::StartPlay(const QString& videoPath, double startPosition, const QStringList& args)
{
    // 使用基类的文件验证功能
    if (!FFmpegPublicUtils::ValidateFilePath(videoPath))
    {
        LOG_WARN("VideoFFmpegUtils::StartPlay() : Invalid video file path");
        emit SigError("无效的视频文件路径");
        return;
    }

    // 检查是否为支持的视频格式
    if (!av_fileSystem::AVFileSystem::IsVideoFile(videoPath.toStdString()))
    {
        LOG_WARN("VideoFFmpegUtils::StartPlay() : Unsupported video format: " + videoPath.toStdString());
        emit SigError("不支持的视频格式");
        return;
    }

    // 停止当前播放
    if (IsPlaying())
    {
        StopPlay();
    }

    // 创建播放线程和工作对象
    m_pPlayWorker = std::make_unique<VideoPlayWorker>();

    // 连接信号槽
    connect(m_pPlayWorker.get(), &VideoPlayWorker::SigPlayStateChanged, this, &VideoFFmpegUtils::SigPlayStateChanged);
    connect(m_pPlayWorker.get(), &VideoPlayWorker::SigPlayProgressUpdated, this, &VideoFFmpegUtils::SigPlayProgressUpdated);
    connect(m_pPlayWorker.get(), &VideoPlayWorker::SigFrameDataUpdated, this, &VideoFFmpegUtils::SlotHandleFrameUpdate);
    connect(m_pPlayWorker.get(), &VideoPlayWorker::SigError, this, &VideoFFmpegUtils::SigError);

    connect(this, &VideoFFmpegUtils::destroyed, m_pPlayWorker.get(), &VideoPlayWorker::deleteLater);

    // 初始化播放器 - 不传入SDL参数，仅使用Qt显示
    if (!m_pPlayWorker->InitPlayer(videoPath, nullptr, nullptr))
    {
        LOG_WARN("VideoFFmpegUtils::StartPlay() : Failed to initialize player");
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
    
    m_pPlayWorker->SlotStartPlay();
    SetPlayState(EM_PlayState::Playing);
    emit SigPlayStateChanged(EM_VideoPlayState::Playing);

    LOG_INFO("Video playback started successfully: " + videoPath.toStdString());
}

void VideoFFmpegUtils::PausePlay()
{
    if (IsPlaying() && m_pPlayWorker)
    {
        m_pPlayWorker->SlotPausePlay();
        SetPlayState(EM_PlayState::Paused);
    }
}

void VideoFFmpegUtils::ResumePlay()
{
    if (IsPaused() && m_pPlayWorker)
    {
        m_pPlayWorker->SlotResumePlay();
        SetPlayState(EM_PlayState::Playing);
    }
}

void VideoFFmpegUtils::StopPlay()
{
    if (!IsPlaying() && !IsPaused())
    {
        return;
    }

    if (m_pPlayWorker)
    {
        m_pPlayWorker->SlotStopPlay();
        m_pPlayWorker.reset();
    }
    
    SetPlayState(EM_PlayState::Stopped);
    emit SigPlayStateChanged(EM_VideoPlayState::Stopped);

    LOG_INFO("Video playback stopped");
}

void VideoFFmpegUtils::StartRecording(const QString& outputPath)
{
    if (outputPath.isEmpty())
    {
        LOG_WARN("VideoFFmpegUtils::StartRecording() : Invalid output file path");
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
    connect(m_pRecordWorker.get(), &VideoRecordWorker::SigRecordStateChanged, this, &VideoFFmpegUtils::SigRecordStateChanged);
    connect(m_pRecordWorker.get(), &VideoRecordWorker::SigError, this, &VideoFFmpegUtils::SigError);
    connect(this, &VideoFFmpegUtils::destroyed, m_pRecordWorker.get(), &VideoRecordWorker::deleteLater);

    // 启动录制线程
    m_pRecordWorker->SlotStartRecord(outputPath);
    SetRecordState(EM_RecordState::Recording);
    emit SigRecordStateChanged(EM_VideoRecordState::Recording);

    LOG_INFO("Video recording started: " + outputPath.toStdString());
}

void VideoFFmpegUtils::StopRecording()
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
    
    SetRecordState(EM_RecordState::Stopped);
    emit SigRecordStateChanged(EM_VideoRecordState::Stopped);

    LOG_INFO("Video recording stopped");
}

void VideoFFmpegUtils::SeekPlay(double seconds)
{
    if (m_pPlayWorker && (IsPlaying() || IsPaused()))
    {
        RecordPlayStartTime(seconds);
        m_pPlayWorker->SlotSeekPlay(seconds);
        LOG_INFO("Video seek to: " + std::to_string(seconds) + " seconds");
    }
}

double VideoFFmpegUtils::GetCurrentPosition() const
{
    // 使用基类的计算方法
    return CalculateCurrentPosition();
}

ST_VideoFrameInfo VideoFFmpegUtils::GetVideoInfo() const
{
    return m_videoInfo;
}

void VideoFFmpegUtils::SlotHandleFrameUpdate(const uint8_t* frameData, int width, int height)
{
    // 将帧数据传递给显示控件
    if (m_pVideoDisplayWidget)
    {
        m_pVideoDisplayWidget->SetVideoFrame(frameData, width, height);
    }

    // 同时发送信号供其他模块使用
    emit SigFrameUpdated(frameData, width, height);
}

void VideoFFmpegUtils::ResetPlayerState()
{
    LOG_INFO("Resetting VideoFFmpegUtils player state");
    
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
    BaseFFmpegUtils::ResetPlayerState();
    
    LOG_INFO("VideoFFmpegUtils player state reset completed");
}

void VideoFFmpegUtils::ForceStop()
{
    LOG_INFO("Force stopping VideoFFmpegUtils");
    
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
    BaseFFmpegUtils::ForceStop();
    
    LOG_INFO("VideoFFmpegUtils force stop completed");
}
