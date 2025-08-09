#include "MainWidget.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QTextCodec>
#include <QToolBar>
#include <QVBoxLayout>
#include "AVFileSystem.h"
#include "AudioWidget/PlayerAudioModuleWidget.h"
#include "BusinessLogic/MediaService.h"

MainWidget::MainWidget(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWidgetClass()), m_currentAVFile("")
{
    ui->setupUi(this);

    // 初始化QtCustomAPI
    m_qtCustomAPI = new QtCustomAPI(this);

    InitializeMenuBar();
    InitializeToolBar();
    ConnectSignals();
}

MainWidget::~MainWidget()
{
}

void MainWidget::InitializeMenuBar()
{
    // 文件菜单
    QMenu* fileMenu = menuBar()->addMenu(tr("文件"));

    auto openFileAction = new QAction(tr("打开文件..."), this);
    openFileAction->setShortcut(QKeySequence::Open);
    openFileAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    fileMenu->addAction(openFileAction);
    connect(openFileAction, &QAction::triggered, this, &MainWidget::SlotOpenAVFile);

    auto openFolderAction = new QAction(tr("打开文件夹..."), this);
    openFolderAction->setShortcut(tr("Ctrl+Shift+O"));
    openFolderAction->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    fileMenu->addAction(openFolderAction);
    connect(openFolderAction, &QAction::triggered, this, &MainWidget::SlotOpenAVFolder);

    fileMenu->addSeparator();

    auto saveAction = new QAction(tr("保存"), this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    fileMenu->addAction(saveAction);

    // 编辑菜单
    QMenu* editMenu = menuBar()->addMenu(tr("编辑"));

    auto removeAction = new QAction(tr("从列表移除"), this);
    removeAction->setShortcut(QKeySequence::Delete);
    removeAction->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
    editMenu->addAction(removeAction);
    connect(removeAction, &QAction::triggered, this, &MainWidget::SlotRemoveFromList);

    auto clearAction = new QAction(tr("清空列表"), this);
    clearAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    editMenu->addAction(clearAction);
    connect(clearAction, &QAction::triggered, this, &MainWidget::SlotClearFileList);

    // 编码转换菜单
    QMenu* convertMenu = menuBar()->addMenu(tr("编码转换"));

    auto audioConvertAction = new QAction(tr("音频格式转换..."), this);
    audioConvertAction->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    convertMenu->addAction(audioConvertAction);
    connect(audioConvertAction, &QAction::triggered, this, &MainWidget::SlotConvertAudioFormat);

    auto videoConvertAction = new QAction(tr("视频格式转换..."), this);
    videoConvertAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    convertMenu->addAction(videoConvertAction);
    connect(videoConvertAction, &QAction::triggered, this, &MainWidget::SlotConvertVideoFormat);
}

void MainWidget::InitializeToolBar()
{
    // 移除原有工具栏
    if (ui->mainToolBar)
    {
        removeToolBar(ui->mainToolBar);
        SAFE_DELETE_POINTER_VALUE(ui->mainToolBar);
    }
}

void MainWidget::ConnectSignals()
{
    // 连接PlayerAudioModuleWidget的信号
    connect(ui->widget, &AVBaseWidget::SigAVFileSelected, [this](const QString& filePath)
    {
        m_currentAVFile = filePath;
    });
}

void MainWidget::SlotOpenAVFile()
{
    QString dir = QDir::currentPath();
    QString filter = QString::fromStdString(av_fileSystem::AVFileSystem::GetAVFileFilter());
    QStringList files = QFileDialog::getOpenFileNames(this, tr("打开音视频文件"), dir, filter);

    if (!files.isEmpty())
    {
        ui->widget->AddAVFiles(files);
        // 选中第一个文件
        if (!files.isEmpty())
        {
            m_currentAVFile = files.first();
            emit ui->widget->SigAVFileSelected(m_currentAVFile);
        }
    }
}

void MainWidget::SlotOpenAVFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择音视频文件夹"), QDir::currentPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty())
    {
        std::vector<std::string> audioFiles = av_fileSystem::AVFileSystem::GetAVFiles(my_sdk::FileSystem::QtPathToStdPath(dir.toStdString()), false);

        QStringList filePaths;
        for (const auto& file : audioFiles)
        {
            filePaths << QString::fromStdString(my_sdk::FileSystem::StdPathToQtPath(file));
        }

        if (!filePaths.isEmpty())
        {
            ui->widget->AddAVFiles(filePaths);
            // 选中第一个文件
            m_currentAVFile = filePaths.first();
            emit ui->widget->SigAVFileSelected(m_currentAVFile);
        }
    }
}

