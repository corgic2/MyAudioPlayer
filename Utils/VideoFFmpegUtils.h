#pragma once

#include <atomic>
#include <memory>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include "BaseFFmpegUtils.h"
#include "VideoPlayWorker.h"
#include "VideoRecordWorker.h"

// 前向声明
class VideoPlayWorker;
class VideoRecordWorker;
class PlayerVideoModuleWidget;

/// <summary>
/// 视频FFmpeg工具类
/// </summary>
class VideoFFmpegUtils : public BaseFFmpegUtils
{
    Q_OBJECT public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit VideoFFmpegUtils(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~VideoFFmpegUtils() override;

    /// <summary>
    /// 设置视频显示控件
    /// </summary>
    /// <param name="videoWidget">视频显示控件</param>
    void SetVideoDisplayWidget(PlayerVideoModuleWidget* videoWidget);

    /// <summary>
    /// 开始播放视频
    /// </summary>
    /// <param name="videoPath">视频文件路径</param>
    /// <param name="startPosition">开始位置（秒）</param>
    /// <param name="args">参数列表</param>
    void StartPlay(const QString& videoPath, double startPosition = 0.0, const QStringList& args = QStringList()) override;

    /// <summary>
    /// 暂停视频播放
    /// </summary>
    void PausePlay() override;

    /// <summary>
    /// 恢复视频播放
    /// </summary>
    void ResumePlay() override;

    /// <summary>
    /// 停止视频播放
    /// </summary>
    void StopPlay() override;

    /// <summary>
    /// 开始录制视频
    /// </summary>
    /// <param name="outputPath">输出文件路径</param>
    void StartRecording(const QString& outputPath) override;

    /// <summary>
    /// 停止录制视频
    /// </summary>
    void StopRecording() override;

    /// <summary>
    /// 移动播放位置
    /// </summary>
    /// <param name="seconds">目标时间（秒）</param>
    void SeekPlay(double seconds) override;

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
    /// 获取当前播放位置
    /// </summary>
    /// <returns>当前播放位置（秒）</returns>
    double GetCurrentPosition() const override;

    /// <summary>
    /// 获取总时长
    /// </summary>
    /// <returns>总时长（秒）</returns>
    double GetDuration() const override;

    /// <summary>
    /// 获取视频信息
    /// </summary>
    /// <returns>视频帧信息</returns>
    ST_VideoFrameInfo GetVideoInfo() const;

signals:
    /// <summary>
    /// 视频播放状态改变信号
    /// </summary>
    /// <param name="state">播放状态</param>
    void SigPlayStateChanged(EM_VideoPlayState state);

    /// <summary>
    /// 视频录制状态改变信号
    /// </summary>
    /// <param name="state">录制状态</param>
    void SigRecordStateChanged(EM_VideoRecordState state);

    /// <summary>
    /// 播放进度更新信号
    /// </summary>
    /// <param name="currentTime">当前时间(秒)</param>
    /// <param name="totalTime">总时间(秒)</param>
    void SigPlayProgressUpdated(double currentTime, double totalTime);

    /// <summary>
    /// 视频帧更新信号
    /// </summary>
    /// <param name="frameData">帧数据</param>
    /// <param name="width">帧宽度</param>
    /// <param name="height">帧高度</param>
    void SigFrameUpdated(const uint8_t* frameData, int width, int height);

    /// <summary>
    /// 错误信号
    /// </summary>
    /// <param name="errorMsg">错误信息</param>
    void SigError(const QString& errorMsg);

private slots:
    /// <summary>
    /// 处理视频帧更新
    /// </summary>
    void SlotHandleFrameUpdate(const uint8_t* frameData, int width, int height);

private:
    /// <summary>
    /// 视频播放状态
    /// </summary>
    EM_VideoPlayState m_playState{EM_VideoPlayState::Stopped};

    /// <summary>
    /// 视频录制状态
    /// </summary>
    EM_VideoRecordState m_recordState{EM_VideoRecordState::Stopped};
    
    /// <summary>
    /// 视频播放工作对象
    /// </summary>
    VideoPlayWorker* m_pPlayWorker{nullptr};
    
    /// <summary>
    /// 视频录制工作对象
    /// </summary>
    VideoRecordWorker* m_pRecordWorker{nullptr};

    /// <summary>
    /// 视频信息
    /// </summary>
    ST_VideoFrameInfo m_videoInfo;

    /// <summary>
    /// 当前播放时间
    /// </summary>
    double m_currentTime{0.0};

    /// <summary>
    /// 视频显示控件指针
    /// </summary>
    PlayerVideoModuleWidget* m_pVideoDisplayWidget{nullptr};
};
