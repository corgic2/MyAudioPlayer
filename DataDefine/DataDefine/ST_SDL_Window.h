#pragma once
#include <string>
#include <SDL3/SDL.h>

/// <summary>
/// SDL窗口封装类
/// </summary>
class ST_SDL_Window
{
public:
    /// <summary>
    /// 构造函数
    /// </summary>
    ST_SDL_Window();

    /// <summary>
    /// 析构函数
    /// </summary>
    ~ST_SDL_Window();

    /// <summary>
    /// 创建窗口
    /// </summary>
    /// <param name="title">窗口标题</param>
    /// <param name="x">窗口x坐标</param>
    /// <param name="y">窗口y坐标</param>
    /// <param name="w">窗口宽度</param>
    /// <param name="h">窗口高度</param>
    /// <param name="flags">窗口标志</param>
    /// <returns>是否创建成功</returns>
    bool CreateWindow(const std::string& title, int x, int y, int w, int h, uint32_t flags = SDL_WINDOW_ALWAYS_ON_TOP);

    /// <summary>
    /// 销毁窗口
    /// </summary>
    void DestroyWindow();

    /// <summary>
    /// 获取窗口指针
    /// </summary>
    /// <returns>SDL_Window指针</returns>
    SDL_Window* GetWindow() const
    {
        return m_pWindow;
    }

    /// <summary>
    /// 获取窗口ID
    /// </summary>
    /// <returns>窗口ID</returns>
    uint32_t GetWindowID() const;

    /// <summary>
    /// 设置窗口标题
    /// </summary>
    /// <param name="title">窗口标题</param>
    void SetTitle(const std::string& title);

    /// <summary>
    /// 设置窗口大小
    /// </summary>
    /// <param name="w">宽度</param>
    /// <param name="h">高度</param>
    void SetSize(int w, int h);

    /// <summary>
    /// 设置窗口位置
    /// </summary>
    /// <param name="x">x坐标</param>
    /// <param name="y">y坐标</param>
    void SetPosition(int x, int y);

    /// <summary>
    /// 显示窗口
    /// </summary>
    void Show();

    /// <summary>
    /// 隐藏窗口
    /// </summary>
    void Hide();

    /// <summary>
    /// 设置窗口全屏模式
    /// </summary>
    /// <param name="fullscreen">是否全屏</param>
    /// <returns>是否设置成功</returns>
    bool SetFullscreen(bool fullscreen);

    /// <summary>
    /// 设置窗口大小是否可调整
    /// </summary>
    /// <param name="resizable">是否可调整</param>
    void SetResizable(bool resizable);

    /// <summary>
    /// 获取窗口大小
    /// </summary>
    /// <param name="w">宽度</param>
    /// <param name="h">高度</param>
    void GetSize(int& w, int& h) const;

    /// <summary>
    /// 获取窗口位置
    /// </summary>
    /// <param name="x">x坐标</param>
    /// <param name="y">y坐标</param>
    void GetPosition(int& x, int& y) const;

    /// <summary>
    /// 设置窗口最小大小
    /// </summary>
    /// <param name="minW">最小宽度</param>
    /// <param name="minH">最小高度</param>
    void SetMinimumSize(int minW, int minH);

    /// <summary>
    /// 设置窗口最大大小
    /// </summary>
    /// <param name="maxW">最大宽度</param>
    /// <param name="maxH">最大高度</param>
    void SetMaximumSize(int maxW, int maxH);

private:
    /// <summary>
    /// SDL窗口指针
    /// </summary>
    SDL_Window* m_pWindow;
};
