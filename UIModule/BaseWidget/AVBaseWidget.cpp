#include "AVBaseWidget.h"
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

#include "../AVFileSystem/AVFileSystem.h"
#include "FileSystem/FileSystem.h"
#include "ControlButtonWidget.h"
#include "CoreServerGlobal.h"
#include "CoreWidget/CustomComboBox.h"
#include "CoreWidget/CustomLabel.h"
#include "DomainWidget/FilePathIconListWidget.h"
#include "CommonDefine/UIWidgetColorDefine.h"
#include "SDKCommonDefine/SDKCommonDefine.h"

AVBaseWidget::AVBaseWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::AVBaseWidgetClass())
{
    ui->setupUi(this);
    InitializeWidget();
    ConnectSignals();

    // 设置文件列表的JSON文件路径并加载
    QString jsonPath = QApplication::applicationDirPath() + "/audiofiles.json";
    ui->audioFileList->SetJsonFilePath(jsonPath);
    ui->audioFileList->EnableAutoSave(true);
    ui->audioFileList->SetAutoSaveInterval(1800000); // 30分钟
    ui->audioFileList->LoadFileListFromJson();
}

AVBaseWidget::~AVBaseWidget()
{
    if (m_isRecording)
    {
        StopAVRecordThread();
    }
    if (m_isPlaying)
    {
        StopAVPlayThread();
    }

    SAFE_DELETE_POINTER_VALUE(m_playTimer)
    CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_playThreadId);
    CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_recordThreadId);
    SAFE_DELETE_POINTER_VALUE(ui);
    SAFE_DELETE_POINTER_VALUE(m_audioPlayerWidget);
    SAFE_DELETE_POINTER_VALUE(m_videoPlayerWidget);
}

void AVBaseWidget::InitializeWidget()
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

    // 设置文件列表样式
    ui->audioFileList->SetBackgroundColor(UIColorDefine::background_color::White);
    ui->audioFileList->SetItemHoverColor(UIColorDefine::background_color::HoverBackground);
    ui->audioFileList->SetItemSelectedColor(UIColorDefine::theme_color::Info);
    ui->audioFileList->SetItemTextColor(UIColorDefine::font_color::Primary);
    ui->audioFileList->SetItemHeight(40);
    ui->audioFileList->SetBorderWidth(1);
    ui->audioFileList->SetBorderColor(UIColorDefine::border_color::Default);

    // 设置各个Frame的边框样式
    ui->AVFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    ui->AVFrame->setLineWidth(1);
    ui->AVFrame->setMidLineWidth(0);

    ui->RightLayoutFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    ui->RightLayoutFrame->setLineWidth(1);
    ui->RightLayoutFrame->setMidLineWidth(0);

    ui->ToolButtonFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    ui->ToolButtonFrame->setLineWidth(1);
    ui->ToolButtonFrame->setMidLineWidth(0);
    m_audioPlayerWidget = new PlayerAudioModuleWidget(this);
    m_videoPlayerWidget = new PlayerVideoModuleWidget(this);
    ui->verticalLayout_6->addWidget(m_audioPlayerWidget);
    ui->verticalLayout_6->addWidget(m_videoPlayerWidget);
    m_audioPlayerWidget->hide();
    m_videoPlayerWidget->hide();
}

void AVBaseWidget::ConnectSignals()
{
    // 连接音频控制按钮的信号
    connect(ui->ControlButtons, &ControlButtonWidget::SigRecordClicked, this, &AVBaseWidget::SlotBtnRecordClicked);
    connect(ui->ControlButtons, &ControlButtonWidget::SigPlayClicked, this, &AVBaseWidget::SlotBtnPlayClicked);
    connect(ui->ControlButtons, &ControlButtonWidget::SigForwardClicked, this, &AVBaseWidget::SlotBtnForwardClicked);
    connect(ui->ControlButtons, &ControlButtonWidget::SigBackwardClicked, this, &AVBaseWidget::SlotBtnBackwardClicked);
    connect(ui->ControlButtons, &ControlButtonWidget::SigNextClicked, this, &AVBaseWidget::SlotBtnNextClicked);
    connect(ui->ControlButtons, &ControlButtonWidget::SigPreviousClicked, this, &AVBaseWidget::SlotBtnPreviousClicked);

    // 文件列表信号
    connect(ui->audioFileList, &FilePathIconListWidget::SigItemSelected, this, &AVBaseWidget::SlotAVFileSelected);
    connect(ui->audioFileList, &FilePathIconListWidget::SigItemDoubleClicked, this, &AVBaseWidget::SlotAVFileDoubleClicked);

    // 音频播放完成信号
    connect(this, &AVBaseWidget::SigAVPlayFinished, this, &AVBaseWidget::SlotAVPlayFinished);

    // 添加录制完成信号连接
    connect(this, &AVBaseWidget::SigAVRecordFinished, this, &AVBaseWidget::SlotAVRecordFinished);
}

