#include "AudioWaveformWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <cmath>

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

std::pair<float, float> AudioWaveformWidget::CalculateRMS(int startIdx, int count) const
{
    if (startIdx >= m_samples.size() || count <= 0)
    {
        return {0.0f, 0.0f};
    }

    float sumSquares = 0.0f;
    float peakValue = 0.0f;
    int actualCount = std::min(count, m_samples.size() - startIdx);

    for (int i = 0; i < actualCount; ++i)
    {
        float sample = m_samples[startIdx + i];
        sumSquares += sample * sample;
        peakValue = std::max(peakValue, std::abs(sample));
    }

    float rms = std::sqrt(sumSquares / actualCount);
    return {rms, peakValue};
}

float AudioWaveformWidget::NormalizeValue(float value) const
{
    // 将值限制在 [-1, 1] 范围内
    return std::max(-1.0f, std::min(1.0f, value));
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
    painter.setBrush(UIColorDefine::theme_color::Primary);

    const int height = this->height();
    const int width = this->width();
    const int centerY = height / 2;

    // 计算每个显示块包含的采样点数
    const int samplesPerPixel = std::max(1, m_samples.size() / width);
    
    // 绘制波形
    for (int x = 0; x < width; ++x)
    {
        int startIdx = x * samplesPerPixel;
        auto [rms, peak] = CalculateRMS(startIdx, samplesPerPixel);

        // 归一化RMS和峰值
        rms = NormalizeValue(rms);
        peak = NormalizeValue(peak);

        // 计算显示高度
        int rmsHeight = static_cast<int>(rms * centerY * 0.8f);  // 使用80%的高度显示RMS
        int peakHeight = static_cast<int>(peak * centerY * 0.9f); // 使用90%的高度显示峰值

        // 绘制RMS区域
        painter.drawRect(x, centerY - rmsHeight, SAMPLE_WIDTH, rmsHeight * 2);

        // 绘制峰值线
        painter.drawLine(x, centerY - peakHeight, x, centerY + peakHeight);
    }
}
