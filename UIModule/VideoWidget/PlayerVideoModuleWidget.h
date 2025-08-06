#pragma once

#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "ui_PlayerVideoModuleWidget.h"
#include "BaseWidget/BaseModuleWidegt.h"
#include "VideoPlayer/VideoPlayWorker.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class PlayerVideoModuleWidgetClass;
};

QT_END_NAMESPACE

/// <summary>
/// 视频播放器模块控件类 - 基于SDL3
/// </summary>
class PlayerVideoModuleWidget : public BaseModuleWidget
{
    Q_OBJECT public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit PlayerVideoModuleWidget(QWidget* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~PlayerVideoModuleWidget() override;

    /// <summary>
    /// 获取FFMpegUtils (已弃用，返回nullptr)
    /// </summary>
    /// <returns>FFmpeg工具类指针</returns>
    BaseFFmpegPlayer* GetFFMpegUtils() override;

    /// <summary>
    /// 获取SDL窗口句柄
    /// </summary>
    /// <returns>SDL窗口指针</returns>
    void* GetSDLWindowHandle();

    /// <summary>
    /// 显示/隐藏SDL窗口
    /// </summary>
    /// <param name="show">是否显示</param>
    void ShowSDLWindow(bool show);

    /// <summary>
    /// 设置SDL窗口标题
    /// </summary>
    /// <param name="title">窗口标题</param>
    void SetSDLWindowTitle(const QString& title);

protected slots:
    /// <summary>
    /// 视频播放进度更新槽函数
    /// </summary>
    /// <param name="currentTime">当前时间</param>
    /// <param name="totalTime">总时间</param>
    void SlotVideoProgressUpdated(double currentTime, double totalTime);

    /// <summary>
    /// 视频帧更新槽函数
    /// </summary>
    void SlotVideoFrameUpdated();

    /// <summary>
    /// 视频错误槽函数
    /// </summary>
    /// <param name="errorMsg">错误信息</param>
    void SlotVideoError(const QString& errorMsg);

private:
    /// <summary>
    /// 初始化界面
    /// </summary>
    void InitializeWidget();

    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();
    /// <summary>
    /// 创建SDL窗口占位控件
    /// </summary>
    void CreateSDLPlaceholder();

private:
    Ui::PlayerVideoModuleWidgetClass* ui;
    QWidget* m_sdlPlaceholder{nullptr};              /// SDL窗口占位控件
    QVBoxLayout* m_mainLayout{nullptr};              /// 主布局
    ST_VideoFrameInfo* m_currentVideoInfo{nullptr};  /// 当前视频信息（使用指针）
    QTimer* m_updateTimer{nullptr};                  /// 更新定时器
    bool m_isSDLWindowVisible{false};                /// SDL窗口是否可见
};
