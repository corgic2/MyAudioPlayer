#pragma once
#include "ST_AVCodecContext.h"

#include "LogSystem/LogSystem.h"

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

bool ST_AVCodecContext::BindParamToContext(const AVCodecParameters* parameters)
{
    int ret = avcodec_parameters_to_context(m_pCodecContext, parameters);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to bind parameters: " + std::string(errbuf));
        return false;
    }
    return true;
}

bool ST_AVCodecContext::OpenCodec(const AVCodec* codec, AVDictionary** options)
{
    int ret = avcodec_open2(m_pCodecContext, codec, options);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to open encoder: " + std::string(errbuf));
        return false;
    }
    return true;
}

void ST_AVCodecContext::FlushBuffer()
{
    avcodec_flush_buffers(m_pCodecContext);
}

bool ST_AVCodecContext::CopyParamtersFromContext(AVCodecParameters* parameters)
{
    // 复制编码器参数到流
    int ret = avcodec_parameters_from_context(parameters,m_pCodecContext);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to copy encoder parameters: " + std::string(errbuf));
        return false;
    }
    return true;
}
