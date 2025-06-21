#pragma once
#include <SDL3/SDL.h>
#include "ST_SDL_Window.h"

/// <summary>
/// SDL渲染器封装类
/// </summary>
class ST_SDL_Renderer
{
public:
    /// <summary>
    /// 构造函数
    /// </summary>
    ST_SDL_Renderer();

    /// <summary>
    /// 析构函数
    /// </summary>
    ~ST_SDL_Renderer();

    /// <summary>
    /// 创建渲染器
    /// </summary>
    /// <param name="window">SDL窗口对象</param>
    /// <param name="driver">渲染驱动索引</param>
    /// <param name="flags">渲染器标志</param>
    /// <returns>是否创建成功</returns>
    bool CreateRenderer(ST_SDL_Window* window, int driver = -1, uint32_t flags = 0);

    /// <summary>
    /// 销毁渲染器
    /// </summary>
    void DestroyRenderer();

    /// <summary>
    /// 获取渲染器指针
    /// </summary>
    /// <returns>SDL_Renderer指针</returns>
    SDL_Renderer* GetRenderer() const { return m_pRenderer; }

    /// <summary>
    /// 清除渲染器
    /// </summary>
    void Clear();

    /// <summary>
    /// 设置渲染器绘制颜色
    /// </summary>
    /// <param name="r">红色分量</param>
    /// <param name="g">绿色分量</param>
    /// <param name="b">蓝色分量</param>
    /// <param name="a">透明度</param>
    void SetDrawColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    /// <summary>
    /// 渲染器呈现
    /// </summary>
    void Present();

    /// <summary>
    /// 复制纹理到渲染器
    /// </summary>
    /// <param name="texture">纹理指针</param>
    /// <param name="srcRect">源矩形区域</param>
    /// <param name="dstRect">目标矩形区域</param>
    /// <returns>是否复制成功</returns>
    bool CopyTexture(SDL_Texture* texture, const SDL_FRect* srcRect = nullptr, const SDL_FRect* dstRect = nullptr);

    /// <summary>
    /// 设置渲染器视口
    /// </summary>
    /// <param name="rect">视口矩形</param>
    /// <returns>是否设置成功</returns>
    bool SetViewport(const SDL_FRect* rect);

    /// <summary>
    /// 设置渲染器缩放
    /// </summary>
    /// <param name="scaleX">X轴缩放</param>
    /// <param name="scaleY">Y轴缩放</param>
    /// <returns>是否设置成功</returns>
    bool SetScale(float scaleX, float scaleY);

    /// <summary>
    /// 设置混合模式
    /// </summary>
    /// <param name="blendMode">混合模式</param>
    /// <returns>是否设置成功</returns>
    bool SetBlendMode(SDL_BlendMode blendMode);

    /// <summary>
    /// 绘制线条
    /// </summary>
    /// <param name="x1">起点X</param>
    /// <param name="y1">起点Y</param>
    /// <param name="x2">终点X</param>
    /// <param name="y2">终点Y</param>
    void DrawLine(float x1, float y1, float x2, float y2);

    /// <summary>
    /// 绘制矩形
    /// </summary>
    /// <param name="rect">矩形区域</param>
    /// <param name="filled">是否填充</param>
    void DrawRect(const SDL_FRect* rect, bool filled = false);

private:
    /// <summary>
    /// SDL渲染器指针
    /// </summary>
    SDL_Renderer* m_pRenderer;
};
