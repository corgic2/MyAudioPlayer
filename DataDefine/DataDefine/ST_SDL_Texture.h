#pragma once
#include <SDL3/SDL.h>
#include "ST_SDL_Renderer.h"
#include "ST_SDL_Suerface.h"

/// <summary>
/// SDL纹理封装类
/// </summary>
class ST_SDL_Texture
{
public:
    /// <summary>
    /// 纹理访问方式枚举
    /// </summary>
    enum EM_TextureAccess
    {
        Static = SDL_TEXTUREACCESS_STATIC,
        // 静态纹理
        Streaming = SDL_TEXTUREACCESS_STREAMING,
        // 流式纹理
        Target = SDL_TEXTUREACCESS_TARGET // 目标纹理
    };

    /// <summary>
    /// 构造函数
    /// </summary>
    ST_SDL_Texture();

    /// <summary>
    /// 析构函数
    /// </summary>
    ~ST_SDL_Texture();

    /// <summary>
    /// 从表面创建纹理
    /// </summary>
    /// <param name="renderer">渲染器对象</param>
    /// <param name="surface">表面对象</param>
    /// <returns>是否创建成功</returns>
    bool CreateFromSurface(ST_SDL_Renderer* renderer, ST_SDL_Surface* surface);

    /// <summary>
    /// 创建纹理
    /// </summary>
    /// <param name="renderer">渲染器对象</param>
    /// <param name="format">像素格式</param>
    /// <param name="access">访问方式</param>
    /// <param name="w">宽度</param>
    /// <param name="h">高度</param>
    /// <returns>是否创建成功</returns>
    bool CreateTexture(ST_SDL_Renderer* renderer, uint32_t format, SDL_TextureAccess access, int w, int h);

    /// <summary>
    /// 销毁纹理
    /// </summary>
    void DestroyTexture();

    /// <summary>
    /// 获取纹理指针
    /// </summary>
    /// <returns>SDL_Texture指针</returns>
    SDL_Texture* GetTexture() const
    {
        return m_pTexture;
    }

    /// <summary>
    /// 更新纹理
    /// </summary>
    /// <param name="rect">更新区域</param>
    /// <param name="pixels">像素数据</param>
    /// <param name="pitch">行间距</param>
    /// <returns>是否更新成功</returns>
    bool UpdateTexture(const SDL_Rect* rect, const void* pixels, int pitch);

    /// <summary>
    /// 锁定纹理
    /// </summary>
    /// <param name="rect">锁定区域</param>
    /// <param name="pixels">像素数据指针</param>
    /// <param name="pitch">行间距指针</param>
    /// <returns>是否锁定成功</returns>
    bool LockTexture(const SDL_Rect* rect, void** pixels, int* pitch);

    /// <summary>
    /// 解锁纹理
    /// </summary>
    void UnlockTexture();

    /// <summary>
    /// 获取纹理宽度
    /// </summary>
    /// <returns>纹理宽度</returns>
    float GetWidth() const;

    /// <summary>
    /// 获取纹理高度
    /// </summary>
    /// <returns>纹理高度</returns>
    float GetHeight() const;

private:
    /// <summary>
    /// SDL纹理指针
    /// </summary>
    SDL_Texture* m_pTexture;
};
