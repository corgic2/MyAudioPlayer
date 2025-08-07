#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <QObject>
#include <QString>
#include "SDLWindowManager.h"
#include "VideoAudioSync.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVFormatContext.h"
#include "BaseDataDefine/ST_AVFrame.h"
#include "BaseDataDefine/ST_AVPacket.h"
#include "DataDefine/ST_AVPlayState.h"
#include "DataDefine/ST_OpenFileResult.h"
#include "DataDefine/ST_SDL_Renderer.h"
#include "DataDefine/ST_SDL_Texture.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavutil/pixdesc.h>
#include <SDL3/SDL.h>
}

// 前向声明
class AudioFFmpegPlayer;


/// <summary>
/// 视频帧信息结构体
/// </summary>
struct ST_VideoFrameInfo
{
    /// <summary>
    /// 视频宽度
    /// </summary>
    float m_width{0.0};

    /// <summary>
    /// 视频高度
    /// </summary>
    float m_height{0.0};

    /// <summary>
    /// 帧率
    /// </summary>
    double m_frameRate{0.0};

    /// <summary>
    /// 总帧数
    /// </summary>
    int64_t m_totalFrames{0};

    /// <summary>
    /// 总时长（秒）
    /// </summary>
    double m_duration{0.0};

    /// <summary>
    /// 像素格式
    /// </summary>
    AVPixelFormat m_pixelFormat{AV_PIX_FMT_NONE};

    /// <summary>
    /// 文件路径
    /// </summary>
    std::string m_filePath;
};

