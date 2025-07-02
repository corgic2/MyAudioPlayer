#pragma once

#include <QWidget>
#include "CoreWidget/CustomToolButton.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class ControlButtonWidget;
}
QT_END_NAMESPACE


/// <summary>
/// 音频控制按钮组件类
/// </summary>
class ControlButtonWidget : public QWidget
{
    Q_OBJECT

public:
    /// <summary>
    /// 控制按钮类型枚举
    /// </summary>
    enum EM_ControlButtonType
    {
        Record = 0,     /// 录制按钮
        Previous,       /// 上一个按钮
        Backward,       /// 后退按钮
        Play,           /// 播放按钮
        Forward,        /// 前进按钮
        Next            /// 下一个按钮
    };

    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit ControlButtonWidget(QWidget* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~ControlButtonWidget() override;

    /// <summary>
    /// 更新播放按钮状态
    /// </summary>
    /// <param name="isPlaying">是否正在播放</param>
    void UpdatePlayState(bool isPlaying);

    /// <summary>
    /// 更新录制按钮状态
    /// </summary>
    /// <param name="isRecording">是否正在录制</param>
    void UpdateRecordState(bool isRecording);

    /// <summary>
    /// 设置当前音频文件
    /// </summary>
    /// <param name="filePath">音频文件路径</param>
    void SetCurrentAudioFile(const QString& filePath);

    /// <summary>
    /// 获取当前音频文件
    /// </summary>
    /// <returns>当前音频文件路径</returns>
    QString GetCurrentAudioFile() const;

    /// <summary>
    /// 更新按钮状态
    /// </summary>
    void UpdateButtonStates();

    /// <summary>
    /// 设置指定按钮的启用状态
    /// </summary>
    /// <param name="buttonType">按钮类型</param>
    /// <param name="enabled">是否启用</param>
    void SetButtonEnabled(EM_ControlButtonType buttonType, bool enabled);

    /// <summary>
    /// 获取指定按钮的启用状态
    /// </summary>
    /// <param name="buttonType">按钮类型</param>
    /// <returns>是否启用</returns>
    bool IsButtonEnabled(EM_ControlButtonType buttonType) const;

signals:
    /// <summary>
    /// 录制按钮点击信号
    /// </summary>
    void SigRecordClicked();

    /// <summary>
    /// 播放按钮点击信号
    /// </summary>
    void SigPlayClicked();

    /// <summary>
    /// 前进按钮点击信号
    /// </summary>
    void SigForwardClicked();

    /// <summary>
    /// 后退按钮点击信号
    /// </summary>
    void SigBackwardClicked();

    /// <summary>
    /// 下一个按钮点击信号
    /// </summary>
    void SigNextClicked();

    /// <summary>
    /// 上一个按钮点击信号
    /// </summary>
    void SigPreviousClicked();

    /// <summary>
    /// 当前音频文件改变信号
    /// </summary>
    /// <param name="filePath">新的音频文件路径</param>
    void SigCurrentAudioFileChanged(const QString& filePath);

private:
    /// <summary>
    /// 初始化界面
    /// </summary>
    void InitializeWidget();

    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();

    /// <summary>
    /// 根据按钮类型获取按钮指针
    /// </summary>
    /// <param name="buttonType">按钮类型</param>
    /// <returns>按钮指针，如果类型无效则返回nullptr</returns>
    CustomToolButton* GetButtonByType(EM_ControlButtonType buttonType) const;

private:
    Ui::ControlButtonWidget* ui;
    QString m_currentAudioFile;                              /// 当前音频文件路径
    QMap<EM_ControlButtonType, bool> m_originalButtonState; /// 按钮原始状态映射
}; 