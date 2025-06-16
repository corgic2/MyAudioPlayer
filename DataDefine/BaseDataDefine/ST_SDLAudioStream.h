#pragma once

extern "C" {
#include "SDL3/SDL_audio.h"
}
/// <summary>
/// SDL音频流封装类
/// </summary>
class ST_SDLAudioStream
{
  public:
    ST_SDLAudioStream() = default;
    ST_SDLAudioStream(SDL_AudioSpec *srcSpec, SDL_AudioSpec *dstSpec);
    ~ST_SDLAudioStream();

    // 移动操作
    ST_SDLAudioStream(ST_SDLAudioStream &&other) noexcept;
    ST_SDLAudioStream &operator=(ST_SDLAudioStream &&other) noexcept;

    /// <summary>
    /// 获取原始音频流指针
    /// </summary>
    SDL_AudioStream *GetRawStream() const
    {
        return m_audioStreamSdl;
    }

  private:
    SDL_AudioStream *m_audioStreamSdl = nullptr;
};
