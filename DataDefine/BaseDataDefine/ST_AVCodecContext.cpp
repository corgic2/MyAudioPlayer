#pragma once
#include "ST_AVCodecContext.h"

ST_AVCodecContext::ST_AVCodecContext(const AVCodec *codec) : m_pCodecContext(avcodec_alloc_context3(codec))
{
}

ST_AVCodecContext::~ST_AVCodecContext()
{
    if (m_pCodecContext)
    {
        avcodec_free_context(&m_pCodecContext);
    }
}

int ST_AVCodecContext::BindParamToContext(const AVCodecParameters *parameters)
{
    return avcodec_parameters_to_context(m_pCodecContext, parameters);
}

int ST_AVCodecContext::OpenCodec(const AVCodec *codec, AVDictionary **options)
{
    return avcodec_open2(m_pCodecContext, codec, options);
}