void AVBaseWidget::SlotBtnRecordClicked()
{
    if (!m_isRecording)
    {
        QString filePath = QFileDialog::getSaveFileName(this, tr("保存录音文件"), QDir::currentPath(), tr("Wave Files (*.wav)"));

        if (!filePath.isEmpty())
        {
            m_isRecording = true;
            ui->ControlButtons->UpdateRecordState(true);

            // 启动录制线程
            StartAVRecordThread(filePath);
        }
    }
    else
    {
        m_isRecording = false;
        ui->ControlButtons->UpdateRecordState(false);
        StopAVRecordThread();
    }
}

void AVBaseWidget::SlotBtnPlayClicked()
{
    LOG_INFO("播放按钮被点击");
    if (!m_currentAVFile.isEmpty())
    {
        if (!m_isPlaying)
        {
            LOG_INFO("恢复播放");

            m_isPlaying = true;
            m_isPaused = false;
            ui->ControlButtons->UpdatePlayState(true);
            StartAVPlayThread();
        }
        else
        {
            LOG_INFO("暂停播放");

            m_isPlaying = false;
            m_isPaused = false;
            ui->ControlButtons->UpdatePlayState(false);
            StopAVPlayThread();
        }
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请先选择要播放的音频文件"));
    }
}

void AVBaseWidget::StartAVPlayThread()
{
    if (m_playThreadRunning)
    {
        StopAVPlayThread();
    }
    
    if (AV_player::AVFileSystem::IsAudioFile(m_currentAVFile.toStdString()))
    {
        m_ffmpeg = m_audioPlayerWidget->GetFFMpegUtils();
        m_audioPlayerWidget->LoadWaveWidegt(m_currentAVFile);
        ShowAVWidget(true);
    }
    else if (AV_player::AVFileSystem::IsVideoFile(m_currentAVFile.toStdString()))
    {
        m_ffmpeg = m_videoPlayerWidget->GetFFMpegUtils();
        
        // 设置视频显示控件
        VideoFFmpegUtils* videoUtils = static_cast<VideoFFmpegUtils*>(m_ffmpeg);
        videoUtils->SetVideoDisplayWidget(m_videoPlayerWidget);
        
        ShowAVWidget(false);
    }
    else
    {
        qDebug() << "Unsupported file type for playback:" << m_currentAVFile;
        return;
    }

    m_playThreadRunning = true;
    m_playThreadId = CoreServerGlobal::Instance().GetThreadPool().CreateDedicatedThread("AVPlayThread", [this]()
    {
        try
        {
            // 从指定位置开始播放
            m_ffmpeg->StartPlay(m_currentAVFile, m_currentPosition);

            // 等待播放完成
            while (m_playThreadRunning && m_ffmpeg->IsPlaying())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // 播放完成，发送信号
            if (m_playThreadRunning)
            {
                emit SigAVPlayFinished();
            }
        } 
        catch (const std::exception& e)
        {
            qDebug() << "AV playback error:" << e.what();
            emit SigAVPlayFinished();
        }
    });
}

void AVBaseWidget::StopAVPlayThread()
{
    if (m_playThreadRunning)
    {
        m_playThreadRunning = false;
        CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_playThreadId);
        m_ffmpeg->StopPlay();
    }
}

void AVBaseWidget::SlotAVPlayFinished()
{
    m_isPlaying = false;
    m_isPaused = false;
    ui->ControlButtons->UpdatePlayState(false);
    m_playThreadRunning = false;
}

