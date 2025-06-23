#pragma once

#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include "ui_MainWidget.h"
#include "CoreWidget/CustomToolBar.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class MainWidgetClass;
};

QT_END_NAMESPACE

/// <summary>
/// 音视频播放器主窗口类
/// </summary>
class MainWidget : public QMainWindow
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit MainWidget(QWidget* parent = nullptr);
    /// <summary>
    /// 析构函数
    /// </summary>
    ~MainWidget() override;

protected slots:
    /// <summary>
    /// 打开音视频文件槽函数
    /// </summary>
    void SlotOpenAVFile();

    /// <summary>
    /// 打开音视频文件夹槽函数
    /// </summary>
    void SlotOpenAVFolder();

    /// <summary>
    /// 清空文件列表槽函数
    /// </summary>
    void SlotClearFileList();
    /// <summary>
    /// 从列表移除文件槽函数
    /// </summary>
    void SlotRemoveFromList();

private:
    /// <summary>
    /// 初始化菜单栏
    /// </summary>
    void InitializeMenuBar();

    /// <summary>
    /// 初始化工具栏
    /// </summary>
    void InitializeToolBar();

    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();

private:
    Ui::MainWidgetClass* ui;
    QString m_currentAVFile; /// 当前音视频文件路径
};
