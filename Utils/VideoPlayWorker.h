#pragma once

#include <atomic>
#include <memory>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThread>
#include <QWaitCondition>
#include "AudioResampler.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVFormatContext.h"
#include "BaseDataDefine/ST_AVFrame.h"
#include "BaseDataDefine/ST_AVPacket.h"
#include "DataDefine/ST_AudioPlayInfo.h"
#include "DataDefine/ST_OpenFileResult.h"
#include "DataDefine/ST_ResampleParams.h"
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
    int m_width{0};

    /// <summary>
    /// 视频高度
    /// </summary>
    int m_height{0};

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
    ~VideoPlayWorker() override;

    /// <summary>
    /// 初始化播放器
    /// </summary>
    /// <param name="filePath">视频文件路径</param>
    /// <param name="renderer">SDL渲染器（可选，可为nullptr表示仅使用Qt显示）</param>
    /// <param name="texture">SDL纹理（可选，可为nullptr表示仅使用Qt显示）</param>
    /// <returns>是否初始化成功</returns>
    bool InitPlayer(const QString& filePath, ST_SDL_Renderer* renderer = nullptr, ST_SDL_Texture* texture = nullptr);

    /// <summary>
    /// 清理资源
    /// </summary>
    void Cleanup();

    /// <summary>
    /// 获取视频信息
    /// </summary>
    /// <returns>视频帧信息</returns>
    ST_VideoFrameInfo GetVideoInfo() const
    {
        return m_videoInfo;
    }

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
    /// 安全停止播放线程
    /// </summary>
    void SafeStopPlayThread();

    /// <summary>
    /// 初始化SDL系统
    /// </summary>
    /// <returns>是否初始化成功</returns>
    bool InitSDLSystem();

    /// <summary>
    /// 清理SDL系统
    /// </summary>
    void CleanupSDLSystem();

    /// <summary>
    /// 安全释放AVChannelLayout
    /// </summary>
    /// <param name="layout">要释放的布局指针</param>
    void SafeFreeChannelLayout(AVChannelLayout** layout);

    /// <summary>
    /// 解码视频帧
    /// </summary>
    /// <returns>是否成功解码到帧</returns>
    bool DecodeVideoFrame();

    /// <summary>
    /// 解码音频帧
    /// </summary>
    /// <returns>是否成功解码到帧</returns>
    bool DecodeAudioFrame();

    /// <summary>
    /// 渲染视频帧
    /// </summary>
    /// <param name="frame">视频帧</param>
    void RenderFrame(AVFrame* frame);

    /// <summary>
    /// 处理音频帧
    /// </summary>
    /// <param name="frame">音频帧</param>
    void ProcessAudioFrame(AVFrame* frame);

    /// <summary>
    /// 初始化音频系统
    /// </summary>
    /// <returns>是否初始化成功</returns>
    bool InitAudioSystem();

    /// <summary>
    /// 清理音频系统
    /// </summary>
    void CleanupAudioSystem();

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
    /// 格式上下文
    /// </summary>
    std::unique_ptr<ST_AVFormatContext> m_pFormatCtx{nullptr};

    /// <summary>
    /// 视频解码器上下文
    /// </summary>
    std::unique_ptr<ST_AVCodecContext> m_pVideoCodecCtx{nullptr};

    /// <summary>
    /// 音频解码器上下文
    /// </summary>
    std::unique_ptr<ST_AVCodecContext> m_pAudioCodecCtx{nullptr};

    /// <summary>
    /// 视频流索引
    /// </summary>
    int m_videoStreamIndex{-1};

    /// <summary>
    /// 音频流索引
    /// </summary>
    int m_audioStreamIndex{-1};

    /// <summary>
    /// 数据包
    /// </summary>
    ST_AVPacket m_pPacket;

    /// <summary>
    /// 视频帧
    /// </summary>
    ST_AVFrame m_pVideoFrame;

    /// <summary>
    /// 音频帧
    /// </summary>
    ST_AVFrame m_pAudioFrame;

    /// <summary>
    /// RGB帧
    /// </summary>
    ST_AVFrame m_pRGBFrame;

    /// <summary>
    /// 图像转换上下文
    /// </summary>
    SwsContext* m_pSwsCtx{nullptr};

    /// <summary>
    /// SDL渲染器
    /// </summary>
    ST_SDL_Renderer* m_pRenderer{nullptr};

    /// <summary>
    /// SDL纹理
    /// </summary>
    ST_SDL_Texture* m_pTexture{nullptr};

    /// <summary>
    /// 音频播放信息
    /// </summary>
    std::unique_ptr<ST_AudioPlayInfo> m_pAudioPlayInfo{nullptr};

    /// <summary>
    /// 音频重采样器
    /// </summary>
    AudioResampler m_audioResampler;

    /// <summary>
    /// 重采样参数
    /// </summary>
    ST_ResampleParams m_resampleParams;

    /// <summary>
    /// 视频信息
    /// </summary>
    ST_VideoFrameInfo m_videoInfo;

    /// <summary>
    /// 播放状态
    /// </summary>
    std::atomic<EM_VideoPlayState> m_playState{EM_VideoPlayState::Stopped};

    /// <summary>
    /// 是否需要停止
    /// </summary>
    std::atomic<bool> m_bNeedStop{false};

    /// <summary>
    /// 播放开始时间
    /// </summary>
    int64_t m_startTime{0};

    /// <summary>
    /// 暂停时间
    /// </summary>
    int64_t m_pauseStartTime{0};

    /// <summary>
    /// 总暂停时间
    /// </summary>
    int64_t m_totalPauseTime{0};

    /// <summary>
    /// 当前播放时间
    /// </summary>
    double m_currentTime{0.0};

    /// <summary>
    /// 跳转请求标志
    /// </summary>
    std::atomic<bool> m_bSeekRequested{false};

    /// <summary>
    /// 跳转目标时间
    /// </summary>
    std::atomic<double> m_seekTarget{0.0};

    /// <summary>
    /// 是否有音频流
    /// </summary>
    bool m_bHasAudio{false};

    /// <summary>
    /// 线程安全锁
    /// </summary>
    QMutex m_mutex;

    /// <summary>
    /// RGB帧缓冲区
    /// </summary>
    std::vector<uint8_t> m_rgbBuffer;

    /// <summary>
    /// 播放线程
    /// </summary>
    std::unique_ptr<QThread> m_pPlayThread{nullptr};

    /// <summary>
    /// 线程等待条件
    /// </summary>
    QWaitCondition m_threadWaitCondition;

    /// <summary>
    /// SDL初始化状态
    /// </summary>
    std::atomic<bool> m_bSDLInitialized{false};

    /// <summary>
    /// 音频设备ID
    /// </summary>
    SDL_AudioDeviceID m_audioDeviceId{0};

    /// <summary>
    /// 输入通道布局指针（用于内存管理）
    /// </summary>
    AVChannelLayout* m_pInputChannelLayout{nullptr};

    /// <summary>
    /// 输出通道布局指针（用于内存管理）
    /// </summary>
    AVChannelLayout* m_pOutputChannelLayout{nullptr};

    /// <summary>
    /// 帧处理计数器（用于性能监控）
    /// </summary>
    std::atomic<int64_t> m_frameCount{0};

    /// <summary>
    /// 最后一次处理时间
    /// </summary>
    std::atomic<int64_t> m_lastProcessTime{0};

    /// <summary>
    /// 音频重采样参数是否已更新标志
    /// </summary>
    std::atomic<bool> m_bAudioResampleParamsUpdated{false};
};
