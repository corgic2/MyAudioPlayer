#pragma once

#include <memory>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>
#include "DataDefine/ST_AudioPlayInfo.h"
#include "DataDefine/ST_AudioPlayState.h"
#include "DataDefine/ST_OpenAudioDevice.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
}

/// <summary>
/// FFmpeg音频工具类
/// </summary>
class AudioFFmpegUtils : public QObject
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit AudioFFmpegUtils(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~AudioFFmpegUtils() override;

    /// <summary>
    /// 注册设备
    /// </summary>
    static void ResigsterDevice();

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
    /// 获取所有可用的音频输入设备
    /// </summary>
    /// <returns>设备名称列表</returns>
    QStringList GetInputAudioDevices();

    /// <summary>
    /// 设置当前使用的输入设备
    /// </summary>
    /// <param name="deviceName">设备名称</param>
    void SetInputDevice(const QString& deviceName);

    /// <summary>
    /// 加载音频波形数据
    /// </summary>
    /// <param name="filePath">音频文件路径</param>
    /// <param name="waveformData">输出的波形数据</param>
    /// <returns>是否成功加载波形数据</returns>
    bool LoadAudioWaveform(const QString &filePath, QVector<float> &waveformData);

    /// <summary>
    /// 获取当前播放状态
    /// </summary>
    /// <returns>true表示正在播放，false表示已停止</returns>
    bool IsPlaying() const
    {
        return m_playState.IsPlaying();
    }

    /// <summary>
    /// 获取当前暂停状态
    /// </summary>
    /// <returns>true表示已暂停，false表示未暂停</returns>
    bool IsPaused() const
    {
        return m_playState.IsPaused();
    }

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
    /// 打开设备
    /// </summary>
    /// <param name="devieceFormat">设备格式</param>
    /// <param name="deviceName">设备名称</param>
    /// <param name="bAudio">是否为音频设备</param>
    /// <returns>设备对象指针</returns>
    std::unique_ptr<ST_OpenAudioDevice> OpenDevice(const QString& devieceFormat, const QString& deviceName, bool bAudio = true);

    /// <summary>
    /// 显示录音设备参数
    /// </summary>
    /// <param name="ctx">格式上下文</param>
    void ShowSpec(AVFormatContext* ctx);

private:
    QString m_currentInputDevice;                       /// 当前选择的FFmpeg输入设备
    std::unique_ptr<ST_OpenAudioDevice> m_recordDevice; /// 录制设备
    std::unique_ptr<ST_AudioPlayInfo> m_playInfo;       /// 播放信息
    ST_AudioPlayState m_playState;                      /// 播放状态
};