void AVBaseWidget::SlotBtnForwardClicked()
{
    if (m_isPlaying)
    {
        // 停止当前播放线程
        StopAVPlayThread();

        // 启动新的播放线程，从新位置开始播放
        m_currentPosition += 15;
        StartAVPlayThread();
    }
}

void AVBaseWidget::SlotBtnBackwardClicked()
{
    if (m_isPlaying)
    {
        // 停止当前播放线程
        StopAVPlayThread();

        // 启动新的播放线程，从新位置开始播放
        m_currentPosition = std::max(0.0, m_currentPosition - 15.0);
        StartAVPlayThread();
    }
}

void AVBaseWidget::SlotBtnNextClicked()
{
    // 获取当前项的索引
    FilePathIconListWidgetItem* currentItem = ui->audioFileList->GetCurrentItem();
    if (currentItem)
    {
        int currentIndex = ui->audioFileList->GetItemCount() - 1;
        for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
        {
            if (ui->audioFileList->GetItem(i) == currentItem)
            {
                currentIndex = i;
                break;
            }
        }

        // 切换到下一个文件
        if (currentIndex < ui->audioFileList->GetItemCount() - 1)
        {
            FilePathIconListWidgetItem* nextItem = ui->audioFileList->GetItem(currentIndex + 1);
            if (nextItem)
            {
                QString filePath = nextItem->GetNodeInfo().filePath;
                m_currentAVFile = filePath;
                ui->ControlButtons->SetCurrentAudioFile(filePath);
                if (m_isPlaying)
                {
                    StopAVPlayThread();
                    StartAVPlayThread();
                }
                ui->audioFileList->setCurrentItem(nextItem);
            }
        }
    }
}

void AVBaseWidget::SlotBtnPreviousClicked()
{
    // 获取当前项的索引
    FilePathIconListWidgetItem* currentItem = ui->audioFileList->GetCurrentItem();
    if (currentItem)
    {
        int currentIndex = 0;
        for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
        {
            if (ui->audioFileList->GetItem(i) == currentItem)
            {
                currentIndex = i;
                break;
            }
        }

        // 切换到上一个文件
        if (currentIndex > 0)
        {
            FilePathIconListWidgetItem* prevItem = ui->audioFileList->GetItem(currentIndex - 1);
            if (prevItem)
            {
                QString filePath = prevItem->GetNodeInfo().filePath;
                m_currentAVFile = filePath;
                ui->ControlButtons->SetCurrentAudioFile(filePath);
                if (m_isPlaying)
                {
                    StopAVPlayThread();
                    StartAVPlayThread();
                }
                ui->audioFileList->setCurrentItem(prevItem);
            }
        }
    }
}

void AVBaseWidget::SlotAVFileSelected(const QString& filePath)
{
    m_currentAVFile = filePath;
    ui->ControlButtons->SetCurrentAudioFile(filePath);
}

void AVBaseWidget::SlotAVFileDoubleClicked(const QString& filePath)
{
    m_currentAVFile = filePath;
    ui->ControlButtons->SetCurrentAudioFile(filePath);
    SlotBtnPlayClicked(); // 自动开始播放
}

void AVBaseWidget::AddAVFiles(const QStringList& filePaths)
{
    LOG_INFO("添加音频文件到列表，文件数量: " + std::to_string(filePaths.size()));
    for (const QString& filePath : filePaths)
    {
        LOG_DEBUG("添加文件: " + filePath.toStdString());
        std::string stdPath = my_sdk::FileSystem::QtPathToStdPath(filePath.toStdString());
        if (!AV_player::AVFileSystem::IsAVFile(stdPath))
        {
            continue;
        }

        if (GetFileIndex(filePath) == -1)
        {
            AV_player::ST_AVFileInfo audioInfo = AV_player::AVFileSystem::GetAVFileInfo(stdPath);
            FilePathIconListWidgetItem::ST_NodeInfo nodeInfo;
            nodeInfo.filePath = filePath;
            nodeInfo.displayName = QString::fromStdString(audioInfo.m_displayName);
            nodeInfo.iconPath = QString::fromStdString(audioInfo.m_iconPath);

            // 在列表顶部插入新项
            ui->audioFileList->InsertFileItem(0, nodeInfo);

            // 如果是第一个文件，自动选中
            if (ui->audioFileList->GetItemCount() == 1)
            {
                SlotAVFileSelected(filePath);
            }
        }
    }
}

