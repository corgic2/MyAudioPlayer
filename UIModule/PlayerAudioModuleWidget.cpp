#include "PlayerAudioModuleWidget.h"
#include "../Utils/FFmpegUtils.h"

PlayerAudioModuleWidget::PlayerAudioModuleWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PlayerAudioModuleWidgetClass())
{
    ui->setupUi(this);
    ConnectSignals();
}

void PlayerAudioModuleWidget::ConnectSignals()
{
    connect(ui->pushButton, &QPushButton::clicked, this, &PlayerAudioModuleWidget::SlotPushButtonRecodingAudioClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &PlayerAudioModuleWidget::SlotPushButtonPlayAudioClicked);
}

PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    delete ui;
}

void PlayerAudioModuleWidget::SlotPushButtonRecodingAudioClicked()
{
    QString filePath = QApplication::applicationDirPath();
    QString fileName = "test.wav";
    FFmpegUtils::StartAudioRecording(filePath + '/' + fileName, "wav");
}

void PlayerAudioModuleWidget::SlotPushButtonPlayAudioClicked()
{
    QString filePath = QApplication::applicationDirPath();
    QString fileName = "test.wav";
    FFmpegUtils::StartAudioPlayback(filePath + '/' + fileName);
}
