#pragma once

#include <QTimer>
#include "AudioFFmpegUtils.h"
#include "AudioWaveformWidget.h"
#include "ui_PlayerAudioModuleWidget.h"
#include "BaseWidget/BaseModuleWidegt.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class PlayerAudioModuleWidgetClass;
};

QT_END_NAMESPACE

/// <summary>
/// 音频播放器模块控件类
/// </summary>
class PlayerAudioModuleWidget : public BaseModuleWidegt
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit PlayerAudioModuleWidget(QWidget* parent = nullptr);
    /// <summary>
    /// 析构函数
    /// </summary>
    ~PlayerAudioModuleWidget() override;
    /// <summary>
    /// 获取FFMpegUtils
    /// </summary>
    /// <returns>FFmpeg工具类指针</returns>
    BaseFFmpegUtils *GetFFMpegUtils() override;
    /// <summary>
    /// 加载波形图
    /// </summary>
    /// <param name="inputFilePath">音频文件路径</param>
    void LoadWaveWidegt(const QString &inputFilePath);
    /// <summary>
    /// 开始播放
    /// </summary>
    /// <param name="filePath">文件路径</param>
    /// <param name="startPosition">开始位置（秒）</param>
    void StartPlay(const QString& filePath, double startPosition = 0.0);
    /// <summary>
    /// 暂停播放
    /// </summary>
    void PausePlay();
    /// <summary>
    /// 恢复播放
    /// </summary>
    void ResumePlay();
    /// <summary>
    /// 停止播放
    /// </summary>
    void StopPlay();
    /// <summary>
    /// 跳转到指定位置
    /// </summary>
    /// <param name="position">位置(0.0-1.0)</param>
    void SeekTo(double position);

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

private slots:
    /// <summary>
    /// 播放进度改变槽函数
    /// </summary>
    /// <param name="position">当前位置</param>
    /// <param name="duration">总时长</param>
    void SlotProgressChanged(qint64 position, qint64 duration);
    /// <summary>
    /// 更新播放进度
    /// </summary>
    void SlotUpdateProgress();

private:
    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();

private:
    Ui::PlayerAudioModuleWidgetClass* ui;
    AudioFFmpegUtils m_audioFFmpeg;
    AudioWaveformWidget *m_waveformWidget = nullptr;
    QTimer* m_progressTimer = nullptr;
    QString m_currentFilePath;
    qint64 m_currentPosition = 0;
    qint64 m_totalDuration = 0;
};
