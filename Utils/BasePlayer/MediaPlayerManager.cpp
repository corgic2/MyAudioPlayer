#include "MediaPlayerManager.h"
#include <QFileInfo>
#include <QMutexLocker>
#include "AVFileSystem.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#pragma execution_character_set("utf-8")

// 静态成员初始化
MediaPlayerManager* MediaPlayerManager::s_instance = nullptr;
std::mutex MediaPlayerManager::s_mutex;

MediaPlayerManager* MediaPlayerManager::Instance()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_instance == nullptr)
    {
        s_instance = new MediaPlayerManager();
        LOG_INFO("MediaPlayerManager instance created");
    }
    return s_instance;
}

MediaPlayerManager::MediaPlayerManager(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("Initializing MediaPlayerManager");

    // 创建音频和视频播放器实例（但不立即使用）
    m_audioPlayer = std::make_unique<AudioFFmpegPlayer>(this);
    m_videoPlayer = std::make_unique<VideoFFmpegPlayer>(this);

    // 连接信号槽
    ConnectPlayerSignals();

    LOG_INFO("MediaPlayerManager initialized successfully");
}

MediaPlayerManager::~MediaPlayerManager()
{
    LOG_INFO("MediaPlayerManager destructor called");
    StopPlay();
    StopRecording();
}

void MediaPlayerManager::ConnectPlayerSignals()
{
    // 连接音频播放器信号
    if (m_audioPlayer)
    {
        connect(m_audioPlayer.get(), &AudioFFmpegPlayer::SigProgressChanged, this, &MediaPlayerManager::SigProgressChanged);
    }

    // 连接视频播放器信号
    if (m_videoPlayer)
    {
        connect(m_videoPlayer.get(), &VideoFFmpegPlayer::SigError, this, &MediaPlayerManager::SigError);
    }
}

bool MediaPlayerManager::PlayMedia(const QString& filePath, double startPosition, const QStringList& args)
{
    if (filePath.isEmpty())
    {
        LOG_WARN("MediaPlayerManager::PlayMedia() : Empty file path");
        return false;
    }

    // 检测媒体类型
    EM_MediaType mediaType = DetectMediaType(filePath);
    if (mediaType == EM_MediaType::Unknown)
    {
        LOG_WARN("MediaPlayerManager::PlayMedia() : Unknown media type for file: " + filePath.toStdString());
        emit SigError("不支持的媒体文件格式");
        return false;
    }

    LOG_INFO("Playing media file: " + filePath.toStdString() + ", type: " + std::to_string(static_cast<int>(mediaType)));

    // 停止当前播放
    StopCurrentPlayer();

    // 根据媒体类型选择合适的播放器
    bool success = false;
    if (mediaType == EM_MediaType::Audio)
    {
        if (m_audioPlayer)
        {
            m_audioPlayer->StartPlay(filePath, true, startPosition, args);
            success = true;
            m_currentMediaType = EM_MediaType::Audio;
        }
    }
    else if (mediaType == EM_MediaType::Video)
    {
        if (m_videoPlayer)
        {
            m_videoPlayer->StartPlay(filePath, true, startPosition, args);
            success = true;
            m_currentMediaType = EM_MediaType::Video;
        }
    }
    else if (mediaType == EM_MediaType::VideoWithAudio)
    {
        m_currentMediaType = EM_MediaType::VideoWithAudio;
        // 同时启动音频和视频播放器
        if (m_audioPlayer && m_videoPlayer)
        {
            LOG_INFO("Starting audio and video playback simultaneously");
            m_audioPlayer->StartPlay(filePath, true, startPosition, args);
            m_videoPlayer->StartPlay(filePath, true, startPosition, args);
            success = true;
        }
    }

    if (success)
    {
        m_currentFilePath = filePath;
        LOG_INFO("Media playback started successfully");
    }
    else
    {
        LOG_WARN("Failed to start media playback");
        emit SigError("播放失败");
    }

    return success;
}

void MediaPlayerManager::PausePlay()
{
    if (m_currentMediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        m_audioPlayer->PausePlay();
    }
    else if (m_currentMediaType == EM_MediaType::Video && m_videoPlayer)
    {
        m_videoPlayer->PausePlay();
    }
    else if (m_currentMediaType == EM_MediaType::VideoWithAudio)
    {
        // 同时暂停音频和视频
        if (m_audioPlayer)
        {
            m_audioPlayer->PausePlay();
        }
        if (m_videoPlayer)
        {
            m_videoPlayer->PausePlay();
        }
    }
}

void MediaPlayerManager::ResumePlay()
{
    if (m_currentMediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        m_audioPlayer->ResumePlay();
    }
    else if (m_currentMediaType == EM_MediaType::Video && m_videoPlayer)
    {
        m_videoPlayer->ResumePlay();
    }
    else if (m_currentMediaType == EM_MediaType::VideoWithAudio)
    {
        // 同时恢复音频和视频
        if (m_audioPlayer)
        {
            m_audioPlayer->ResumePlay();
        }
        if (m_videoPlayer)
        {
            m_videoPlayer->ResumePlay();
        }
    }
}

