#include "AVBaseWidget.h"
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

#include "ControlButtonWidget.h"
#include "CoreServerGlobal.h"
#include "../AVFileSystem/AVFileSystem.h"
#include "BasePlayer/FFmpegPublicUtils.h"
#include "BasePlayer/MediaPlayerManager.h"
#include "CommonDefine/UIWidgetColorDefine.h"
#include "CoreWidget/CustomComboBox.h"
#include "CoreWidget/CustomLabel.h"
#include "DomainWidget/FilePathIconListWidget.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"
#include "TimeSystem/TimeSystem.h"

AVBaseWidget::AVBaseWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::AVBaseWidgetClass())
{
    ui->setupUi(this);

    // 获取媒体播放管理器实例
    m_playerManager = MediaPlayerManager::Instance();

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
        StopAVRecord();
    }
    if (GetIsPlaying())
    {
        StopAVPlay();
    }

    SAFE_DELETE_POINTER_VALUE(m_playTimer)
    SAFE_DELETE_POINTER_VALUE(ui);
    SAFE_DELETE_POINTER_VALUE(m_audioPlayerWidget);
    SAFE_DELETE_POINTER_VALUE(m_videoPlayerWidget);
}

void AVBaseWidget::InitializeWidget()
{
    // 初始化播放定时器
    m_playTimer = new QTimer(this);
    m_playTimer->setInterval(100); // 100ms更新一次

    // 设置各个Frame的边框样式
    ui->AVFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    ui->AVFrame->setLineWidth(1);
    ui->AVFrame->setMidLineWidth(0);
    // 清空显示
    ui->RightLayoutFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    ui->RightLayoutFrame->setLineWidth(1);
    ui->RightLayoutFrame->setMidLineWidth(0);
    ui->customProgressBar->setFixedHeight(15);
    // 设置进度条范围
    ui->customProgressBar->setRange(0, 100); // 使用1000为最大值，提高精度
    ui->customProgressBar->SetProgressValue(0);

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
    connect(ui->customProgressBar, &CustomProgressBar::SigProgressValueChanged, this, &AVBaseWidget::SlotProgressBarValueChanged);

    // 连接播放进度更新定时器
    connect(m_playTimer, &QTimer::timeout, this, &AVBaseWidget::SlotUpdatePlayProgress);

    // 连接MediaPlayerManager的信号
    if (m_playerManager)
    {
        connect(m_playerManager, &MediaPlayerManager::SigRecordStateChanged, this, [this](bool isRecording)
        {
            ui->ControlButtons->UpdateRecordState(isRecording);
        });
    }
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
            // 启动录制
            StartAVRecord(filePath);
        }
    }
    else
    {
        StopAVRecord();
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
            if (GetIsPaused())
            {
                LOG_INFO("恢复播放");
                if (m_playerManager)
                {
                    m_playerManager->ResumePlay();
                }
            }
            else
            {
                LOG_INFO("开始播放");
                StartAVPlay(m_currentAVFile);
            }
            ui->ControlButtons->UpdatePlayState(true);
        }
        else
        {
            LOG_INFO("暂停播放");
            if (m_playerManager)
            {
                m_playerManager->PausePlay();
                ui->ControlButtons->UpdatePlayState(false);
            }
        }
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请先选择要播放的媒体文件"));
    }

    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Play, true);
}

