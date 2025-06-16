#include "ST_OpenAudioDevice.h"

// ST_OpenAudioDevice implementation
ST_OpenAudioDevice::ST_OpenAudioDevice(ST_OpenAudioDevice &&other) noexcept : m_pInputFormatCtx(std::move(other.m_pInputFormatCtx)), m_pFormatCtx(std::move(other.m_pFormatCtx))
{
}

ST_OpenAudioDevice &ST_OpenAudioDevice::operator=(ST_OpenAudioDevice &&other) noexcept
{
    if (this != &other)
    {
        m_pInputFormatCtx = std::move(other.m_pInputFormatCtx);
        m_pFormatCtx = std::move(other.m_pFormatCtx);
    }
    return *this;
}
