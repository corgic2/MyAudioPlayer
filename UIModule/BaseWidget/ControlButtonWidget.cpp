#include "ControlButtonWidget.h"
#include "ui_ControlButtonWidget.h"
#include <QTimer>
#include "CommonDefine/UIWidgetColorDefine.h"
#include "UtilsWidget/CustomToolTips.h"
#include "SDKCommonDefine/SDKCommonDefine.h"

ControlButtonWidget::ControlButtonWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ControlButtonWidget())
{
    ui->setupUi(this);
    InitializeWidget();
    ConnectSignals();
    ui->btnRecord->setHidden(true);
}

ControlButtonWidget::~ControlButtonWidget()
{
    SAFE_DELETE_POINTER_VALUE(ui);
}

void ControlButtonWidget::InitializeWidget()
{
    // 初始化按钮状态
    UpdateButtonEnabledStates();

    // 设置按钮图标
    InitializeButtonIcons();
}

void ControlButtonWidget::InitializeButtonIcons()
{
    // 设置各个按钮的图标
    ui->btnRecord->SetPixMapFilePath(":/AVBaseWidget/images/PlayerButton.png");
    ui->btnPlay->SetPixMapFilePath(":/AVBaseWidget/images/PlayerButton.png");
    ui->btnForward->SetPixMapFilePath(":/AVBaseWidget/images/ForwardButton.png");
    ui->btnBackward->SetPixMapFilePath(":/AVBaseWidget/images/BackButton.png");
    ui->btnNext->SetPixMapFilePath(":/AVBaseWidget/images/NextMediaButton.png");
    ui->btnPrevious->SetPixMapFilePath(":/AVBaseWidget/images/PreviousButton.png");
}

void ControlButtonWidget::ConnectSignals()
{
    connect(ui->btnRecord, &CustomToolButton::clicked, this, &ControlButtonWidget::SigRecordClicked);
    connect(ui->btnPlay, &CustomToolButton::clicked, this, &ControlButtonWidget::SigPlayClicked);
    connect(ui->btnForward, &CustomToolButton::clicked, this, &ControlButtonWidget::SigForwardClicked);
    connect(ui->btnBackward, &CustomToolButton::clicked, this, &ControlButtonWidget::SigBackwardClicked);
    connect(ui->btnNext, &CustomToolButton::clicked, this, &ControlButtonWidget::SigNextClicked);
    connect(ui->btnPrevious, &CustomToolButton::clicked, this, &ControlButtonWidget::SigPreviousClicked);
    connect(ui->musicProgressBar, &MusicProgressBar::SigPositionChanged, this, &ControlButtonWidget::SigProgressChanged);
}

void ControlButtonWidget::UpdatePlayState(bool isPlaying)
{
    if (isPlaying)
    {
        ui->btnPlay->setText(tr(""));
        ui->btnPlay->SetPixMapFilePath(":/AVBaseWidget/images/PauseButton.png");
        ui->btnPlay->setToolTip(tr("暂停"));
    }
    else
    {
        ui->btnPlay->setText(tr(""));
        ui->btnPlay->SetPixMapFilePath(":/AVBaseWidget/images/PlayerButton.png");
        ui->btnPlay->setToolTip(tr("播放"));
    }
}

void ControlButtonWidget::UpdateRecordState(bool isRecording)
{
    if (isRecording)
    {
        ui->btnRecord->setText(tr(""));
        ui->btnRecord->SetPixMapFilePath(":/AVBaseWidget/images/ForwardButton.png");
        ui->btnRecord->setToolTip(tr("停止录制"));
    }
    else
    {
        ui->btnRecord->setText(tr(""));
        ui->btnRecord->SetPixMapFilePath(":/AVBaseWidget/images/PlayerButton.png");
        ui->btnRecord->setToolTip(tr("录制"));
    }
}

void ControlButtonWidget::SetCurrentAudioFile(const QString& filePath)
{
    if (m_currentAudioFile != filePath)
    {
        m_currentAudioFile = filePath;
        UpdateButtonEnabledStates();
        emit SigCurrentAudioFileChanged(filePath);
    }
}

QString ControlButtonWidget::GetCurrentAudioFile() const
{
    return m_currentAudioFile;
}

void ControlButtonWidget::UpdateButtonEnabledStates()
{
    bool hasFile = !m_currentAudioFile.isEmpty();

    // 播放相关按钮只有在有文件时才能使用
    ui->btnPlay->setEnabled(hasFile);
    ui->btnForward->setEnabled(hasFile);
    ui->btnBackward->setEnabled(hasFile);

    // 录制按钮始终可用
    ui->btnRecord->setEnabled(true);
}

void ControlButtonWidget::SetButtonEnabled(EM_ControlButtonType buttonType, bool enabled)
{
    CustomToolButton* button = GetButtonByType(buttonType);
    if (button)
    {
        button->setEnabled(enabled);
    }
}

bool ControlButtonWidget::IsButtonEnabled(EM_ControlButtonType buttonType) const
{
    CustomToolButton* button = GetButtonByType(buttonType);
    return button ? button->isEnabled() : false;
}

CustomToolButton* ControlButtonWidget::GetButtonByType(EM_ControlButtonType buttonType) const
{
    switch (buttonType)
    {
        case EM_ControlButtonType::Record:
            return ui->btnRecord;
        case EM_ControlButtonType::Previous:
            return ui->btnPrevious;
        case EM_ControlButtonType::Backward:
            return ui->btnBackward;
        case EM_ControlButtonType::Play:
            return ui->btnPlay;
        case EM_ControlButtonType::Forward:
            return ui->btnForward;
        case EM_ControlButtonType::Next:
            return ui->btnNext;
        default:
            return nullptr;
    }
}

void ControlButtonWidget::SetProgressValue(int value)
{
    ui->musicProgressBar->SetPosition(value);
}

void ControlButtonWidget::SetDuration(qint64 duration)
{
    ui->musicProgressBar->SetDuration(duration);
}

void ControlButtonWidget::SetBufferProgress(int bufferProgress)
{
    ui->musicProgressBar->SetBufferPosition(bufferProgress);
}
