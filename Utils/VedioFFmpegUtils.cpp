#include "VedioFFmpegUtils.h"
#include <QDebug>
#include <QThread>
#include "LogSystem/LogSystem.h"

VedioFFmpegUtils::VedioFFmpegUtils(QObject* parent)
    : QObject(parent), m_bSDLInitialized(false), m_playState(EM_VideoPlayState::Stopped), m_recordState(EM_VideoRecordState::Stopped), m_pPlayThread(nullptr), m_pRecordThread(nullptr)
{
    InitSDL();
}

VedioFFmpegUtils::~VedioFFmpegUtils()
{
    StopVideo();
    StopRecord();
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
        LOG_WARN("Invalid BMP file path");
        return;
    }

    if (!m_bSDLInitialized)
    {
        if (!InitSDL())
        {
            LOG_WARN("Failed to initialize SDL");
            return;
        }
    }

    // 创建窗口
    if (!m_window.CreateWindow(windowTitle.toStdString(), 100, 100, 800, 600))
    {
        LOG_WARN("Failed to create window:", SDL_GetError());
        return;
    }

    // 创建渲染器
    if (!m_renderer.CreateRenderer(&m_window))
    {
        LOG_WARN("Failed to create renderer:", SDL_GetError());
        m_window.DestroyWindow();
        return;
    }

    // 加载BMP图片
    if (!m_surface.CreateFromImage(bmpFilePath.toStdString()))
    {
        LOG_WARN("Failed to load BMP file:", SDL_GetError());
        m_renderer.DestroyRenderer();
        m_window.DestroyWindow();
        return;
    }

    // 创建纹理
    if (!m_texture.CreateFromSurface(&m_renderer, &m_surface))
    {
        LOG_WARN("Failed to create texture:", SDL_GetError());
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

bool VedioFFmpegUtils::StartPlayVideo(const QString& videoPath, const QString& windowTitle)
{
    if (videoPath.isEmpty())
    {
        LOG_WARN("Invalid video file path");
        return false;
    }

    if (m_playState != EM_VideoPlayState::Stopped)
    {
        StopVideo();
    }

    if (!m_bSDLInitialized)
    {
        if (!InitSDL())
        {
            LOG_WARN("Failed to initialize SDL");
            return false;
        }
    }

    // 创建窗口
    if (!m_window.CreateWindow(windowTitle.toStdString(), 100, 100, 1280, 720))
    {
        LOG_WARN("Failed to create window:", SDL_GetError());
        return false;
    }

    // 创建渲染器
    if (!m_renderer.CreateRenderer(&m_window))
    {
        LOG_WARN("Failed to create renderer:", SDL_GetError());
        m_window.DestroyWindow();
        return false;
    }

    // 创建播放线程
    m_pPlayThread = new QThread();
    // TODO: 在线程中实现视频解码和播放逻辑

    m_playState = EM_VideoPlayState::Playing;
    emit SigPlayStateChanged(m_playState);
    return true;
}

void VedioFFmpegUtils::PauseVideo()
{
    if (m_playState == EM_VideoPlayState::Playing)
    {
        m_playState = EM_VideoPlayState::Paused;
        emit SigPlayStateChanged(m_playState);
    }
}

void VedioFFmpegUtils::ResumeVideo()
{
    if (m_playState == EM_VideoPlayState::Paused)
    {
        m_playState = EM_VideoPlayState::Playing;
        emit SigPlayStateChanged(m_playState);
    }
}

void VedioFFmpegUtils::StopVideo()
{
    if (m_playState != EM_VideoPlayState::Stopped)
    {
        // 停止播放线程
        if (m_pPlayThread)
        {
            m_pPlayThread->quit();
            m_pPlayThread->wait();
            delete m_pPlayThread;
            m_pPlayThread = nullptr;
        }

        // 清理资源
        m_texture.DestroyTexture();
        m_surface.FreeSurface();
        m_renderer.DestroyRenderer();
        m_window.DestroyWindow();

        m_playState = EM_VideoPlayState::Stopped;
        emit SigPlayStateChanged(m_playState);
    }
}

bool VedioFFmpegUtils::StartRecordVideo(const QString& outputPath, int width, int height, int frameRate)
{
    if (outputPath.isEmpty())
    {
        LOG_WARN("Invalid output file path");
        return false;
    }

    if (m_recordState != EM_VideoRecordState::Stopped)
    {
        StopRecord();
    }

    // 创建录制线程
    m_pRecordThread = new QThread();
    // TODO: 在线程中实现视频编码和录制逻辑

    m_recordState = EM_VideoRecordState::Recording;
    emit SigRecordStateChanged(m_recordState);
    return true;
}

void VedioFFmpegUtils::PauseRecord()
{
    if (m_recordState == EM_VideoRecordState::Recording)
    {
        m_recordState = EM_VideoRecordState::Paused;
        emit SigRecordStateChanged(m_recordState);
    }
}

void VedioFFmpegUtils::ResumeRecord()
{
    if (m_recordState == EM_VideoRecordState::Paused)
    {
        m_recordState = EM_VideoRecordState::Recording;
        emit SigRecordStateChanged(m_recordState);
    }
}

void VedioFFmpegUtils::StopRecord()
{
    if (m_recordState != EM_VideoRecordState::Stopped)
    {
        // 停止录制线程
        if (m_pRecordThread)
        {
            m_pRecordThread->quit();
            m_pRecordThread->wait();
            delete m_pRecordThread;
            m_pRecordThread = nullptr;
        }

        m_recordState = EM_VideoRecordState::Stopped;
        emit SigRecordStateChanged(m_recordState);
    }
}

