#include "ST_SDL_Texture.h"

ST_SDL_Texture::ST_SDL_Texture()
    : m_pTexture(nullptr)
{
}

ST_SDL_Texture::~ST_SDL_Texture()
{
    DestroyTexture();
}

bool ST_SDL_Texture::CreateFromSurface(ST_SDL_Renderer* renderer, ST_SDL_Surface* surface)
{
    if (!renderer || !surface)
    {
        return false;
    }

    if (m_pTexture)
    {
        DestroyTexture();
    }

    m_pTexture = SDL_CreateTextureFromSurface(renderer->GetRenderer(), surface->GetSurface());
    return m_pTexture != nullptr;
}

bool ST_SDL_Texture::CreateTexture(ST_SDL_Renderer* renderer, uint32_t format, SDL_TextureAccess access, int w, int h)
{
    if (!renderer)
    {
        return false;
    }

    if (m_pTexture)
    {
        DestroyTexture();
    }

    auto pixelFormat = static_cast<SDL_PixelFormat>(format);
    m_pTexture = SDL_CreateTexture(renderer->GetRenderer(), pixelFormat, access, w, h);
    return m_pTexture != nullptr;
}

void ST_SDL_Texture::DestroyTexture()
{
    if (m_pTexture)
    {
        SDL_DestroyTexture(m_pTexture);
        m_pTexture = nullptr;
    }
}

bool ST_SDL_Texture::UpdateTexture(const SDL_Rect* rect, const void* pixels, int pitch)
{
    if (!m_pTexture)
    {
        return false;
    }

    return SDL_UpdateTexture(m_pTexture, rect, pixels, pitch) == 0;
}

bool ST_SDL_Texture::LockTexture(const SDL_Rect* rect, void** pixels, int* pitch)
{
    if (!m_pTexture)
    {
        return false;
    }

    return SDL_LockTexture(m_pTexture, rect, pixels, pitch) == 0;
}

void ST_SDL_Texture::UnlockTexture()
{
    if (m_pTexture)
    {
        SDL_UnlockTexture(m_pTexture);
    }
}

float ST_SDL_Texture::GetWidth() const
{
    if (!m_pTexture)
    {
        return 0;
    }

    float w = 0;
    float h = 0;
    SDL_GetTextureSize(m_pTexture, &w, &h);
    return w;
}

float ST_SDL_Texture::GetHeight() const
{
    if (!m_pTexture)
    {
        return 0;
    }

    float w = 0;
    float h = 0;
    SDL_GetTextureSize(m_pTexture, &w, &h);
    return h;
}
