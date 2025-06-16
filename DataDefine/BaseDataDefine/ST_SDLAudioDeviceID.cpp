#pragma once
#include "ST_SDLAudioDeviceID.h"

ST_SDLAudioDeviceID::ST_SDLAudioDeviceID(SDL_AudioDeviceID deviceId, const SDL_AudioSpec *spec) : m_audioDevice(SDL_OpenAudioDevice(deviceId, spec))
{
}

ST_SDLAudioDeviceID::~ST_SDLAudioDeviceID()
{
    if (m_audioDevice)
    {
        SDL_CloseAudioDevice(m_audioDevice);
    }
}

ST_SDLAudioDeviceID::ST_SDLAudioDeviceID(ST_SDLAudioDeviceID &&other) noexcept : m_audioDevice(other.m_audioDevice)
{
    other.m_audioDevice = 0;
}

ST_SDLAudioDeviceID &ST_SDLAudioDeviceID::operator=(ST_SDLAudioDeviceID &&other) noexcept
{
    if (this != &other)
    {
        if (m_audioDevice)
        {
            SDL_CloseAudioDevice(m_audioDevice);
        }
        m_audioDevice = other.m_audioDevice;
        other.m_audioDevice = 0;
    }
    return *this;
}