void MediaPlayerManager::StopPlay()
{
    StopCurrentPlayer();
    m_currentMediaType = EM_MediaType::Unknown;
    m_currentFilePath.clear();
}

void MediaPlayerManager::SeekPlay(double seconds)
{
    LOG_INFO("MediaPlayerManager::SeekPlay - Seek to " + std::to_string(seconds) + " seconds");
    
    if (m_currentMediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        m_audioPlayer->PausePlay();
        m_audioPlayer->SeekPlay(seconds);
        m_audioPlayer->ResumePlay();
    }
    else if (m_currentMediaType == EM_MediaType::Video && m_videoPlayer)
    {
        m_videoPlayer->PausePlay();
        m_videoPlayer->SeekPlay(seconds);
        m_videoPlayer->ResumePlay();
    }
    else if (m_currentMediaType == EM_MediaType::VideoWithAudio)
    {
        // 先暂停
        if (m_audioPlayer)
        {
            m_audioPlayer->PausePlay();
        }
        if (m_videoPlayer)
        {
            m_videoPlayer->PausePlay();
        }
        if (m_videoPlayer)
        {
            m_videoPlayer->SeekPlay(seconds);
        }
        if (m_audioPlayer)
        {
            m_audioPlayer->SeekPlay(seconds);
        }
        // seek后统一调用ResumePlay，确保时间基准同步
        LOG_INFO("MediaPlayerManager::SeekPlay - Synchronized resume after seek with target: " + std::to_string(seconds));
        if (m_audioPlayer)
        {
            m_audioPlayer->ResumePlay();
        }
        if (m_videoPlayer)
        {
            m_videoPlayer->ResumePlay();
        }
    }
}

void MediaPlayerManager::StartRecording(const QString& outputPath, EM_MediaType mediaType)
{
    if (outputPath.isEmpty())
    {
        LOG_WARN("MediaPlayerManager::StartRecording() : Empty output path");
        return;
    }

    // 停止当前录制
    StopRecording();

    if (mediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        m_audioPlayer->StartRecording(outputPath);
        LOG_INFO("Audio recording started: " + outputPath.toStdString());
    }
    else if (mediaType == EM_MediaType::Video && m_videoPlayer)
    {
        m_videoPlayer->StartRecording(outputPath);
        LOG_INFO("Video recording started: " + outputPath.toStdString());
    }
}

void MediaPlayerManager::StopRecording()
{
    if (m_audioPlayer && m_audioPlayer->IsRecording())
    {
        m_audioPlayer->StopRecording();
    }

    if (m_videoPlayer && m_videoPlayer->IsRecording())
    {
        m_videoPlayer->StopRecording();
    }
}

bool MediaPlayerManager::IsPlaying() const
{
    if (m_currentMediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        return m_audioPlayer->IsPlaying();
    }
    else if (m_currentMediaType == EM_MediaType::Video && m_videoPlayer)
    {
        return m_videoPlayer->IsPlaying();
    }
    else if (m_currentMediaType == EM_MediaType::VideoWithAudio)
    {
        // 音视频同播时，以视频播放状态为准
        if (m_videoPlayer)
        {
            return m_videoPlayer->IsPlaying();
        }
    }

    return false;
}

bool MediaPlayerManager::IsPaused() const
{
    if (m_currentMediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        return m_audioPlayer->IsPaused();
    }
    else if (m_currentMediaType == EM_MediaType::Video && m_videoPlayer)
    {
        return m_videoPlayer->IsPaused();
    }
    else if (m_currentMediaType == EM_MediaType::VideoWithAudio)
    {
        // 音视频同播时，以视频暂停状态为准
        if (m_videoPlayer)
        {
            return m_videoPlayer->IsPaused();
        }
    }

    return false;
}

bool MediaPlayerManager::IsRecording() const
{
    bool audioRecording = m_audioPlayer ? m_audioPlayer->IsRecording() : false;
    bool videoRecording = m_videoPlayer ? m_videoPlayer->IsRecording() : false;

    return audioRecording || videoRecording;
}

double MediaPlayerManager::GetCurrentPosition() const
{
    if (m_currentMediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        return m_audioPlayer->GetCurrentPosition();
    }
    else if (m_currentMediaType == EM_MediaType::Video && m_videoPlayer)
    {
        return m_videoPlayer->GetCurrentPosition();
    }
    else if (m_currentMediaType == EM_MediaType::VideoWithAudio)
    {
        // 音视频同播时，以视频位置为准
        if (m_videoPlayer)
        {
            return m_videoPlayer->GetCurrentPosition();
        }
    }

    return 0.0;
}

