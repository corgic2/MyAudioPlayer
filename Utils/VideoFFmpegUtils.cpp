#include "VideoFFmpegUtils.h"
#include <QDebug>
#include <QMutexLocker>
#include <QThread>
#include "AVFileSystem.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "VideoWidget/PlayerVideoModuleWidget.h"

VideoFFmpegUtils::VideoFFmpegUtils(QObject* parent)
    : BaseFFmpegUtils(parent), m_playState(EM_VideoPlayState::Stopped), 
      m_recordState(EM_VideoRecordState::Stopped), m_pPlayThread(nullptr), 
      m_pPlayWorker(nullptr), m_pRecordThread(nullptr), m_pRecordWorker(nullptr),
      m_currentTime(0.0), m_pVideoDisplayWidget(nullptr)
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
    if (!AV_player::AVFileSystem::IsVideoFile(videoPath.toStdString()))
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
        m_pPlayThread = new QThread(this);
        m_pPlayWorker = new VideoPlayWorker();
        m_pPlayWorker->moveToThread(m_pPlayThread);

        // 连接信号槽
        connect(m_pPlayWorker, &VideoPlayWorker::SigPlayStateChanged, 
                this, &VideoFFmpegUtils::SigPlayStateChanged);
        connect(m_pPlayWorker, &VideoPlayWorker::SigPlayProgressUpdated, 
                this, &VideoFFmpegUtils::SigPlayProgressUpdated);
        connect(m_pPlayWorker, &VideoPlayWorker::SigFrameDataUpdated, 
                this, &VideoFFmpegUtils::SlotHandleFrameUpdate);
        connect(m_pPlayWorker, &VideoPlayWorker::SigError, 
                this, &VideoFFmpegUtils::SigError);

        connect(this, &VideoFFmpegUtils::destroyed, m_pPlayWorker, &VideoPlayWorker::deleteLater);
        connect(m_pPlayThread, &QThread::finished, m_pPlayThread, &QThread::deleteLater);

        // 初始化播放器
        if (!m_pPlayWorker->InitPlayer(videoPath, nullptr, nullptr))
        {
            LOG_WARN("VideoFFmpegUtils::StartPlay() : Failed to initialize player");
            emit SigError("播放器初始化失败");

            // 清理资源
            m_pPlayThread->quit();
            m_pPlayThread->wait();
            m_pPlayThread = nullptr;
            m_pPlayWorker = nullptr;
            return;
        }

        // 获取视频信息
        m_videoInfo = m_pPlayWorker->GetVideoInfo();

        // 启动线程
        m_pPlayThread->start();

        // 开始播放
        QMetaObject::invokeMethod(m_pPlayWorker, "SlotStartPlay", Qt::QueuedConnection);

        // 处理跳转
        if (startPosition > 0.0)
        {
            QMetaObject::invokeMethod(m_pPlayWorker, "SlotSeekPlay", 
                                    Qt::QueuedConnection, Q_ARG(double, startPosition));
        }

        m_playState = EM_VideoPlayState::Playing;
        emit SigPlayStateChanged(m_playState);

        LOG_INFO("Video playback started successfully: " + videoPath.toStdString());
    } 
    catch (const std::exception& e)
    {
        LOG_WARN("Exception in StartPlay: " + std::string(e.what()));
        emit SigError("播放启动异常");
    }
}

void VideoFFmpegUtils::PausePlay()
{
    if (m_playState == EM_VideoPlayState::Playing && m_pPlayWorker)
    {
        QMetaObject::invokeMethod(m_pPlayWorker, "SlotPausePlay", Qt::QueuedConnection);
        m_playState = EM_VideoPlayState::Paused;
        emit SigPlayStateChanged(m_playState);
    }
}

void VideoFFmpegUtils::ResumePlay()
{
    if (m_playState == EM_VideoPlayState::Paused && m_pPlayWorker)
    {
        QMetaObject::invokeMethod(m_pPlayWorker, "SlotResumePlay", Qt::QueuedConnection);
        m_playState = EM_VideoPlayState::Playing;
        emit SigPlayStateChanged(m_playState);
    }
}

void VideoFFmpegUtils::StopPlay()
{
    if (m_playState != EM_VideoPlayState::Stopped)
    {
        // 停止播放线程
        if (m_pPlayWorker)
        {
            QMetaObject::invokeMethod(m_pPlayWorker, "SlotStopPlay", Qt::QueuedConnection);
        }

        if (m_pPlayThread)
        {
            m_pPlayThread->quit();
            m_pPlayThread->wait(3000); // 等待最多3秒
            if (m_pPlayThread->isRunning())
            {
                m_pPlayThread->terminate();
                m_pPlayThread->wait(1000);
            }
            delete m_pPlayThread;
            m_pPlayThread = nullptr;
        }

        m_pPlayWorker = nullptr;
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
        m_pRecordThread = new QThread(this);
        m_pRecordWorker = new VideoRecordWorker();
        m_pRecordWorker->moveToThread(m_pRecordThread);

        // 连接录制信号槽
        connect(m_pRecordWorker, &VideoRecordWorker::SigRecordStateChanged, 
                this, &VideoFFmpegUtils::SigRecordStateChanged);
        connect(m_pRecordWorker, &VideoRecordWorker::SigError, 
                this, &VideoFFmpegUtils::SigError);
        connect(this, &VideoFFmpegUtils::destroyed, m_pRecordWorker, &VideoRecordWorker::deleteLater);
        connect(m_pRecordThread, &QThread::finished, m_pRecordThread, &QThread::deleteLater);

        // 启动录制线程
        m_pRecordThread->start();

        // 开始录制
        QMetaObject::invokeMethod(m_pRecordWorker, "SlotStartRecord", 
                                Qt::QueuedConnection, Q_ARG(QString, outputPath));

        m_recordState = EM_VideoRecordState::Recording;
        emit SigRecordStateChanged(m_recordState);

        LOG_INFO("Video recording started: " + outputPath.toStdString());
    }
    catch (const std::exception& e)
    {
        LOG_WARN("Exception in StartRecording: " + std::string(e.what()));
        emit SigError("录制启动异常");
    }
}

void VideoFFmpegUtils::StopRecording()
{
    if (m_recordState != EM_VideoRecordState::Stopped)
    {
        // 停止录制线程
        if (m_pRecordWorker)
        {
            QMetaObject::invokeMethod(m_pRecordWorker, "SlotStopRecord", Qt::QueuedConnection);
        }

        if (m_pRecordThread)
        {
            m_pRecordThread->quit();
            m_pRecordThread->wait(3000);
            if (m_pRecordThread->isRunning())
            {
                m_pRecordThread->terminate();
                m_pRecordThread->wait(1000);
            }
            delete m_pRecordThread;
            m_pRecordThread = nullptr;
        }

        m_pRecordWorker = nullptr;
        m_recordState = EM_VideoRecordState::Stopped;
        emit SigRecordStateChanged(m_recordState);

        LOG_INFO("Video recording stopped");
    }
}

void VideoFFmpegUtils::SeekPlay(double seconds)
{
    if (m_pPlayWorker && m_playState != EM_VideoPlayState::Stopped)
    {
        QMetaObject::invokeMethod(m_pPlayWorker, "SlotSeekPlay", 
                                Qt::QueuedConnection, Q_ARG(double, seconds));
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