void AVBaseWidget::StartAVPlay(const QString& filePath, double startPosition)
{
    LOG_INFO("Starting AV play: " + filePath.toStdString() + ", position: " + std::to_string(startPosition));

    if (!m_playerManager)
    {
        LOG_WARN("MediaPlayerManager is null");
        return;
    }

    // 确定文件类型并显示相应的控件
    if (av_fileSystem::AVFileSystem::IsAudioFile(filePath.toStdString()))
    {
        m_audioPlayerWidget->LoadWaveWidegt(filePath);
        ShowAVWidget(true);
    }
    else if (av_fileSystem::AVFileSystem::IsVideoFile(filePath.toStdString()))
    {
        // 设置视频显示控件
        m_playerManager->SetVideoDisplayWidget(m_videoPlayerWidget);
        ShowAVWidget(false);
    }
    else
    {
        QMessageBox::warning(this, "错误", "不支持的文件类型: " + filePath);
        return;
    }

    // 使用MediaPlayerManager播放媒体文件
    if (m_playerManager->PlayMedia(filePath, startPosition))
    {
        m_playTimer->start();
        LOG_INFO("Media playback started successfully");

        // 更新文件信息显示
        UpdateFileInfoDisplay(filePath);
    }
    else
    {
        m_playTimer->stop();
        LOG_WARN("Failed to start media playback");
        QMessageBox::warning(this, "错误", "播放失败");
    }
}

void AVBaseWidget::StopAVPlay()
{
    LOG_INFO("Stopping AV play");

    if (m_playerManager)
    {
        m_playerManager->StopPlay();
    }

    // 停止进度更新定时器
    m_playTimer->stop();

    LOG_INFO("媒体播放已停止");
}

void AVBaseWidget::SlotAVPlayFinished()
{
    StopAVPlay();

    // 播放完成后重置播放位置和进度条
    m_currentPosition = 0.0;
    ui->customProgressBar->SetProgressValue(0);
    ui->customProgressBar->setFormat("00:00 / 00:00");

    LOG_INFO("媒体播放完成，状态已重置");
}

void AVBaseWidget::SlotBtnForwardClicked()
{
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Forward, false);

    if (GetIsPlaying() && m_playerManager)
    {
        // 快进15秒
        double currentPos = m_playerManager->GetCurrentPosition();
        double duration = m_playerManager->GetDuration();
        double newPosition = std::min(duration, currentPos + 15.0);
        m_playerManager->SeekPlay(newPosition);
        m_currentPosition = newPosition;
    }

    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Forward, true);
}

