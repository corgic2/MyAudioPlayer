#pragma once

#include "ui_PlayerVideoModuleWidget.h"
#include "VideoFFmpegUtils.h"
#include "BaseWidget/BaseModuleWidegt.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class PlayerVideoModuleWidgetClass;
};

QT_END_NAMESPACE

/// <summary>
/// 音频播放器模块控件类
/// </summary>
class PlayerVideoModuleWidget : public BaseModuleWidegt
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit PlayerVideoModuleWidget(QWidget* parent = nullptr);
    /// <summary>
    /// 析构函数
    /// </summary>
    ~PlayerVideoModuleWidget() override;
    /// <summary>
    /// 获取FFMpegUtils
    /// </summary>
    /// <returns></returns>
    BaseFFmpegUtils *GetFFMpegUtils() override;

private:
    Ui::PlayerVideoModuleWidgetClass* ui;
    VideoFFmpegUtils m_audioFFmpeg;
};
