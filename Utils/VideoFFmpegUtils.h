#pragma once

#include <QObject>
#include <QString>
#include <SDL3/SDL.h>
#include "BaseFFmpegUtils.h"
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
class VideoFFmpegUtils : public BaseFFmpegUtils
{
    Q_OBJECT

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit VideoFFmpegUtils(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~VideoFFmpegUtils();

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
    void StartPlay(const QString& videoPath,double startPosition = 0.0, const QStringList &args = QStringList()) override;

    /// <summary>
    /// 暂停视频播放
    /// </summary>
    void PausePlay() override;

    /// <summary>
    /// 恢复视频播放
    /// </summary>
    void ResumePlay() override;

    /// <summary>
    /// 停止视频播放
    /// </summary>
    void StopPlay() override;

    /// <summary>
    /// 开始录制视频
    /// </summary>
    /// <param name="outputPath">输出文件路径</param>
    /// <param name="width">视频宽度</param>
    /// <param name="height">视频高度</param>
    /// <param name="frameRate">帧率</param>
    /// <returns>是否成功开始录制</returns>
    void StartRecording(const QString& outputPath) override;

    /// <summary>
    /// 暂停视频录制
    /// </summary>
    void StopRecording() override;
    /// <summary>
    /// 移动播放位置
    /// </summary>
    /// <param name="seconds"></param>
    void SeekPlay(double seconds) override;
    /// <summary>
    /// 获取当前播放状态
    /// </summary>
    /// <returns>true表示正在播放，false表示已停止</returns>
    bool IsPlaying() override;
    /// <summary>
    /// 获取当前暂停状态
    /// </summary>
    /// <returns>true表示已暂停，false表示未暂停</returns>
    bool IsPaused() override;
    /// <summary>
    /// 获取当前录制状态
    /// </summary>
    /// <returns>true表示正在录制，false表示已停止录制</returns>
    bool IsRecording() override;
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

