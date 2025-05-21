#include "FFmpegAudioDataDefine.h"

#include "SDL3/SDL_timer.h"

ST_AudioPlayInfo::~ST_AudioPlayInfo()
{
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

void ST_AudioPlayInfo::InitAudioSpec(bool bsrc, int sampleRate, AVSampleFormat format, int channels)
{
    if (bsrc)
    {
        SDL_zero(m_srcSpec);
        m_srcSpec.freq = sampleRate; // 采样率
        m_srcSpec.format = FFmpegPublicUtils::FFmpegToSDLFormat(format); // 格式转换（见下文）
        m_srcSpec.channels = channels; // 声道数
    }
    else
    {
        SDL_zero(m_dstSpec);
        m_dstSpec.freq = sampleRate; // 采样率
        m_dstSpec.format = FFmpegPublicUtils::FFmpegToSDLFormat(format); // 格式转换（见下文）
        m_dstSpec.channels = channels; // 声道数
    }
}

void ST_AudioPlayInfo::InitAudioStream()
{
    m_audioStream = std::move(ST_SDLAudioStream(&m_srcSpec, &m_dstSpec));
}

void ST_AudioPlayInfo::InitAudioDevice(SDL_AudioDeviceID devid)
{
    m_audioDeviceId = std::move(ST_SDLAudioDeviceID(devid, &m_dstSpec));
}

void ST_AudioPlayInfo::BindStreamAndDevice()
{
    SDL_BindAudioStream(m_audioDeviceId.m_audioDevice, m_audioStream.m_audioStreamSdl);
}

void ST_AudioPlayInfo::BeginPlayAudio()
{
    SDL_ResumeAudioDevice(m_audioDeviceId.m_audioDevice);
}

void ST_AudioPlayInfo::PutDataToStream(const void* buf, int len)
{
    SDL_PutAudioStreamData(m_audioStream.m_audioStreamSdl, buf, len);
}

int ST_AudioPlayInfo::GetDataIsEnd()
{
    return SDL_GetAudioStreamQueued(m_audioStream.m_audioStreamSdl);
}

void ST_AudioPlayInfo::Delay(Uint32 ms)
{
    SDL_Delay(ms);
}

