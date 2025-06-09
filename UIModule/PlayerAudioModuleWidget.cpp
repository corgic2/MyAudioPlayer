#include "PlayerAudioModuleWidget.h"
#include "AudioMainWidget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include <QApplication>
#include "CommonDefine/UIWidgetColorDefine.h"
#include "DomainWidget/FilePathIconListWidgetItem.h"
#include "CoreWidget/CustomLabel.h"
#include "CoreWidget/CustomToolButton.h"
#include "CoreWidget/CustomComboBox.h"
#include "UtilsWidget/CustomToolTips.h"

PlayerAudioModuleWidget::PlayerAudioModuleWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PlayerAudioModuleWidgetClass())
    , m_isRecording(false)
    , m_isPlaying(false)
    , m_playTimer(nullptr)
{
    ui->setupUi(this);
    InitializeWidget();
    ConnectSignals();
    InitAudioDevices();
}

void PlayerAudioModuleWidget::InitializeWidget()
{
    // 初始化播放定时器
    m_playTimer = new QTimer(this);
    m_playTimer->setInterval(100); // 100ms更新一次

    // 设置文件列表标签样式
    ui->labelFileList->SetFontSize(CustomLabel::FontSize_Medium);
    ui->labelFileList->SetFontStyle(CustomLabel::FontStyle_Bold);
    ui->labelFileList->SetFontColor(UIColorDefine::font_color::Primary);
    ui->labelFileList->SetBackgroundType(CustomLabel::BackgroundType_Transparent);
    ui->labelFileList->SetBackgroundColor(UIColorDefine::background_color::Transparent);
    ui->labelFileList->EnableBorder(true, UIColorDefine::border_color::Default, 1);
    
    // 设置文件列表样式
    ui->audioFileList->SetBackgroundColor(UIColorDefine::background_color::Light);
    ui->audioFileList->SetItemHoverColor(UIColorDefine::background_color::HoverBackground);
    ui->audioFileList->SetItemSelectedColor(UIColorDefine::theme_color::Info);
    ui->audioFileList->SetItemTextColor(UIColorDefine::font_color::Primary);
    ui->audioFileList->SetItemHeight(40);
    ui->audioFileList->SetBorderWidth(1);
    ui->audioFileList->SetBorderColor(UIColorDefine::border_color::Default);

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

    // 设置暂停按钮样式
    ui->btnPause->SetBackgroundType(CustomToolButton::BackgroundType_Solid);
    ui->btnPause->SetBackgroundColor(UIColorDefine::theme_color::Info);
    ui->btnPause->SetFontColor(UIColorDefine::font_color::White);
    ui->btnPause->SetTipsType(CustomToolTips::Info);
    ui->btnPause->SetBorderWidth(1);
    ui->btnPause->SetBorderColor(UIColorDefine::border_color::Default);
    ui->btnPause->setEnabled(false);

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

    // 设置输入设备标签样式
    ui->labelInput->SetFontSize(CustomLabel::FontSize_Normal);
    ui->labelInput->SetFontStyle(CustomLabel::FontStyle_Normal);
    ui->labelInput->SetFontColor(UIColorDefine::font_color::Primary);
    ui->labelInput->SetBackgroundType(CustomLabel::BackgroundType_Transparent);
    ui->labelInput->SetBackgroundColor(UIColorDefine::background_color::Transparent);
    ui->labelInput->EnableBorder(true, UIColorDefine::border_color::Default, 1);

    // 设置输入设备下拉框样式
    ui->comboBoxInput->SetFontSize(CustomComboBox::FontSize_Normal);
    ui->comboBoxInput->SetBackgroundType(CustomComboBox::BackgroundType_Transparent);
    ui->comboBoxInput->SetBackgroundColor(UIColorDefine::background_color::Transparent);
    ui->comboBoxInput->SetBorderWidth(1);
    ui->comboBoxInput->SetBorderColor(UIColorDefine::border_color::Default);
}

