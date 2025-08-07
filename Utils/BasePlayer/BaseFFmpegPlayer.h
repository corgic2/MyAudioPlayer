#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <QObject>
#include <QString>
#include "DataDefine/ST_OpenFileResult.h"
#include "DataDefine/ST_AVPlayState.h"

/// <summary>
/// FFmpeg工具基类
/// </summary>
class BaseFFmpegPlayer : public QObject
{
    Q_OBJECT public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    BaseFFmpegPlayer(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~BaseFFmpegPlayer() override;

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
    virtual void StartPlay(const QString& filePath, bool bStart = true,double startPosition = 0.0, const QStringList& args = QStringList()) = 0;

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
    virtual bool IsPlaying();

    /// <summary>
    /// 获取是否已暂停
    /// </summary>
    /// <returns>是否已暂停</returns>
    virtual bool IsPaused();

    /// <summary>
    /// 获取是否正在录制
    /// </summary>
    /// <returns>是否正在录制</returns>
    virtual bool IsRecording();

    /// <summary>
    /// 获取总时长
    /// </summary>
    /// <returns>总时长（秒）</returns>
    virtual double GetDuration();

protected:
    /// <summary>
    /// 通用文件打开功能
    /// </summary>
    /// <param name="filePath">文件路径</param>
    /// <returns>打开文件结果</returns>
    std::unique_ptr<ST_OpenFileResult> OpenMediaFile(const QString& filePath);

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
    /// 强制停止所有活动
    /// </summary>
    virtual void ForceStop();

protected:
    //播放和暂停状态由基类管理
    /// <summary>
    /// 播放状态管理器
    /// </summary>
    ST_AVPlayState m_playState;

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
    /// 暂停时的位置（秒）
    /// </summary>
    double m_pauseTime{0.0};

    /// <summary>
    /// 是否已暂停
    /// </summary>
    bool m_isPaused{false};

    /// <summary>
    /// 线程安全互斥锁
    /// </summary>
    mutable std::recursive_mutex m_mutex;
};
