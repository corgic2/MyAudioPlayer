#include "VideoFFmpegUtils.h"
#include <QDebug>
#include <QMutexLocker>
#include <QThread>
#include "AVFileSystem.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"
#include "VideoWidget/PlayerVideoModuleWidget.h"

VideoFFmpegUtils::VideoFFmpegUtils(QObject* parent)
    : BaseFFmpegUtils(parent), m_playState(EM_VideoPlayState::Stopped), m_recordState(EM_VideoRecordState::Stopped),m_pPlayWorker(nullptr), m_pRecordWorker(nullptr), m_currentTime(0.0), m_pVideoDisplayWidget(nullptr)
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
    if (videoPath.isEmpty())
    {
        LOG_WARN("VideoFFmpegUtils::StartPlay() : Invalid video file path");
        emit SigError("无效的视频文件路径");
        return;
    }

    // 检查文件是否存在
    if (!my_sdk::FileSystem::Exists(videoPath.toStdString()))
    {
        LOG_WARN("VideoFFmpegUtils::StartPlay() : Video file does not exist: " + videoPath.toStdString());
        emit SigError("视频文件不存在");
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
    if (m_playState != EM_VideoPlayState::Stopped)
    {
        StopPlay();
    }

    try
    {
        // 创建播放线程和工作对象
        m_pPlayWorker = new VideoPlayWorker();

        // 连接信号槽
        connect(m_pPlayWorker, &VideoPlayWorker::SigPlayStateChanged, this, &VideoFFmpegUtils::SigPlayStateChanged);
        connect(m_pPlayWorker, &VideoPlayWorker::SigPlayProgressUpdated, this, &VideoFFmpegUtils::SigPlayProgressUpdated);
        connect(m_pPlayWorker, &VideoPlayWorker::SigFrameDataUpdated, this, &VideoFFmpegUtils::SlotHandleFrameUpdate);
        connect(m_pPlayWorker, &VideoPlayWorker::SigError, this, &VideoFFmpegUtils::SigError);

        connect(this, &VideoFFmpegUtils::destroyed, m_pPlayWorker, &VideoPlayWorker::deleteLater);

        // 初始化播放器 - 不传入SDL参数，仅使用Qt显示
        if (!m_pPlayWorker->InitPlayer(videoPath, nullptr, nullptr))
        {
            LOG_WARN("VideoFFmpegUtils::StartPlay() : Failed to initialize player");
            emit SigError("播放器初始化失败");

            // 清理资源
            SAFE_DELETE_POINTER_VALUE(m_pPlayWorker);
            return;
        }

        // 获取视频信息
        m_videoInfo = m_pPlayWorker->GetVideoInfo();

        m_pPlayWorker->SlotStartPlay();
        m_playState = EM_VideoPlayState::Playing;
        emit SigPlayStateChanged(m_playState);

        LOG_INFO("Video playback started successfully: " + videoPath.toStdString());
    } catch (const std::exception& e)
    {
        LOG_WARN("Exception in StartPlay: " + std::string(e.what()));
        emit SigError("播放启动异常");
    }
}

void VideoFFmpegUtils::PausePlay()
{
    if (m_playState == EM_VideoPlayState::Playing && m_pPlayWorker)
    {
        m_pPlayWorker->SlotPausePlay();
    }
}

void VideoFFmpegUtils::ResumePlay()
{
    if (m_playState == EM_VideoPlayState::Paused && m_pPlayWorker)
    {
        m_pPlayWorker->SlotResumePlay();
    }
}

void VideoFFmpegUtils::StopPlay()
{
    if (m_playState != EM_VideoPlayState::Stopped)
    {
        if (m_pPlayWorker)
        {
            m_pPlayWorker->SlotStopPlay();
        }
        m_pPlayWorker->deleteLater();
        m_playState = EM_VideoPlayState::Stopped;
        emit SigPlayStateChanged(m_playState);

        LOG_INFO("Video playback stopped");
    }
}

void VideoFFmpegUtils::StartRecording(const QString& outputPath)
{
    if (outputPath.isEmpty())
    {
        LOG_WARN("VideoFFmpegUtils::StartRecording() : Invalid output file path");
        emit SigError("无效的输出文件路径");
        return;
    }

    if (m_recordState != EM_VideoRecordState::Stopped)
    {
        StopRecording();
    }

    try
    {
        // 创建录制线程和工作对象
        m_pRecordWorker = new VideoRecordWorker();

        // 连接录制信号槽
        connect(m_pRecordWorker, &VideoRecordWorker::SigRecordStateChanged, this, &VideoFFmpegUtils::SigRecordStateChanged);
        connect(m_pRecordWorker, &VideoRecordWorker::SigError, this, &VideoFFmpegUtils::SigError);
        connect(this, &VideoFFmpegUtils::destroyed, m_pRecordWorker, &VideoRecordWorker::deleteLater);

        // 启动录制线程
        m_pRecordWorker->SlotStartRecord(outputPath);
        m_recordState = EM_VideoRecordState::Recording;
        emit SigRecordStateChanged(m_recordState);

        LOG_INFO("Video recording started: " + outputPath.toStdString());
    } catch (const std::exception& e)
    {
        LOG_WARN("Exception in StartRecording: " + std::string(e.what()));
        emit SigError("录制启动异常");
    }
}

void VideoFFmpegUtils::StopRecording()
{
    if (m_recordState != EM_VideoRecordState::Stopped)
    {
        if (m_pRecordWorker)
        {
            m_pRecordWorker->SlotStopRecord();
        }
        SAFE_DELETE_POINTER_VALUE(m_pRecordWorker);
        m_recordState = EM_VideoRecordState::Stopped;
        emit SigRecordStateChanged(m_recordState);

        LOG_INFO("Video recording stopped");
    }
}

void VideoFFmpegUtils::SeekPlay(double seconds)
{
    if (m_pPlayWorker && m_playState != EM_VideoPlayState::Stopped)
    {
        m_pPlayWorker->SlotSeekPlay(seconds);
        LOG_INFO("Video seek to: " + std::to_string(seconds) + " seconds");
    }
}

bool VideoFFmpegUtils::IsPlaying()
{
    return m_playState == EM_VideoPlayState::Playing;
}

bool VideoFFmpegUtils::IsPaused()
{
    return m_playState == EM_VideoPlayState::Paused;
}

bool VideoFFmpegUtils::IsRecording()
{
    return m_recordState == EM_VideoRecordState::Recording;
}

double VideoFFmpegUtils::GetCurrentPosition() const
{
    return m_currentTime;
}

double VideoFFmpegUtils::GetDuration() const
{
    return m_videoInfo.m_duration;
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
