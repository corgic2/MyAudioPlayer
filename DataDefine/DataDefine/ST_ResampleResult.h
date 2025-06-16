#pragma once

#include <vector>
#include "BaseDataDefine/ST_AVPacket.h"
#include "BaseDataDefine/ST_AVSampleFormat.h"

/// <summary>
/// 重采样结果封装类
/// </summary>
class ST_ResampleResult
{
  public:
    const std::vector<uint8_t> &GetData() const
    {
        return data;
    }
    int GetSamples() const
    {
        return m_samples;
    }
    int GetChannels() const
    {
        return m_channels;
    }
    int GetSampleRate() const
    {
        return m_sampleRate;
    }
    const ST_AVSampleFormat &GetSampleFormat() const
    {
        return m_sampleFmt;
    }

    void SetData(const std::vector<uint8_t> &newData)
    {
        data = newData;
    }
    void SetSamples(int samples)
    {
        m_samples = samples;
    }
    void SetChannels(int channels)
    {
        m_channels = channels;
    }
    void SetSampleRate(int rate)
    {
        m_sampleRate = rate;
    }
    void SetSampleFormat(const ST_AVSampleFormat &fmt)
    {
        m_sampleFmt = fmt;
    }

  private:
    std::vector<uint8_t> data;     /// 重采样后的数据
    int m_samples = 0;             /// 实际采样点数
    int m_channels = 0;            /// 输出声道数
    int m_sampleRate = 0;          /// 输出采样率
    ST_AVSampleFormat m_sampleFmt; /// 输出格式
};




