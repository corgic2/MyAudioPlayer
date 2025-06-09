#include "AudioMainWidget.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyle>
#include <QToolBar>
#include "PlayerAudioModuleWidget.h"
#include "CoreWidget/CustomToolBar.h"

AudioMainWidget::AudioMainWidget(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::AudioMainWidgetClass())
    , m_currentAudioFile("")
{
    ui->setupUi(this);
    InitializeMenuBar();
    InitializeToolBar();
    ConnectSignals();
}

AudioMainWidget::~AudioMainWidget()
{
    delete ui;
}

void AudioMainWidget::InitializeMenuBar()
{
    // 文件菜单
    QMenu* fileMenu = menuBar()->addMenu(tr("文件"));

    auto openFileAction = new QAction(tr("打开文件..."), this);
    openFileAction->setShortcut(QKeySequence::Open);
    openFileAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    fileMenu->addAction(openFileAction);
    connect(openFileAction, &QAction::triggered, this, &AudioMainWidget::SlotOpenAudioFile);

    auto openFolderAction = new QAction(tr("打开文件夹..."), this);
    openFolderAction->setShortcut(tr("Ctrl+Shift+O"));
    openFolderAction->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    fileMenu->addAction(openFolderAction);
    connect(openFolderAction, &QAction::triggered, this, &AudioMainWidget::SlotOpenAudioFolder);

    fileMenu->addSeparator();

    auto saveAction = new QAction(tr("保存"), this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    fileMenu->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, &AudioMainWidget::SlotSaveAudioFile);

    auto saveAsAction = new QAction(tr("另存为..."), this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    saveAsAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    fileMenu->addAction(saveAsAction);
    connect(saveAsAction, &QAction::triggered, this, &AudioMainWidget::SlotSaveAsAudioFile);

    // 编辑菜单
    QMenu* editMenu = menuBar()->addMenu(tr("编辑"));

    auto removeAction = new QAction(tr("从列表移除"), this);
    removeAction->setShortcut(QKeySequence::Delete);
    removeAction->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
    editMenu->addAction(removeAction);
    connect(removeAction, &QAction::triggered, this, &AudioMainWidget::SlotRemoveFromList);

    auto clearAction = new QAction(tr("清空列表"), this);
    clearAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    editMenu->addAction(clearAction);
    connect(clearAction, &QAction::triggered, this, &AudioMainWidget::SlotClearFileList);
}

void AudioMainWidget::InitializeToolBar()
{
    // 移除原有工具栏
    if (ui->mainToolBar)
    {
        removeToolBar(ui->mainToolBar);
        SAFE_DELETE_POINTER_VALUE(ui->mainToolBar);
    }

    // 创建自定义工具栏
    m_toolBar = new CustomToolBar(this);
    addToolBar(m_toolBar);

    // 添加工具栏按钮
    m_toolBar->AddCustomAction(tr("打开文件"),
                               style()->standardIcon(QStyle::SP_DialogOpenButton),
                               tr("打开音频文件"));

    m_toolBar->AddCustomAction(tr("打开文件夹"),
                               style()->standardIcon(QStyle::SP_DirOpenIcon),
                               tr("打开音频文件夹"));

    m_toolBar->addSeparator();

    m_toolBar->AddCustomAction(tr("保存"),
                               style()->standardIcon(QStyle::SP_DialogSaveButton),
                               tr("保存当前音频文件"));
}

void AudioMainWidget::ConnectSignals()
{
    // 连接PlayerAudioModuleWidget的信号
    connect(ui->widget, &PlayerAudioModuleWidget::SigAudioFileSelected,
            [this](const QString& filePath)
            {
                m_currentAudioFile = filePath;
            });
}

void AudioMainWidget::SlotOpenAudioFile()
{
    QStringList files = QFileDialog::getOpenFileNames(
                                                      this,
                                                      tr("打开音频文件"),
                                                      QDir::currentPath(),
                                                      tr("音频文件 (*.mp3 *.wav *.flac *.m4a);;所有文件 (*.*)"));

    if (!files.isEmpty())
    {
        ui->widget->AddAudioFiles(files);
    }
}

void AudioMainWidget::SlotOpenAudioFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
                                                    this,
                                                    tr("选择音频文件夹"),
                                                    QDir::currentPath(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty())
    {
        QDir folder(dir);
        QStringList filters;
        filters << "*.mp3" << "*.wav" << "*.flac" << "*.m4a";
        QStringList files = folder.entryList(filters, QDir::Files);

        QStringList fullPaths;
        for (const QString& file : files)
        {
            fullPaths << folder.filePath(file);
        }

        if (!fullPaths.isEmpty())
        {
            ui->widget->AddAudioFiles(fullPaths);
        }
    }
}

void AudioMainWidget::SlotSaveAudioFile()
{
    if (m_currentAudioFile.isEmpty())
    {
        SlotSaveAsAudioFile();
        return;
    }

    // 这里可以添加保存当前音频文件的逻辑
    QMessageBox::information(this, tr("保存"), tr("文件已保存"));
}

void AudioMainWidget::SlotSaveAsAudioFile()
{
    QString filePath = QFileDialog::getSaveFileName(
                                                    this,
                                                    tr("另存为"),
                                                    m_currentAudioFile.isEmpty() ? QDir::currentPath() : m_currentAudioFile,
                                                    tr("Wave文件 (*.wav);;MP3文件 (*.mp3);;所有文件 (*.*)"));

    if (!filePath.isEmpty())
    {
        m_currentAudioFile = filePath;
        // 这里可以添加另存为的具体逻辑
        QMessageBox::information(this, tr("另存为"), tr("文件已保存"));
    }
}

void AudioMainWidget::SlotClearFileList()
{
    if (QMessageBox::question(this,
                              tr("清空列表"),
                              tr("确定要清空文件列表吗？"),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        ui->widget->ClearAudioFiles();
    }
}

void AudioMainWidget::SlotRemoveFromList()
{
    if (!m_currentAudioFile.isEmpty())
    {
        ui->widget->RemoveAudioFile(m_currentAudioFile);
        m_currentAudioFile.clear();
    }
}
