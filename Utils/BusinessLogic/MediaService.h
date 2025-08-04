#pragma once

#include <functional>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

// 前向声明
class ST_AudioConvertParams;
class ST_VideoConvertParams;

/// <summary>
/// 媒体文件服务类
/// 负责处理文件操作、格式转换等业务逻辑
/// </summary>
class MediaService : public QObject
{
    Q_OBJECT public:
    /// <summary>
    /// 获取单例实例
    /// </summary>
    static MediaService* instance();

    /// <summary>
    /// 析构函数
    /// </summary>
    ~MediaService() override;

    // 文件操作相关接口
    QStringList GetAudioFilesFromDirectory(const QString& directory, bool recursive = false);
    QStringList GetVideoFilesFromDirectory(const QString& directory, bool recursive = false);
    bool IsAudioFile(const QString& filePath);
    bool IsVideoFile(const QString& filePath);
    QString GetFileFilter();
    // 格式转换相关接口
    bool ConvertAudioFormat(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params);
    bool ConvertVideoFormat(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params);

    // 命令构建接口
    QString BuildAudioConvertCommand(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params);
    QString BuildVideoConvertCommand(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params);
    QStringList BuildAudioConvertArguments(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params);
    QStringList BuildVideoConvertArguments(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params);

    // 工具函数
    QString NormalizeFilePath(const QString& filePath);
    QString GetAudioExtension(const QString& format);
    QString GetVideoExtension(const QString& format);

    // 异步操作接口
    void ConvertAudioFormatAsync(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params, std::function<void(bool, const QString&)> callback);
    void ConvertVideoFormatAsync(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params, std::function<void(bool, const QString&)> callback);

signals:
    /// <summary>
    /// 转换进度信号
    /// </summary>
    void SigConversionProgress(const QString& filePath, int progress);

    /// <summary>
    /// 转换完成信号
    /// </summary>
    void SigConversionFinished(const QString& filePath, bool success, const QString& message);

    /// <summary>
    /// 错误信号
    /// </summary>
    void SigErrorOccurred(const QString& error);

private:
    /// <summary>
    /// 私有构造函数（单例模式）
    /// </summary>
    explicit MediaService(QObject* parent = nullptr);

    /// <summary>
    /// 禁用拷贝构造函数
    /// </summary>
    MediaService(const MediaService&) = delete;
    MediaService& operator=(const MediaService&) = delete;

    /// <summary>
    /// 执行FFmpeg命令
    /// </summary>
    bool ExecuteFFmpegCommand(const QString& command);

    /// <summary>
    /// 执行FFmpeg命令（异步）
    /// </summary>
    void ExecuteFFmpegCommandAsync(const QString& command, std::function<void(bool, const QString&)> callback);

private:
    static MediaService* m_instance;

    // 文件扩展名映射
    QMap<QString, QString> m_audioExtensions;
    QMap<QString, QString> m_videoExtensions;
    QStringList m_audioFilePatterns;
    QStringList m_videoFilePatterns;
};

/// <summary>
/// 音频转换参数结构体
/// </summary>
class ST_AudioConvertParams
{
public:
    QString outputFormat;
    QString audioCodec;
    int bitrate;
    int sampleRate;
    int channels;

    ST_AudioConvertParams()
        : outputFormat("mp3"), audioCodec("libmp3lame"), bitrate(128), sampleRate(44100), channels(2)
    {
    }
};

/// <summary>
/// 视频转换参数结构体
/// </summary>
class ST_VideoConvertParams
{
public:
    QString outputFormat;
    QString videoCodec;
    QString audioCodec;
    int videoBitrate;
    int audioBitrate;
    int width;
    int height;
    int frameRate;

    ST_VideoConvertParams()
        : outputFormat("mp4"), videoCodec("libx264"), audioCodec("aac"), videoBitrate(1000), audioBitrate(128), width(1920), height(1080), frameRate(30)
    {
    }
};
