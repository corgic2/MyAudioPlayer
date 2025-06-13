#pragma once

#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include "ui_AudioMainWidget.h"
#include "CoreWidget/CustomToolBar.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class AudioMainWidgetClass;
};

QT_END_NAMESPACE

/// <summary>
/// 音频播放器主窗口类
/// </summary>
class AudioMainWidget : public QMainWindow
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit AudioMainWidget(QWidget* parent = nullptr);
    /// <summary>
    /// 析构函数
    /// </summary>
    ~AudioMainWidget() override;

protected slots:
    /// <summary>
    /// 打开音频文件槽函数
    /// </summary>
    void SlotOpenAudioFile();

    /// <summary>
    /// 打开音频文件夹槽函数
    /// </summary>
    void SlotOpenAudioFolder();

    /// <summary>
    /// 清空文件列表槽函数
    /// </summary>
    void SlotClearFileList();

    /// <summary>
    /// 保存UI数据槽函数
    /// </summary>
    void SlotSaveUIData();

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
    /// <summary>
    /// 保存UI数据槽函数
    /// </summary>
    /// <returns></returns>
    bool SaveUIDataToFile();

private:
    Ui::AudioMainWidgetClass* ui;
    QString m_currentAudioFile; /// 当前音频文件路径
};
