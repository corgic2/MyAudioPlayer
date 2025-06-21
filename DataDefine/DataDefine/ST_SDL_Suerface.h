#pragma once
#include <string>
#include <SDL3/SDL.h>

/// <summary>
/// SDL表面封装类
/// </summary>
class ST_SDL_Surface
{
public:
    /// <summary>
    /// 构造函数
    /// </summary>
    ST_SDL_Surface();

    /// <summary>
    /// 析构函数
    /// </summary>
    ~ST_SDL_Surface();

    /// <summary>
    /// 从图片文件创建表面
    /// </summary>
    /// <param name="filePath">图片文件路径</param>
    /// <returns>是否创建成功</returns>
    bool CreateFromImage(const std::string& filePath);

    /// <summary>
    /// 创建RGB表面
    /// </summary>
    /// <param name="width">宽度</param>
    /// <param name="height">高度</param>
    /// <param name="depth">颜色深度</param>
    /// <returns>是否创建成功</returns>
    bool CreateRGBSurface(int width, int height, int depth);

    /// <summary>
    /// 销毁表面
    /// </summary>
    void FreeSurface();

    /// <summary>
    /// 获取表面指针
    /// </summary>
    /// <returns>SDL_Surface指针</returns>
    SDL_Surface* GetSurface() const
    {
        return m_pSurface;
    }

    /// <summary>
    /// 获取表面宽度
    /// </summary>
    /// <returns>表面宽度</returns>
    int GetWidth() const;

    /// <summary>
    /// 获取表面高度
    /// </summary>
    /// <returns>表面高度</returns>
    int GetHeight() const;

    /// <summary>
    /// 获取表面像素格式
    /// </summary>
    /// <returns>像素格式</returns>
    uint32_t GetFormat() const;

private:
    /// <summary>
    /// SDL表面指针
    /// </summary>
    SDL_Surface* m_pSurface;
};
