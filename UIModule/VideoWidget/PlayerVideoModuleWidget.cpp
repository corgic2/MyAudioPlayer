#include "PlayerVideoModuleWidget.h"
#include <QDebug>
#include "CommonDefine/UIWidgetColorDefine.h"
#include "SDKCommonDefine/SDKCommonDefine.h"

PlayerVideoModuleWidget::PlayerVideoModuleWidget(QWidget* parent)
    : BaseModuleWidget(parent), ui(new Ui::PlayerVideoModuleWidgetClass())
{
    ui->setupUi(this);
    
    m_currentVideoInfo = new ST_VideoFrameInfo();

    InitializeWidget();
    ConnectSignals();
}

PlayerVideoModuleWidget::~PlayerVideoModuleWidget()
{
    if (m_updateTimer)
    {
        m_updateTimer->stop();
        SAFE_DELETE_POINTER_VALUE(m_updateTimer);
    }
    SAFE_DELETE_POINTER_VALUE(m_currentVideoInfo);
    SAFE_DELETE_POINTER_VALUE(ui);
}

BaseFFmpegPlayer* PlayerVideoModuleWidget::GetFFMpegUtils()
{
    // 已弃用：现在使用MediaPlayerManager统一管理
    return nullptr;
}

void* PlayerVideoModuleWidget::GetSDLWindowHandle()
{
    // SDL窗口由VideoPlayWorker管理，这里返回nullptr
    // 实际的SDL窗口创建在VideoPlayWorker中
    return nullptr;
}

void PlayerVideoModuleWidget::ShowSDLWindow(bool show)
{
    m_isSDLWindowVisible = show;
    // SDL窗口的显示/隐藏由VideoPlayWorker控制
    // 这里只是更新占位控件的显示状态
    if (m_sdlPlaceholder)
    {
        m_sdlPlaceholder->setVisible(true); // 占位控件始终可见
    }
}

void PlayerVideoModuleWidget::SetSDLWindowTitle(const QString& title)
{
    // SDL窗口标题由VideoPlayWorker设置
    Q_UNUSED(title);
}

void PlayerVideoModuleWidget::InitializeWidget()
{
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 创建SDL窗口占位控件
    CreateSDLPlaceholder();

    setLayout(m_mainLayout);
}

void PlayerVideoModuleWidget::CreateSDLPlaceholder()
{
    m_sdlPlaceholder = new QWidget(this);
    m_sdlPlaceholder->setMinimumSize(1080, 720);
    m_sdlPlaceholder->setStyleSheet(QString("background-color: %1; border: 1px solid %2;").arg(UIColorDefine::color_convert::ToCssString(UIColorDefine::background_color::Dark)).arg(UIColorDefine::color_convert::ToCssString(UIColorDefine::border_color::Default)));
    m_mainLayout->addWidget(m_sdlPlaceholder);
}

void PlayerVideoModuleWidget::ConnectSignals()
{
}

void PlayerVideoModuleWidget::SlotVideoProgressUpdated(double currentTime, double totalTime)
{
    // 可以在这里更新进度条或时间显示
    Q_UNUSED(currentTime);
    Q_UNUSED(totalTime);
}

void PlayerVideoModuleWidget::SlotVideoFrameUpdated()
{
    // 这个槽函数现在不需要特殊处理，帧数据直接通过SetVideoFrame设置
}

void PlayerVideoModuleWidget::SlotVideoError(const QString& errorMsg)
{
    qDebug() << "Video error:" << errorMsg;
    // SDL窗口错误由VideoPlayWorker处理
}
