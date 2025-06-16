#pragma once
#include "BaseDataDefine/ST_AVChannelLayout.h"
#include "BaseDataDefine/ST_AVPacket.h"
#include "BaseDataDefine/ST_AVSampleFormat.h"


/// <summary>
/// 重采样简单数据封装类
/// </summary>
class ST_ResampleSimpleData
{
  public:
    ST_ResampleSimpleData &operator=(const ST_ResampleSimpleData &obj)
    {
        if (this != &obj)
        {
            m_sampleRate = obj.m_sampleRate;
            m_sampleFmt = obj.m_sampleFmt;
            m_channelLayout = obj.m_channelLayout;
        }
        return *this;
    }
    int GetSampleRate() const
    {
        return m_sampleRate;
    }

    int GetChannels() const
    {
        return m_channelLayout.GetRawLayout()->nb_channels;
    }
    const ST_AVSampleFormat &GetSampleFormat() const
    {
        return m_sampleFmt;
    }
    const ST_AVChannelLayout &GetChannelLayout() const
    {
        return m_channelLayout;
    }

    void SetSampleRate(int rate)
    {
        m_sampleRate = rate;
    }

    void SetChannels(int channels)
    {
        m_channelLayout.GetRawLayout()->nb_channels = channels;
    }
    void SetSampleFormat(const ST_AVSampleFormat &fmt)
    {
        m_sampleFmt = fmt;
    }
    void SetChannelLayout(const ST_AVChannelLayout &layout)
    {
        m_channelLayout = layout;
    }

  private:
    int m_sampleRate = 0;               /// 采样率
    ST_AVSampleFormat m_sampleFmt;      /// 采样格式
    ST_AVChannelLayout m_channelLayout; /// 通道布局
};




