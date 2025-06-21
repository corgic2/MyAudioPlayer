#include "ST_SDL_Window.h"

ST_SDL_Window::ST_SDL_Window()
    : m_pWindow(nullptr)
{
}

ST_SDL_Window::~ST_SDL_Window()
{
    DestroyWindow();
}

bool ST_SDL_Window::CreateWindow(const std::string& title, int x, int y, int w, int h, uint32_t flags)
{
    if (m_pWindow)
    {
        DestroyWindow();
    }

    SDL_WindowFlags windowFlags = static_cast<SDL_WindowFlags>(flags);
    m_pWindow = SDL_CreateWindow(title.c_str(), w, h, windowFlags);
    if (m_pWindow)
    {
        SDL_SetWindowPosition(m_pWindow, x, y);
        return true;
    }
    return false;
}

void ST_SDL_Window::DestroyWindow()
{
    if (m_pWindow)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }
}

uint32_t ST_SDL_Window::GetWindowID() const
{
    if (m_pWindow)
    {
        return SDL_GetWindowID(m_pWindow);
    }
    return 0;
}

void ST_SDL_Window::SetTitle(const std::string& title)
{
    if (m_pWindow)
    {
        SDL_SetWindowTitle(m_pWindow, title.c_str());
    }
}

void ST_SDL_Window::SetSize(int w, int h)
{
    if (m_pWindow)
    {
        SDL_SetWindowSize(m_pWindow, w, h);
    }
}

void ST_SDL_Window::SetPosition(int x, int y)
{
    if (m_pWindow)
    {
        SDL_SetWindowPosition(m_pWindow, x, y);
    }
}

void ST_SDL_Window::Show()
{
    if (m_pWindow)
    {
        SDL_ShowWindow(m_pWindow);
    }
}

void ST_SDL_Window::Hide()
{
    if (m_pWindow)
    {
        SDL_HideWindow(m_pWindow);
    }
}
