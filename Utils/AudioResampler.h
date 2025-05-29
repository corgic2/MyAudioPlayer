#pragma once
#include "FFmpegAudioDataDefine.h"
#include "libswresample/swresample.h"

// �ز������֧ࣨ�������ز�����
class AudioResampler
{
  public:
    AudioResampler();

    ~AudioResampler() = default;

    // ���ز�������
    void Resample(const uint8_t **inputData, int inputSamples, ST_ResampleResult &output, const ST_ResampleParams &params);

    // ˢ���ز���������ȡʣ�����ݣ�
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

