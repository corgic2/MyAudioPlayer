#pragma once
#include "ST_AVInputFormat.h"
// ST_AVInputFormat implementation
ST_AVInputFormat::ST_AVInputFormat(ST_AVInputFormat &&other) noexcept : m_pInputFormatCtx(other.m_pInputFormatCtx)
{
    other.m_pInputFormatCtx = nullptr;
}

ST_AVInputFormat &ST_AVInputFormat::operator=(ST_AVInputFormat &&other) noexcept
{
    if (this != &other)
    {
        m_pInputFormatCtx = other.m_pInputFormatCtx;
        other.m_pInputFormatCtx = nullptr;
    }
    return *this;
}

void ST_AVInputFormat::FindInputFormat(const std::string &deviceFormat)
{
    m_pInputFormatCtx = av_find_input_format(deviceFormat.c_str());
}
