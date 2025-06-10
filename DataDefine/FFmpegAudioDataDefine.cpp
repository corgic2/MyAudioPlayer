#include "FFmpegAudioDataDefine.h"

#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_timer.h"

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
        m_srcSpec.freq = sampleRate; // 采样率
        m_srcSpec.format = format; // 格式转换
        m_srcSpec.channels = channels; // 声道数
    }
    else
    {
        SDL_zero(m_dstSpec);
        m_dstSpec.freq = sampleRate; // 采样率
        m_dstSpec.format = format; // 格式转换
        m_dstSpec.channels = channels; // 声道数
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
    SDL_BindAudioStream(m_audioDeviceId.m_audioDevice, m_audioStream.m_audioStreamSdl);
}

void ST_AudioPlayInfo::BeginPlayAudio()
{
    m_startTime = SDL_GetTicks();
    m_currentPosition = 0;
    SDL_ResumeAudioDevice(m_audioDeviceId.m_audioDevice);
}

void ST_AudioPlayInfo::PauseAudio()
{
    SDL_PauseAudioDevice(m_audioDeviceId.m_audioDevice);
    // 更新当前播放位置
    m_currentPosition += (SDL_GetTicks() - m_startTime) / 1000.0;
}

void ST_AudioPlayInfo::ResumeAudio()
{
    m_startTime = SDL_GetTicks();
    SDL_ResumeAudioDevice(m_audioDeviceId.m_audioDevice);
}

void ST_AudioPlayInfo::StopAudio()
{
    SDL_PauseAudioDevice(m_audioDeviceId.m_audioDevice);
    FlushAudioStream();
    m_currentPosition = 0;
}

void ST_AudioPlayInfo::SeekAudio(int seconds)
{
    // 计算新的播放位置
    double newPosition = m_currentPosition + seconds;
    if (newPosition < 0)
    {
        newPosition = 0;
    }
    else if (newPosition > m_duration)
    {
        newPosition = m_duration;
    }

    // 更新播放位置
    m_currentPosition = newPosition;
    m_startTime = SDL_GetTicks();

    // 清空音频流并重新开始播放
    FlushAudioStream();
    SDL_ResumeAudioDevice(m_audioDeviceId.m_audioDevice);
}

void ST_AudioPlayInfo::PutDataToStream(const void* buf, int len)
{
    SDL_PutAudioStreamData(m_audioStream.m_audioStreamSdl, buf, len);
}

bool ST_AudioPlayInfo::GetDataIsEnd()
{
    return SDL_GetAudioStreamQueued(m_audioStream.m_audioStreamSdl) > 0;
}

void ST_AudioPlayInfo::Delay(Uint32 ms)
{
    SDL_Delay(ms);
}

double ST_AudioPlayInfo::GetCurrentPosition() const
{
    if (!SDL_AudioDevicePaused(m_audioDeviceId.m_audioDevice))
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
    SDL_FlushAudioStream(m_audioStream.m_audioStreamSdl);
}

int ST_AudioPlayInfo::GetAudioStreamAvailable() const
{
    return SDL_GetAudioStreamAvailable(m_audioStream.m_audioStreamSdl);
}

