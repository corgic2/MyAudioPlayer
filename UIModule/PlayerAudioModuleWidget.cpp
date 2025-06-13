#include "PlayerAudioModuleWidget.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
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
    : QWidget(parent), ui(new Ui::PlayerAudioModuleWidgetClass()), m_isRecording(false), m_isPlaying(false), m_isPaused(false), m_playTimer(nullptr), m_jsonFileName("audiofiles.json"), m_autoSaveTimer(nullptr)
{
    ui->setupUi(this);
    InitializeWidget();
    ConnectSignals();
    InitAudioDevices();
    LoadFileListFromJson(); // 加载保存的文件列表
    InitializeAutoSaveTimer();
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

    // 设置文件列表样式
    ui->audioFileList->SetBackgroundColor(UIColorDefine::background_color::White);
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
    // 录制和播放按钮
    connect(ui->btnRecord, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnRecordClicked);
    connect(ui->btnPlay, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnPlayClicked);

    // 前进后退按钮
    connect(ui->btnForward, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnForwardClicked);
    connect(ui->btnBackward, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnBackwardClicked);

    // 上一个下一个按钮
    connect(ui->btnNext, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnNextClicked);
    connect(ui->btnPrevious, &CustomToolButton::clicked, this, &PlayerAudioModuleWidget::SlotBtnPreviousClicked);

    // 输入设备选择
    connect(ui->comboBoxInput, QOverload<int>::of(&CustomComboBox::currentIndexChanged), this, &PlayerAudioModuleWidget::SlotInputDeviceChanged);

    // 文件列表信号
    connect(ui->audioFileList, &FilePathIconListWidget::SigItemSelected, this, &PlayerAudioModuleWidget::SlotAudioFileSelected);
    connect(ui->audioFileList, &FilePathIconListWidget::SigItemDoubleClicked, this, &PlayerAudioModuleWidget::SlotAudioFileDoubleClicked);
}

PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    // 析构时保存一次
    SaveFileListToJson();

    if (m_isRecording)
    {
        m_ffmpeg.StopAudioRecording();
    }
    if (m_isPlaying)
    {
        m_ffmpeg.StopAudioPlayback();
    }

    if (m_autoSaveTimer)
    {
        m_autoSaveTimer->stop();
        delete m_autoSaveTimer;
        m_autoSaveTimer = nullptr;
    }

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
            ui->btnRecord->setText(tr("停止录制"));
            ui->btnRecord->SetBackgroundColor(UIColorDefine::theme_color::Error);

            // 使用线程池启动录制任务
            CoreServerGlobal::Instance().GetThreadPool().Submit([this, filePath]()
            {
                m_ffmpeg.StartAudioRecording(filePath, "wav");
            }, EM_TaskPriority::Normal);
        }
    }
    else
    {
        m_isRecording = false;
        ui->btnRecord->setText(tr("录制"));
        ui->btnRecord->SetBackgroundColor(UIColorDefine::theme_color::Info);
        m_ffmpeg.StopAudioRecording();
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
            UpdatePlayState(true);

            // 使用线程池启动播放任务
            CoreServerGlobal::Instance().GetThreadPool().Submit([this]()
            {
                m_ffmpeg.StartAudioPlayback(m_currentAudioFile);
            }, EM_TaskPriority::Normal);
        }
        else
        {
            m_isPlaying = false;
            m_isPaused = false;
            UpdatePlayState(false);
            m_ffmpeg.StopAudioPlayback();
        }
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请先选择要播放的音频文件"));
    }
}

void PlayerAudioModuleWidget::SlotBtnForwardClicked()
{
    if (m_isPlaying)
    {
        CoreServerGlobal::Instance().GetThreadPool().Submit([this]()
        {
            m_ffmpeg.SeekAudioForward(15); // 前进15秒
        }, EM_TaskPriority::High);
    }
}

