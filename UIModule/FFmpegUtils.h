#pragma once

#include <QDebug>
#include <QObject>
#include <QString>
#include "FFmpegAudioDataDefine.h"
#include "SDL3/SDL_stdinc.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswresample/swresample.h"
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
}

class FFmpegUtils : public QObject
{
    Q_OBJECT

public:
    FFmpegUtils(QObject* parent = Q_NULLPTR);
    ~FFmpegUtils() override;
    // 注册设备
    static void ResigsterDevice();

public:
    /// <summary>
    /// 开始录音
    /// </summary>
    /// <param name="outputFilePath">输出文件路径</param>
    /// <param name="encoderFormat">编码格式</param>
    void StartAudioRecording(const QString& outputFilePath, const QString& encoderFormat);

    /// <summary>
    /// 停止录音
    /// </summary>
    void StopAudioRecording();

    /// <summary>
    /// 播放音频文件
    /// </summary>
    /// <param name="inputFilePath">输入文件路径</param>
    /// <param name="args">播放参数</param>
    void StartAudioPlayback(const QString& inputFilePath, const QStringList& args = QStringList());

    /// <summary>
    /// 暂停音频播放
    /// </summary>
    void PauseAudioPlayback();

    /// <summary>
    /// 继续音频播放
    /// </summary>
    void ResumeAudioPlayback();

    /// <summary>
    /// 停止音频播放
    /// </summary>
    void StopAudioPlayback();

    /// <summary>
    /// 音频快进
    /// </summary>
    /// <param name="seconds">快进秒数</param>
    void SeekAudioForward(int seconds);

    /// <summary>
    /// 音频快退
    /// </summary>
    /// <param name="seconds">快退秒数</param>
    void SeekAudioBackward(int seconds);

    /// <summary>
    /// 获取所有可用的音频输入设备（FFmpeg设备）
    /// </summary>
    /// <returns>返回设备名称列表</returns>
    QStringList GetInputAudioDevices();

    /// <summary>
    /// 设置当前使用的输入设备（FFmpeg设备）
    /// </summary>
    void SetInputDevice(const QString& deviceName);

    /// <summary>
    /// 获取当前播放状态
    /// </summary>
    /// <returns>true表示正在播放，false表示已停止</returns>
    bool IsPlaying() const { return m_playState.m_isPlaying; }

    /// <summary>
    /// 获取当前暂停状态
    /// </summary>
    /// <returns>true表示已暂停，false表示未暂停</returns>
    bool IsPaused() const { return m_playState.m_isPaused; }

signals:
    /// <summary>
    /// 播放状态改变信号
    /// </summary>
    /// <param name="isPlaying">是否正在播放</param>
    void SigPlayStateChanged(bool isPlaying);

    /// <summary>
    /// 播放进度改变信号
    /// </summary>
    /// <param name="position">当前位置（秒）</param>
    /// <param name="duration">总时长（秒）</param>
    void SigProgressChanged(qint64 position, qint64 duration);

private:
    /// <summary>
    /// 编码或解码 ffmpeg
    /// </summary>
    /// <param name="inputFilePath">inputFilePath 表示需要修改的文件路径</param>
    /// <param name="outputFilePath">outputFilePath 表示输出的文件路径</param>
    /// <param name="bEncoder">bEncoder 表示是否编码 true表示编码，false表示解码</param>
    /// <param name="args">args 表示ffmpeg的参数</param>
    void EncoderOrDecoder(const QString& inputFilePath, const QString& outputFilePath, bool bEncoder, const QStringList& args = QStringList());
    /// <summary>
    /// 查看音视频的参数信息 ffprobe
    /// </summary>
    /// <param name="inputFilePath">inputFilePath 表示需要查看的文件路径</param>
    /// <param name="args">args 表示ffprobe的参数</param>
    /// <returns></returns>
    QString GetFileInfomation(const QString& inputFilePath, const QStringList& args = QStringList());
    /// <summary>
    /// 打开设备
    /// </summary>
    /// <param name="devieceFormat"></param>
    /// <param name="deviceName"></param>
    /// <returns></returns>
    std::unique_ptr<ST_OpenAudioDevice> OpenDevice(const QString& devieceFormat, const QString& deviceName, bool bAudio = true);
    /// <summary>
    /// 显示录音设备的参数S
    /// </summary>
    /// <param name="ctx"></param>
    void ShowSpec(AVFormatContext* ctx);
    /// <summary>
    /// 音频重采样
    /// </summary>
    void ResampleAudio(const uint8_t* input, size_t input_size, ST_ResampleResult& output, const ST_ResampleParams& params);

private:
    QString m_currentInputDevice;  // 当前选择的FFmpeg输入设备
    SDL_AudioDeviceID m_currentOutputDevice; // 当前选择的SDL输出设备
    ST_AudioPlayState m_playState;  /// 播放状态
    std::unique_ptr<ST_AudioPlayInfo> m_playInfo; // 播放信息
    std::unique_ptr<ST_OpenAudioDevice> m_recordDevice; // 录制设备
};
