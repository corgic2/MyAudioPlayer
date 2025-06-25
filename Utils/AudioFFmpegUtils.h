#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>
#include "AudioResampler.h"
#include "BaseFFmpegUtils.h"
#include "DataDefine/ST_AudioPlayInfo.h"
#include "DataDefine/ST_AudioPlayState.h"
#include "DataDefine/ST_OpenAudioDevice.h"
#include "DataDefine/ST_OpenFileResult.h"

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
class AudioFFmpegUtils : public BaseFFmpegUtils
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
    void StartRecording(const QString& outputFilePath) override;

    /// <summary>
    /// 停止录音
    /// </summary>
    void StopRecording() override;

    /// <summary>
    /// 播放音频文件
    /// </summary>
    /// <param name="inputFilePath">输入文件路径</param>
    /// <param name="startPosition">开始播放位置（秒），默认从头开始</param>
    /// <param name="args">播放参数</param>
    void StartPlay(const QString& inputFilePath, double startPosition = 0.0, const QStringList& args = QStringList()) override;

    /// <summary>
    /// 暂停音频播放
    /// </summary>
    void PausePlay() override;

    /// <summary>
    /// 继续音频播放
    /// </summary>
    void ResumePlay() override;

    /// <summary>
    /// 停止音频播放
    /// </summary>
    void StopPlay() override;

    /// <summary>
    /// 音频快进
    /// </summary>
    /// <param name="seconds">快进秒数</param>
    void SeekPlay(double seconds) override;

    /// <summary>
    /// 加载音频波形数据
    /// </summary>
    /// <param name="filePath">音频文件路径</param>
    /// <param name="waveformData">输出的波形数据</param>
    /// <returns>是否成功加载波形数据</returns>
    bool LoadAudioWaveform(const QString& filePath, QVector<float>& waveformData);

    /// <summary>
    /// 获取当前播放状态
    /// </summary>
    /// <returns>true表示正在播放，false表示已停止</returns>
    bool IsPlaying() override;

    /// <summary>
    /// 获取当前暂停状态
    /// </summary>
    /// <returns>true表示已暂停，false表示未暂停</returns>
    bool IsPaused() override;
    /// <summary>
    /// 获取当前录制状态
    /// </summary>
    /// <returns>true表示正在录制，false表示已停止录制</returns>
    bool IsRecording() override;

    /// <summary>
    /// 获取音频总时长
    /// </summary>
    /// <returns>音频总时长（秒）</returns>
    double GetDuration() const;

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

    /// <summary>
    /// 获取所有音频输入设备
    /// </summary>
    /// <returns></returns>
    QStringList GetInputAudioDevices();
    /// <summary>
    /// 设置输入设备
    /// </summary>
    /// <param name="deviceName"></param>
    void SetInputDevice(const QString& deviceName);

    /// <summary>
    /// 音频定位
    /// </summary>
    /// <param name="seconds">定位时间（秒），正数表示前进，负数表示后退</param>
    /// <returns>是否定位成功</returns>
    bool SeekAudio(double seconds);

    /// <summary>
    /// 处理音频帧以生成波形数据
    /// </summary>
    /// <param name="frame">音频帧</param>
    /// <param name="waveformData">波形数据输出</param>
    /// <param name="samplesPerPixel">每像素采样数</param>
    /// <param name="currentSum">当前累计值</param>
    /// <param name="sampleCount">当前采样计数</param>
    /// <param name="maxSample">最大采样值</param>
    void ProcessAudioFrame(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample);

    /// <summary>
    /// 处理浮点采样格式
    /// </summary>
    void ProcessFloatSamples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample);

    /// <summary>
    /// 处理16位整数采样格式
    /// </summary>
    void ProcessInt16Samples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample);

    /// <summary>
    /// 处理32位整数采样格式
    /// </summary>
    void ProcessInt32Samples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample);

    /// <summary>
    /// 处理音频数据流
    /// </summary>
    /// <param name="openFileResult">打开文件结果</param>
    /// <param name="resampler">重采样器</param>
    /// <param name="resampleParams">重采样参数</param>
    void ProcessAudioData(ST_OpenFileResult& openFileResult, AudioResampler& resampler, ST_ResampleParams& resampleParams);
    /// <summary>
    /// 清除播放状态操作
    /// </summary>
    void PlayerStateReSet();
private:
    QString m_currentInputDevice;                       /// 当前选择的FFmpeg输入设备
    std::unique_ptr<ST_OpenAudioDevice> m_recordDevice; /// 录制设备
    std::unique_ptr<ST_AudioPlayInfo> m_playInfo;       /// 播放信息
    ST_AudioPlayState m_playState;                      /// 播放状态
    std::atomic<bool> m_isRecording{false};             /// 录制状态标志
    QString m_currentFilePath;                          /// 当前播放的文件路径
    double m_duration{0.0};                             /// 音频总时长（秒）
    QStringList m_inputAudioDevices; /// 音频输入设备列表
};