void PlayerAudioModuleWidget::SlotBtnBackwardClicked()
{
    if (m_isPlaying)
    {
        CoreServerGlobal::Instance().GetThreadPool().Submit([this]()
        {
            m_ffmpeg.SeekAudioBackward(15); // 后退15秒
        }, EM_TaskPriority::High);
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
                m_currentAudioFile = nextItem->GetNodeInfo().filePath;
                if (m_isPlaying)
                {
                    m_ffmpeg.StopAudioPlayback();
                    CoreServerGlobal::Instance().GetThreadPool().Submit([this]()
                    {
                        m_ffmpeg.StartAudioPlayback(m_currentAudioFile);
                    }, EM_TaskPriority::Normal);
                }
                ui->audioFileList->MoveItemToTop(nextItem);
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
                m_currentAudioFile = prevItem->GetNodeInfo().filePath;
                if (m_isPlaying)
                {
                    m_ffmpeg.StopAudioPlayback();
                    CoreServerGlobal::Instance().GetThreadPool().Submit([this]()
                    {
                        m_ffmpeg.StartAudioPlayback(m_currentAudioFile);
                    }, EM_TaskPriority::Normal);
                }
                ui->audioFileList->MoveItemToTop(prevItem);
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
}

void PlayerAudioModuleWidget::SlotAudioFileDoubleClicked(const QString& filePath)
{
    m_currentAudioFile = filePath;
    MoveFileToTop(filePath); // 将双击的文件移动到顶部
    SlotBtnPlayClicked();    // 自动开始播放
}

void PlayerAudioModuleWidget::UpdatePlayState(bool isPlaying)
{
    if (isPlaying)
    {
        ui->btnPlay->setText(tr("停止"));
        ui->btnPlay->SetBackgroundColor(UIColorDefine::theme_color::Error);
        m_isPaused = false;
    }
    else
    {
        ui->btnPlay->setText(tr("播放"));
        ui->btnPlay->SetBackgroundColor(UIColorDefine::theme_color::Success);
        m_isPaused = false;
    }
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

        if (!IsFileExists(filePath))
        {
            audio_player::ST_AudioFileInfo audioInfo = audio_player::AudioFileSystem::GetAudioFileInfo(stdPath);
            FilePathIconListWidgetItem::ST_NodeInfo nodeInfo;
            nodeInfo.filePath = filePath;
            nodeInfo.displayName = QString::fromStdString(audioInfo.m_displayName);
            nodeInfo.iconPath = QString::fromStdString(audioInfo.m_iconPath);

            // 在列表顶部插入新项
            ui->audioFileList->InsertFileItem(0, nodeInfo);
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
            m_ffmpeg.StopAudioPlayback();
            m_isPlaying = false;
            UpdatePlayState(false);
        }

        ui->audioFileList->RemoveItemByIndex(index);

        // 如果移除的是当前文件，清空当前文件路径
        if (m_currentAudioFile == filePath)
        {
            m_currentAudioFile.clear();
        }
    }
}

void PlayerAudioModuleWidget::ClearAudioFiles()
{
    // 如果正在播放，先停止播放
    if (m_isPlaying)
    {
        m_ffmpeg.StopAudioPlayback();
        m_isPlaying = false;
        UpdatePlayState(false);
    }

    ui->audioFileList->Clear();
    m_currentAudioFile.clear();
}

void PlayerAudioModuleWidget::MoveFileToTop(const QString& filePath)
{
    int index = GetFileIndex(filePath);
    if (index > 0) // 如果文件不在顶部
    {
        FilePathIconListWidgetItem* item = ui->audioFileList->GetItem(index);
        if (item)
        {
            FilePathIconListWidgetItem::ST_NodeInfo nodeInfo = item->GetNodeInfo();
            ui->audioFileList->RemoveItemByIndex(index);
            ui->audioFileList->InsertFileItem(0, nodeInfo);
        }
    }
}

bool PlayerAudioModuleWidget::IsFileExists(const QString& filePath) const
{
    return GetFileIndex(filePath) != -1;
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
    // 关闭窗口时保存
    SaveFileListToJson();
    event->accept();
}

