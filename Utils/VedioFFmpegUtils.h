#pragma once

#include <QObject>
#include <QString>
#include <SDL3/SDL.h>
#include "DataDefine/ST_SDL_Renderer.h"
#include "DataDefine/ST_SDL_Suerface.h"
#include "DataDefine/ST_SDL_Texture.h"
#include "DataDefine/ST_SDL_Window.h"

/// <summary>
/// 视频播放状态枚举
/// </summary>
enum class EM_VideoPlayState
{
    /// <summary>
    /// 停止
    /// </summary>
    Stopped,

    /// <summary>
    /// 播放中
    /// </summary>
    Playing,

    /// <summary>
    /// 暂停
    /// </summary>
    Paused
};

/// <summary>
/// 视频录制状态枚举
/// </summary>
enum class EM_VideoRecordState
{
    /// <summary>
    /// 停止
    /// </summary>
    Stopped,

    /// <summary>
    /// 录制中
    /// </summary>
    Recording,

    /// <summary>
    /// 暂停
    /// </summary>
    Paused
};

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

    /// <summary>
    /// 开始播放视频
    /// </summary>
    /// <param name="videoPath">视频文件路径</param>
    /// <param name="windowTitle">窗口标题</param>
    /// <returns>是否成功开始播放</returns>
    bool StartPlayVideo(const QString& videoPath, const QString& windowTitle = "Video Player");

    /// <summary>
    /// 暂停视频播放
    /// </summary>
    void PauseVideo();

    /// <summary>
    /// 恢复视频播放
    /// </summary>
    void ResumeVideo();

    /// <summary>
    /// 停止视频播放
    /// </summary>
    void StopVideo();

    /// <summary>
    /// 开始录制视频
    /// </summary>
    /// <param name="outputPath">输出文件路径</param>
    /// <param name="width">视频宽度</param>
    /// <param name="height">视频高度</param>
    /// <param name="frameRate">帧率</param>
    /// <returns>是否成功开始录制</returns>
    bool StartRecordVideo(const QString& outputPath, int width = 1920, int height = 1080, int frameRate = 30);

    /// <summary>
    /// 暂停视频录制
    /// </summary>
    void PauseRecord();

    /// <summary>
    /// 恢复视频录制
    /// </summary>
    void ResumeRecord();

    /// <summary>
    /// 停止视频录制
    /// </summary>
    void StopRecord();

signals:
    /// <summary>
    /// 视频播放状态改变信号
    /// </summary>
    /// <param name="state">播放状态</param>
    void SigPlayStateChanged(EM_VideoPlayState state);

    /// <summary>
    /// 视频录制状态改变信号
    /// </summary>
    /// <param name="state">录制状态</param>
    void SigRecordStateChanged(EM_VideoRecordState state);

    /// <summary>
    /// 播放进度更新信号
    /// </summary>
    /// <param name="currentTime">当前时间(秒)</param>
    /// <param name="totalTime">总时间(秒)</param>
    void SigPlayProgressUpdated(double currentTime, double totalTime);

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

    /// <summary>
    /// 视频播放状态
    /// </summary>
    EM_VideoPlayState m_playState;

    /// <summary>
    /// 视频录制状态
    /// </summary>
    EM_VideoRecordState m_recordState;

    /// <summary>
    /// 视频播放线程
    /// </summary>
    QThread* m_pPlayThread;

    /// <summary>
    /// 视频录制线程
    /// </summary>
    QThread* m_pRecordThread;
};

