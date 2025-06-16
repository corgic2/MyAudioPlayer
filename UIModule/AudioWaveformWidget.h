#pragma once

#include <QWidget>
#include <QVector>
#include "CommonDefine/UIWidgetColorDefine.h"

/// <summary>
/// 音频波形显示控件类
/// </summary>
class AudioWaveformWidget : public QWidget
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit AudioWaveformWidget(QWidget* parent = nullptr);

    /// <summary>
    /// 设置波形数据
    /// </summary>
    /// <param name="samples">音频采样数据</param>
    void SetWaveformData(const QVector<float>& samples);

    /// <summary>
    /// 清除波形数据
    /// </summary>
    void ClearWaveform();

protected:
    /// <summary>
    /// 绘制事件
    /// </summary>
    /// <param name="event">绘制事件指针</param>
    void paintEvent(QPaintEvent* event) override;

private:
    /// <summary>
    /// 计算RMS值
    /// </summary>
    /// <param name="startIdx">起始索引</param>
    /// <param name="count">计算数量</param>
    /// <returns>RMS值和峰值对</returns>
    std::pair<float, float> CalculateRMS(int startIdx, int count) const;

    /// <summary>
    /// 归一化数据
    /// </summary>
    /// <param name="value">输入值</param>
    /// <returns>归一化后的值</returns>
    float NormalizeValue(float value) const;

private:
    QVector<float> m_samples;           /// 音频采样数据
    static const int SAMPLE_WIDTH = 2;  /// 采样点宽度
    static const int SAMPLE_SPACING = 1;/// 采样点间距
};