void AVBaseWidget::SlotBtnBackwardClicked()
{
    ui->ControlButtons->SetButtonEnabled(ControlButtonWidget::EM_ControlButtonType::Backward, false);

    if (GetIsPlaying() && m_playerManager)
    {
        // 快退15秒
        double currentPos = m_playerManager->GetCurrentPosition();
        double newPosition = std::max(0.0, currentPos - 15.0);
        m_playerManager->SeekPlay(newPosition);
        m_currentPosition = newPosition;
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

        // 直接切换到下一个文件，复用现有播放器
        bool wasPlaying = GetIsPlaying();
        ui->audioFileList->setCurrentItem(nextItem);

        if (wasPlaying)
        {
            // 复用现有播放器，不创建新线程
            StartAVPlay(m_currentAVFile, 0.0);
        }

        emit SigAVFileSelected(filePath);
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

        // 直接切换到上一个文件，复用现有播放器
        bool wasPlaying = GetIsPlaying();
        ui->audioFileList->setCurrentItem(prevItem);

        if (wasPlaying)
        {
            // 复用现有播放器，不创建新线程
            StartAVPlay(filePath, 0.0);
        }

        emit SigAVFileSelected(filePath);
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

    // 更新文件信息显示
    UpdateFileInfoDisplay(filePath);

    emit SigAVFileSelected(m_currentAVFile);
}

void AVBaseWidget::SlotAVFileDoubleClicked(const QString& filePath)
{
    // 切换到不同的文件，重置播放位置
    m_currentPosition = 0.0;
    m_currentAVFile = filePath;
    ui->ControlButtons->SetCurrentAudioFile(filePath);
    ui->ControlButtons->UpdatePlayState(true);
    // 直接开始播放新文件，复用现有播放器
    StartAVPlay(filePath, 0.0);

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
            StopAVPlay();
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
        StopAVPlay();
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
    LOG_INFO("AVBaseWidget closing - stopping all playback and recording");
    
    // 停止所有播放和录制
    StopAVPlay();
    StopAVRecord();
    
    // 等待工作线程完成
    if (m_playerManager)
    {
        // 确保播放管理器完成清理
        m_playerManager->StopPlay();
        m_playerManager->StopRecording();
    }
    
    event->accept();
}

void AVBaseWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    m_playerManager->ResizeVideoWindows(ui->AVFrame->width(), ui->AVFrame->height());
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

void AVBaseWidget::StartAVRecord(const QString& filePath)
{
    LOG_INFO("Starting AV record: " + filePath.toStdString());

    if (!m_playerManager)
    {
        LOG_WARN("MediaPlayerManager is null");
        return;
    }

    // 根据当前文件类型或默认设置确定录制类型
    auto recordType = EM_MediaType::Audio; // 默认录制音频

    if (!m_currentAVFile.isEmpty())
    {
        if (av_fileSystem::AVFileSystem::IsVideoFile(m_currentAVFile.toStdString()))
        {
            recordType = EM_MediaType::Video;
        }
    }

    // 使用MediaPlayerManager开始录制
    m_playerManager->StartRecording(filePath, recordType);
}

void AVBaseWidget::StopAVRecord()
{
    LOG_INFO("Stopping AV record");

    if (!m_playerManager)
    {
        LOG_WARN("MediaPlayerManager is null");
        return;
    }

    // 使用MediaPlayerManager停止录制
    m_playerManager->StopRecording();
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
    emit SigAVRecordFinished();
}

bool AVBaseWidget::GetIsPlaying() const
{
    return m_playerManager && m_playerManager->IsPlaying();
}

bool AVBaseWidget::GetIsPaused() const
{
    return m_playerManager && m_playerManager->IsPaused();
}

bool AVBaseWidget::GetIsRecording() const
{
    return m_playerManager && m_playerManager->IsRecording();
}

void AVBaseWidget::SlotProgressBarValueChanged(int value)
{
    if (m_isProgressBarUpdating || !m_playerManager)
    {
        return;
    }

    // 计算跳转位置
    double duration = m_playerManager->GetDuration();
    if (duration > 0)
    {
        double seekPosition = (static_cast<double>(value) / 1000.0) * duration;
        m_currentPosition = seekPosition;

        // 跳转播放位置
        if (GetIsPlaying())
        {
            m_playerManager->SeekPlay(seekPosition);
        }
    }
}

void AVBaseWidget::SlotUpdatePlayProgress()
{
    if (!m_playerManager || !GetIsPlaying())
    {
        return;
    }

    m_isProgressBarUpdating = true;

    double currentPos = m_playerManager->GetCurrentPosition();
    double duration = m_playerManager->GetDuration();

    m_currentPosition = currentPos;

    if (duration > 0)
    {
        int progressValue = static_cast<int>((currentPos / duration) * 100);
        ui->customProgressBar->SetProgressValue(progressValue);

        // 更新音频波形图的播放位置（如果是音频文件）
        if (m_playerManager->GetCurrentMediaType() == EM_MediaType::Audio && m_audioPlayerWidget)
        {
            m_audioPlayerWidget->UpdateWaveformPosition(currentPos, duration);
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

void AVBaseWidget::UpdateFileInfoDisplay(const QString& filePath)
{
    if (filePath.isEmpty())
    {
        // 清空显示
        ui->labelFileName->setText("文件名：--");
        ui->labelFileSize->setText("文件大小：--");
        ui->labelDuration->setText("时长：--");
        ui->labelFormat->setText("格式：--");
        ui->labelBitrate->setText("比特率：--");
        ui->labelResolution->setText("分辨率：--");
        ui->labelSampleRate->setText("采样率：--");
        ui->labelChannels->setText("声道：--");
        return;
    }

    QString fileName;
    qint64 fileSize = 0;
    double duration = 0.0;
    QString format;
    int bitrate = 0;
    int width = 0;
    int height = 0;
    int sampleRate = 0;
    int channels = 0;

    if (FFmpegPublicUtils::GetMediaFileInfo(filePath, fileName, fileSize, duration, format, bitrate, width, height, sampleRate, channels))
    {
        // 更新显示信息
        ui->labelFileName->setText(QString("文件名：%1").arg(fileName));

        // 格式化文件大小
        QString sizeText;
        if (fileSize < 1024)
        {
            sizeText = QString("%1 B").arg(fileSize);
        }
        else if (fileSize < 1024 * 1024)
        {
            sizeText = QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 1);
        }
        else if (fileSize < 1024 * 1024 * 1024)
        {
            sizeText = QString("%1 MB").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 1);
        }
        else
        {
            sizeText = QString("%1 GB").arg(fileSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
        }
        ui->labelFileSize->setText(QString("文件大小：%1").arg(sizeText));

        // 格式化时长
        ui->labelDuration->setText(QString("时长：%1").arg(FormatTime(static_cast<int>(duration))));

        // 格式化格式
        ui->labelFormat->setText(QString("格式：%1").arg(format.toUpper()));

        // 格式化比特率
        if (bitrate > 0)
        {
            QString bitrateText;
            if (bitrate < 1000)
            {
                bitrateText = QString("%1 bps").arg(bitrate);
            }
            else if (bitrate < 1000000)
            {
                bitrateText = QString("%1 kbps").arg(bitrate / 1000);
            }
            else
            {
                bitrateText = QString("%1 Mbps").arg(bitrate / 1000000.0, 0, 'f', 1);
            }
            ui->labelBitrate->setText(QString("比特率：%1").arg(bitrateText));
        }
        else
        {
            ui->labelBitrate->setText("比特率：--");
        }

        // 分辨率（视频文件）
        if (width > 0 && height > 0 && av_fileSystem::AVFileSystem::IsVideoFile(filePath.toStdString()))
        {
            ui->labelResolution->setText(QString("分辨率：%1x%2").arg(width).arg(height));
        }
        else
        {
            ui->labelResolution->setText("分辨率：--");
        }

        // 采样率（音频文件）
        if (sampleRate > 0)
        {
            ui->labelSampleRate->setText(QString("采样率：%1 Hz").arg(sampleRate));
        }
        else
        {
            ui->labelSampleRate->setText("采样率：--");
        }

        // 声道数
        if (channels > 0)
        {
            QString channelsText;
            switch (channels)
            {
                case 1:
                    channelsText = "单声道";
                    break;
                case 2:
                    channelsText = "立体声";
                    break;
                case 6:
                    channelsText = "5.1 声道";
                    break;
                case 8:
                    channelsText = "7.1 声道";
                    break;
                default:
                    channelsText = QString("%1 声道").arg(channels);
                    break;
            }
            ui->labelChannels->setText(QString("声道：%1").arg(channelsText));
        }
        else
        {
            ui->labelChannels->setText("声道：--");
        }
    }
    else
    {
        // 获取信息失败，只显示文件名和大小
        QFileInfo fileInfo(filePath);
        ui->labelFileName->setText(QString("文件名：%1").arg(fileInfo.fileName()));

        qint64 size = fileInfo.size();
        QString sizeText;
        if (size < 1024)
        {
            sizeText = QString("%1 B").arg(size);
        }
        else if (size < 1024 * 1024)
        {
            sizeText = QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
        }
        else if (size < 1024 * 1024 * 1024)
        {
            sizeText = QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
        }
        else
        {
            sizeText = QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
        }
        ui->labelFileSize->setText(QString("文件大小：%1").arg(sizeText));

        // 其他信息设为不可用
        ui->labelDuration->setText("时长：获取失败");
        ui->labelFormat->setText("格式：获取失败");
        ui->labelBitrate->setText("比特率：获取失败");
        ui->labelResolution->setText("分辨率：获取失败");
        ui->labelSampleRate->setText("采样率：获取失败");
        ui->labelChannels->setText("声道：获取失败");
    }
}