double MediaPlayerManager::GetDuration() const
{
    if (m_currentMediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        return m_audioPlayer->GetDuration();
    }
    else if (m_currentMediaType == EM_MediaType::Video && m_videoPlayer)
    {
        return m_videoPlayer->GetDuration();
    }
    else if (m_currentMediaType == EM_MediaType::VideoWithAudio)
    {
        // 音视频同播时，以视频时长为准
        if (m_videoPlayer)
        {
            return m_videoPlayer->GetDuration();
        }
    }

    return 0.0;
}

EM_MediaType MediaPlayerManager::GetCurrentMediaType() const
{
    return m_currentMediaType;
}

QString MediaPlayerManager::GetCurrentFilePath() const
{
    return m_currentFilePath;
}

void MediaPlayerManager::SetVideoDisplayWidget(PlayerVideoModuleWidget* videoWidget)
{
    if (m_videoPlayer)
    {
        m_videoPlayer->SetVideoDisplayWidget(videoWidget);
    }
}

AudioFFmpegPlayer* MediaPlayerManager::GetAudioPlayerPtr()
{
    return m_audioPlayer.get();
}

VideoFFmpegPlayer* MediaPlayerManager::GetVideoPlayerPtr()
{
    return m_videoPlayer.get();
}

EM_MediaType MediaPlayerManager::DetectMediaType(const QString& filePath)
{
    if (filePath.isEmpty())
    {
        return EM_MediaType::Unknown;
    }

    // 使用已有的文件系统检测方法
    std::string stdFilePath = filePath.toStdString();

    // 检查是否为音频文件
    if (av_fileSystem::AVFileSystem::IsAudioFile(stdFilePath))
    {
        return EM_MediaType::Audio;
    }

    // 检查是否为视频文件
    if (av_fileSystem::AVFileSystem::IsVideoFile(stdFilePath))
    {
        // 对于视频文件，进一步检测是否包含音频轨道
        return DetectVideoWithAudio(filePath);
    }

    // 如果上述方法无法检测，则通过文件扩展名检测
    QString extension = QString::fromStdString(my_sdk::FileSystem::GetExtension(stdFilePath)).toLower();

    // 音频格式
    QStringList audioFormats = {"mp3", "wav", "flac", "aac", "ogg", "m4a", "wma", "ape", "pcm"};
    if (audioFormats.contains(extension))
    {
        return EM_MediaType::Audio;
    }

    // 视频格式
    QStringList videoFormats = {"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v", "3gp", "ts", "mts"};
    if (videoFormats.contains(extension))
    {
        // 对于视频格式，进一步检测是否包含音频轨道
        return DetectVideoWithAudio(filePath);
    }

    LOG_WARN("Unknown media type for file: " + stdFilePath + ", extension: " + extension.toStdString());
    return EM_MediaType::Unknown;
}

EM_MediaType MediaPlayerManager::DetectVideoWithAudio(const QString& filePath)
{
    AVFormatContext* formatCtx = nullptr;
    bool hasVideo = false;
    bool hasAudio = false;

    // 打开输入文件
    if (avformat_open_input(&formatCtx, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
    {
        LOG_WARN("Failed to open file for stream detection: " + filePath.toStdString());
        return EM_MediaType::Unknown;
    }

    // 获取流信息
    if (avformat_find_stream_info(formatCtx, nullptr) < 0)
    {
        LOG_WARN("Failed to find stream info: " + filePath.toStdString());
        avformat_close_input(&formatCtx);
        return EM_MediaType::Unknown;
    }

    // 检查所有流
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++)
    {
        AVCodecParameters* codecParams = formatCtx->streams[i]->codecpar;

        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            hasVideo = true;
        }
        else if (codecParams->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            hasAudio = true;
        }
    }

    // 清理资源
    avformat_close_input(&formatCtx);

    // 根据检测结果返回对应的媒体类型
    if (hasVideo && hasAudio)
    {
        LOG_INFO("Detected video file with audio track: " + filePath.toStdString());
        return EM_MediaType::VideoWithAudio;
    }
    else if (hasVideo)
    {
        LOG_INFO("Detected video file without audio track: " + filePath.toStdString());
        return EM_MediaType::Video;
    }
    else if (hasAudio)
    {
        LOG_INFO("Detected audio-only file: " + filePath.toStdString());
        return EM_MediaType::Audio;
    }

    LOG_WARN("No video or audio streams found in file: " + filePath.toStdString());
    return EM_MediaType::Unknown;
}

void MediaPlayerManager::StopCurrentPlayer()
{
    if (m_currentMediaType == EM_MediaType::Audio && m_audioPlayer)
    {
        m_audioPlayer->StopPlay();
    }
    else if (m_currentMediaType == EM_MediaType::Video && m_videoPlayer)
    {
        m_videoPlayer->StopPlay();
    }
    else if (m_currentMediaType == EM_MediaType::VideoWithAudio)
    {
        // 同时停止音频和视频
        if (m_audioPlayer)
        {
            m_audioPlayer->StopPlay();
        }
        if (m_videoPlayer)
        {
            m_videoPlayer->StopPlay();
        }
    }
}
