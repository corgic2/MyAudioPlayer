#include "AVBaseWidget.h"
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
#include <QInputDialog>
#include <QDateTime>
#include <QDir>
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
    InitializePlaylistManager();
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
    connect(ui->ControlButtons, &ControlButtonWidget::SigProgressChanged , this, &AVBaseWidget::SlotProgressBarValueChanged);

    // 连接播放进度更新定时器
    connect(m_playTimer, &QTimer::timeout, this, &AVBaseWidget::SlotUpdatePlayProgress);

    // 连接MediaPlayerManager的信号
    if (m_playerManager)
    {
        connect(m_playerManager, &MediaPlayerManager::SigRecordStateChanged, this, [this](bool isRecording)
        {
            ui->ControlButtons->UpdateRecordState(isRecording);
        });
        connect(m_playerManager, &MediaPlayerManager::SigPlayerFinished, this, &AVBaseWidget::SlotAVPlayFinished);
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

    // 使用MediaPlayerManager播放媒体文件
    if (m_playerManager->PlayMedia(filePath, startPosition))
    {
        // 获取并设置总时长（转换为毫秒）
        double duration = m_playerManager->GetDuration();
        qint64 durationMs = static_cast<qint64>(duration * 1000);
        ui->ControlButtons->SetDuration(durationMs);
        
        // 设置初始进度（毫秒）
        qint64 startPositionMs = static_cast<qint64>(startPosition * 1000);
        ui->ControlButtons->SetProgressValue(startPositionMs);
        
        m_playTimer->start();
        LOG_INFO("Media playback started successfully, duration: " + std::to_string(duration) + " seconds");
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
    ui->ControlButtons->UpdatePlayState(false);
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
        ui->audioFileList->setCurrentItem(nextItem);
        ui->ControlButtons->UpdatePlayState(true);
        // 复用现有播放器，不创建新线程
        StartAVPlay(m_currentAVFile, 0.0);
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
        ui->audioFileList->setCurrentItem(prevItem);
        ui->ControlButtons->UpdatePlayState(true);
        // 复用现有播放器，不创建新线程
        StartAVPlay(filePath, 0.0);
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
        std::string stdPath = filePath.toStdString();
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
    ui->audioFileList->SaveFileListToJson();
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
    ui->audioFileList->SaveFileListToJson();
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
    ui->audioFileList->SaveFileListToJson();
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

void AVBaseWidget::SlotProgressBarValueChanged(qint64 value)
{
    if (m_isProgressBarUpdating || !m_playerManager)
    {
        return;
    }

    // 计算跳转位置（value是毫秒，需要转换为秒）
    double duration = m_playerManager->GetDuration();
    if (duration > 0)
    {
        m_currentPosition = value / 1000.0;

        // 跳转播放位置（转换为秒）
        m_playerManager->SeekPlay(m_currentPosition);
    }
}

void AVBaseWidget::SlotUpdatePlayProgress()
{
    if (!m_playerManager)
    {
        return;
    }

    m_isProgressBarUpdating = true;

    double currentPos = m_playerManager->GetCurrentPosition();
    double duration = m_playerManager->GetDuration();

    m_currentPosition = currentPos;

    if (duration > 0)
    {
        // 更新进度条（转换为毫秒）
        qint64 currentPosMs = static_cast<qint64>(currentPos * 1000);
        ui->ControlButtons->SetProgressValue(currentPosMs);
    }

    m_isProgressBarUpdating = false;

    // 检查播放是否完成
    if (currentPos >= duration && duration > 0)
    {
        emit SigAVPlayFinished();
    }
}

void AVBaseWidget::InitializePlaylistManager()
{
    // 创建ContentDirectory文件夹
    QString playlistDir = GetPlaylistDirectory();
    QDir dir;
    if (!dir.exists(playlistDir))
    {
        dir.mkpath(playlistDir);
    }

    // 连接左侧歌单列表的信号
    connect(ui->AudioDirectory, &FilePathIconListWidget::SigItemSelected, this, &AVBaseWidget::SlotPlaylistSelected);
    connect(ui->AudioDirectory, &FilePathIconListWidget::SigItemDoubleClicked, this, &AVBaseWidget::SlotPlaylistSelected);

    // 连接添加/删除按钮的信号
    connect(ui->AddDirectory, &QAbstractButton::clicked, this, &AVBaseWidget::SlotAddDirectoryClicked);
    connect(ui->DeleteDirectory, &QAbstractButton::clicked, this, &AVBaseWidget::SlotDeleteDirectoryClicked);

    // 设置左侧歌单列表的JSON文件路径
    QString playlistsJsonPath = GetPlaylistDirectory() + "/playlists.json";
    ui->AudioDirectory->SetJsonFilePath(playlistsJsonPath);
    ui->AudioDirectory->EnableAutoSave(true);
    ui->AudioDirectory->SetAutoSaveInterval(1800000); // 30分钟

    // 加载歌单列表
    LoadPlaylists();
}

QString AVBaseWidget::GetPlaylistDirectory() const
{
    return QApplication::applicationDirPath() + "/ContentDirectory";
}

void AVBaseWidget::LoadPlaylists()
{
    if (!ui->AudioDirectory->LoadFileListFromJson())
    {
        // 如果没有歌单，创建默认歌单
        CreateDefaultPlaylist();
    }
}

void AVBaseWidget::CreateDefaultPlaylist()
{
    QString defaultPlaylistPath = GetPlaylistDirectory() + "/default_playlist.json";
    
    // 创建默认歌单文件
    FilePathIconListWidgetItem::ST_NodeInfo playlistInfo;
    playlistInfo.filePath = defaultPlaylistPath;
    playlistInfo.displayName = "默认歌单";
    playlistInfo.iconPath = "";
    
    ui->AudioDirectory->AddFileItem(playlistInfo);
    ui->AudioDirectory->SaveFileListToJson();
    
    // 创建空的默认歌单内容文件
    QFile defaultPlaylistFile(defaultPlaylistPath);
    if (defaultPlaylistFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        defaultPlaylistFile.write("{}");
        defaultPlaylistFile.close();
    }
}

void AVBaseWidget::AddNewPlaylist()
{
    QString playlistName = QInputDialog::getText(this, tr("新建歌单"), tr("请输入歌单名称:"));
    if (playlistName.isEmpty())
    {
        return;
    }

    // 生成唯一的文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = playlistName + "_" + timestamp + ".json";
    QString playlistPath = GetPlaylistDirectory() + "/" + filename;

    // 创建新的歌单文件
    FilePathIconListWidgetItem::ST_NodeInfo playlistInfo;
    playlistInfo.filePath = playlistPath;
    playlistInfo.displayName = playlistName;
    playlistInfo.iconPath = "";

    ui->AudioDirectory->AddFileItem(playlistInfo);
    ui->AudioDirectory->SaveFileListToJson();

    // 创建空的歌单内容文件
    QFile playlistFile(playlistPath);
    if (playlistFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        playlistFile.write("{}");
        playlistFile.close();
    }

    // 自动选中新创建的歌单
    ui->AudioDirectory->setCurrentItem(ui->AudioDirectory->item(ui->AudioDirectory->GetItemCount() - 1));
    LoadPlaylistContent(playlistPath);
}

void AVBaseWidget::DeleteCurrentPlaylist()
{
    FilePathIconListWidgetItem* currentItem = ui->AudioDirectory->GetCurrentItem();
    if (!currentItem)
    {
        QMessageBox::warning(this, tr("警告"), tr("请先选择要删除的歌单"));
        return;
    }

    QString playlistName = currentItem->GetNodeInfo().displayName;
    QString playlistPath = currentItem->GetNodeInfo().filePath;

    // 不允许删除默认歌单
    if (playlistName == "默认歌单")
    {
        QMessageBox::warning(this, tr("警告"), tr("默认歌单不能被删除"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("确认删除"), 
        tr("确定要删除歌单 \"%1\" 吗？\n此操作将同时删除对应的JSON文件。").arg(playlistName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        // 删除JSON文件
        QFile::remove(playlistPath);

        // 从列表中移除
        ui->AudioDirectory->RemoveItem(currentItem);
        ui->AudioDirectory->SaveFileListToJson();

        // 清空音频文件列表
        ui->audioFileList->Clear();

        // 如果没有歌单了，创建默认歌单
        if (ui->AudioDirectory->GetItemCount() == 0)
        {
            CreateDefaultPlaylist();
        }
        else
        {
            // 选中第一个歌单
            ui->AudioDirectory->setCurrentItem(ui->AudioDirectory->item(0));
            SlotPlaylistSelected(ui->AudioDirectory->GetItem(0)->GetNodeInfo().filePath);
        }
    }
}

void AVBaseWidget::LoadPlaylistContent(const QString& playlistPath)
{
    // 设置音频文件列表的JSON路径为当前歌单的路径
    ui->audioFileList->SetJsonFilePath(playlistPath);
    ui->audioFileList->LoadFileListFromJson();
}

void AVBaseWidget::SlotPlaylistSelected(const QString& playlistPath)
{
    LoadPlaylistContent(playlistPath);
}

void AVBaseWidget::SlotAddDirectoryClicked()
{
    AddNewPlaylist();
}

void AVBaseWidget::SlotDeleteDirectoryClicked()
{
    DeleteCurrentPlaylist();
}
