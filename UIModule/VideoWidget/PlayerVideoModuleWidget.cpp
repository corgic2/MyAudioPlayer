#include "PlayerVideoModuleWidget.h"
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include "VideoFFmpegUtils.h"  // 在cpp文件中包含具体实现
#include "CommonDefine/UIWidgetColorDefine.h"
#include "SDKCommonDefine/SDKCommonDefine.h"

PlayerVideoModuleWidget::PlayerVideoModuleWidget(QWidget* parent)
    : BaseModuleWidget(parent), ui(new Ui::PlayerVideoModuleWidgetClass())
{
    ui->setupUi(this);
    
    // 创建VideoFFmpegUtils实例
    m_videoFFmpeg = new VideoFFmpegUtils(this);
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
    // m_videoFFmpeg 会由Qt的父子关系自动删除
}

BaseFFmpegUtils* PlayerVideoModuleWidget::GetFFMpegUtils()
{
    return m_videoFFmpeg;
}

QWidget* PlayerVideoModuleWidget::GetVideoDisplayWidget()
{
    return m_videoDisplayLabel;
}

void PlayerVideoModuleWidget::InitializeWidget()
{
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 创建视频显示标签
    m_videoDisplayLabel = new QLabel(this);
    m_videoDisplayLabel->setMinimumSize(1080, 720);
    m_videoDisplayLabel->setStyleSheet(QString("background-color: %1; border: 1px solid %2;").arg(UIColorDefine::color_convert::ToCssString(UIColorDefine::background_color::Dark)).arg(UIColorDefine::color_convert::ToCssString(UIColorDefine::border_color::Default)));
    m_videoDisplayLabel->setAlignment(Qt::AlignCenter);
    m_videoDisplayLabel->setText("选择视频文件开始播放");
    m_videoDisplayLabel->setScaledContents(true);

    // 添加到布局
    m_mainLayout->addWidget(m_videoDisplayLabel);
    setLayout(m_mainLayout);

    // 设置显示控件到VideoFFmpegUtils
    if (m_videoFFmpeg)
    {
        m_videoFFmpeg->SetVideoDisplayWidget(this);
    }
}

void PlayerVideoModuleWidget::ConnectSignals()
{
    // 连接视频FFmpeg工具类的信号
    connect(m_videoFFmpeg, &VideoFFmpegUtils::SigPlayStateChanged, this, &PlayerVideoModuleWidget::SlotVideoPlayStateChanged);
    connect(m_videoFFmpeg, &VideoFFmpegUtils::SigPlayProgressUpdated, this, &PlayerVideoModuleWidget::SlotVideoProgressUpdated);
    connect(m_videoFFmpeg, &VideoFFmpegUtils::SigFrameUpdated, this, &PlayerVideoModuleWidget::SlotVideoFrameUpdated);
    connect(m_videoFFmpeg, &VideoFFmpegUtils::SigError, this, &PlayerVideoModuleWidget::SlotVideoError);
}

void PlayerVideoModuleWidget::SetVideoFrame(const uint8_t* frameData, int width, int height)
{
    if (!frameData || width <= 0 || height <= 0)
    {
        return;
    }

    // 创建QImage从RGBA数据
    QImage image(frameData, width, height, QImage::Format_RGBA8888);
    if (image.isNull())
    {
        return;
    }

    // 转换为QPixmap并显示
    QPixmap pixmap = QPixmap::fromImage(image);
    if (!pixmap.isNull())
    {
        // 计算保持纵横比的缩放尺寸
        QSize labelSize = m_videoDisplayLabel->size();
        QSize scaledSize = pixmap.size().scaled(labelSize, Qt::KeepAspectRatio);

        QPixmap scaledPixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_videoDisplayLabel->setPixmap(scaledPixmap);
    }
}

void PlayerVideoModuleWidget::ClearVideoDisplay()
{
    m_videoDisplayLabel->clear();
    m_videoDisplayLabel->setText("选择视频文件开始播放");
}

void PlayerVideoModuleWidget::ResizeVideoDisplay()
{
    // 使用新的QLabel::pixmap()接口
    QPixmap currentPixmap = m_videoDisplayLabel->pixmap(Qt::ReturnByValue);
    if (!currentPixmap.isNull())
    {
        QSize labelSize = m_videoDisplayLabel->size();
        QPixmap scaledPixmap = currentPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_videoDisplayLabel->setPixmap(scaledPixmap);
    }
}

void PlayerVideoModuleWidget::SlotVideoPlayStateChanged(EM_VideoPlayState state)
{
    switch (state)
    {
        case EM_VideoPlayState::Playing:
            // 播放状态不需要定时器，直接通过信号更新
            break;
        case EM_VideoPlayState::Paused:
            // 暂停状态
            break;
        case EM_VideoPlayState::Stopped:
            ClearVideoDisplay();
            break;
    }
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
    m_videoDisplayLabel->setText("视频播放错误: " + errorMsg);
}
