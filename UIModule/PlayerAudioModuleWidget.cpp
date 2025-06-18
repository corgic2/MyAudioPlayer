#include "PlayerAudioModuleWidget.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QThread>
#include <QTimer>
#include "AudioMainWidget.h"

#include "AudioFileSystem.h"
#include "CoreServerGlobal.h"
#include "CommonDefine/UIWidgetColorDefine.h"
#include "CoreWidget/CustomComboBox.h"
#include "CoreWidget/CustomLabel.h"
#include "CoreWidget/CustomToolButton.h"
#include "DomainWidget/FilePathIconListWidgetItem.h"
#include "UtilsWidget/CustomToolTips.h"

PlayerAudioModuleWidget::PlayerAudioModuleWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::PlayerAudioModuleWidgetClass())
{
    ui->setupUi(this);
    InitializeWidget();
    ConnectSignals();
    InitAudioDevices();

    // 设置文件列表的JSON文件路径并加载
    QString jsonPath = QApplication::applicationDirPath() + "/audiofiles.json";
    ui->audioFileList->SetJsonFilePath(jsonPath);
    ui->audioFileList->EnableAutoSave(true);
    ui->audioFileList->SetAutoSaveInterval(1800000); // 30分钟
    ui->audioFileList->LoadFileListFromJson();
}

void PlayerAudioModuleWidget::InitializeWidget()
{
    // 初始化播放定时器
    m_playTimer = new QTimer(this);
    m_playTimer->setInterval(100); // 100ms更新一次

    // 初始化波形控件
    m_waveformWidget = new AudioWaveformWidget(ui->VedioFrame);
    m_waveformWidget->setGeometry(0, 0, ui->VedioFrame->width(), ui->VedioFrame->height());

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
    ui->VedioFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    ui->VedioFrame->setLineWidth(1);
    ui->VedioFrame->setMidLineWidth(0);

    ui->RightLayoutFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    ui->RightLayoutFrame->setLineWidth(1);
    ui->RightLayoutFrame->setMidLineWidth(0);

    ui->ToolButtonFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    ui->ToolButtonFrame->setLineWidth(1);
    ui->ToolButtonFrame->setMidLineWidth(0);
}

void PlayerAudioModuleWidget::ConnectSignals()
{
    // 连接音频控制按钮的信号
    connect(ui->audioControlButtons, &AudioControlButtonWidget::SigRecordClicked, this, &PlayerAudioModuleWidget::SlotBtnRecordClicked);
    connect(ui->audioControlButtons, &AudioControlButtonWidget::SigPlayClicked, this, &PlayerAudioModuleWidget::SlotBtnPlayClicked);
    connect(ui->audioControlButtons, &AudioControlButtonWidget::SigForwardClicked, this, &PlayerAudioModuleWidget::SlotBtnForwardClicked);
    connect(ui->audioControlButtons, &AudioControlButtonWidget::SigBackwardClicked, this, &PlayerAudioModuleWidget::SlotBtnBackwardClicked);
    connect(ui->audioControlButtons, &AudioControlButtonWidget::SigNextClicked, this, &PlayerAudioModuleWidget::SlotBtnNextClicked);
    connect(ui->audioControlButtons, &AudioControlButtonWidget::SigPreviousClicked, this, &PlayerAudioModuleWidget::SlotBtnPreviousClicked);

    // 输入设备选择
    connect(ui->comboBoxInput, QOverload<int>::of(&CustomComboBox::currentIndexChanged), this, &PlayerAudioModuleWidget::SlotInputDeviceChanged);

    // 文件列表信号
    connect(ui->audioFileList, &FilePathIconListWidget::SigItemSelected, this, &PlayerAudioModuleWidget::SlotAudioFileSelected);
    connect(ui->audioFileList, &FilePathIconListWidget::SigItemDoubleClicked, this, &PlayerAudioModuleWidget::SlotAudioFileDoubleClicked);

    // 音频播放完成信号
    connect(this, &PlayerAudioModuleWidget::SigAudioPlayFinished, this, &PlayerAudioModuleWidget::SlotAudioPlayFinished);

    // 添加录制完成信号连接
    connect(this, &PlayerAudioModuleWidget::SigAudioRecordFinished, this, &PlayerAudioModuleWidget::SlotAudioRecordFinished);
}

PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    if (m_isRecording)
    {
        StopAudioRecordThread();
    }
    if (m_isPlaying)
    {
        StopAudioPlayThread();
    }

    SAFE_DELETE_POINTER_VALUE(m_playTimer)
    CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_playThreadId);
    CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_recordThreadId);
    delete ui;
}

