#include "FFmpegAudioDataDefine.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_timer.h>

// ST_OpenAudioDevice implementation
ST_OpenAudioDevice::ST_OpenAudioDevice(ST_OpenAudioDevice&& other) noexcept
    : m_pInputFormatCtx(std::move(other.m_pInputFormatCtx))
    , m_pFormatCtx(std::move(other.m_pFormatCtx))
{
}

ST_OpenAudioDevice& ST_OpenAudioDevice::operator=(ST_OpenAudioDevice&& other) noexcept
{
    if (this != &other)
    {
        m_pInputFormatCtx = std::move(other.m_pInputFormatCtx);
        m_pFormatCtx = std::move(other.m_pFormatCtx);
    }
    return *this;
}

// ST_AudioPlayInfo implementation
ST_AudioPlayInfo::~ST_AudioPlayInfo()
{
    StopAudio();
    SDL_Quit();
}

ST_SDLAudioStream& ST_AudioPlayInfo::GetAudioStream()
{
    return m_audioStream;
}

SDL_AudioSpec& ST_AudioPlayInfo::GetAudioSpec(bool bsrc)
{
    return bsrc ? m_srcSpec : m_dstSpec;
}

ST_SDLAudioDeviceID& ST_AudioPlayInfo::GetDeviceId()
{
    return m_audioDeviceId;
}

void ST_AudioPlayInfo::InitAudioSpec(bool bsrc, int sampleRate, SDL_AudioFormat format, int channels)
{
    if (bsrc)
    {
        SDL_zero(m_srcSpec);
        m_srcSpec.freq = sampleRate;
        m_srcSpec.format = format;
        m_srcSpec.channels = channels;
    }
    else
    {
        SDL_zero(m_dstSpec);
        m_dstSpec.freq = sampleRate;
        m_dstSpec.format = format;
        m_dstSpec.channels = channels;
    }
}

void ST_AudioPlayInfo::InitAudioStream()
{
    m_audioStream = std::move(ST_SDLAudioStream(&m_srcSpec, &m_dstSpec));
}

void ST_AudioPlayInfo::InitAudioDevice(SDL_AudioDeviceID deviceId)
{
    m_audioDeviceId = std::move(ST_SDLAudioDeviceID(deviceId, &m_dstSpec));
}

void ST_AudioPlayInfo::BindStreamAndDevice()
{
    SDL_BindAudioStream(m_audioDeviceId.GetRawDeviceID(), m_audioStream.GetRawStream());
}

void ST_AudioPlayInfo::BeginPlayAudio()
{
    m_startTime = SDL_GetTicks();
    m_currentPosition = 0;
    SDL_ResumeAudioDevice(m_audioDeviceId.GetRawDeviceID());
}

void ST_AudioPlayInfo::PauseAudio()
{
    SDL_PauseAudioDevice(m_audioDeviceId.GetRawDeviceID());
    m_currentPosition += (SDL_GetTicks() - m_startTime) / 1000.0;
}

void ST_AudioPlayInfo::ResumeAudio()
{
    m_startTime = SDL_GetTicks();
    SDL_ResumeAudioDevice(m_audioDeviceId.GetRawDeviceID());
}

void ST_AudioPlayInfo::StopAudio()
{
    SDL_PauseAudioDevice(m_audioDeviceId.GetRawDeviceID());
    FlushAudioStream();
    m_currentPosition = 0;
}

void ST_AudioPlayInfo::SeekAudio(int seconds)
{
    double newPosition = m_currentPosition + seconds;
    if (newPosition < 0)
    {
        newPosition = 0;
    }
    else if (newPosition > m_duration)
    {
        newPosition = m_duration;
    }

    m_currentPosition = newPosition;
    m_startTime = SDL_GetTicks();

    FlushAudioStream();
    SDL_ResumeAudioDevice(m_audioDeviceId.GetRawDeviceID());
}

void ST_AudioPlayInfo::PutDataToStream(const void* buf, int len)
{
    SDL_PutAudioStreamData(m_audioStream.GetRawStream(), buf, len);
}

bool ST_AudioPlayInfo::GetDataIsEnd()
{
    return SDL_GetAudioStreamQueued(m_audioStream.GetRawStream()) > 0;
}

void ST_AudioPlayInfo::Delay(Uint32 ms)
{
    SDL_Delay(ms);
}

double ST_AudioPlayInfo::GetCurrentPosition() const
{
    if (!SDL_AudioDevicePaused(m_audioDeviceId.GetRawDeviceID()))
    {
        return m_currentPosition + (SDL_GetTicks() - m_startTime) / 1000.0;
    }
    return m_currentPosition;
}

double ST_AudioPlayInfo::GetDuration() const
{
    return m_duration;
}

void ST_AudioPlayInfo::FlushAudioStream()
{
    SDL_FlushAudioStream(m_audioStream.GetRawStream());
}

int ST_AudioPlayInfo::GetAudioStreamAvailable() const
{
    return SDL_GetAudioStreamAvailable(m_audioStream.GetRawStream());
}

// ST_ResampleParams implementation
ST_ResampleParams::ST_ResampleParams()
{
    ST_ResampleSimpleData input;
    input.SetSampleRate(48000);
    input.SetChannels(2);
    input.SetSampleFormat(ST_AVSampleFormat{AV_SAMPLE_FMT_FLTP});

    ST_ResampleSimpleData output;
    output.SetSampleRate(44100);
    output.SetChannels(2);
    output.SetSampleFormat(ST_AVSampleFormat{AV_SAMPLE_FMT_S16});

    SetInput(input);
    SetOutput(output);
}

