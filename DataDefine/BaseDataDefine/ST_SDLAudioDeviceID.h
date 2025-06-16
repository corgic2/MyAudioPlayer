#pragma once

extern "C"
{
#include "SDL3/SDL_audio.h"
}
/// <summary>
/// SDL音频设备ID封装类
/// </summary>
class ST_SDLAudioDeviceID
{
  public:
    ST_SDLAudioDeviceID() = default;
    ST_SDLAudioDeviceID(SDL_AudioDeviceID deviceId, const SDL_AudioSpec *spec);
    ~ST_SDLAudioDeviceID();

    // 移动操作
    ST_SDLAudioDeviceID(ST_SDLAudioDeviceID &&other) noexcept;
    ST_SDLAudioDeviceID &operator=(ST_SDLAudioDeviceID &&other) noexcept;

    /// <summary>
    /// 获取原始设备ID
    /// </summary>
    SDL_AudioDeviceID GetRawDeviceID() const
    {
        return m_audioDevice;
    }

  private:
    SDL_AudioDeviceID m_audioDevice = 0;
};