void PlayerAudioModuleWidget::SlotBtnRecordClicked()
{
    if (!m_isRecording)
    {
        QString filePath = QFileDialog::getSaveFileName(this, tr("保存录音文件"), QDir::currentPath(), tr("Wave Files (*.wav)"));

        if (!filePath.isEmpty())
        {
            m_isRecording = true;
            ui->audioControlButtons->UpdateRecordState(true);

            // 启动录制线程
            StartAudioRecordThread(filePath);
        }
    }
    else
    {
        m_isRecording = false;
        ui->audioControlButtons->UpdateRecordState(false);
        StopAudioRecordThread();
    }
}

void PlayerAudioModuleWidget::SlotBtnPlayClicked()
{
    if (!m_currentAudioFile.isEmpty())
    {
        if (!m_isPlaying)
        {
            m_isPlaying = true;
            m_isPaused = false;
            ui->audioControlButtons->UpdatePlayState(true);
            StartAudioPlayThread();
        }
        else
        {
            m_isPlaying = false;
            m_isPaused = false;
            ui->audioControlButtons->UpdatePlayState(false);
            StopAudioPlayThread();
        }
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请先选择要播放的音频文件"));
    }
}

void PlayerAudioModuleWidget::StartAudioPlayThread()
{
    if (m_playThreadRunning)
    {
        StopAudioPlayThread();
    }

    m_playThreadRunning = true;
    m_playThreadId = CoreServerGlobal::Instance().GetThreadPool().CreateDedicatedThread("AudioPlayThread", [this]()
    {
        try
        {
            // 在播放前加载音频数据并生成波形
            QVector<float> waveformData;
            if (m_ffmpeg.LoadAudioWaveform(m_currentAudioFile, waveformData))
            {
                // 在主线程中更新波形显示
                QMetaObject::invokeMethod(this, [this, waveformData]()
                {
                    m_waveformWidget->SetWaveformData(waveformData);
                }, Qt::QueuedConnection);
            }

            // 从指定位置开始播放
            m_ffmpeg.StartAudioPlayback(m_currentAudioFile, m_currentPosition);

            // 等待播放完成
            while (m_playThreadRunning && m_ffmpeg.IsPlaying())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // 播放完成，发送信号
            if (m_playThreadRunning)
            {
                emit SigAudioPlayFinished();
            }
        } catch (const std::exception& e)
        {
            qDebug() << "Audio playback error:" << e.what();
            emit SigAudioPlayFinished();
        }
    });
}

void PlayerAudioModuleWidget::StopAudioPlayThread()
{
    if (m_playThreadRunning)
    {
        m_playThreadRunning = false;
        CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_playThreadId);
        m_ffmpeg.StopAudioPlayback();
        if (m_currentPosition < 1e-5)
        {
            m_waveformWidget->ClearWaveform();
        }
    }
}

void PlayerAudioModuleWidget::SlotAudioPlayFinished()
{
    m_isPlaying = false;
    m_isPaused = false;
    ui->audioControlButtons->UpdatePlayState(false);
    m_playThreadRunning = false;
}

void PlayerAudioModuleWidget::SlotBtnForwardClicked()
{
    if (m_isPlaying)
    {
        // 停止当前播放线程
        StopAudioPlayThread();

        // 启动新的播放线程，从新位置开始播放
        m_currentPosition += 15;
        StartAudioPlayThread();
    }
}

void PlayerAudioModuleWidget::SlotBtnBackwardClicked()
{
    if (m_isPlaying)
    {
        // 停止当前播放线程
        StopAudioPlayThread();

        // 启动新的播放线程，从新位置开始播放
        m_currentPosition = std::max(0.0, m_currentPosition - 15.0);
        StartAudioPlayThread();
    }
}

void PlayerAudioModuleWidget::SlotBtnNextClicked()
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
                m_currentAudioFile = filePath;
                ui->audioControlButtons->SetCurrentAudioFile(filePath);
                if (m_isPlaying)
                {
                    StopAudioPlayThread();
                    StartAudioPlayThread();
                }
                ui->audioFileList->setCurrentItem(nextItem);
            }
        }
    }
}

void PlayerAudioModuleWidget::SlotBtnPreviousClicked()
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
                m_currentAudioFile = filePath;
                ui->audioControlButtons->SetCurrentAudioFile(filePath);
                if (m_isPlaying)
                {
                    StopAudioPlayThread();
                    StartAudioPlayThread();
                }
                ui->audioFileList->setCurrentItem(prevItem);
            }
        }
    }
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
    ui->audioControlButtons->SetCurrentAudioFile(filePath);
}

