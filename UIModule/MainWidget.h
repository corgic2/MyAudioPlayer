#pragma once

#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include "ui_MainWidget.h"
#include "Utils/QtCustomAPI.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class MainWidgetClass;
};

QT_END_NAMESPACE

/// <summary>
/// 音频转换参数结构体
/// </summary>
struct ST_AudioConvertParams
{
    /// <summary>
    /// 输出格式
    /// </summary>
    QString outputFormat;
    
    /// <summary>
    /// 音频编码器
    /// </summary>
    QString audioCodec;
    
    /// <summary>
    /// 比特率
    /// </summary>
    int bitrate;
    
    /// <summary>
    /// 采样率
    /// </summary>
    int sampleRate;
    
    /// <summary>
    /// 声道数
    /// </summary>
    int channels;

    /// <summary>
    /// 构造函数
    /// </summary>
    ST_AudioConvertParams()
        : outputFormat("mp3")
        , audioCodec("libmp3lame")
        , bitrate(128)
        , sampleRate(44100)
        , channels(2)
    {
    }
};

/// <summary>
/// 视频转换参数结构体
/// </summary>
struct ST_VideoConvertParams
{
    /// <summary>
    /// 输出格式
    /// </summary>
    QString outputFormat;
    
    /// <summary>
    /// 视频编码器
    /// </summary>
    QString videoCodec;
    
    /// <summary>
    /// 音频编码器
    /// </summary>
    QString audioCodec;
    
    /// <summary>
    /// 视频比特率
    /// </summary>
    int videoBitrate;
    
    /// <summary>
    /// 音频比特率
    /// </summary>
    int audioBitrate;
    
    /// <summary>
    /// 分辨率宽度
    /// </summary>
    int width;
    
    /// <summary>
    /// 分辨率高度
    /// </summary>
    int height;
    
    /// <summary>
    /// 帧率
    /// </summary>
    int frameRate;

    /// <summary>
    /// 构造函数
    /// </summary>
    ST_VideoConvertParams()
        : outputFormat("mp4")
        , videoCodec("libx264")
        , audioCodec("aac")
        , videoBitrate(1000)
        , audioBitrate(128)
        , width(1920)
        , height(1080)
        , frameRate(30)
    {
    }
};

/// <summary>
/// 音视频播放器主窗口类
/// </summary>
class MainWidget : public QMainWindow
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit MainWidget(QWidget* parent = nullptr);
    
    /// <summary>
    /// 析构函数
    /// </summary>
    ~MainWidget() override;

protected slots:
    /// <summary>
    /// 打开音视频文件槽函数
    /// </summary>
    void SlotOpenAVFile();

    /// <summary>
    /// 打开音视频文件夹槽函数
    /// </summary>
    void SlotOpenAVFolder();

    /// <summary>
    /// 清空文件列表槽函数
    /// </summary>
    void SlotClearFileList();
    
    /// <summary>
    /// 从列表移除文件槽函数
    /// </summary>
    void SlotRemoveFromList();

    /// <summary>
    /// 音频格式转换槽函数
    /// </summary>
    void SlotConvertAudioFormat();

    /// <summary>
    /// 视频格式转换槽函数
    /// </summary>
    void SlotConvertVideoFormat();

