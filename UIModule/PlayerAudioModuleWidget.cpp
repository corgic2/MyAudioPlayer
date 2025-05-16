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
    connect(ui->pushButton, &QPushButton::clicked, this, &PlayerAudioModuleWidget::SlotPushButtonClicked);
}

PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    delete ui;
}

void PlayerAudioModuleWidget::SlotPushButtonClicked()
{
    QString filePath = QApplication::applicationDirPath();
    QString fileName = "test.wav";
    FFmpegUtils::StartAudioRecording(filePath + '/' + fileName, "wav");
}
