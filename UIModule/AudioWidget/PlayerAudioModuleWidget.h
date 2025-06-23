#pragma once

#include "ui_PlayerAudioModuleWidget.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class PlayerAudioModuleWidgetClass;
};

QT_END_NAMESPACE

/// <summary>
/// 音频播放器模块控件类
/// </summary>
class PlayerAudioModuleWidget : public QWidget
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

private:
    Ui::PlayerAudioModuleWidgetClass* ui;
};
