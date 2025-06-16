#include "AudioWaveformWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

AudioWaveformWidget::AudioWaveformWidget(QWidget* parent)
    : QWidget(parent)
{
    // 设置背景色为白色
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, UIColorDefine::background_color::White);
    setPalette(pal);
}

void AudioWaveformWidget::SetWaveformData(const QVector<float>& samples)
{
    m_samples = samples;
    update(); // 触发重绘
}

void AudioWaveformWidget::ClearWaveform()
{
    m_samples.clear();
    update();
}

void AudioWaveformWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_samples.isEmpty())
    {
        return;
    }

    // 设置波形颜色为主题色
    painter.setPen(QPen(UIColorDefine::theme_color::Primary, 1));

    const int height = this->height();
    const int width = this->width();
    const int centerY = height / 3;

    // 计算每个采样点的x坐标步进
    const float xStep = static_cast<float>(width) / m_samples.size();

    // 绘制波形
    QPainterPath path;
    bool firstPoint = true;

    for (int i = 0; i < m_samples.size(); ++i)
    {
        const float x = i * xStep;
        const float amplitude = m_samples[i];
        const float y = centerY + (amplitude * centerY); // 将振幅映射到控件高度

        if (firstPoint)
        {
            path.moveTo(x, y);
            firstPoint = false;
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    painter.drawPath(path);
}
