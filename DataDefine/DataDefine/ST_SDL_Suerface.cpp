#include "ST_SDL_Suerface.h"

ST_SDL_Surface::ST_SDL_Surface()
    : m_pSurface(nullptr)
{
}

ST_SDL_Surface::~ST_SDL_Surface()
{
    FreeSurface();
}

bool ST_SDL_Surface::CreateFromImage(const std::string& filePath)
{
    if (m_pSurface)
    {
        FreeSurface();
    }

    m_pSurface = SDL_LoadBMP(filePath.c_str());
    return m_pSurface != nullptr;
}

bool ST_SDL_Surface::CreateRGBSurface(int width, int height, int depth)
{
    if (m_pSurface)
    {
        FreeSurface();
    }

    m_pSurface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    return m_pSurface != nullptr;
}

void ST_SDL_Surface::FreeSurface()
{
    if (m_pSurface)
    {
        SDL_DestroySurface(m_pSurface);
        m_pSurface = nullptr;
    }
}

int ST_SDL_Surface::GetWidth() const
{
    if (m_pSurface)
    {
        return m_pSurface->w;
    }
    return 0;
}

int ST_SDL_Surface::GetHeight() const
{
    if (m_pSurface)
    {
        return m_pSurface->h;
    }
    return 0;
}

uint32_t ST_SDL_Surface::GetFormat() const
{
    if (m_pSurface)
    {
        return m_pSurface->format;
    }
    return 0;
}
