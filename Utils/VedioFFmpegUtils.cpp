#include "VedioFFmpegUtils.h"
#include <QDebug>

VedioFFmpegUtils::VedioFFmpegUtils(QObject* parent)
    : QObject(parent), m_bSDLInitialized(false)
{
    InitSDL();
}

VedioFFmpegUtils::~VedioFFmpegUtils()
{
    QuitSDL();
}

bool VedioFFmpegUtils::InitSDL()
{
    if (m_bSDLInitialized)
    {
        return true;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        qDebug() << "SDL initialization failed:" << SDL_GetError();
        return false;
    }

    m_bSDLInitialized = true;
    return true;
}

void VedioFFmpegUtils::QuitSDL()
{
    if (m_bSDLInitialized)
    {
        SDL_Quit();
        m_bSDLInitialized = false;
    }
}

void VedioFFmpegUtils::ShowBMPImageFile(const QString& bmpFilePath, const QString& windowTitle)
{
    if (bmpFilePath.isEmpty())
    {
        qDebug() << "Invalid BMP file path";
        return;
    }

    if (!m_bSDLInitialized)
    {
        if (!InitSDL())
        {
            qDebug() << "Failed to initialize SDL";
            return;
        }
    }

    // 创建窗口
    if (!m_window.CreateWindow(windowTitle.toStdString(), 100, 100, 800, 600))
    {
        qDebug() << "Failed to create window:" << SDL_GetError();
        return;
    }

    // 创建渲染器
    if (!m_renderer.CreateRenderer(&m_window))
    {
        qDebug() << "Failed to create renderer:" << SDL_GetError();
        m_window.DestroyWindow();
        return;
    }

    // 加载BMP图片
    if (!m_surface.CreateFromImage(bmpFilePath.toStdString()))
    {
        qDebug() << "Failed to load BMP file:" << SDL_GetError();
        m_renderer.DestroyRenderer();
        m_window.DestroyWindow();
        return;
    }

    // 创建纹理
    if (!m_texture.CreateFromSurface(&m_renderer, &m_surface))
    {
        qDebug() << "Failed to create texture:" << SDL_GetError();
        m_surface.FreeSurface();
        m_renderer.DestroyRenderer();
        m_window.DestroyWindow();
        return;
    }

    // 设置渲染器颜色
    m_renderer.SetDrawColor(255, 255, 255, 255);
    m_renderer.Clear();

    // 获取图片尺寸
    int imageWidth = m_surface.GetWidth();
    int imageHeight = m_surface.GetHeight();

    // 计算目标矩形
    SDL_FRect dstRect;
    dstRect.x = (800.0f - static_cast<float>(imageWidth)) / 2.0f; // 居中显示
    dstRect.y = (600.0f - static_cast<float>(imageHeight)) / 2.0f;
    dstRect.w = static_cast<float>(imageWidth);
    dstRect.h = static_cast<float>(imageHeight);

    // 渲染纹理
    m_renderer.CopyTexture(m_texture.GetTexture(), nullptr, &dstRect);
    m_renderer.Present();

    // 等待用户关闭窗口
    SDL_Event event;
    bool quit = false;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                quit = true;
                break;
            }
        }
        SDL_Delay(10);
    }

    // 清理资源
    m_texture.DestroyTexture();
    m_surface.FreeSurface();
    m_renderer.DestroyRenderer();
    m_window.DestroyWindow();
}

