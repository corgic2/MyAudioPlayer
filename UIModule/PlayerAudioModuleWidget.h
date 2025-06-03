#pragma once

#include <QWidget>
#include "FFmpegUtils.h"
#include "ui_PlayerAudioModuleWidget.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class PlayerAudioModuleWidgetClass;
};
QT_END_NAMESPACE

class PlayerAudioModuleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerAudioModuleWidget(QWidget* parent = nullptr);
    ~PlayerAudioModuleWidget();

protected slots:
    void SlotPushButtonRecodingAudioClicked();
    void SlotPushButtonPlayAudioClicked();
    void SlotInputDeviceChanged(int index);
private:
    void ConnectSignals();
    void InitAudioDevices();

private:
    Ui::PlayerAudioModuleWidgetClass* ui;
    FFmpegUtils m_ffmpeg;
};
