#include "PlayerAudioModuleWidget.h"

#include "AudioMainWidget.h"
#include "../Utils/FFmpegUtils.h"

PlayerAudioModuleWidget::PlayerAudioModuleWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PlayerAudioModuleWidgetClass())
{
    ui->setupUi(this);
    ConnectSignals();
    InitAudioDevices();
}

void PlayerAudioModuleWidget::ConnectSignals()
{
    connect(ui->pushButton, &QPushButton::clicked, this, &PlayerAudioModuleWidget::SlotPushButtonRecodingAudioClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &PlayerAudioModuleWidget::SlotPushButtonPlayAudioClicked);
    connect(ui->comboBoxInput, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PlayerAudioModuleWidget::SlotInputDeviceChanged);
}

PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    delete ui;
}

void PlayerAudioModuleWidget::SlotPushButtonRecodingAudioClicked()
{
    QString filePath = QApplication::applicationDirPath();
    QString fileName = "test.wav";
    m_ffmpeg.StartAudioRecording(filePath + '/' + fileName, "wav");
}

void PlayerAudioModuleWidget::SlotPushButtonPlayAudioClicked()
{
    QString filePath = QApplication::applicationDirPath();
    QString fileName = "test.wav";
    m_ffmpeg.StartAudioPlayback(filePath + '/' + fileName);
}

void PlayerAudioModuleWidget::InitAudioDevices()
{
    // 获取并设置输入设备
    QStringList inputDevices = m_ffmpeg.GetInputAudioDevices();
    ui->comboBoxInput->addItems(inputDevices);
    if (!inputDevices.isEmpty()) {
        m_ffmpeg.SetInputDevice(inputDevices.first());
    }
}

void PlayerAudioModuleWidget::SlotInputDeviceChanged(int index)
{
    QString deviceName = ui->comboBoxInput->itemText(index);
    m_ffmpeg.SetInputDevice(deviceName);
}