void PlayerAudioModuleWidget::ConnectSignals()
{
    // 录制和播放按钮
    connect(ui->btnRecord, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnRecordClicked);
    connect(ui->btnPlay, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnPlayClicked);
    connect(ui->btnPause, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnPauseClicked);

    // 前进后退按钮
    connect(ui->btnForward, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnForwardClicked);
    connect(ui->btnBackward, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnBackwardClicked);

    // 上一个下一个按钮
    connect(ui->btnNext, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnNextClicked);
    connect(ui->btnPrevious, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnPreviousClicked);

    // 输入设备选择
    connect(ui->comboBoxInput, QOverload<int>::of(&CustomComboBox::currentIndexChanged),
            this, &PlayerAudioModuleWidget::SlotInputDeviceChanged);

    // 文件列表信号
    connect(ui->audioFileList, &FilePathIconListWidget::SigItemSelected,
            this, &PlayerAudioModuleWidget::SlotAudioFileSelected);
    connect(ui->audioFileList, &FilePathIconListWidget::SigItemDoubleClicked,
            this, &PlayerAudioModuleWidget::SlotAudioFileDoubleClicked);
}

PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    //     if (m_isRecording)
    //     {
    //         m_ffmpeg.StopAudioRecording();
    //     }
    //     if (m_isPlaying)
    //     {
    //         m_ffmpeg.StopAudioPlayback();
    //     }
    delete ui;
}

void PlayerAudioModuleWidget::SlotBtnRecordClicked()
{
    //     if (!m_isRecording)
    //     {
    //         QString filePath = QFileDialog::getSaveFileName(this,
    //             tr("保存录音文件"),
    //             QDir::currentPath(),
    //             tr("Wave Files (*.wav)"));
    // 
    //         if (!filePath.isEmpty())
    //         {
    //             m_isRecording = true;
    //             ui->btnRecord->setText(tr("停止录制"));
    //             ui->btnRecord->SetBackgroundColor(UIColorDefine::theme_color::Error);
    //             m_ffmpeg.StartAudioRecording(filePath, "wav");
    //         }
    //     }
    //     else
    //     {
    //         m_isRecording = false;
    //         ui->btnRecord->setText(tr("录制"));
    //         ui->btnRecord->SetBackgroundColor(UIColorDefine::theme_color::Primary);
    //         m_ffmpeg.StopAudioRecording();
    //     }
}

void PlayerAudioModuleWidget::SlotBtnPlayClicked()
{
    if (!m_currentAudioFile.isEmpty())
    {
        if (!m_isPlaying)
        {
            m_isPlaying = true;
            UpdatePlayState(true);
            m_ffmpeg.StartAudioPlayback(m_currentAudioFile);
        }
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请先选择要播放的音频文件"));
    }
}

void PlayerAudioModuleWidget::SlotBtnPauseClicked()
{
    //if (m_isPlaying)
    //{
    //    m_isPlaying = false;
    //    UpdatePlayState(false);
    //    m_ffmpeg.PauseAudioPlayback();
    //}
}

void PlayerAudioModuleWidget::SlotBtnForwardClicked()
{
    //if (m_isPlaying)
    //{
    //    m_ffmpeg.SeekAudioPlayback(15); // 前进15秒
    //}
}

void PlayerAudioModuleWidget::SlotBtnBackwardClicked()
{
    //if (m_isPlaying)
    //{
    //    m_ffmpeg.SeekAudioPlayback(-15); // 后退15秒
    //}
}

void PlayerAudioModuleWidget::SlotBtnNextClicked()
{
    // 获取当前项的索引
    //     FilePathIconListWidgetItem* currentItem = ui->audioFileList->GetCurrentItem();
    //     if (currentItem)
    //     {
    //         int currentIndex = ui->audioFileList->GetItemCount() - 1;
    //         for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
    //         {
    //             if (ui->audioFileList->GetItem(i) == currentItem)
    //             {
    //                 currentIndex = i;
    //                 break;
    //             }
    //         }
    // 
    //         // 切换到下一个文件
    //         if (currentIndex < ui->audioFileList->GetItemCount() - 1)
    //         {
    //             FilePathIconListWidgetItem* nextItem = ui->audioFileList->GetItem(currentIndex + 1);
    //             if (nextItem)
    //             {
    //                 m_currentAudioFile = nextItem->GetNodeInfo().filePath;
    //                 if (m_isPlaying)
    //                 {
    //                     m_ffmpeg.StopAudioPlayback();
    //                     m_ffmpeg.StartAudioPlayback(m_currentAudioFile);
    //                 }
    //             }
    //         }
    //     }
}

