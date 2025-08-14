#pragma once

#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include "ui_MainWidget.h"
#include "BusinessLogic/MediaService.h"

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

    /// <summary>
    /// 音频格式转换槽函数
    /// </summary>
    void SlotConvertAudioFormat();

    /// <summary>
    /// 视频格式转换槽函数
    /// </summary>
    void SlotConvertVideoFormat();

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
    /// 创建音频转换参数对话框
    /// </summary>
    /// <param name="params">音频转换参数</param>
    /// <returns>是否确认转换</returns>
    bool CreateAudioConvertDialog(ST_AudioConvertParams& params);

    /// <summary>
    /// 创建视频转换参数对话框
    /// </summary>
    /// <param name="params">视频转换参数</param>
    /// <returns>是否确认转换</returns>
    bool CreateVideoConvertDialog(ST_VideoConvertParams& params);

    /// <summary>
    /// 获取音频格式扩展名
    /// </summary>
    /// <param name="format">格式名称</param>
    /// <returns>扩展名</returns>
    QString GetAudioExtension(const QString& format);

    /// <summary>
    /// 获取视频格式扩展名
    /// </summary>
    /// <param name="format">格式名称</param>
    /// <returns>扩展名</returns>
    QString GetVideoExtension(const QString& format);

private:
    /// <summary>
    /// UI界面指针
    /// </summary>
    Ui::MainWidgetClass* ui = nullptr;
    
    /// <summary>
    /// 当前音视频文件路径
    /// </summary>
    QString m_currentAVFile;
};