QString PlayerAudioModuleWidget::GetJsonFilePath() const
{
    return QApplication::applicationDirPath() + "/" + m_jsonFileName;
}

void PlayerAudioModuleWidget::SaveFileListToJson()
{
    QJsonObject rootObject;

    for (int i = 0; i < ui->audioFileList->GetItemCount(); ++i)
    {
        FilePathIconListWidgetItem* item = ui->audioFileList->GetItem(i);
        if (item)
        {
            QJsonObject fileObject;
            fileObject["filePath"] = item->GetNodeInfo().filePath;
            fileObject["displayName"] = item->GetNodeInfo().displayName;
            fileObject["iconPath"] = item->GetNodeInfo().iconPath;

            rootObject[QString::number(i)] = fileObject;
        }
    }

    QJsonDocument doc(rootObject);
    QString jsonQStr = doc.toJson(QJsonDocument::Indented);
    std::string jsonStr = jsonQStr.toUtf8().constData();

    qDebug() << "JsonStr : " << jsonQStr;

    // 使用FileSystem API保存JSON
    my_sdk::EM_JsonOperationResult result = my_sdk::FileSystem::WriteJsonToFile(GetJsonFilePath().toStdString(), jsonStr, true);

    if (result != my_sdk::EM_JsonOperationResult::Success)
    {
        qDebug() << "Failed to save file list to JSON, error code:" << static_cast<int>(result);
    }
}

void PlayerAudioModuleWidget::LoadFileListFromJson()
{
    std::string jsonStr;
    my_sdk::EM_JsonOperationResult result = my_sdk::FileSystem::ReadJsonFromFile(GetJsonFilePath().toStdString(), jsonStr);

    if (result == my_sdk::EM_JsonOperationResult::Success && my_sdk::FileSystem::ValidateJsonString(jsonStr))
    {
        // 使用Qt的JSON解析功能解析数据
        QJsonDocument document = QJsonDocument::fromJson(QString::fromStdString(jsonStr).toUtf8());
        if (document.isObject())
        {
            QJsonObject rootObject = document.object();
            // 遍历根对象的所有子对象
            for (auto it = rootObject.begin(); it != rootObject.end(); ++it)
            {
                QJsonObject fileObject = it.value().toObject();
                if (!fileObject.isEmpty())
                {
                    std::string filePath = my_sdk::FileSystem::QtPathToStdPath(fileObject["filePath"].toString().toStdString());
                    QString qtFilePath = QString::fromStdString(my_sdk::FileSystem::StdPathToQtPath(filePath));

                    // 使用FileSystem API检查文件是否存在
                    if (my_sdk::FileSystem::Exists(filePath))
                    {
                        FilePathIconListWidgetItem::ST_NodeInfo nodeInfo;
                        nodeInfo.filePath = qtFilePath;
                        nodeInfo.displayName = fileObject["displayName"].toString();
                        nodeInfo.iconPath = fileObject["iconPath"].toString();

                        // 添加到列表
                        ui->audioFileList->InsertFileItem(ui->audioFileList->GetItemCount(), nodeInfo);
                    }
                }
            }
        }
    }
    else if (result != my_sdk::EM_JsonOperationResult::FileNotFound)
    {
        qDebug() << "Failed to load file list from JSON, error code:" << static_cast<int>(result);
    }
}

void PlayerAudioModuleWidget::InitializeAutoSaveTimer()
{
    if (!m_autoSaveTimer)
    {
        m_autoSaveTimer = new QTimer(this);
        m_autoSaveTimer->setInterval(AUTO_SAVE_INTERVAL);
        connect(m_autoSaveTimer, &QTimer::timeout, this, &PlayerAudioModuleWidget::SlotAutoSave);
        m_autoSaveTimer->start();
    }
}

void PlayerAudioModuleWidget::SlotAutoSave()
{
    SaveFileListToJson();
    qDebug() << "Auto saved file list to JSON";
}

int PlayerAudioModuleWidget::GetFileCount() const
{
    return ui->audioFileList->GetItemCount();
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
