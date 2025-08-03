#pragma once

#include <QLabel>
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
/// 视频播放器模块控件类
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
    /// 获取视频显示控件
    /// </summary>
    /// <returns>视频显示控件指针</returns>
    QWidget* GetVideoDisplayWidget();

    /// <summary>
    /// 设置视频帧数据
    /// </summary>
    /// <param name="frameData">帧数据</param>
    /// <param name="width">帧宽度</param>
    /// <param name="height">帧高度</param>
    void SetVideoFrame(const uint8_t* frameData, int width, int height);

    /// <summary>
    /// 清空视频显示
    /// </summary>
    void ClearVideoDisplay();

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
    /// 调整视频显示尺寸
    /// </summary>
    void ResizeVideoDisplay();

private:
    Ui::PlayerVideoModuleWidgetClass* ui;
    QLabel* m_videoDisplayLabel{nullptr};            /// 视频显示标签
    QVBoxLayout* m_mainLayout{nullptr};              /// 主布局
    ST_VideoFrameInfo* m_currentVideoInfo{nullptr};  /// 当前视频信息（使用指针）
    QTimer* m_updateTimer{nullptr};                  /// 更新定时器
};
