#include "SDLWindowManager.h"
#include "LogSystem/LogSystem.h"

SDLWindowManager::SDLWindowManager(QObject* parent)
    : QObject(parent)
    , m_window(nullptr)
    , m_renderer(nullptr)
    , m_texture(nullptr)
    , m_windowVisible(false)
    , m_windowValid(false)
{
}

SDLWindowManager::~SDLWindowManager()
{
    DestroyWindow();
}

bool SDLWindowManager::CreateWindow(int width, int height, const QString& title)
{
    if (m_window)
    {
        LOG_WARN("SDL window already exists");
        return true;
    }

    // 创建SDL窗口
    m_window = SDL_CreateWindow(title.toUtf8().constData(), width, height, SDL_WINDOW_RESIZABLE);
    if (!m_window)
    {
        QString error = QString("Failed to create SDL window: %1").arg(SDL_GetError());
        LOG_ERROR(error.toStdString());
        emit ErrorOccurred(error);
        return false;
    }

    // 设置窗口居中
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // 创建渲染器
    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer)
    {
        QString error = QString("Failed to create SDL renderer: %1").arg(SDL_GetError());
        LOG_ERROR(error.toStdString());
        emit ErrorOccurred(error);
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }

    // 启用垂直同步
    SDL_SetRenderVSync(m_renderer, 1);

    m_windowValid = true;
    m_windowVisible = true;
    
    LOG_INFO("SDL window created successfully: " + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

void SDLWindowManager::DestroyWindow()
{
    if (m_texture)
    {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    if (m_renderer)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    m_windowValid = false;
    m_windowVisible = false;
    
    LOG_INFO("SDL window destroyed");
    emit WindowClosed();
}

void SDLWindowManager::ShowWindow(bool show)
{
    if (!m_window)
    {
        return;
    }

    if (show)
    {
        SDL_ShowWindow(m_window);
    }
    else
    {
        SDL_HideWindow(m_window);
    }
    
    m_windowVisible = show;
}

void SDLWindowManager::SetWindowTitle(const QString& title)
{
    if (m_window)
    {
        SDL_SetWindowTitle(m_window, title.toUtf8().constData());
    }
}

bool SDLWindowManager::CreateVideoTexture(float width, float height)
{
    if (!m_renderer)
    {
        LOG_ERROR("Cannot create texture: renderer not initialized");
        return false;
    }

    if (m_texture)
    {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!m_texture)
    {
        QString error = QString("Failed to create SDL texture: %1").arg(SDL_GetError());
        LOG_ERROR(error.toStdString());
        emit ErrorOccurred(error);
        return false;
    }

    LOG_INFO("SDL texture created successfully: " + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

bool SDLWindowManager::UpdateTexture(const void* data, int pitch)
{
    if (!m_texture || !data)
    {
        return false;
    }

    if (SDL_UpdateTexture(m_texture, nullptr, data, pitch) != 0)
    {
        QString error = QString("Failed to update SDL texture: %1").arg(SDL_GetError());
        LOG_ERROR(error.toStdString());
        emit ErrorOccurred(error);
        return false;
    }

    return true;
}

void SDLWindowManager::RenderFrame()
{
    if (!m_renderer || !m_texture)
    {
        return;
    }

    SDL_RenderClear(m_renderer);
    SDL_RenderTexture(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

bool SDLWindowManager::UpdateTextureFromRGBData(const uint8_t* rgbData, float width, float height)
{
    if (!m_texture || !rgbData || width <= 0 || height <= 0)
    {
        return false;
    }

    float textureWidth, textureHeight;
    SDL_GetTextureSize(m_texture, &textureWidth, &textureHeight);

    if (std::abs(width - textureWidth) > 1e-3 || std::abs(height - textureHeight) > 1e-3)
    {
        LOG_WARN("Texture size mismatch, recreating texture: " + std::to_string(width) + "x" + std::to_string(height));
        if (!CreateVideoTexture(width, height))
        {
            return false;
        }
    }

    int pitch = width * 3; // RGB24: 3 bytes per pixel
    return UpdateTexture(rgbData, pitch);
}

void SDLWindowManager::ProcessEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            emit WindowClosed();
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            emit WindowResized(event.window.data1, event.window.data2);
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            emit WindowClosed();
            break;
        case SDL_EVENT_KEY_DOWN:
            // 处理键盘事件
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            // 处理鼠标事件
            break;
        }
    }
}

bool SDLWindowManager::IsWindowValid() const
{
    return m_windowValid.load() && m_window && m_renderer;
}

bool SDLWindowManager::IsWindowVisible() const
{
    return m_windowVisible.load();
}

void SDLWindowManager::SetWindowSize(int width, int height)
{
    if (m_window)
    {
        SDL_SetWindowSize(m_window, width, height);
    }
}

void SDLWindowManager::CenterWindow()
{
    if (m_window)
    {
        SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

bool SDLWindowManager::RecreateTexture(int width, int height)
{
    if (!m_renderer)
    {
        LOG_ERROR("Cannot recreate texture: renderer not initialized");
        return false;
    }

    // 销毁旧纹理
    if (m_texture)
    {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    // 创建新纹理
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!m_texture)
    {
        QString error = QString("Failed to recreate SDL texture: %1").arg(SDL_GetError());
        LOG_ERROR(error.toStdString());
        emit ErrorOccurred(error);
        return false;
    }

    LOG_INFO("SDL texture recreated successfully: " + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

void SDLWindowManager::GetWindowSize(int& width, int& height)
{
    if (m_window)
    {
        SDL_GetWindowSize(m_window, &width, &height);
    }
    else
    {
        width = 0;
        height = 0;
    }
}