/// <summary>
/// 视频播放工作线程
/// </summary>
class VideoPlayWorker : public QObject
{
    Q_OBJECT public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit VideoPlayWorker(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~VideoPlayWorker() override;

    /// <summary>
    /// 初始化播放器
    /// </summary>
    /// <param name="openFileResult">打开文件结果</param>
    /// <param name="renderer">SDL渲染器（可选，可为nullptr表示仅使用Qt显示）</param>
    /// <param name="texture">SDL纹理（可选，可为nullptr表示仅使用Qt显示）</param>
    /// <param name="parentWindowId">父窗口句柄（用于嵌入到Qt控件）</param>
    /// <returns>是否初始化成功</returns>
    bool InitPlayer(std::unique_ptr<ST_OpenFileResult> openFileResult, WId parentWindowId = 0, ST_SDL_Renderer* renderer = nullptr, ST_SDL_Texture* texture = nullptr);

    /// <summary>
    /// 清理资源
    /// </summary>
    void Cleanup();

    /// <summary>
    /// 获取视频信息
    /// </summary>
    /// <returns>视频帧信息</returns>
    ST_VideoFrameInfo GetVideoInfo();

    /// <summary>
    /// 设置音频播放器（用于音视频同步）
    /// </summary>
    /// <param name="audioPlayer">音频播放器实例</param>
    void SetAudioPlayer(AudioFFmpegPlayer* audioPlayer);

public slots:
    /// <summary>
    /// 开始播放
    /// </summary>
    void SlotStartPlay();

    /// <summary>
    /// 停止播放
    /// </summary>
    void SlotStopPlay();

    /// <summary>
    /// 暂停播放
    /// </summary>
    void SlotPausePlay();

    /// <summary>
    /// 恢复播放
    /// </summary>
    void SlotResumePlay();

    /// <summary>
    /// 跳转播放位置
    /// </summary>
    /// <param name="seconds">目标时间（秒）</param>
    void SlotSeekPlay(double seconds);

signals:
    /// <summary>
    /// 播放进度更新信号
    /// </summary>
    /// <param name="currentTime">当前时间(秒)</param>
    /// <param name="totalTime">总时间(秒)</param>
    void SigPlayProgressUpdated(double currentTime, double totalTime);

    /// <summary>
    /// 视频帧更新信号
    /// </summary>
    void SigFrameUpdated();

    /// <summary>
    /// 错误信号
    /// </summary>
    /// <param name="errorMsg">错误信息</param>
    void SigError(const QString& errorMsg);

    /// <summary>
    /// SDL窗口创建完成信号
    /// </summary>
    void SigSDLWindowCreated();

    /// <summary>
    /// SDL窗口关闭信号
    /// </summary>
    void SigSDLWindowClosed();
    /// <summary>
    /// 提交给主线程渲染帧信号
    /// </summary>
    /// <param name="rgbData"></param>
    /// <param name="pitch"></param>
    /// <param name="width"></param>
    /// <param name="height"></param>
    void SigRenderFrameOnMainThread(const uint8_t* rgbData, int pitch, float width, float height);
private:
    /// <summary>
    /// 播放循环
    /// </summary>
    void PlayLoop();
    /// <summary>
    /// SDL窗口管理器
    /// </summary>
    std::unique_ptr<SDLWindowManager> m_sdlManager;


    /// <summary>
    /// 解码视频帧
    /// </summary>
    /// <returns>是否成功解码到帧</returns>
    bool DecodeVideoFrame();

    /// <summary>
    /// 渲染视频帧
    /// </summary>
    /// <param name="frameInfo">视频帧信息</param>
    void RenderFrame(AVFrame* frame);

    // 音频相关方法已移除，音频播放由AudioFFmpegPlayer处理

    /// <summary>
    /// 计算帧时间延迟
    /// </summary>
    /// <param name="pts">帧的时间戳</param>
    /// <returns>延迟时间（毫秒）</returns>
    int CalculateFrameDelay(int64_t pts);

    /// <summary>
    /// 创建安全的图像转换上下文
    /// </summary>
    /// <param name="srcFormat">源像素格式</param>
    /// <param name="dstFormat">目标像素格式</param>
    /// <returns>转换上下文指针，失败返回nullptr</returns>
    SwsContext* CreateSafeSwsContext(AVPixelFormat srcFormat, AVPixelFormat dstFormat);

    /// <summary>
    /// 获取安全的像素格式
    /// </summary>
    /// <param name="format">原始格式</param>
    /// <returns>安全的像素格式</returns>
    AVPixelFormat GetSafePixelFormat(AVPixelFormat format);

private:
    /// <summary>
    /// 播放线程
    /// </summary>
    // 使用第三方线程池，无需QThread成员变量

    /// <summary>
    /// 格式上下文
    /// </summary>
    std::unique_ptr<ST_AVFormatContext> m_pFormatCtx = nullptr;

    /// <summary>
    /// 视频解码器上下文
    /// </summary>
    std::unique_ptr<ST_AVCodecContext> m_pVideoCodecCtx = nullptr;
    /// <summary>
    /// 视频流索引
    /// </summary>
    int m_videoStreamIndex = -1;

    /// <summary>
    /// 音频流索引（仅用于检测）
    /// </summary>
    int m_audioStreamIndex = -1;

    /// <summary>
    /// 数据包
    /// </summary>
    ST_AVPacket m_pPacket;

    /// <summary>
    /// 视频帧
    /// </summary>
    ST_AVFrame m_pVideoFrame;

    /// <summary>
    /// RGB帧
    /// </summary>
    ST_AVFrame m_pRGBFrame;

    /// <summary>
    /// 图像转换上下文
    /// </summary>
    SwsContext* m_pSwsCtx = nullptr;

    /// <summary>
    /// SDL窗口
    /// </summary>
    SDL_Window* m_pSDLWindow = nullptr;

    /// <summary>
    /// SDL渲染器
    /// </summary>
    SDL_Renderer* m_pSDLRenderer = nullptr;

    /// <summary>
    /// SDL纹理
    /// </summary>
    SDL_Texture* m_pSDLTexture = nullptr;


     
    /// <summary>
    /// 视频信息
    /// </summary>
    ST_VideoFrameInfo m_videoInfo;

    /// <summary>
    /// 播放开始时间
    /// </summary>
    int64_t m_startTime = 0;

    /// <summary>
    /// 暂停时间
    /// </summary>
    int64_t m_pauseStartTime = 0;

    /// <summary>
    /// 总暂停时间
    /// </summary>
    int64_t m_totalPauseTime = 0;

    /// <summary>
    /// 当前播放时间
    /// </summary>
    double m_currentTime = 0.0;

    /// <summary>
    /// 是否有音频流
    /// </summary>
    bool m_bHasAudio = false;

    /// <summary>
    /// RGB帧缓冲区
    /// </summary>
    std::vector<uint8_t> m_rgbBuffer;

    /// <summary>
    /// 是否请求seek操作
    /// </summary>
    std::atomic<bool> m_bSeekRequested = false;

    /// <summary>
    /// seek目标时间
    /// </summary>
    std::atomic<double> m_seekTarget = 0.0;

    /// <summary>
    /// 播放状态管理器
    /// </summary>
    ST_AVPlayState m_playState;
    /// <summary>
    /// 是否正在播放
    /// </summary>
    std::atomic<bool> m_bIsPlaying = false;

    /// <summary>
    /// 停止播放标志
    /// </summary>
    std::atomic<bool> m_bNeedStop = false;
    /// <summary>
    /// 音视频同步器
    /// </summary>
    std::unique_ptr<VideoAudioSync> m_videoAudioSync;

    /// <summary>
    /// 音频播放器（用于获取音频时钟）
    /// </summary>
    AudioFFmpegPlayer* m_audioPlayer = nullptr;

    /// <summary>
    /// 线程信息
    /// </summary>
    size_t m_threadId;
};
