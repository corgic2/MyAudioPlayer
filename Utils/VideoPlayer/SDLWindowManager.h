#pragma once

#include <QObject>
#include <QString>
#include <atomic>
#include <memory>
#include <qwindowdefs.h>
#include "DataDefine/ST_SDL_Renderer.h"
#include "DataDefine/ST_SDL_Texture.h"

extern "C" {
#include <SDL3/SDL.h>
}

/// <summary>
/// SDL3窗口管理器
/// 负责SDL窗口的创建、销毁和事件处理
/// </summary>
class SDLWindowManager : public QObject
{
    Q_OBJECT

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit SDLWindowManager(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~SDLWindowManager() override;
    static void RegisterDevice();
    /// <summary>
    /// 创建SDL窗口
    /// </summary>
    /// <param name="width">窗口宽度</param>
    /// <param name="height">窗口高度</param>
    /// <param name="title">窗口标题</param>
    /// <returns>是否创建成功</returns>
    bool CreateWindow(int width, int height, const QString& title = "SDL Window");

    /// <summary>
    /// 创建嵌入Qt控件的SDL窗口
    /// </summary>
    /// <param name="width">窗口宽度</param>
    /// <param name="height">窗口高度</param>
    /// <param name="parentWindowId">父窗口句柄（Qt控件）</param>
    /// <returns>是否创建成功</returns>
    bool CreateEmbeddedWindow(int width, int height, WId parentWindowId);

    /// <summary>
    /// 销毁SDL窗口和渲染器
    /// </summary>
    void DestroyWindow();

    /// <summary>
    /// 显示/隐藏窗口
    /// </summary>
    /// <param name="show">是否显示</param>
    void ShowWindow(bool show);

    /// <summary>
    /// 设置窗口标题
    /// </summary>
    /// <param name="title">窗口标题</param>
    void SetWindowTitle(const QString& title);

    /// <summary>
    /// 获取SDL窗口指针
    /// </summary>
    /// <returns>SDL窗口指针</returns>
    SDL_Window* GetSDLWindow() const { return m_window; }

    /// <summary>
    /// 获取SDL渲染器指针
    /// </summary>
    /// <returns>SDL渲染器指针</returns>
    SDL_Renderer* GetSDLRenderer() const { return m_renderer; }

    /// <summary>
    /// 获取SDL纹理指针
    /// </summary>
    /// <returns>SDL纹理指针</returns>
    SDL_Texture* GetSDLTexture() const { return m_texture; }

    /// <summary>
    /// 创建视频纹理
    /// </summary>
    /// <param name="width">纹理宽度</param>
    /// <param name="height">纹理高度</param>
    /// <returns>是否创建成功</returns>
    bool CreateVideoTexture(float width, float height);

    /// <summary>
    /// 更新纹理数据
    /// </summary>
    /// <param name="data">图像数据</param>
    /// <param name="pitch">行间距</param>
    /// <returns>是否更新成功</returns>
    bool UpdateTexture(const void* data, int pitch);

    /// <summary>
    /// 从RGB数据更新纹理
    /// </summary>
    /// <param name="rgbData">RGB24格式图像数据</param>
    /// <param name="width">图像宽度</param>
    /// <param name="height">图像高度</param>
    /// <returns>是否更新成功</returns>
    bool UpdateTextureFromRGBData(const uint8_t* rgbData, int pitch, float width, float height);
    /// <summary>
    /// 渲染当前帧
    /// </summary>
    void RenderFrame();

    /// <summary>
    /// 处理SDL事件
    /// </summary>
    void ProcessEvents();

    /// <summary>
    /// 检查窗口是否有效
    /// </summary>
    /// <returns>窗口是否有效</returns>
    bool IsWindowValid() const;

    /// <summary>
    /// 检查窗口是否可见
    /// </summary>
    /// <returns>窗口是否可见</returns>
    bool IsWindowVisible() const;

    /// <summary>
    /// 设置窗口尺寸
    /// </summary>
    /// <param name="width">宽度</param>
    /// <param name="height">高度</param>
    void SetWindowSize(int width, int height);

    /// <summary>
    /// 窗口居中
    /// </summary>
    void CenterWindow();
    /// <summary>
    /// 获取窗口尺寸
    /// </summary>
    /// <param name="width">宽度输出</param>
    /// <param name="height">高度输出</param>
    void GetWindowSize(int& width, int& height);

signals:
    /// <summary>
    /// 窗口关闭信号
    /// </summary>
    void WindowClosed();

    /// <summary>
    /// 窗口大小改变信号
    /// </summary>
    /// <param name="width">新宽度</param>
    /// <param name="height">新高度</param>
    void WindowResized(int width, int height);

    /// <summary>
    /// 错误信号
    /// </summary>
    /// <param name="error">错误信息</param>
    void ErrorOccurred(const QString& error);

private:
    SDL_Window* m_window{nullptr};           /// SDL窗口
    SDL_Renderer* m_renderer{nullptr};       /// SDL渲染器
    SDL_Texture* m_texture{nullptr};       /// SDL纹理
    std::atomic<bool> m_windowVisible{false}; /// 窗口是否可见
    std::atomic<bool> m_windowValid{false};   /// 窗口是否有效
};