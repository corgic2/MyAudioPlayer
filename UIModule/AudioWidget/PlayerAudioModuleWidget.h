#pragma once

#include <QTimer>
#include "AudioWaveformWidget.h"
#include "ui_PlayerAudioModuleWidget.h"
#include "BaseWidget/BaseModuleWidegt.h"
#include "BaseWidget/ControlButtonWidget.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class PlayerAudioModuleWidgetClass;
};

QT_END_NAMESPACE

/// <summary>
/// 音频播放器模块控件类
/// </summary>
class PlayerAudioModuleWidget : public BaseModuleWidget
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
    /// 加载波形图
    /// </summary>
    /// <param name="inputFilePath"></param>
    void LoadWaveWidegt(const QString& inputFilePath);

    /// <summary>
    /// 更新波形图播放位置
    /// </summary>
    /// <param name="currentPos">当前播放位置（秒）</param>
    /// <param name="duration">总时长（秒）</param>
    void UpdateWaveformPosition(double currentPos, double duration);
signals:
    /// <summary>
    /// 播放进度改变信号
    /// </summary>
    /// <param name="position">当前位置（秒）</param>
    /// <param name="duration">总时长（秒）</param>
    void SigProgressChanged(qint64 position, qint64 duration);
    /// <summary>
    /// 线程退出函数
    /// </summary>
    void SigThreadExit();

private slots:
    /// <summary>
    /// 播放进度改变槽函数
    /// </summary>
    /// <param name="position">当前位置</param>
    /// <param name="duration">总时长</param>
    void SlotProgressChanged(qint64 position, qint64 duration);

private:
    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();

private:
    Ui::PlayerAudioModuleWidgetClass* ui;
    AudioWaveformWidget* m_waveformWidget{nullptr};
    QString m_currentFilePath;
    qint64 m_currentPosition{0};
    qint64 m_totalDuration{0};
    QVector<float> m_waveformData;
    size_t m_threadID = 0;
};
