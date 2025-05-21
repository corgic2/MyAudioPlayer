#pragma once

#include <QWidget>
#include "ui_PlayerAudioModuleWidget.h"
#include "../Utils/FFmpegUtils.h"

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
    PlayerAudioModuleWidget(QWidget* parent = nullptr);
    ~PlayerAudioModuleWidget() override;

protected slots:
    void SlotPushButtonRecodingAudioClicked();

    void SlotPushButtonPlayAudioClicked();

private:
    void ConnectSignals();

private:
    Ui::PlayerAudioModuleWidgetClass* ui;
    FFmpegUtils m_ffmpeg;
};
