#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>
#include "FFmpegUtils.h"
#include "ui_PlayerAudioModuleWidget.h"
#include "CommonDefine/UIWidgetGlobal.h"
#include "DomainWidget/FilePathIconListWidget.h"
#include "DomainWidget/FilePathIconListWidgetItem.h"
#include "CoreWidget/CustomToolButton.h"
#include "CoreWidget/CustomLabel.h"
#include "CoreWidget/CustomComboBox.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PlayerAudioModuleWidgetClass; };
QT_END_NAMESPACE

/// <summary>
/// 音频播放器模块控件类
/// </summary>
class PlayerAudioModuleWidget : public QWidget
{
    Q_OBJECT

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit PlayerAudioModuleWidget(QWidget* parent = nullptr);
    /// <summary>
    /// 析构函数
    /// </summary>
    ~PlayerAudioModuleWidget();

    /// <summary>
    /// 添加音频文件到列表
    /// </summary>
    /// <param name="filePaths">音频文件路径列表</param>
    void AddAudioFiles(const QStringList& filePaths);

    /// <summary>
    /// 从列表移除音频文件
    /// </summary>
    /// <param name="filePath">音频文件路径</param>
    void RemoveAudioFile(const QString& filePath);

    /// <summary>
    /// 清空音频文件列表
    /// </summary>
    void ClearAudioFiles();

signals:
    /// <summary>
    /// 音频文件被选中信号
    /// </summary>
    /// <param name="filePath">音频文件路径</param>
    void SigAudioFileSelected(const QString& filePath);

protected slots:
    /// <summary>
    /// 录制按钮点击槽函数
    /// </summary>
    void SlotBtnRecordClicked();

    /// <summary>
    /// 播放按钮点击槽函数
    /// </summary>
    void SlotBtnPlayClicked();

    /// <summary>
    /// 暂停按钮点击槽函数
    /// </summary>
    void SlotBtnPauseClicked();

    /// <summary>
    /// 前进按钮点击槽函数
    /// </summary>
    void SlotBtnForwardClicked();

    /// <summary>
    /// 后退按钮点击槽函数
    /// </summary>
    void SlotBtnBackwardClicked();

    /// <summary>
    /// 下一个按钮点击槽函数
    /// </summary>
    void SlotBtnNextClicked();

    /// <summary>
    /// 上一个按钮点击槽函数
    /// </summary>
    void SlotBtnPreviousClicked();

    /// <summary>
    /// 输入设备改变槽函数
    /// </summary>
    void SlotInputDeviceChanged(int index);

    /// <summary>
    /// 音频文件选中槽函数
    /// </summary>
    void SlotAudioFileSelected(const QString& filePath);

    /// <summary>
    /// 音频文件双击槽函数
    /// </summary>
    void SlotAudioFileDoubleClicked(const QString& filePath);

private:
    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();

    /// <summary>
    /// 初始化音频设备
    /// </summary>
    void InitAudioDevices();

    /// <summary>
    /// 初始化界面
    /// </summary>
    void InitializeWidget();

    /// <summary>
    /// 更新播放状态
    /// </summary>
    void UpdatePlayState(bool isPlaying);

private:
    Ui::PlayerAudioModuleWidgetClass* ui;
    FFmpegUtils m_ffmpeg;
    QString m_currentAudioFile;  /// 当前播放的音频文件
    bool m_isRecording;         /// 是否正在录制
    bool m_isPlaying;           /// 是否正在播放
    QTimer* m_playTimer;        /// 播放定时器
};
