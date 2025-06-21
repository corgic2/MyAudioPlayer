#pragma once

#include <QObject>
#include <QString>
#include <SDL3/SDL.h>
#include "DataDefine/ST_SDL_Renderer.h"
#include "DataDefine/ST_SDL_Suerface.h"
#include "DataDefine/ST_SDL_Texture.h"
#include "DataDefine/ST_SDL_Window.h"

/// <summary>
/// 视频FFmpeg工具类
/// </summary>
class VedioFFmpegUtils : public QObject
{
    Q_OBJECT

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit VedioFFmpegUtils(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~VedioFFmpegUtils();

    /// <summary>
    /// 显示BMP图片文件
    /// </summary>
    /// <param name="bmpFilePath">BMP文件路径</param>
    /// <param name="windowTitle">窗口标题</param>
    void ShowBMPImageFile(const QString& bmpFilePath, const QString& windowTitle = "BMP Image Viewer");

    /// <summary>
    /// 初始化SDL
    /// </summary>
    /// <returns>是否初始化成功</returns>
    bool InitSDL();

    /// <summary>
    /// 退出SDL
    /// </summary>
    void QuitSDL();

private:
    /// <summary>
    /// SDL窗口对象
    /// </summary>
    ST_SDL_Window m_window;

    /// <summary>
    /// SDL渲染器对象
    /// </summary>
    ST_SDL_Renderer m_renderer;

    /// <summary>
    /// SDL表面对象
    /// </summary>
    ST_SDL_Surface m_surface;

    /// <summary>
    /// SDL纹理对象
    /// </summary>
    ST_SDL_Texture m_texture;

    /// <summary>
    /// SDL是否已初始化
    /// </summary>
    bool m_bSDLInitialized;
};

