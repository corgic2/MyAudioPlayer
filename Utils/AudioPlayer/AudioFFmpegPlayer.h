#pragma once

#include <atomic>
#include <memory>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>
#include "AudioResampler.h"
#include "../BasePlayer/BaseFFmpegPlayer.h"
#include "DataDefine/ST_AudioPlayInfo.h"
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
/// FFmpeg音频播放类
/// </summary>
class AudioFFmpegPlayer : public BaseFFmpegPlayer
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit AudioFFmpegPlayer(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~AudioFFmpegPlayer() override;

    /// <summary>
    /// 开始录音
    /// </summary>
    /// <param name="outputFilePath">输出文件路径</param>
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
    void StartPlay(const QString& inputFilePath, bool bStart = true, double startPosition = 0.0, const QStringList& args = QStringList()) override;

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
    /// 获取当前播放位置
    /// </summary>
    /// <returns>当前播放位置（秒）</returns>
    double GetCurrentPosition();

    /// <summary>
    /// 重置播放器状态（重写基类方法）
    /// </summary>
    void ResetPlayerState() override;

    /// <summary>
    /// 强制停止所有活动（重写基类方法）
    /// </summary>
    void ForceStop() override;

signals:
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
    /// 设置输入设备
    /// </summary>
    /// <param name="deviceName">设备名称</param>
    void SetInputDevice(const QString& deviceName);

    /// <summary>
    /// 音频定位
    /// </summary>
    /// <param name="seconds">定位时间（秒）</param>
    /// <returns>是否定位成功</returns>
    bool SeekAudio(double seconds);

    /// <summary>
    /// 处理音频数据流
    /// </summary>
    /// <param name="openFileResult">打开文件结果</param>
    /// <param name="resampler">重采样器</param>
    /// <param name="resampleParams">重采样参数</param>
    /// <param name="startSeconds">起始播放位置（秒），默认从当前位置开始</param>
    void ProcessAudioData(const ST_OpenFileResult& openFileResult, AudioResampler& resampler, ST_ResampleParams& resampleParams, double startSeconds = 0.0);

    /// <summary>
    /// 重置播放器状态
    /// </summary>
    void PlayerStateReSet();

private:
    QString m_currentInputDevice;                                /// 当前选择的FFmpeg输入设备
    std::unique_ptr<ST_OpenAudioDevice> m_recordDevice{nullptr}; /// 录制设备
    std::unique_ptr<ST_AudioPlayInfo> m_playInfo{nullptr};       /// 播放信息
    QStringList m_inputAudioDevices;                             /// 音频输入设备列表
    
    std::vector<uint8_t> m_audioBuffer;                          /// 音频数据缓冲区
    std::recursive_mutex m_bufferMutex;                        /// 缓冲区互斥锁
    
    // 缓存的重采样相关资源
    std::unique_ptr<AudioResampler> m_resampler{nullptr};       /// 重采样器实例
    std::unique_ptr<ST_ResampleParams> m_resampleParams{nullptr}; /// 重采样参数
    int m_audioStreamIdx{-1};                                   /// 音频流索引
    bool m_bResampleParamsUpdated{false};                       /// 重采样参数是否已更新
};
