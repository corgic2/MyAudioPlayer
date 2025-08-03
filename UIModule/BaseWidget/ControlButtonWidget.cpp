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
}

ControlButtonWidget::~ControlButtonWidget()
{
    SAFE_DELETE_POINTER_VALUE(ui);
}

void ControlButtonWidget::InitializeWidget()
{
    // 设置录制按钮样式
    ui->btnRecord->SetBackgroundType(CustomToolButton::BackgroundType_Solid);
    ui->btnRecord->SetBackgroundColor(UIColorDefine::theme_color::Info);
    ui->btnRecord->SetFontColor(UIColorDefine::font_color::White);
    ui->btnRecord->SetTipsType(CustomToolTips::Info);
    ui->btnRecord->SetBorderWidth(1);
    ui->btnRecord->SetBorderColor(UIColorDefine::border_color::Default);

    // 设置播放按钮样式
    ui->btnPlay->SetBackgroundType(CustomToolButton::BackgroundType_Solid);
    ui->btnPlay->SetBackgroundColor(UIColorDefine::theme_color::Success);
    ui->btnPlay->SetFontColor(UIColorDefine::font_color::White);
    ui->btnPlay->SetTipsType(CustomToolTips::Info);
    ui->btnPlay->SetBorderWidth(1);
    ui->btnPlay->SetBorderColor(UIColorDefine::border_color::Default);

    // 设置前进后退按钮样式
    auto SetControlButtonStyle = [](CustomToolButton* btn)
    {
        btn->SetBackgroundType(CustomToolButton::BackgroundType_Solid);
        btn->SetBackgroundColor(UIColorDefine::theme_color::Info);
        btn->SetFontColor(UIColorDefine::font_color::White);
        btn->SetTipsType(CustomToolTips::Info);
        btn->SetBorderWidth(1);
        btn->SetBorderColor(UIColorDefine::border_color::Default);
    };

    SetControlButtonStyle(ui->btnForward);
    SetControlButtonStyle(ui->btnBackward);
    SetControlButtonStyle(ui->btnNext);
    SetControlButtonStyle(ui->btnPrevious);

    // 初始化按钮状态
    UpdateButtonEnabledStates();
}

void ControlButtonWidget::ConnectSignals()
{
    connect(ui->btnRecord, &CustomToolButton::clicked, this, &ControlButtonWidget::SigRecordClicked);
    connect(ui->btnPlay, &CustomToolButton::clicked, this, &ControlButtonWidget::SigPlayClicked);
    connect(ui->btnForward, &CustomToolButton::clicked, this, &ControlButtonWidget::SigForwardClicked);
    connect(ui->btnBackward, &CustomToolButton::clicked, this, &ControlButtonWidget::SigBackwardClicked);
    connect(ui->btnNext, &CustomToolButton::clicked, this, &ControlButtonWidget::SigNextClicked);
    connect(ui->btnPrevious, &CustomToolButton::clicked, this, &ControlButtonWidget::SigPreviousClicked);
}

void ControlButtonWidget::UpdatePlayState(bool isPlaying)
{
    if (isPlaying)
    {
        ui->btnPlay->setText(tr("暂停"));
        ui->btnPlay->SetBackgroundColor(UIColorDefine::theme_color::Error);
    }
    else
    {
        ui->btnPlay->setText(tr("播放"));
        ui->btnPlay->SetBackgroundColor(UIColorDefine::theme_color::Success);
    }
}

void ControlButtonWidget::UpdateRecordState(bool isRecording)
{
    if (isRecording)
    {
        ui->btnRecord->setText(tr("停止录制"));
        ui->btnRecord->SetBackgroundColor(UIColorDefine::theme_color::Error);
    }
    else
    {
        ui->btnRecord->setText(tr("录制"));
        ui->btnRecord->SetBackgroundColor(UIColorDefine::theme_color::Info);
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

