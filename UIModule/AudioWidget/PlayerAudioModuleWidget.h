#pragma once

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
    /// <returns></returns>
    BaseFFmpegUtils *GetFFMpegUtils() override;
    /// <summary>
    /// 加载波形图
    /// </summary>
    /// <param name="inputFilePath"></param>
    void LoadWaveWidegt(const QString &inputFilePath);

  private:
    Ui::PlayerAudioModuleWidgetClass* ui;
    AudioFFmpegUtils m_audioFFmpeg;
    AudioWaveformWidget *m_waveformWidget = nullptr;
};