void AVBaseWidget::RemoveAVFile(const QString& filePath)
{
    int index = GetFileIndex(filePath);
    if (index != -1)
    {
        // 如果正在播放这个文件，先停止播放
        if (m_currentAVFile == filePath && m_isPlaying)
        {
            StopAVPlayThread();
            m_isPlaying = false;
            ui->ControlButtons->UpdatePlayState(false);
        }

        ui->audioFileList->RemoveItemByIndex(index);

        // 如果移除的是当前文件，清空当前文件路径
        if (m_currentAVFile == filePath)
        {
            m_currentAVFile.clear();
            ui->ControlButtons->SetCurrentAudioFile(QString());
        }
    }
}

void AVBaseWidget::ClearAVFiles()
{
    // 如果正在播放，先停止播放
    if (m_isPlaying)
    {
        StopAVPlayThread();
        m_isPlaying = false;
        ui->ControlButtons->UpdatePlayState(false);
    }

    ui->audioFileList->Clear();
    m_currentAVFile.clear();
    ui->ControlButtons->SetCurrentAudioFile(QString());
}

int AVBaseWidget::GetFileIndex(const QString& filePath) const
{
    for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
    {
        FilePathIconListWidgetItem* item = ui->audioFileList->GetItem(i);
        if (item && item->GetNodeInfo().filePath == filePath)
        {
            return i;
        }
    }
    return -1;
}

void AVBaseWidget::closeEvent(QCloseEvent* event)
{
    event->accept();
}

FilePathIconListWidgetItem::ST_NodeInfo AVBaseWidget::GetFileInfo(int index) const
{
    FilePathIconListWidgetItem* item = ui->audioFileList->GetItem(index);
    if (item)
    {
        return item->GetNodeInfo();
    }
    return FilePathIconListWidgetItem::ST_NodeInfo();
}

void AVBaseWidget::StartAVRecordThread(const QString& filePath)
{
    if (m_recordThreadRunning)
    {
        StopAVRecordThread();
    }
    if (AV_player::AVFileSystem::IsAudioFile(m_currentAVFile.toStdString()))
    {
        m_ffmpeg = m_audioPlayerWidget->GetFFMpegUtils();
    }
    else if (AV_player::AVFileSystem::IsVideoFile(m_currentAVFile.toStdString()))
    {
        m_ffmpeg = m_videoPlayerWidget->GetFFMpegUtils();
    }
    else
    {
        qDebug() << "Unsupported file type for playback:" << m_currentAVFile;
        return;
    }
    m_recordThreadRunning = true;
    m_recordThreadId = CoreServerGlobal::Instance().GetThreadPool().CreateDedicatedThread("AudioRecordThread", [this, filePath]()
    {
        try
        {
            m_ffmpeg->StartRecording(filePath);

            // 等待录制停止
            while (m_recordThreadRunning && m_ffmpeg->IsRecording())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // 录制完成，发送信号
            if (m_recordThreadRunning)
            {
                emit SigAVRecordFinished();
            }
        } catch (const std::exception& e)
        {
            qDebug() << "Audio recording error:" << e.what();
            emit SigAVRecordFinished();
        }
    });
}

void AVBaseWidget::StopAVRecordThread()
{
    if (m_recordThreadRunning)
    {
        m_recordThreadRunning = false;
        m_ffmpeg->StopRecording();
        CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_recordThreadId);
    }
}

void AVBaseWidget::ShowAVWidget(bool bAudio)
{
    if (bAudio)
    {
        m_audioPlayerWidget->show();
        m_videoPlayerWidget->hide();
    }
    else
    {
        m_audioPlayerWidget->hide();
        m_videoPlayerWidget->show();
    }
}

void AVBaseWidget::SlotAVRecordFinished()
{
    m_isRecording = false;
    ui->ControlButtons->UpdateRecordState(false);
    m_recordThreadRunning = false;
}
