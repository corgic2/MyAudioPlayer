#pragma once
#include "ST_SDLAudioStream.h"

// ST_SDLAudioStream implementation
ST_SDLAudioStream::ST_SDLAudioStream(SDL_AudioSpec *srcSpec, SDL_AudioSpec *dstSpec) : m_audioStreamSdl(SDL_CreateAudioStream(srcSpec, dstSpec))
{
}

ST_SDLAudioStream::~ST_SDLAudioStream()
{
    if (m_audioStreamSdl)
    {
        SDL_DestroyAudioStream(m_audioStreamSdl);
    }
}

ST_SDLAudioStream::ST_SDLAudioStream(ST_SDLAudioStream &&other) noexcept : m_audioStreamSdl(other.m_audioStreamSdl)
{
    other.m_audioStreamSdl = nullptr;
}

ST_SDLAudioStream &ST_SDLAudioStream::operator=(ST_SDLAudioStream &&other) noexcept
{
    if (this != &other)
    {
        if (m_audioStreamSdl)
        {
            SDL_DestroyAudioStream(m_audioStreamSdl);
        }
        m_audioStreamSdl = other.m_audioStreamSdl;
        other.m_audioStreamSdl = nullptr;
    }
    return *this;
}