#pragma once

#include <QObject>

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
    virtual bool IsPlaying() = 0;

    /// <summary>
    /// 获取是否已暂停
    /// </summary>
    /// <returns>是否已暂停</returns>
    virtual bool IsPaused() = 0;

    /// <summary>
    /// 获取是否正在录制
    /// </summary>
    /// <returns>是否正在录制</returns>
    virtual bool IsRecording() = 0;

    /// <summary>
    /// 获取当前播放位置
    /// </summary>
    /// <returns>当前播放位置（秒）</returns>
    virtual double GetCurrentPosition() const { return 0.0; }

    /// <summary>
    /// 获取总时长
    /// </summary>
    /// <returns>总时长（秒）</returns>
    virtual double GetDuration() const { return 0.0; }

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
};
