#pragma once
#include <cstdint>
#include <QDebug>
#include "../DataDefine/FFmpegAudioDataDefine.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
}

/// <summary>
/// 音频重采样器类（支持连续重采样）
/// </summary>
class AudioResampler
{
public:
    /// <summary>
    /// 构造函数
    /// </summary>
    AudioResampler();

    /// <summary>
    /// 析构函数
    /// </summary>
    ~AudioResampler() = default;

    /// <summary>
    /// 主重采样函数
    /// </summary>
    /// <param name="inputData">输入数据</param>
    /// <param name="inputSamples">输入样本数</param>
    /// <param name="output">输出结果</param>
    /// <param name="params">重采样参数</param>
    void Resample(const uint8_t** inputData, int inputSamples, ST_ResampleResult& output, const ST_ResampleParams& params);

    /// <summary>
    /// 刷新重采样器（获取剩余数据）
    /// </summary>
    /// <param name="output">输出结果</param>
    /// <param name="params">重采样参数</param>
    void Flush(ST_ResampleResult& output, const ST_ResampleParams& params);

private:
    ST_SwrContext m_swrCtx;               /// 重采样上下文
    ST_AVChannelLayout m_lastInLayout;    /// 上次输入通道布局
    ST_AVChannelLayout m_lastOutLayout;   /// 上次输出通道布局
    int m_lastInRate = 0;                 /// 上次输入采样率
    int m_lastOutRate = 0;                /// 上次输出采样率
    ST_AVSampleFormat m_lastInFmt;        /// 上次输入格式
    ST_AVSampleFormat m_lastOutFmt;       /// 上次输出格式
};

