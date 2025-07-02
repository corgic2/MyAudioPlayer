#include "AVBaseWidget.h"
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

#include "ControlButtonWidget.h"
#include "CoreServerGlobal.h"
#include "../AVFileSystem/AVFileSystem.h"
#include "CommonDefine/UIWidgetColorDefine.h"
#include "CoreWidget/CustomComboBox.h"
#include "CoreWidget/CustomLabel.h"
#include "DomainWidget/FilePathIconListWidget.h"
#include "FileSystem/FileSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"
#include "TimeSystem/TimeSystem.h"

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
    if (GetIsRecording())
    {
        StopAVRecordThread();
    }
    if (GetIsPlaying())
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

    // 设置现代化进度条样式
    ui->customProgressBar->SetProgressBarStyle(CustomProgressBar::ProgressBarStyle_Rounded);
    ui->customProgressBar->SetBackgroundColor(UIColorDefine::background_color::HoverBackground);
    ui->customProgressBar->SetProgressColor(UIColorDefine::theme_color::Primary);
    ui->customProgressBar->SetTextColor(UIColorDefine::font_color::Primary);
    ui->customProgressBar->SetEnableGradient(true);
    ui->customProgressBar->SetGradientStartColor(UIColorDefine::gradient_color::Primary.start);
    ui->customProgressBar->SetGradientEndColor(UIColorDefine::gradient_color::Primary.end);
    ui->customProgressBar->SetEnableAnimation(true);
    ui->customProgressBar->SetAnimationDuration(300);
    ui->customProgressBar->SetTextPosition(CustomProgressBar::TextPosition_Center);
    ui->customProgressBar->SetEnableBorder(true);
    ui->customProgressBar->SetBorderColor(UIColorDefine::border_color::Primary);
    ui->customProgressBar->SetBorderWidth(1);
    ui->customProgressBar->SetBorderRadius(8);
    ui->customProgressBar->SetEnableShadow(true);
    ui->customProgressBar->SetShadowColor(UIColorDefine::shadow_color::Light);
    ui->customProgressBar->setFixedHeight(15);
    // 设置进度条范围
    ui->customProgressBar->setRange(0, 1000); // 使用1000为最大值，提高精度
    ui->customProgressBar->setValue(0);

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

    // 连接进度条信号
    connect(ui->customProgressBar, &CustomProgressBar::SigValueChanged, this, &AVBaseWidget::SlotProgressBarValueChanged);

    // 连接播放进度更新定时器
    connect(m_playTimer, &QTimer::timeout, this, &AVBaseWidget::SlotUpdatePlayProgress);
}

void AVBaseWidget::SlotBtnRecordClicked()
{
    // 立即禁用录制按钮，防止快速点击
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Record, false);

    if (!GetIsRecording())
    {
        QString filePath = QFileDialog::getSaveFileName(this, tr("保存录音文件"), QDir::currentPath(), tr("Wave Files (*.wav)"));

        if (!filePath.isEmpty())
        {
            ui->ControlButtons->UpdateRecordState(true);

            // 启动录制线程
            StartAVRecordThread(filePath);
        }
    }
    else
    {
        ui->ControlButtons->UpdateRecordState(false);
        StopAVRecordThread();
    }
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Record, true);
}

void AVBaseWidget::SlotBtnPlayClicked()
{
    LOG_INFO("播放按钮被点击");
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Play, false);
    if (!m_currentAVFile.isEmpty())
    {
        if (!GetIsPlaying())
        {
            LOG_INFO("恢复播放");

            ui->ControlButtons->UpdatePlayState(true);
            StartAVPlayThread();
        }
        else
        {
            LOG_INFO("暂停播放");

            ui->ControlButtons->UpdatePlayState(false);
            StopAVPlayThread();
        }
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请先选择要播放的音频文件"));
    }
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Play, true);
}

void AVBaseWidget::StartAVPlayThread()
{
    if (m_playThreadRunning)
    {
        StopAVPlayThread();
    }

    if (av_fileSystem::AVFileSystem::IsAudioFile(m_currentAVFile.toStdString()))
    {
        m_ffmpeg = m_audioPlayerWidget->GetFFMpegUtils();
        m_audioPlayerWidget->LoadWaveWidegt(m_currentAVFile);
        ShowAVWidget(true);
    }
    else if (av_fileSystem::AVFileSystem::IsVideoFile(m_currentAVFile.toStdString()))
    {
        m_ffmpeg = m_videoPlayerWidget->GetFFMpegUtils();
        // 设置视频显示控件
        auto videoUtils = static_cast<VideoFFmpegUtils*>(m_ffmpeg);
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
        AUTO_TIMER_LOG("AVPlayThread", EM_TimingLogLevel::Info);

        try
        {
            // 从指定位置开始播放
            m_ffmpeg->StartPlay(m_currentAVFile, m_currentPosition);

            // 等待播放完成
            while (m_playThreadRunning && m_ffmpeg->IsPlaying())
            {
                TIME_SLEEP_MS(100);
            }

            // 播放完成，发送信号
            if (m_playThreadRunning)
            {
                emit SigAVPlayFinished();
            }
        } catch (const std::exception& e)
        {
            qDebug() << "AV playback error:" << e.what();
            emit SigAVPlayFinished();
        }
    });

    // 在播放开始后启动进度更新定时器
    if (m_playThreadRunning)
    {
        m_playTimer->start();
    }
}

