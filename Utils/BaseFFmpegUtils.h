#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <QObject>
#include <QString>
#include "DataDefine/ST_OpenFileResult.h"

/// <summary>
/// 通用播放状态枚举
/// </summary>
enum class EM_PlayState
{
    /// <summary>
    /// 停止
    /// </summary>
    Stopped,

    /// <summary>
    /// 播放中
    /// </summary>
    Playing,

    /// <summary>
    /// 暂停
    /// </summary>
    Paused
};

/// <summary>
/// 通用录制状态枚举
/// </summary>
enum class EM_RecordState
{
    /// <summary>
    /// 停止
    /// </summary>
    Stopped,

    /// <summary>
    /// 录制中
    /// </summary>
    Recording,

    /// <summary>
    /// 暂停
    /// </summary>
    Paused
};

/// <summary>
/// FFmpeg工具基类
/// </summary>
class BaseFFmpegUtils : public QObject
{
    Q_OBJECT 

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    BaseFFmpegUtils(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~BaseFFmpegUtils() override;

    /// <summary>
    /// 开始录制
    /// </summary>
    /// <param name="saveFilePath">保存文件路径</param>
    virtual void StartRecording(const QString& saveFilePath) = 0;

    /// <summary>
    /// 停止录制
    /// </summary>
    virtual void StopRecording() = 0;

    /// <summary>
    /// 开始播放
    /// </summary>
    /// <param name="filePath">文件路径</param>
    /// <param name="startPosition">开始位置（秒）</param>
    /// <param name="args">参数列表</param>
    virtual void StartPlay(const QString& filePath, double startPosition = 0.0, const QStringList& args = QStringList()) = 0;

    /// <summary>
    /// 暂停播放
    /// </summary>
    virtual void PausePlay() = 0;

    /// <summary>
    /// 恢复播放
    /// </summary>
    virtual void ResumePlay() = 0;

    /// <summary>
    /// 停止播放
    /// </summary>
    virtual void StopPlay() = 0;

    /// <summary>
    /// 定位播放位置
    /// </summary>
    /// <param name="position">位置（秒）</param>
    virtual void SeekPlay(double position) = 0;

    /// <summary>
    /// 获取是否正在播放
    /// </summary>
    /// <returns>是否正在播放</returns>
    virtual bool IsPlaying() { return m_playState.load() == EM_PlayState::Playing; }

    /// <summary>
    /// 获取是否已暂停
    /// </summary>
    /// <returns>是否已暂停</returns>
    virtual bool IsPaused() { return m_playState.load() == EM_PlayState::Paused; }

    /// <summary>
    /// 获取是否正在录制
    /// </summary>
    /// <returns>是否正在录制</returns>
    virtual bool IsRecording() { return m_recordState.load() == EM_RecordState::Recording; }

    /// <summary>
    /// 获取当前播放位置
    /// </summary>
    /// <returns>当前播放位置（秒）</returns>
    virtual double GetCurrentPosition() const { return 0.0; }

    /// <summary>
    /// 获取总时长
    /// </summary>
    /// <returns>总时长（秒）</returns>
    virtual double GetDuration() const { return m_duration; }

protected:
    /// <summary>
    /// 通用文件打开功能
    /// </summary>
    /// <param name="filePath">文件路径</param>
    /// <returns>打开文件结果</returns>
    std::unique_ptr<ST_OpenFileResult> OpenMediaFile(const QString& filePath);

    /// <summary>
    /// 设置播放状态
    /// </summary>
    /// <param name="state">播放状态</param>
    void SetPlayState(EM_PlayState state);

    /// <summary>
    /// 设置录制状态
    /// </summary>
    /// <param name="state">录制状态</param>
    void SetRecordState(EM_RecordState state);

    /// <summary>
    /// 设置当前文件路径
    /// </summary>
    /// <param name="filePath">文件路径</param>
    void SetCurrentFilePath(const QString& filePath);

    /// <summary>
    /// 获取当前文件路径
    /// </summary>
    /// <returns>当前文件路径</returns>
    QString GetCurrentFilePath() const;

    /// <summary>
    /// 设置总时长
    /// </summary>
    /// <param name="duration">总时长（秒）</param>
    void SetDuration(double duration);

    /// <summary>
    /// 重置播放状态
    /// </summary>
    void ResetPlayState();

    /// <summary>
    /// 重置录制状态
    /// </summary>
    void ResetRecordState();

    /// <summary>
    /// 获取线程安全的互斥锁
    /// </summary>
    /// <returns>互斥锁引用</returns>
    std::recursive_mutex& GetMutex() { return m_mutex; }

    /// <summary>
    /// 记录播放开始时间
    /// </summary>
    /// <param name="startPosition">开始位置（秒）</param>
    void RecordPlayStartTime(double startPosition = 0.0);

    /// <summary>
    /// 计算当前播放位置（基于时间）
    /// </summary>
    /// <returns>当前播放位置（秒）</returns>
    double CalculateCurrentPosition() const;

    /// <summary>
    /// 重置播放器状态（用于复用播放器）
    /// </summary>
    virtual void ResetPlayerState();

    /// <summary>
    /// 检查播放器是否可以复用
    /// </summary>
    /// <returns>是否可以复用</returns>
    virtual bool CanReusePlayer() const;

    /// <summary>
    /// 强制停止所有活动
    /// </summary>
    virtual void ForceStop();

signals:
    /// <summary>
    /// 播放状态改变信号
    /// </summary>
    /// <param name="isPlaying">是否正在播放</param>
    void SigPlayStateChanged(bool isPlaying);

    /// <summary>
    /// 录制状态改变信号
    /// </summary>
    /// <param name="isRecording">是否正在录制</param>
    void SigRecordStateChanged(bool isRecording);

protected:
    /// <summary>
    /// 播放状态
    /// </summary>
    std::atomic<EM_PlayState> m_playState{EM_PlayState::Stopped};

    /// <summary>
    /// 录制状态
    /// </summary>
    std::atomic<EM_RecordState> m_recordState{EM_RecordState::Stopped};

    /// <summary>
    /// 当前文件路径
    /// </summary>
    QString m_currentFilePath;

    /// <summary>
    /// 总时长（秒）
    /// </summary>
    double m_duration{0.0};

    /// <summary>
    /// 播放开始时间（秒）
    /// </summary>
    double m_startTime{0.0};

    /// <summary>
    /// 播放开始的时间点
    /// </summary>
    std::chrono::steady_clock::time_point m_playStartTimePoint;

    /// <summary>
    /// 线程安全互斥锁
    /// </summary>
    mutable std::recursive_mutex m_mutex;
};
