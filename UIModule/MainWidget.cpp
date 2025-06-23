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
#include "../AVFileSystem/AVFileSystem.h"
#include "AudioWidget/PlayerAudioModuleWidget.h"
#include "CoreWidget/CustomToolBar.h"
#include "FileSystem/FileSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"

MainWidget::MainWidget(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWidgetClass()), m_currentAVFile("")
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
    QString filter = QString::fromStdString(AV_player::AVFileSystem::GetAVFileFilter());
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
        std::vector<std::string> audioFiles = AV_player::AVFileSystem::GetAudioFiles(my_sdk::FileSystem::QtPathToStdPath(dir.toStdString()), false);

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