void MainWidget::SlotClearFileList()
{
    if (QMessageBox::question(this, tr("清空列表"), tr("确定要清空文件列表吗？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        ui->widget->ClearAVFiles();
        m_currentAVFile.clear();
    }
}

void MainWidget::SlotRemoveFromList()
{
    if (!m_currentAVFile.isEmpty())
    {
        if (QMessageBox::question(this, tr("移除文件"), tr("确定要从列表中移除选中的文件吗？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            ui->widget->RemoveAVFile(m_currentAVFile);
            m_currentAVFile.clear();
        }
    }
}

void MainWidget::SlotConvertAudioFormat()
{
    if (m_currentAVFile.isEmpty())
    {
        QMessageBox::warning(this, tr("音频转换"), tr("请先选择要转换的音频文件。"));
        return;
    }
    if (!av_fileSystem::AVFileSystem::IsAudioFile(m_currentAVFile.toStdString()))
    {
        QMessageBox::warning(this, tr("音频转换"), tr("不是音频文件"));
        return;
    }
    ST_AudioConvertParams params;
    if (!CreateAudioConvertDialog(params))
    {
        return;
    }

    QFileInfo inputInfo(m_currentAVFile);
    QString outputFileName = inputInfo.baseName() + "_converted." + GetAudioExtension(params.outputFormat);
    QString outputFile = QFileDialog::getSaveFileName(this, tr("保存转换后的音频文件"), inputInfo.dir().filePath(outputFileName), tr("音频文件 (*.%1)").arg(GetAudioExtension(params.outputFormat)));

    if (outputFile.isEmpty())
    {
        return;
    }

    QProgressDialog progress(tr("正在转换音频文件..."), tr("取消"), 0, 0, this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setFixedSize(400, 100);
    progress.show();

    MediaService::instance()->ConvertAudioFormatAsync(m_currentAVFile, outputFile, params, [this, &progress](bool success, const QString& error)
    {
        progress.close();
        if (success)
        {
            QMessageBox::information(this, tr("转换完成"), tr("音频文件转换成功！"));
        }
        else
        {
            QMessageBox::warning(this, tr("转换失败"), tr("音频文件转换失败！\n错误信息：%1").arg(error));
        }
    });
}

void MainWidget::SlotConvertVideoFormat()
{
    if (m_currentAVFile.isEmpty())
    {
        QMessageBox::warning(this, tr("视频转换"), tr("请先选择要转换的视频文件。"));
        return;
    }
    if (!av_fileSystem::AVFileSystem::IsVideoFile(m_currentAVFile.toStdString()))
    {
        QMessageBox::warning(this, tr("音频转换"), tr("不是视频文件"));
        return;
    }
    ST_VideoConvertParams params;
    if (!CreateVideoConvertDialog(params))
    {
        return;
    }

    QFileInfo inputInfo(m_currentAVFile);
    QString outputFileName = inputInfo.baseName() + "_converted." + GetVideoExtension(params.outputFormat);
    QString outputFile = QFileDialog::getSaveFileName(this, tr("保存转换后的视频文件"), inputInfo.dir().filePath(outputFileName), tr("视频文件 (*.%1)").arg(GetVideoExtension(params.outputFormat)));

    if (outputFile.isEmpty())
    {
        return;
    }

    QProgressDialog progress(tr("正在转换视频文件..."), tr("取消"), 0, 0, this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setFixedSize(400, 100);
    progress.show();

    MediaService::instance()->ConvertVideoFormatAsync(m_currentAVFile, outputFile, params, [this, &progress](bool success, const QString& error)
    {
        progress.close();
        if (success)
        {
            QMessageBox::information(this, tr("转换完成"), tr("视频文件转换成功！"));
        }
        else
        {
            QMessageBox::warning(this, tr("转换失败"), tr("视频文件转换失败！\n错误信息：%1").arg(error));
        }
    });
}

bool MainWidget::CreateAudioConvertDialog(ST_AudioConvertParams& params)
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("音频格式转换参数"));
    dialog.setModal(true);
    dialog.resize(400, 300);

    auto mainLayout = new QVBoxLayout(&dialog);

    // 输出格式
    auto formatGroup = new QGroupBox(tr("输出格式"), &dialog);
    auto formatLayout = new QFormLayout(formatGroup);

    auto formatCombo = new QComboBox(&dialog);
    formatCombo->addItems({"mp3", "wav", "aac", "flac", "ogg", "m4a", "wma"});
    formatCombo->setCurrentText(params.outputFormat);
    formatLayout->addRow(tr("格式:"), formatCombo);

    mainLayout->addWidget(formatGroup);

    // 音频参数
    auto audioGroup = new QGroupBox(tr("音频参数"), &dialog);
    auto audioLayout = new QFormLayout(audioGroup);

    auto bitrateSpinBox = new QSpinBox(&dialog);
    bitrateSpinBox->setRange(32, 320);
    bitrateSpinBox->setSuffix(" kbps");
    bitrateSpinBox->setValue(params.bitrate);
    audioLayout->addRow(tr("比特率:"), bitrateSpinBox);

    auto sampleRateCombo = new QComboBox(&dialog);
    sampleRateCombo->addItems({"8000", "11025", "22050", "44100", "48000", "96000"});
    sampleRateCombo->setCurrentText(QString::number(params.sampleRate));
    audioLayout->addRow(tr("采样率:"), sampleRateCombo);

    auto channelsCombo = new QComboBox(&dialog);
    channelsCombo->addItems({"1", "2"});
    channelsCombo->setCurrentText(QString::number(params.channels));
    audioLayout->addRow(tr("声道数:"), channelsCombo);

    mainLayout->addWidget(audioGroup);

    // 按钮
    auto buttonLayout = new QHBoxLayout();
    auto okButton = new QPushButton(tr("确定"), &dialog);
    auto cancelButton = new QPushButton(tr("取消"), &dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        params.outputFormat = formatCombo->currentText();
        params.bitrate = bitrateSpinBox->value();
        params.sampleRate = sampleRateCombo->currentText().toInt();
        params.channels = channelsCombo->currentText().toInt();
        return true;
    }

    return false;
}

bool MainWidget::CreateVideoConvertDialog(ST_VideoConvertParams& params)
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("视频格式转换参数"));
    dialog.setModal(true);
    dialog.resize(450, 400);

    auto mainLayout = new QVBoxLayout(&dialog);

    // 输出格式
    auto formatGroup = new QGroupBox(tr("输出格式"), &dialog);
    auto formatLayout = new QFormLayout(formatGroup);

    auto formatCombo = new QComboBox(&dialog);
    formatCombo->addItems({"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm"});
    formatCombo->setCurrentText(params.outputFormat);
    formatLayout->addRow(tr("格式:"), formatCombo);

    mainLayout->addWidget(formatGroup);

    // 视频参数
    auto videoGroup = new QGroupBox(tr("视频参数"), &dialog);
    auto videoLayout = new QFormLayout(videoGroup);

    auto videoBitrateSpinBox = new QSpinBox(&dialog);
    videoBitrateSpinBox->setRange(100, 10000);
    videoBitrateSpinBox->setSuffix(" kbps");
    videoBitrateSpinBox->setValue(params.videoBitrate);
    videoLayout->addRow(tr("视频比特率:"), videoBitrateSpinBox);

    auto audioBitrateSpinBox = new QSpinBox(&dialog);
    audioBitrateSpinBox->setRange(32, 320);
    audioBitrateSpinBox->setSuffix(" kbps");
    audioBitrateSpinBox->setValue(params.audioBitrate);
    videoLayout->addRow(tr("音频比特率:"), audioBitrateSpinBox);

    mainLayout->addWidget(videoGroup);

    // 按钮
    auto buttonLayout = new QHBoxLayout();
    auto okButton = new QPushButton(tr("确定"), &dialog);
    auto cancelButton = new QPushButton(tr("取消"), &dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        params.outputFormat = formatCombo->currentText();
        params.videoBitrate = videoBitrateSpinBox->value();
        params.audioBitrate = audioBitrateSpinBox->value();
        return true;
    }

    return false;
}

QString MainWidget::GetAudioExtension(const QString& format)
{
    static QMap<QString, QString> audioExtensions = {{"mp3", "mp3"}, {"wav", "wav"}, {"aac", "aac"}, {"flac", "flac"}, {"ogg", "ogg"}, {"m4a", "m4a"}, {"wma", "wma"}};

    return audioExtensions.value(format, format);
}

QString MainWidget::GetVideoExtension(const QString& format)
{
    static QMap<QString, QString> videoExtensions = {{"mp4", "mp4"}, {"avi", "avi"}, {"mkv", "mkv"}, {"mov", "mov"}, {"wmv", "wmv"}, {"flv", "flv"}, {"webm", "webm"}};

    return videoExtensions.value(format, format);
}