void AVBaseWidget::StopAVPlayThread()
{
    if (m_playThreadRunning)
    {
        // 停止进度更新定时器
        m_playTimer->stop();

        m_playThreadRunning = false;

        // 首先停止FFmpeg播放器
        if (m_ffmpeg)
        {
            m_ffmpeg->StopPlay();
        }

        // 然后停止线程池中的专用线程
        CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_playThreadId);

        LOG_INFO("音频播放线程已停止");
    }
}

void AVBaseWidget::SlotAVPlayFinished()
{
    StopAVPlayThread();
    ui->ControlButtons->UpdatePlayState(false);
    m_playThreadRunning = false;

    // 播放完成后重置播放位置和进度条
    m_currentPosition = 0.0;
    ui->customProgressBar->setValue(0);
    ui->customProgressBar->setFormat("00:00 / 00:00");

    LOG_INFO("音频播放完成，状态已重置");
}

void AVBaseWidget::SlotBtnForwardClicked()
{
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Forward, false);
    if (GetIsPlaying() && m_ffmpeg)
    {
        // 快进15秒
        double duration = m_ffmpeg->GetDuration();
        m_currentPosition = std::min(duration, m_currentPosition + 15.0);
        m_ffmpeg->SeekPlay(m_currentPosition);
    }
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Forward, true);
}

void AVBaseWidget::SlotBtnBackwardClicked()
{
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Backward, false);

    if (GetIsPlaying() && m_ffmpeg)
    {
        // 快退15秒
        m_currentPosition = std::max(0.0, m_currentPosition - 15.0);
        m_ffmpeg->SeekPlay(m_currentPosition);
    }
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Backward, true);
}

void AVBaseWidget::SlotBtnNextClicked()
{
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Next, false);
    // 获取当前项的索引
    FilePathIconListWidgetItem* currentItem = ui->audioFileList->GetCurrentItem();
    int currentIndex = -1;
    if (currentItem)
    {
        currentIndex = ui->audioFileList->GetItemCount() - 1;
        for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
        {
            if (ui->audioFileList->GetItem(i) == currentItem)
            {
                currentIndex = i;
                break;
            }
        }
    }

    FilePathIconListWidgetItem* nextItem = ui->audioFileList->GetItem((currentIndex + 1) % ui->audioFileList->GetItemCount());
    if (nextItem)
    {
        QString filePath = nextItem->GetNodeInfo().filePath;
        m_currentAVFile = filePath;
        ui->ControlButtons->SetCurrentAudioFile(filePath);

        // 重置播放位置为从头开始
        m_currentPosition = 0.0;

        if (GetIsPlaying())
        {
            StopAVPlayThread();
        }
        ui->audioFileList->setCurrentItem(nextItem);
        StartAVPlayThread();
        ui->ControlButtons->UpdatePlayState(true);
    }

    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Next, true);
}

void AVBaseWidget::SlotBtnPreviousClicked()
{
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Previous, false);
    // 获取当前项的索引
    FilePathIconListWidgetItem* currentItem = ui->audioFileList->GetCurrentItem();
    int currentIndex = -1;
    if (currentItem)
    {
        currentIndex = 0;
        for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
        {
            if (ui->audioFileList->GetItem(i) == currentItem)
            {
                currentIndex = i;
                break;
            }
        }
    }
    FilePathIconListWidgetItem* prevItem = ui->audioFileList->GetItem((currentIndex - 1 + ui->audioFileList->GetItemCount()) % ui->audioFileList->GetItemCount());
    if (prevItem)
    {
        QString filePath = prevItem->GetNodeInfo().filePath;
        m_currentAVFile = filePath;
        ui->ControlButtons->SetCurrentAudioFile(filePath);

        // 重置播放位置为从头开始
        m_currentPosition = 0.0;

        if (GetIsPlaying())
        {
            StopAVPlayThread();
        }
        ui->audioFileList->setCurrentItem(prevItem);
        StartAVPlayThread();
        ui->ControlButtons->UpdatePlayState(true);
    }

    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Previous, true);
}

void AVBaseWidget::SlotAVFileSelected(const QString& filePath)
{
    // 如果切换到不同的文件，重置播放位置
    if (m_currentAVFile != filePath)
    {
        m_currentPosition = 0.0;
    }

    m_currentAVFile = filePath;
    ui->ControlButtons->SetCurrentAudioFile(filePath);
    emit SigAVFileSelected(m_currentAVFile);
}