void PlayerAudioModuleWidget::SlotBtnPreviousClicked()
{
    // 获取当前项的索引
    //     FilePathIconListWidgetItem* currentItem = ui->audioFileList->GetCurrentItem();
    //     if (currentItem)
    //     {
    //         int currentIndex = 0;
    //         for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
    //         {
    //             if (ui->audioFileList->GetItem(i) == currentItem)
    //             {
    //                 currentIndex = i;
    //                 break;
    //             }
    //         }
    // 
    //         // 切换到上一个文件
    //         if (currentIndex > 0)
    //         {
    //             FilePathIconListWidgetItem* prevItem = ui->audioFileList->GetItem(currentIndex - 1);
    //             if (prevItem)
    //             {
    //                 m_currentAudioFile = prevItem->GetNodeInfo().filePath;
    //                 if (m_isPlaying)
    //                 {
    //                     m_ffmpeg.StopAudioPlayback();
    //                     m_ffmpeg.StartAudioPlayback(m_currentAudioFile);
    //                 }
    //             }
    //         }
    //     }
}

void PlayerAudioModuleWidget::InitAudioDevices()
{
    // 获取并设置输入设备
    QStringList inputDevices = m_ffmpeg.GetInputAudioDevices();
    ui->comboBoxInput->clear();
    ui->comboBoxInput->addItems(inputDevices);
    if (!inputDevices.isEmpty())
    {
        m_ffmpeg.SetInputDevice(inputDevices.first());
    }

    if (inputDevices.size() == 0)
    {
        ui->comboBoxInput->setEnabled(false);
    }
    else
    {
        ui->comboBoxInput->setEnabled(true);
    }
}

void PlayerAudioModuleWidget::SlotInputDeviceChanged(int index)
{
    QString deviceName = ui->comboBoxInput->itemText(index);
    m_ffmpeg.SetInputDevice(deviceName);
}

void PlayerAudioModuleWidget::SlotAudioFileSelected(const QString& filePath)
{
    m_currentAudioFile = filePath;
}

void PlayerAudioModuleWidget::SlotAudioFileDoubleClicked(const QString& filePath)
{
    m_currentAudioFile = filePath;
    SlotBtnPlayClicked(); // 自动开始播放
}

void PlayerAudioModuleWidget::UpdatePlayState(bool isPlaying)
{
    if (isPlaying)
    {
        ui->btnPlay->setText(tr("停止"));
        ui->btnPlay->SetBackgroundColor(UIColorDefine::theme_color::Error);
        ui->btnPause->setEnabled(true);
    }
    else
    {
        ui->btnPlay->setText(tr("播放"));
        ui->btnPlay->SetBackgroundColor(UIColorDefine::theme_color::Success);
        ui->btnPause->setEnabled(false);
    }
}

void PlayerAudioModuleWidget::AddAudioFiles(const QStringList& filePaths)
{
    for (const QString& filePath : filePaths)
    {
        FilePathIconListWidgetItem::ST_NodeInfo nodeInfo;
        nodeInfo.filePath = filePath;
        nodeInfo.displayName = QFileInfo(filePath).fileName();
        nodeInfo.filePath = ":/icons/audio.png";

        ui->audioFileList->AddFileItem(nodeInfo);
    }
}

void PlayerAudioModuleWidget::RemoveAudioFile(const QString& filePath)
{
    //FilePathIconListWidgetItem* item = nullptr;
    //for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
    //{
    //    FilePathIconListWidgetItem* currentItem = ui->audioFileList->GetItem(i);
    //    if (currentItem && currentItem->GetNodeInfo().filePath == filePath)
    //    {
    //        item = currentItem;
    //        break;
    //    }
    //}

    //if (item)
    //{
    //     如果正在播放这个文件，先停止播放
    //    if (m_currentAudioFile == filePath && m_isPlaying)
    //    {
    //        m_ffmpeg.StopAudioPlayback();
    //        m_isPlaying = false;
    //        UpdatePlayState(false);
    //    }

    //    ui->audioFileList->RemoveItem(item);
    //    
    //     如果移除的是当前文件，清空当前文件路径
    //    if (m_currentAudioFile == filePath)
    //    {
    //        m_currentAudioFile.clear();
    //    }
    //}
}

void PlayerAudioModuleWidget::ClearAudioFiles()
{
    //// 如果正在播放，先停止播放
    //if (m_isPlaying)
    //{
    //    m_ffmpeg.StopAudioPlayback();
    //    m_isPlaying = false;
    //    UpdatePlayState(false);
    //}

    //ui->audioFileList->Clear();
    //m_currentAudioFile.clear();
}
