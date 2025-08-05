#pragma once

#include <atomic>
#include <memory>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include "../BasePlayer/BaseFFmpegPlayer.h"
#include "VideoPlayWorker.h"
#include "VideoRecordWorker.h"

// 前向声明
class VideoPlayWorker;
class VideoRecordWorker;
class PlayerVideoModuleWidget;

/// <summary>
/// 视频FFmpeg播放类
/// </summary>
class VideoFFmpegPlayer : public BaseFFmpegPlayer
{
    Q_OBJECT public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit VideoFFmpegPlayer(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~VideoFFmpegPlayer() override;

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
    void StartPlay(const QString& videoPath, bool bStart, double startPosition = 0.0, const QStringList& args = QStringList()) override;

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
    /// 获取当前播放位置
    /// </summary>
    /// <returns>当前播放位置（秒）</returns>
    double GetCurrentPosition();

    /// <summary>
    /// 获取视频信息
    /// </summary>
    /// <returns>视频帧信息</returns>
    ST_VideoFrameInfo GetVideoInfo() const;

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
    void SlotHandleFrameUpdate(std::vector<uint8_t> frameData, int width, int height);

private:
    /// <summary>
    /// 视频播放工作对象
    /// </summary>
    std::unique_ptr<VideoPlayWorker> m_pPlayWorker{nullptr};
    
    /// <summary>
    /// 视频录制工作对象
    /// </summary>
    std::unique_ptr <VideoRecordWorker> m_pRecordWorker{nullptr};

    /// <summary>
    /// 视频信息
    /// </summary>
    ST_VideoFrameInfo m_videoInfo;

    /// <summary>
    /// 视频显示控件指针
    /// </summary>
    PlayerVideoModuleWidget* m_pVideoDisplayWidget{nullptr};

    std::recursive_mutex m_mutex;
};
