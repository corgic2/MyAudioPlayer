#include "ControlButtonWidget.h"

#include "ControlButtonWidget.h"
#include "ui_ControlButtonWidget.h"
#include "CommonDefine/UIWidgetColorDefine.h"
#include "UtilsWidget/CustomToolTips.h"

ControlButtonWidget::ControlButtonWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ControlButtonWidget())
{
    ui->setupUi(this);
    InitializeWidget();
    ConnectSignals();
}

ControlButtonWidget::~ControlButtonWidget()
{
    delete ui;
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
    UpdateButtonStates();
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
        ui->btnPlay->setText(tr("停止"));
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
        UpdateButtonStates();
        emit SigCurrentAudioFileChanged(filePath);
    }
}

QString ControlButtonWidget::GetCurrentAudioFile() const
{
    return m_currentAudioFile;
}

void ControlButtonWidget::UpdateButtonStates()
{
    bool hasFile = !m_currentAudioFile.isEmpty();

    // 播放相关按钮只有在有文件时才能使用
    ui->btnPlay->setEnabled(hasFile);
    ui->btnForward->setEnabled(hasFile);
    ui->btnBackward->setEnabled(hasFile);

    // 录制按钮始终可用
    ui->btnRecord->setEnabled(true);
}