private:
    /// <summary>
    /// 初始化菜单栏
    /// </summary>
    void InitializeMenuBar();

    /// <summary>
    /// 初始化工具栏
    /// </summary>
    void InitializeToolBar();

    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();

    /// <summary>
    /// 创建音频转换参数对话框
    /// </summary>
    /// <param name="params">音频转换参数</param>
    /// <returns>是否确认转换</returns>
    bool CreateAudioConvertDialog(ST_AudioConvertParams& params);

    /// <summary>
    /// 创建视频转换参数对话框
    /// </summary>
    /// <param name="params">视频转换参数</param>
    /// <returns>是否确认转换</returns>
    bool CreateVideoConvertDialog(ST_VideoConvertParams& params);

    /// <summary>
    /// 转换为MP3格式
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="bitrate">比特率</param>
    /// <returns>FFmpeg命令行</returns>
    QString BuildMp3ConvertCommand(const QString& inputFile, const QString& outputFile, int bitrate = 128);

    /// <summary>
    /// 转换为WAV格式
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="sampleRate">采样率</param>
    /// <param name="channels">声道数</param>
    /// <returns>FFmpeg命令行</returns>
    QString BuildWavConvertCommand(const QString& inputFile, const QString& outputFile, int sampleRate = 44100, int channels = 2);

    /// <summary>
    /// 转换为AAC格式
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="bitrate">比特率</param>
    /// <returns>FFmpeg命令行</returns>
    QString BuildAacConvertCommand(const QString& inputFile, const QString& outputFile, int bitrate = 128);

    /// <summary>
    /// 转换为FLAC格式
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <returns>FFmpeg命令行</returns>
    QString BuildFlacConvertCommand(const QString& inputFile, const QString& outputFile);

    /// <summary>
    /// 转换为MP4格式
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="videoBitrate">视频比特率</param>
    /// <param name="audioBitrate">音频比特率</param>
    /// <returns>FFmpeg命令行</returns>
    QString BuildMp4ConvertCommand(const QString& inputFile, const QString& outputFile, int videoBitrate = 1000, int audioBitrate = 128);

    /// <summary>
    /// 转换为AVI格式
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="videoBitrate">视频比特率</param>
    /// <param name="audioBitrate">音频比特率</param>
    /// <returns>FFmpeg命令行</returns>
    QString BuildAviConvertCommand(const QString& inputFile, const QString& outputFile, int videoBitrate = 1000, int audioBitrate = 128);

    /// <summary>
    /// 构建指定格式的音频转换命令
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="params">转换参数</param>
    /// <returns>FFmpeg命令行</returns>
    QString BuildAudioConvertCommand(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params);

    /// <summary>
    /// 构建指定格式的视频转换命令
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="params">转换参数</param>
    /// <returns>FFmpeg命令行</returns>
    QString BuildVideoConvertCommand(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params);

    /// <summary>
    /// 构建音频转换参数列表
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="params">转换参数</param>
    /// <returns>FFmpeg参数列表</returns>
    QStringList BuildAudioConvertArguments(const QString& inputFile, const QString& outputFile, const ST_AudioConvertParams& params);

    /// <summary>
    /// 构建视频转换参数列表
    /// </summary>
    /// <param name="inputFile">输入文件</param>
    /// <param name="outputFile">输出文件</param>
    /// <param name="params">转换参数</param>
    /// <returns>FFmpeg参数列表</returns>
    QStringList BuildVideoConvertArguments(const QString& inputFile, const QString& outputFile, const ST_VideoConvertParams& params);

    /// <summary>
    /// 规范化文件路径（使用正斜杠）
    /// </summary>
    /// <param name="filePath">原始文件路径</param>
    /// <returns>规范化后的文件路径</returns>
    QString NormalizeFilePath(const QString& filePath);

    /// <summary>
    /// 获取音频格式扩展名
    /// </summary>
    /// <param name="format">格式名称</param>
    /// <returns>扩展名</returns>
    QString GetAudioExtension(const QString& format);

    /// <summary>
    /// 获取视频格式扩展名
    /// </summary>
    /// <param name="format">格式名称</param>
    /// <returns>扩展名</returns>
    QString GetVideoExtension(const QString& format);

private:
    /// <summary>
    /// UI界面指针
    /// </summary>
    Ui::MainWidgetClass* ui = nullptr;
    
    /// <summary>
    /// 当前音视频文件路径
    /// </summary>
    QString m_currentAVFile;
    
    /// <summary>
    /// Qt自定义API对象指针
    /// </summary>
    QtCustomAPI* m_qtCustomAPI = nullptr;
};
