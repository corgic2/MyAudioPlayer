#include "ST_SDL_Renderer.h"

ST_SDL_Renderer::ST_SDL_Renderer()
    : m_pRenderer(nullptr)
{
}

ST_SDL_Renderer::~ST_SDL_Renderer()
{
    DestroyRenderer();
}

bool ST_SDL_Renderer::CreateRenderer(ST_SDL_Window* window, int driver, uint32_t flags)
{
    if (m_pRenderer)
    {
        DestroyRenderer();
    }

    if (!window)
    {
        return false;
    }

    m_pRenderer = SDL_CreateRenderer(window->GetWindow(), nullptr); // SDL3简化了创建渲染器的方式
    if (m_pRenderer)
    {
        SDL_SetRenderVSync(m_pRenderer, 1); // 启用垂直同步
    }
    return m_pRenderer != nullptr;
}

void ST_SDL_Renderer::DestroyRenderer()
{
    if (m_pRenderer)
    {
        SDL_DestroyRenderer(m_pRenderer);
        m_pRenderer = nullptr;
    }
}

void ST_SDL_Renderer::Clear()
{
    if (m_pRenderer)
    {
        SDL_RenderClear(m_pRenderer);
    }
}

void ST_SDL_Renderer::SetDrawColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (m_pRenderer)
    {
        SDL_SetRenderDrawColor(m_pRenderer, r, g, b, a);
    }
}

void ST_SDL_Renderer::Present()
{
    if (m_pRenderer)
    {
        SDL_RenderPresent(m_pRenderer);
    }
}

bool ST_SDL_Renderer::CopyTexture(SDL_Texture* texture, const SDL_FRect* srcRect, const SDL_FRect* dstRect)
{
    if (!m_pRenderer || !texture)
    {
        return false;
    }

    return SDL_RenderTexture(m_pRenderer, texture, srcRect, dstRect) == 0;
}
