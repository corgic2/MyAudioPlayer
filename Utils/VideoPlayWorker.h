#pragma once

#include <QObject>
#include <QString>
#include <QMutex>
#include <atomic>
#include <memory>
#include "DataDefine/ST_SDL_Renderer.h"
#include "DataDefine/ST_SDL_Texture.h"
#include "BaseDataDefine/ST_AVFormatContext.h"
#include "BaseDataDefine/ST_AVCodecContext.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}
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
/// 视频帧信息结构体
/// </summary>
struct ST_VideoFrameInfo
{
    /// <summary>
    /// 视频宽度
    /// </summary>
    int m_width;

    /// <summary>
    /// 视频高度
    /// </summary>
    int m_height;

    /// <summary>
    /// 帧率
    /// </summary>
    double m_frameRate;

    /// <summary>
    /// 总帧数
    /// </summary>
    int64_t m_totalFrames;

    /// <summary>
    /// 总时长（秒）
    /// </summary>
    double m_duration;

    /// <summary>
    /// 像素格式
    /// </summary>
    AVPixelFormat m_pixelFormat;

    /// <summary>
    /// 构造函数
    /// </summary>
    ST_VideoFrameInfo()
        : m_width(0), m_height(0), m_frameRate(0.0), m_totalFrames(0), 
          m_duration(0.0), m_pixelFormat(AV_PIX_FMT_NONE)
    {
    }
};

/// <summary>
/// 视频播放工作线程
/// </summary>
class VideoPlayWorker : public QObject
{
    Q_OBJECT

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit VideoPlayWorker(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~VideoPlayWorker();

    /// <summary>
    /// 初始化播放器
    /// </summary>
    /// <param name="filePath">视频文件路径</param>
    /// <param name="renderer">SDL渲染器</param>
    /// <param name="texture">SDL纹理</param>
    /// <returns>是否初始化成功</returns>
    bool InitPlayer(const QString& filePath, ST_SDL_Renderer* renderer, ST_SDL_Texture* texture);

    /// <summary>
    /// 清理资源
    /// </summary>
    void Cleanup();

    /// <summary>
    /// 获取视频信息
    /// </summary>
    /// <returns>视频帧信息</returns>
    ST_VideoFrameInfo GetVideoInfo() const { return m_videoInfo; }

public slots:
    /// <summary>
    /// 开始播放
    /// </summary>
    void SlotStartPlay();

    /// <summary>
    /// 暂停播放
    /// </summary>
    void SlotPausePlay();

    /// <summary>
    /// 恢复播放
    /// </summary>
    void SlotResumePlay();

    /// <summary>
    /// 停止播放
    /// </summary>
    void SlotStopPlay();

    /// <summary>
    /// 跳转播放位置
    /// </summary>
    /// <param name="seconds">目标时间（秒）</param>
    void SlotSeekPlay(double seconds);

signals:
    /// <summary>
    /// 播放状态改变信号
    /// </summary>
    /// <param name="state">播放状态</param>
    void SigPlayStateChanged(EM_VideoPlayState state);

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
    /// 视频帧数据更新信号（用于Qt显示）
    /// </summary>
    /// <param name="frameData">帧数据</param>
    /// <param name="width">帧宽度</param>
    /// <param name="height">帧高度</param>
    void SigFrameDataUpdated(const uint8_t* frameData, int width, int height);

private:
    /// <summary>
    /// 播放循环
    /// </summary>
    void PlayLoop();

    /// <summary>
    /// 解码视频帧
    /// </summary>
    /// <returns>是否成功解码到帧</returns>
    bool DecodeVideoFrame();

    /// <summary>
    /// 渲染视频帧
    /// </summary>
    /// <param name="frame">视频帧</param>
    void RenderFrame(AVFrame* frame);

    /// <summary>
    /// 计算帧时间延迟
    /// </summary>
    /// <param name="pts">帧的时间戳</param>
    /// <returns>延迟时间（毫秒）</returns>
    int CalculateFrameDelay(int64_t pts);

private:
    /// <summary>
    /// 格式上下文
    /// </summary>
    std::unique_ptr<ST_AVFormatContext> m_pFormatCtx;

    /// <summary>
    /// 解码器上下文
    /// </summary>
    std::unique_ptr<ST_AVCodecContext> m_pCodecCtx;

    /// <summary>
    /// 视频流索引
    /// </summary>
    int m_videoStreamIndex;

    /// <summary>
    /// 数据包
    /// </summary>
    AVPacket* m_pPacket;

    /// <summary>
    /// 视频帧
    /// </summary>
    AVFrame* m_pVideoFrame;

    /// <summary>
    /// RGB帧
    /// </summary>
    AVFrame* m_pRGBFrame;

    /// <summary>
    /// 图像转换上下文
    /// </summary>
    SwsContext* m_pSwsCtx;

    /// <summary>
    /// SDL渲染器
    /// </summary>
    ST_SDL_Renderer* m_pRenderer;

    /// <summary>
    /// SDL纹理
    /// </summary>
    ST_SDL_Texture* m_pTexture;

    /// <summary>
    /// 视频信息
    /// </summary>
    ST_VideoFrameInfo m_videoInfo;

    /// <summary>
    /// 播放状态
    /// </summary>
    std::atomic<EM_VideoPlayState> m_playState;

    /// <summary>
    /// 是否需要停止
    /// </summary>
    std::atomic<bool> m_bNeedStop;

    /// <summary>
    /// 播放开始时间
    /// </summary>
    int64_t m_startTime;

    /// <summary>
    /// 暂停时间
    /// </summary>
    int64_t m_pauseStartTime;

    /// <summary>
    /// 总暂停时间
    /// </summary>
    int64_t m_totalPauseTime;

    /// <summary>
    /// 当前播放时间
    /// </summary>
    double m_currentTime;

    /// <summary>
    /// 跳转请求标志
    /// </summary>
    std::atomic<bool> m_bSeekRequested;

    /// <summary>
    /// 跳转目标时间
    /// </summary>
    std::atomic<double> m_seekTarget;

    /// <summary>
    /// 线程安全锁
    /// </summary>
    QMutex m_mutex;

    /// <summary>
    /// RGB帧缓冲区
    /// </summary>
    std::vector<uint8_t> m_rgbBuffer;
};

