#include "MediaService.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include "CoreServerGlobal.h"
#include "../AVFileSystem/AVFileSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"
#include "ThreadPool/ThreadPool.h"

MediaService* MediaService::m_instance = nullptr;

MediaService* MediaService::instance()
{
    if (!m_instance)
    {
        m_instance = new MediaService();
    }
    return m_instance;
}

MediaService::MediaService(QObject* parent)
    : QObject(parent)
{
    // 初始化音频文件扩展名映射
    m_audioExtensions = {{"mp3", "mp3"}, {"wav", "wav"}, {"aac", "aac"}, {"flac", "flac"}, {"ogg", "ogg"}, {"m4a", "m4a"}, {"wma", "wma"}};

    // 初始化视频文件扩展名映射
    m_videoExtensions = {{"mp4", "mp4"}, {"avi", "avi"}, {"mkv", "mkv"}, {"mov", "mov"}, {"wmv", "wmv"}, {"flv", "flv"}, {"webm", "webm"}};

    // 初始化文件模式
    m_audioFilePatterns = QStringList{"*.mp3", "*.wav", "*.aac", "*.flac", "*.ogg", "*.m4a", "*.wma"};
    m_videoFilePatterns = QStringList{"*.mp4", "*.avi", "*.mkv", "*.mov", "*.wmv", "*.flv", "*.webm"};
}

MediaService::~MediaService()
{
}

QStringList MediaService::GetAudioFilesFromDirectory(const QString& directory, bool recursive)
{
    QStringList result;

    std::vector<std::string> audioFiles = av_fileSystem::AVFileSystem::GetAudioFiles(my_sdk::FileSystem::QtPathToStdPath(directory.toStdString()), recursive);

    for (const auto& file : audioFiles)
    {
        result << QString::fromStdString(my_sdk::FileSystem::StdPathToQtPath(file));
    }
    return result;
}

QStringList MediaService::GetVideoFilesFromDirectory(const QString& directory, bool recursive)
{
    QStringList result;

    std::vector<std::string> videoFiles = av_fileSystem::AVFileSystem::GetVideoFiles(my_sdk::FileSystem::QtPathToStdPath(directory.toStdString()), recursive);

    for (const auto& file : videoFiles)
    {
        result << QString::fromStdString(my_sdk::FileSystem::StdPathToQtPath(file));
    }

    return result;
}

bool MediaService::IsAudioFile(const QString& filePath)
{
    return av_fileSystem::AVFileSystem::IsAudioFile(filePath.toStdString());
}

bool MediaService::IsVideoFile(const QString& filePath)
{
    return av_fileSystem::AVFileSystem::IsVideoFile(filePath.toStdString());
}

QString MediaService::GetFileFilter()
{
    return QString::fromStdString(av_fileSystem::AVFileSystem::GetAVFileFilter());
}

bool MediaService::ConvertAudioFormat(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params)
{
    QString command = BuildAudioConvertCommand(inputFile, outputFile, params);
    return ExecuteFFmpegCommand(command);
}

bool MediaService::ConvertVideoFormat(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params)
{
    QString command = BuildVideoConvertCommand(inputFile, outputFile, params);
    return ExecuteFFmpegCommand(command);
}

QString MediaService::NormalizeFilePath(const QString& filePath)
{
    return QDir::fromNativeSeparators(filePath);
}

QString MediaService::GetAudioExtension(const QString& format)
{
    return m_audioExtensions.value(format, format);
}

QString MediaService::GetVideoExtension(const QString& format)
{
    return m_videoExtensions.value(format, format);
}

QString MediaService::BuildAudioConvertCommand(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params)
{
    QStringList arguments = BuildAudioConvertArguments(inputFile, outputFile, params);
    return "ffmpeg " + arguments.join(" ");
}

QString MediaService::BuildVideoConvertCommand(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params)
{
    QStringList arguments = BuildVideoConvertArguments(inputFile, outputFile, params);
    return "ffmpeg " + arguments.join(" ");
}

QStringList MediaService::BuildAudioConvertArguments(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params)
{
    QStringList arguments;

    // 输入文件
    arguments << "-i" << NormalizeFilePath(inputFile);

    // 音频编码器
    arguments << "-codec:a";
    if (params.outputFormat == "mp3")
    {
        arguments << "libmp3lame";
    }
    else if (params.outputFormat == "wav")
    {
        arguments << "pcm_s16le";
    }
    else if (params.outputFormat == "aac")
    {
        arguments << "aac";
    }
    else if (params.outputFormat == "flac")
    {
        arguments << "flac";
    }
    else
    {
        arguments << params.audioCodec;
    }

    // 比特率（除了WAV和FLAC无损格式）
    if (params.outputFormat != "wav" && params.outputFormat != "flac")
    {
        arguments << "-b:a" << QString("%1k").arg(params.bitrate);
    }

    // 采样率
    arguments << "-ar" << QString::number(params.sampleRate);

    // 声道数
    arguments << "-ac" << QString::number(params.channels);

    // 只保留音频流
    arguments << "-vn";

    // 覆盖输出文件
    arguments << "-y";

    // 输出文件
    arguments << NormalizeFilePath(outputFile);

    return arguments;
}

QStringList MediaService::BuildVideoConvertArguments(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params)
{
    QStringList arguments;

    // 输入文件
    arguments << "-i" << NormalizeFilePath(inputFile);

    // 视频编码器
    arguments << "-codec:v" << params.videoCodec;

    // 视频比特率
    arguments << "-b:v" << QString("%1k").arg(params.videoBitrate);

    // 分辨率
    if (params.width > 0 && params.height > 0)
    {
        arguments << "-s" << QString("%1x%2").arg(params.width).arg(params.height);
    }

    // 帧率
    if (params.frameRate > 0)
    {
        arguments << "-r" << QString::number(params.frameRate);
    }

    // 音频编码器
    arguments << "-codec:a" << params.audioCodec;

    // 音频比特率
    arguments << "-b:a" << QString("%1k").arg(params.audioBitrate);

    // 覆盖输出文件
    arguments << "-y";

    // 输出文件
    arguments << NormalizeFilePath(outputFile);

    return arguments;
}

bool MediaService::ExecuteFFmpegCommand(const QString& command)
{
    QProcess process;
    process.start(command);

    if (!process.waitForStarted())
    {
        return false;
    }

    if (!process.waitForFinished())
    {
        process.kill();
        return false;
    }

    return process.exitCode() == 0;
}

void MediaService::ConvertAudioFormatAsync(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params, std::function<void(bool, const QString&)> callback)
{
    QString command = BuildAudioConvertCommand(inputFile, outputFile, params);
    ExecuteFFmpegCommandAsync(command, callback);
}

void MediaService::ConvertVideoFormatAsync(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params, std::function<void(bool, const QString&)> callback)
{
    QString command = BuildVideoConvertCommand(inputFile, outputFile, params);
    ExecuteFFmpegCommandAsync(command, callback);
}

void MediaService::ExecuteFFmpegCommandAsync(const QString& command, std::function<void(bool, const QString&)> callback)
{
    CoreServerGlobal::Instance().GetThreadPool().Submit([=]()
    {
        bool success = ExecuteFFmpegCommand(command);
        QString message = success ? "转换完成" : "转换失败";

        if (callback)
        {
            callback(success, message);
        }
    }, EM_TaskPriority::Normal);
}
