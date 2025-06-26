#include "AudioWaveformWidget.h"
#include <cmath>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <string>
#include "LogSystem/LogSystem.h"
#include "TimeSystem/TimeSystem.h"

AudioWaveformWidget::AudioWaveformWidget(QWidget* parent)
    : QWidget(parent)
{
    // 设置背景色为白色
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, UIColorDefine::background_color::White);
    setPalette(pal);

    // 设置最小尺寸
    setMinimumHeight(100);
    setMinimumWidth(200);
}

void AudioWaveformWidget::SetWaveformData(const QVector<float>& samples)
{
    AUTO_TIMER_LOG("AudioWaveform", EM_TimingLogLevel::Info);
    LOG_INFO("Setting audio waveform data, sample count: " + std::to_string(samples.size()));
    
    m_samples = samples;
    update(); // 触发重绘
}

void AudioWaveformWidget::SetPlaybackPosition(double position)
{
    if (position != m_playbackPosition)
    {
        m_playbackPosition = position;
        update();
    }
}

void AudioWaveformWidget::ClearWaveform()
{
    m_samples.clear();
    m_playbackPosition = 0.0;
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
    AUTO_TIMER_LOG("AudioWaveformPaint", EM_TimingLogLevel::Debug);
    
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_samples.isEmpty())
    {
        // 绘制空白状态提示
        painter.setPen(UIColorDefine::font_color::Secondary);
        painter.drawText(rect(), Qt::AlignCenter, "点击加载音频文件以显示波形");
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

    TIME_START("WaveformDraw");
    
    // 绘制波形
    for (int x = 0; x < width; ++x)
    {
        int startIdx = x * samplesPerPixel;
        auto [rms, peak] = CalculateRMS(startIdx, samplesPerPixel);

        // 归一化RMS和峰值
        rms = NormalizeValue(rms);
        peak = NormalizeValue(peak);

        // 计算显示高度
        int rmsHeight = static_cast<int>(rms * centerY * 0.8f);   // 使用80%的高度显示RMS
        int peakHeight = static_cast<int>(peak * centerY * 0.9f); // 使用90%的高度显示峰值

        // 绘制RMS区域（填充）
        QColor fillColor = UIColorDefine::theme_color::Primary;
        fillColor.setAlpha(128); // 半透明
        painter.fillRect(x, centerY - rmsHeight, SAMPLE_WIDTH, rmsHeight * 2, fillColor);

        // 绘制峰值线
        painter.setPen(QPen(UIColorDefine::theme_color::Primary, 1));
        painter.drawLine(x, centerY - peakHeight, x, centerY + peakHeight);
    }
    
    double waveformDrawDuration = TimeSystem::Instance().StopTiming("WaveformDraw", EM_TimeUnit::Microseconds);
    
    // 只在绘制耗时较长时记录
    if (waveformDrawDuration > 1000) // 大于1ms
    {
        LOG_DEBUG("Waveform drawing took " + std::to_string(waveformDrawDuration) + " μs");
    }

    // 绘制播放位置指示器
    TIME_START("IndicatorDraw");
    if (m_playbackPosition > 0.0 && !m_samples.isEmpty())
    {
        int positionX = static_cast<int>(m_playbackPosition * width);
        if (positionX >= 0 && positionX < width)
        {
            painter.setPen(QPen(UIColorDefine::theme_color::Error, 2));
            painter.drawLine(positionX, 0, positionX, height);
        }
    }
    // 绘制中心线
    painter.setPen(QPen(UIColorDefine::border_color::Default, 1));
    painter.drawLine(0, centerY, width, centerY);
    
    double indicatorDrawDuration = TimeSystem::Instance().StopTiming("IndicatorDraw", EM_TimeUnit::Microseconds);
    
    // 只在绘制耗时较长时记录
    if (indicatorDrawDuration > 500) // 大于0.5ms
    {
        LOG_DEBUG("Indicator drawing took " + std::to_string(indicatorDrawDuration) + " μs");
    }
}

void AudioWaveformWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !m_samples.isEmpty())
    {
        // 计算点击位置对应的播放位置
        double position = static_cast<double>(event->x()) / width();
        emit SigSeekPosition(position);
    }
    QWidget::mousePressEvent(event);
}