void AVBaseWidget::SlotAVFileDoubleClicked(const QString& filePath)
{
    // 如果切换到不同的文件，重置播放位置
    m_currentPosition = 0.0;
    if (m_ffmpeg)
    {
        //停止播放
        SlotBtnPlayClicked();
    }
    m_currentAVFile = filePath;
    ui->ControlButtons->SetCurrentAudioFile(filePath);
    SlotBtnPlayClicked(); // 自动开始播放
    emit SigAVFileSelected(m_currentAVFile);
}

void AVBaseWidget::AddAVFiles(const QStringList& filePaths)
{
    LOG_INFO("添加音频文件到列表，文件数量: " + std::to_string(filePaths.size()));
    for (const QString& filePath : filePaths)
    {
        LOG_DEBUG("添加文件: " + filePath.toStdString());
        std::string stdPath = my_sdk::FileSystem::QtPathToStdPath(filePath.toStdString());
        if (!av_fileSystem::AVFileSystem::IsAVFile(stdPath))
        {
            continue;
        }

        if (GetFileIndex(filePath) == -1)
        {
            av_fileSystem::ST_AVFileInfo audioInfo = av_fileSystem::AVFileSystem::GetAVFileInfo(stdPath);
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
        if (m_currentAVFile == filePath && GetIsPlaying())
        {
            StopAVPlayThread();
            ui->ControlButtons->UpdatePlayState(false);
        }

        ui->audioFileList->RemoveItemByIndex(index);

        // 如果移除的是当前文件，清空当前文件路径
        if (m_currentAVFile == filePath)
        {
            m_currentAVFile.clear();
            ui->ControlButtons->SetCurrentAudioFile(QString());
            emit SigAVFileSelected(QString());
        }
    }
}

void AVBaseWidget::ClearAVFiles()
{
    // 如果正在播放，先停止播放
    if (GetIsPlaying())
    {
        StopAVPlayThread();
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
    if (av_fileSystem::AVFileSystem::IsAudioFile(m_currentAVFile.toStdString()))
    {
        m_ffmpeg = m_audioPlayerWidget->GetFFMpegUtils();
    }
    else if (av_fileSystem::AVFileSystem::IsVideoFile(m_currentAVFile.toStdString()))
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
        AUTO_TIMER_LOG("AVRecordThread", EM_TimingLogLevel::Info);

        try
        {
            m_ffmpeg->StartRecording(filePath);

            // 等待录制停止
            while (m_recordThreadRunning && m_ffmpeg->IsRecording())
            {
                TIME_SLEEP_MS(100);
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
    ui->ControlButtons->UpdateRecordState(false);
    m_recordThreadRunning = false;
    StopAVRecordThread();
}

bool AVBaseWidget::GetIsPlaying() const
{
    return m_ffmpeg && m_ffmpeg->IsPlaying();
}

bool AVBaseWidget::GetIsPaused() const
{
    return m_ffmpeg && m_ffmpeg->IsPaused();
}

bool AVBaseWidget::GetIsRecording() const
{
    return m_ffmpeg && m_ffmpeg->IsRecording();
}

void AVBaseWidget::SlotProgressBarValueChanged(int value)
{
    if (m_isProgressBarUpdating || !m_ffmpeg)
    {
        return;
    }

    // 计算跳转位置
    double duration = m_ffmpeg->GetDuration();
    if (duration > 0)
    {
        double seekPosition = (static_cast<double>(value) / 1000.0) * duration;
        m_currentPosition = seekPosition;

        // 跳转播放位置
        if (GetIsPlaying())
        {
            m_ffmpeg->SeekPlay(seekPosition);
        }
    }
}

void AVBaseWidget::SlotUpdatePlayProgress()
{
    if (!m_ffmpeg || !GetIsPlaying())
    {
        return;
    }

    m_isProgressBarUpdating = true;

    double currentPos = m_ffmpeg->GetCurrentPosition();
    double duration = m_ffmpeg->GetDuration();

    m_currentPosition = currentPos;

    if (duration > 0)
    {
        int progressValue = static_cast<int>((currentPos / duration) * 1000);
        ui->customProgressBar->SetValueWithAnimation(progressValue);
        
        AudioFFmpegUtils* audioFFmpeg = dynamic_cast<AudioFFmpegUtils*>(m_ffmpeg);
        if (audioFFmpeg)
        {
            // 更新音频波形图的播放位置
            if (m_audioPlayerWidget)
            {
                m_audioPlayerWidget->UpdateWaveformPosition(currentPos, duration);
            }
        }
        
        // 更新进度条文本显示
        QString timeText = QString("%1 / %2").arg(FormatTime(static_cast<int>(currentPos))).arg(FormatTime(static_cast<int>(duration)));
        ui->customProgressBar->setFormat(timeText);
    }

    m_isProgressBarUpdating = false;

    // 检查播放是否完成
    if (currentPos >= duration && duration > 0)
    {
        emit SigAVPlayFinished();
    }
}

QString AVBaseWidget::FormatTime(int seconds) const
{
    int minutes = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}