void PlayerAudioModuleWidget::SlotAudioFileDoubleClicked(const QString& filePath)
{
    m_currentAudioFile = filePath;
    ui->audioControlButtons->SetCurrentAudioFile(filePath);
    SlotBtnPlayClicked(); // 自动开始播放
}

void PlayerAudioModuleWidget::AddAudioFiles(const QStringList& filePaths)
{
    for (const QString& filePath : filePaths)
    {
        std::string stdPath = my_sdk::FileSystem::QtPathToStdPath(filePath.toStdString());
        if (!audio_player::AudioFileSystem::IsAudioFile(stdPath))
        {
            continue;
        }

        if (GetFileIndex(filePath) == -1)
        {
            audio_player::ST_AudioFileInfo audioInfo = audio_player::AudioFileSystem::GetAudioFileInfo(stdPath);
            FilePathIconListWidgetItem::ST_NodeInfo nodeInfo;
            nodeInfo.filePath = filePath;
            nodeInfo.displayName = QString::fromStdString(audioInfo.m_displayName);
            nodeInfo.iconPath = QString::fromStdString(audioInfo.m_iconPath);

            // 在列表顶部插入新项
            ui->audioFileList->InsertFileItem(0, nodeInfo);

            // 如果是第一个文件，自动选中
            if (ui->audioFileList->GetItemCount() == 1)
            {
                SlotAudioFileSelected(filePath);
            }
        }
    }
}

void PlayerAudioModuleWidget::RemoveAudioFile(const QString& filePath)
{
    int index = GetFileIndex(filePath);
    if (index != -1)
    {
        // 如果正在播放这个文件，先停止播放
        if (m_currentAudioFile == filePath && m_isPlaying)
        {
            StopAudioPlayThread();
            m_isPlaying = false;
            ui->audioControlButtons->UpdatePlayState(false);
        }

        ui->audioFileList->RemoveItemByIndex(index);

        // 如果移除的是当前文件，清空当前文件路径
        if (m_currentAudioFile == filePath)
        {
            m_currentAudioFile.clear();
            ui->audioControlButtons->SetCurrentAudioFile(QString());
        }
    }
}

void PlayerAudioModuleWidget::ClearAudioFiles()
{
    // 如果正在播放，先停止播放
    if (m_isPlaying)
    {
        StopAudioPlayThread();
        m_isPlaying = false;
        ui->audioControlButtons->UpdatePlayState(false);
    }

    ui->audioFileList->Clear();
    m_currentAudioFile.clear();
    ui->audioControlButtons->SetCurrentAudioFile(QString());
}

int PlayerAudioModuleWidget::GetFileIndex(const QString& filePath) const
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

void PlayerAudioModuleWidget::closeEvent(QCloseEvent* event)
{
    event->accept();
}

FilePathIconListWidgetItem::ST_NodeInfo PlayerAudioModuleWidget::GetFileInfo(int index) const
{
    FilePathIconListWidgetItem* item = ui->audioFileList->GetItem(index);
    if (item)
    {
        return item->GetNodeInfo();
    }
    return FilePathIconListWidgetItem::ST_NodeInfo();
}

void PlayerAudioModuleWidget::StartAudioRecordThread(const QString& filePath)
{
    if (m_recordThreadRunning)
    {
        StopAudioRecordThread();
    }

    m_recordThreadRunning = true;
    m_recordThreadId = CoreServerGlobal::Instance().GetThreadPool().CreateDedicatedThread("AudioRecordThread", [this, filePath]()
    {
        try
        {
            m_ffmpeg.StartAudioRecording(filePath, "wav");

            // 等待录制停止
            while (m_recordThreadRunning && m_ffmpeg.IsRecording())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // 录制完成，发送信号
            if (m_recordThreadRunning)
            {
                emit SigAudioRecordFinished();
            }
        } catch (const std::exception& e)
        {
            qDebug() << "Audio recording error:" << e.what();
            emit SigAudioRecordFinished();
        }
    });
}

void PlayerAudioModuleWidget::StopAudioRecordThread()
{
    if (m_recordThreadRunning)
    {
        m_recordThreadRunning = false;
        m_ffmpeg.StopAudioRecording();
        CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_recordThreadId);
    }
}

void PlayerAudioModuleWidget::SlotAudioRecordFinished()
{
    m_isRecording = false;
    ui->audioControlButtons->UpdateRecordState(false);
    m_recordThreadRunning = false;
}
