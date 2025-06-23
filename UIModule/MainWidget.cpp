#include "MainWidget.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyle> 
#include <QToolBar>
#include "AudioFileSystem.h"
#include "AudioWidget/PlayerAudioModuleWidget.h"
#include "CoreWidget/CustomToolBar.h"
#include "FileSystem/FileSystem.h"

MainWidget::MainWidget(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWidgetClass()), m_currentAudioFile("")
{
    ui->setupUi(this);
    InitializeMenuBar();
    InitializeToolBar();
    ConnectSignals();
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::InitializeMenuBar()
{
    // 文件菜单
    QMenu* fileMenu = menuBar()->addMenu(tr("文件"));

    auto openFileAction = new QAction(tr("打开文件..."), this);
    openFileAction->setShortcut(QKeySequence::Open);
    openFileAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    fileMenu->addAction(openFileAction);
    connect(openFileAction, &QAction::triggered, this, &MainWidget::SlotOpenAudioFile);

    auto openFolderAction = new QAction(tr("打开文件夹..."), this);
    openFolderAction->setShortcut(tr("Ctrl+Shift+O"));
    openFolderAction->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    fileMenu->addAction(openFolderAction);
    connect(openFolderAction, &QAction::triggered, this, &MainWidget::SlotOpenAudioFolder);

    fileMenu->addSeparator();

    auto saveAction = new QAction(tr("保存"), this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    fileMenu->addAction(saveAction);
    //connect(saveAction, &QAction::triggered, this, &MainWidget::SlotSaveUIData);

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
    connect(ui->widget, &PlayerAudioModuleWidget::SigAudioFileSelected, [this](const QString& filePath)
    {
        m_currentAudioFile = filePath;
    });
}

void MainWidget::SlotOpenAudioFile()
{
    QString dir = QDir::currentPath();
    QString filter = QString::fromStdString(audio_player::AudioFileSystem::GetAudioFileFilter());
    QStringList files = QFileDialog::getOpenFileNames(this, tr("打开音频文件"), dir, filter);

    if (!files.isEmpty())
    {
        ui->widget->AddAudioFiles(files);
        // 选中第一个文件
        if (!files.isEmpty())
        {
            m_currentAudioFile = files.first();
            emit ui->widget->SigAudioFileSelected(m_currentAudioFile);
        }
    }
}

void MainWidget::SlotOpenAudioFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择音频文件夹"), QDir::currentPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty())
    {
        std::vector<std::string> audioFiles = audio_player::AudioFileSystem::GetAudioFiles(my_sdk::FileSystem::QtPathToStdPath(dir.toStdString()), false);

        QStringList filePaths;
        for (const auto& file : audioFiles)
        {
            filePaths << QString::fromStdString(my_sdk::FileSystem::StdPathToQtPath(file));
        }

        if (!filePaths.isEmpty())
        {
            ui->widget->AddAudioFiles(filePaths);
            // 选中第一个文件
            m_currentAudioFile = filePaths.first();
            emit ui->widget->SigAudioFileSelected(m_currentAudioFile);
        }
    }
}

void MainWidget::SlotClearFileList()
{
    if (QMessageBox::question(this, tr("清空列表"), tr("确定要清空文件列表吗？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        ui->widget->ClearAudioFiles();
        m_currentAudioFile.clear();
    }
}

void MainWidget::SlotRemoveFromList()
{
    if (!m_currentAudioFile.isEmpty())
    {
        if (QMessageBox::question(this, tr("移除文件"), tr("确定要从列表中移除选中的文件吗？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            ui->widget->RemoveAudioFile(m_currentAudioFile);
            m_currentAudioFile.clear();
        }
    }
}
