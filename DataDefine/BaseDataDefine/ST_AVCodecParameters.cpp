#pragma once
#include "ST_AVCodecParameters.h"

ST_AVCodecParameters::ST_AVCodecParameters(AVCodecParameters *obj) : m_codecParameters(obj)
{
}

ST_AVCodecParameters::~ST_AVCodecParameters()
{
    m_codecParameters = nullptr;
}

AVCodecID ST_AVCodecParameters::GetCodecId() const
{
    return m_codecParameters ? m_codecParameters->codec_id : AV_CODEC_ID_NONE;
}

int ST_AVCodecParameters::GetSamplePerRate() const
{
    return m_codecParameters ? av_get_bytes_per_sample(GetSampleFormat()) : 0;
}

AVSampleFormat ST_AVCodecParameters::GetSampleFormat() const
{
    return m_codecParameters ? static_cast<AVSampleFormat>(m_codecParameters->format) : AV_SAMPLE_FMT_NONE;
}

int ST_AVCodecParameters::GetBitPerSample() const
{
    return m_codecParameters ? av_get_bits_per_sample(m_codecParameters->codec_id) : 0;
}