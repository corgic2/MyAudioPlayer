#pragma once
#include "FFmpegAudioDataDefine.h"
#include "libswresample/swresample.h"

// 重采样器类（支持连续重采样）
class AudioResampler
{
  public:
    AudioResampler();

    ~AudioResampler() = default;

    // 主重采样函数
    void Resample(const uint8_t **inputData, int inputSamples, ST_ResampleResult &output, const ST_ResampleParams &params);

    // 刷新重采样器（获取剩余数据）
    void Flush(ST_ResampleResult &output, const ST_ResampleParams &params);
  private:
    ST_SwrContext m_swrCtx;
    AVChannelLayout m_lastInLayout;
    AVChannelLayout m_lastOutLayout;
    int m_lastInRate = 0;
    int m_lastOutRate = 0;
    AVSampleFormat m_lastInFmt;
    AVSampleFormat m_lastOutFmt;
};

