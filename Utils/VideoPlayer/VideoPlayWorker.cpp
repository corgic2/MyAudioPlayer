#include "VideoPlayWorker.h"
#include <QMutexLocker>
#include <QThread>
#include "../BasePlayer/FFmpegPublicUtils.h"
#include "BaseDataDefine/ST_AVCodec.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVFormatContext.h"
#include "BaseDataDefine/ST_Buffer.h"
#include "DataDefine/ST_ResampleResult.h"
#include "LogSystem/LogSystem.h"
#include "TimeSystem/TimeSystem.h"

VideoPlayWorker::VideoPlayWorker(QObject* parent)
    : QObject(parent)
{
    InitSDLSystem();
}

VideoPlayWorker::~VideoPlayWorker()
{
    LOG_INFO("VideoPlayWorker destructor called");
    SlotStopPlay();
    Cleanup();
}

bool VideoPlayWorker::InitSDLSystem()
{
    // 初始化SDL音频子系统
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        LOG_WARN("Failed to initialize SDL audio: " + std::string(SDL_GetError()));
        return false;
    }

    LOG_INFO("SDL audio system initialized successfully");
    return true;
}

void VideoPlayWorker::SafeFreeChannelLayout(AVChannelLayout** layout)
{
    if (layout && *layout)
    {
        av_channel_layout_uninit(*layout);
        av_free(*layout);
        *layout = nullptr;
    }
}

void VideoPlayWorker::Cleanup()
{
    LOG_INFO("Cleaning up video player resources");

    // 清理视频相关资源
    if (m_pSwsCtx)
    {
        sws_freeContext(m_pSwsCtx);
        m_pSwsCtx = nullptr;
    }

    m_pVideoCodecCtx.reset();
    m_pAudioCodecCtx.reset(); // 虽然不处理音频，但仍需清理解码器上下文
    m_pFormatCtx.reset();

    // 清理缓冲区
    m_rgbBuffer.clear();
    m_rgbBuffer.shrink_to_fit();

    LOG_INFO("Video player cleanup completed");
}

void VideoPlayWorker::SlotStartPlay()
{
    if (m_pFormatCtx == nullptr || m_pVideoCodecCtx == nullptr)
    {
        LOG_WARN("VideoPlayWorker::SlotStartPlay: 播放器未初始化");
        return;
    }

    LOG_INFO("VideoPlayWorker::SlotStartPlay: 开始播放");
    m_bIsPlaying.store(true);
    m_bNeedStop.store(false);
    m_bIsPaused.store(false);
    m_startTime = av_gettime();
    m_totalPauseTime = 0;
    m_currentTime = 0.0;
    m_bSeekRequested.store(false);

    // 创建播放线程
    if (m_playThread == nullptr)
    {
        m_playThread = new QThread();
        this->moveToThread(m_playThread);
        connect(m_playThread, &QThread::started, this, &VideoPlayWorker::PlayLoop, Qt::QueuedConnection);
        m_playThread->start();
    }
    else
    {
        QMetaObject::invokeMethod(this, "PlayLoop", Qt::QueuedConnection);
    }

    LOG_INFO("Play thread started");
}

void VideoPlayWorker::SlotStopPlay()
{
    LOG_INFO("Stop play requested");
    {
        m_bNeedStop.store(true);
        m_bIsPlaying.store(false);
        m_bIsPaused.store(false);
    }

    // 等待播放线程结束
    if (m_playThread)
    {
        m_playThread->quit();
        m_playThread->wait();
        delete m_playThread;
        m_playThread = nullptr;
    }
}

void VideoPlayWorker::SlotPausePlay()
{
    if (m_bIsPlaying.load() && !m_bIsPaused.load())
    {
        m_bIsPaused.store(true);
        m_pauseStartTime = av_gettime();
        LOG_INFO("Video playback paused");
    }
}

void VideoPlayWorker::SlotResumePlay()
{
    if (m_bIsPlaying.load() && m_bIsPaused.load())
    {
        m_bIsPaused.store(false);

        // 累计暂停时间
        if (m_pauseStartTime > 0)
        {
            m_totalPauseTime += (av_gettime() - m_pauseStartTime);
            m_pauseStartTime = 0;
        }

        LOG_INFO("Video playback resumed");
    }
}

void VideoPlayWorker::SlotSeekPlay(double seconds)
{
    LOG_INFO("Video seek requested to: " + std::to_string(seconds) + " seconds");

    if (seconds >= 0.0 && seconds <= m_videoInfo.m_duration)
    {
        m_seekTarget.store(seconds);
        m_bSeekRequested.store(true);
    }
}

void VideoPlayWorker::PlayLoop()
{
    LOG_INFO("Video playback loop started");

    const int MAX_CONSECUTIVE_ERRORS = 10;
    int consecutiveErrors = 0;

    while (m_bIsPlaying.load() && !m_bNeedStop.load())
    {
        // 如果处于暂停状态，等待恢复
        if (m_bIsPaused.load())
        {
            SDL_Delay(50); // 50ms延迟避免CPU占用过高
            continue;
        }

        bool frameProcessed = false;

        // 处理跳转请求
        if (m_bSeekRequested.load())
        {
            double seekTarget = m_seekTarget.load();
            int64_t timestamp = static_cast<int64_t>(seekTarget * AV_TIME_BASE);

            if (m_pFormatCtx->SeekFrame(-1, timestamp, AVSEEK_FLAG_BACKWARD))
            {
                // 清空解码器缓冲
                if (m_pVideoCodecCtx)
                {
                    m_pVideoCodecCtx->FlushBuffer();
                }
                if (m_pAudioCodecCtx && m_bHasAudio)
                {
                    m_pAudioCodecCtx->FlushBuffer();
                }

                m_currentTime = seekTarget;
                m_startTime = av_gettime();
                m_totalPauseTime = 0;
            }
            m_bSeekRequested.store(false);
        }

        // 读取数据包
        if (!m_pPacket.ReadPacket(m_pFormatCtx->GetRawContext()))
        {
            // 文件结束
            LOG_INFO("End of file reached");
            break;
        }

        // 处理视频包
        if (m_pPacket.GetStreamIndex() == m_videoStreamIndex)
        {
            if (DecodeVideoFrame())
            {
                frameProcessed = true;
                consecutiveErrors = 0; // 重置错误计数
            }
        }
        // 音频包跳过（由AudioFFmpegPlayer处理）
        else if (m_pPacket.GetStreamIndex() == m_audioStreamIndex)
        {
            // 跳过音频包，因为音频播放由其他组件处理
            frameProcessed = true; // 标记已处理避免错误计数
        }

        m_pPacket.UnrefPacket();

        // 如果没有处理任何帧，增加错误计数
        if (!frameProcessed)
        {
            consecutiveErrors++;
            if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
            {
                LOG_WARN("Too many consecutive errors, stopping playback");
                break;
            }

            // 短暂延迟避免CPU占用过高
            SDL_Delay(1);
        }

        // 定期更新播放进度
        static int frameCount = 0;
        if (++frameCount % 25 == 0) // 每25帧更新一次进度（约每秒）
        {
            emit SigPlayProgressUpdated(m_currentTime, m_videoInfo.m_duration);
        }
    }

    // 播放结束，设置状态，没有中途结束时
    if (!m_bNeedStop.load())
    {
        m_bIsPlaying.store(false);
    }
    LOG_INFO("Video playback completed");
}

// ProcessAudioFrame方法已移除，音频处理由AudioFFmpegPlayer处理

AVPixelFormat VideoPlayWorker::GetSafePixelFormat(AVPixelFormat format)
{
    // 检查格式是否有效
    if (format == AV_PIX_FMT_NONE)
    {
        LOG_WARN("Invalid pixel format: AV_PIX_FMT_NONE, using YUV420P");
        return AV_PIX_FMT_YUV420P;
    }

    // 获取像素格式描述
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(format);
    if (!desc)
    {
        LOG_WARN("Cannot get pixel format descriptor for format: " + std::to_string(static_cast<int>(format)) + ", using YUV420P");
        return AV_PIX_FMT_YUV420P;
    }

    // 检查是否为硬件加速格式
    if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL)
    {
        LOG_INFO("Hardware accelerated format detected: " + std::string(av_get_pix_fmt_name(format)) + ", converting to YUV420P");
        return AV_PIX_FMT_YUV420P;
    }

    // 检查一些不支持的格式
    switch (format)
    {
        case AV_PIX_FMT_DXVA2_VLD:
        case AV_PIX_FMT_D3D11VA_VLD:
        case AV_PIX_FMT_VAAPI:
        case AV_PIX_FMT_VDPAU:
        case AV_PIX_FMT_VIDEOTOOLBOX:
        case AV_PIX_FMT_CUDA:
        case AV_PIX_FMT_QSV: LOG_INFO("Unsupported hardware format: " + std::string(av_get_pix_fmt_name(format)) + ", using YUV420P");
            return AV_PIX_FMT_YUV420P;
        default:
            return format;
    }
}

SwsContext* VideoPlayWorker::CreateSafeSwsContext(AVPixelFormat srcFormat, AVPixelFormat dstFormat)
{
    // 验证输入参数
    if (m_videoInfo.m_width <= 0 || m_videoInfo.m_height <= 0)
    {
        LOG_WARN("Invalid video dimensions: " + std::to_string(m_videoInfo.m_width) + "x" + std::to_string(m_videoInfo.m_height));
        return nullptr;
    }

    if (m_videoInfo.m_width > 4096 || m_videoInfo.m_height > 4096)
    {
        LOG_WARN("Video dimensions too large: " + std::to_string(m_videoInfo.m_width) + "x" + std::to_string(m_videoInfo.m_height));
        return nullptr;
    }

    // 获取安全的像素格式
    AVPixelFormat safeSrcFormat = GetSafePixelFormat(srcFormat);

    LOG_INFO("Creating swscale context:");
    LOG_INFO("  Source: " + std::to_string(m_videoInfo.m_width) + "x" + std::to_string(m_videoInfo.m_height) + " format=" + std::string(av_get_pix_fmt_name(safeSrcFormat)));
    LOG_INFO("  Target: " + std::to_string(m_videoInfo.m_width) + "x" + std::to_string(m_videoInfo.m_height) + " format=" + std::string(av_get_pix_fmt_name(dstFormat)));

    // 尝试创建转换上下文
    SwsContext* swsCtx = nullptr;


    swsCtx = sws_getContext(m_videoInfo.m_width, m_videoInfo.m_height, safeSrcFormat, m_videoInfo.m_width, m_videoInfo.m_height, dstFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);


    if (!swsCtx)
    {
        LOG_WARN("Failed to create swscale context with format: " + std::string(av_get_pix_fmt_name(safeSrcFormat)));

        // 尝试使用YUV420P格式
        if (safeSrcFormat != AV_PIX_FMT_YUV420P)
        {
            LOG_INFO("Trying with YUV420P format...");

            swsCtx = sws_getContext(m_videoInfo.m_width, m_videoInfo.m_height, AV_PIX_FMT_YUV420P, m_videoInfo.m_width, m_videoInfo.m_height, dstFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
        }

        // 如果还是失败，尝试RGB24格式
        if (!swsCtx && dstFormat != AV_PIX_FMT_RGB24)
        {
            LOG_INFO("Trying with RGB24 output format...");

            swsCtx = sws_getContext(m_videoInfo.m_width, m_videoInfo.m_height, AV_PIX_FMT_YUV420P, m_videoInfo.m_width, m_videoInfo.m_height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
        }
    }

    if (swsCtx)
    {
        LOG_INFO("Successfully created swscale context");
    }
    else
    {
        LOG_WARN("Failed to create any working swscale context");
    }

    return swsCtx;
}

bool VideoPlayWorker::InitPlayer(const QString& filePath, ST_SDL_Renderer* renderer, ST_SDL_Texture* texture)
{
    TIME_START("VideoPlayerInit");
    LOG_INFO("=== Initializing video player for: " + filePath.toStdString() + " ===");

    if (filePath.isEmpty())
    {
        LOG_WARN("VideoPlayWorker::InitPlayer() : Invalid file path");
        return false;
    }

    // renderer和texture可以为nullptr，表示仅使用Qt显示
    m_pRenderer = renderer;
    m_pTexture = texture;


    // 创建格式上下文
    TIME_START("VideoFormatCtxOpen");
    m_pFormatCtx = std::make_unique<ST_AVFormatContext>();
    if (!m_pFormatCtx->OpenInputFilePath(filePath.toLocal8Bit().constData()))
    {
        return false;
    }

    TimeSystem::Instance().StopTimingWithLog("VideoFormatCtxOpen", EM_TimingLogLevel::Info);

    // 查找视频流
    TIME_START("VideoStreamFind");
    m_videoStreamIndex = m_pFormatCtx->FindBestStream(AVMEDIA_TYPE_VIDEO);
    if (m_videoStreamIndex < 0)
    {
        LOG_ERROR("No video stream found in file");
        return false;
    }
    TimeSystem::Instance().StopTimingWithLog("VideoStreamFind", EM_TimingLogLevel::Info, EM_TimeUnit::Milliseconds, "Video stream found (index: " + std::to_string(m_videoStreamIndex) + ")");

    // 获取视频流信息
    AVStream* videoStream = m_pFormatCtx->GetRawContext()->streams[m_videoStreamIndex];
    AVCodecParameters* codecPar = videoStream->codecpar;

    // 查找解码器
    ST_AVCodec decoder((AVCodecID)codecPar->codec_id);
    if (!decoder.GetRawCodec())
    {
        LOG_WARN("Decoder not found for codec ID: " + std::to_string(codecPar->codec_id));
        return false;
    }

    // 创建视频解码器上下文
    TIME_START("VideoCodecSetup");
    m_pVideoCodecCtx = std::make_unique<ST_AVCodecContext>(decoder.GetRawCodec());
    // 绑定参数到上下文
    if (!m_pVideoCodecCtx->BindParamToContext(codecPar))
    {
        return false;
    }

    // 打开解码器
    if (!m_pVideoCodecCtx->OpenCodec(decoder.GetRawCodec()))
    {
        return false;
    }

    TimeSystem::Instance().StopTimingWithLog("VideoCodecSetup", EM_TimingLogLevel::Info);

    // 初始化视频信息
    m_videoInfo.m_width = codecPar->width;
    m_videoInfo.m_height = codecPar->height;
    m_videoInfo.m_pixelFormat = static_cast<AVPixelFormat>(codecPar->format);

    // 验证视频信息
    if (m_videoInfo.m_width <= 0 || m_videoInfo.m_height <= 0)
    {
        LOG_WARN("Invalid video dimensions: " + std::to_string(m_videoInfo.m_width) + "x" + std::to_string(m_videoInfo.m_height));
        return false;
    }

    // 计算帧率
    if (videoStream->avg_frame_rate.num != 0 && videoStream->avg_frame_rate.den != 0)
    {
        m_videoInfo.m_frameRate = av_q2d(videoStream->avg_frame_rate);
    }
    else if (videoStream->r_frame_rate.num != 0 && videoStream->r_frame_rate.den != 0)
    {
        m_videoInfo.m_frameRate = av_q2d(videoStream->r_frame_rate);
    }
    else
    {
        m_videoInfo.m_frameRate = 25.0; // 默认25fps
    }

    // 计算总时长
    if (m_pFormatCtx->GetRawContext()->duration != AV_NOPTS_VALUE)
    {
        m_videoInfo.m_duration = static_cast<double>(m_pFormatCtx->GetRawContext()->duration) / AV_TIME_BASE;
    }
    else
    {
        m_videoInfo.m_duration = 0.0;
    }

    // 计算总帧数
    m_videoInfo.m_totalFrames = static_cast<int64_t>(m_videoInfo.m_duration * m_videoInfo.m_frameRate);

    // 创建安全的图像转换上下文
    TIME_START("VideoSwsCtxCreate");
    m_pSwsCtx = CreateSafeSwsContext(m_videoInfo.m_pixelFormat, AV_PIX_FMT_RGBA);
    if (!m_pSwsCtx)
    {
        LOG_ERROR("Failed to create swscale context");
        return false;
    }
    TimeSystem::Instance().StopTimingWithLog("VideoSwsCtxCreate", EM_TimingLogLevel::Info);

    // 为RGB帧分配缓冲区
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_videoInfo.m_width, m_videoInfo.m_height, 1);
    if (bufferSize <= 0)
    {
        LOG_WARN("Invalid buffer size: " + std::to_string(bufferSize));
        return false;
    }

    // 确保缓冲区足够大
    m_rgbBuffer.resize(bufferSize);

    int ret = av_image_fill_arrays(m_pRGBFrame.GetRawFrame()->data, m_pRGBFrame.GetRawFrame()->linesize, m_rgbBuffer.data(), AV_PIX_FMT_RGBA, m_videoInfo.m_width, m_videoInfo.m_height, 1);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to fill RGB frame arrays: " + std::string(errbuf));
        return false;
    }

    // 查找音频流
    m_audioStreamIndex = -1;
    m_bHasAudio = false;

    for (unsigned int i = 0; i < m_pFormatCtx->GetRawContext()->nb_streams; i++)
    {
        if (m_pFormatCtx->GetRawContext()->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            m_audioStreamIndex = static_cast<int>(i);
            m_bHasAudio = true;
            break;
        }
    }

    // 音频流检测（但不在VideoPlayWorker中处理音频播放）
    if (m_bHasAudio)
    {
        LOG_INFO("Audio stream detected in video file (audio will be handled separately)");
        // 注意：音频播放应该由AudioFFmpegPlayer或统一的多媒体播放器处理
        // 这里只做检测，不做实际的音频播放处理
        m_bHasAudio = false; // 暂时禁用VideoPlayWorker中的音频处理
    }

    LOG_INFO("=== Video player initialized successfully ===");
    LOG_INFO("Video info - Width: " + std::to_string(m_videoInfo.m_width) + ", Height: " + std::to_string(m_videoInfo.m_height) + ", FPS: " + std::to_string(m_videoInfo.m_frameRate) + ", Duration: " + std::to_string(m_videoInfo.m_duration));

    TimeSystem::Instance().StopTimingWithLog("VideoPlayerInit", EM_TimingLogLevel::Info, EM_TimeUnit::Milliseconds, "Video player initialization completed");
    return true;
}

bool VideoPlayWorker::DecodeVideoFrame()
{
    if (!m_pVideoCodecCtx)
    {
        return false;
    }

    // 发送数据包到解码器
    if (!m_pPacket.SendPacket(m_pVideoCodecCtx->GetRawContext()))
    {
        return false;
    }

    // 接收解码后的视频帧
    while (m_pVideoFrame.GetCodecFrame(m_pVideoCodecCtx->GetRawContext()))
    {
        RenderFrame(m_pVideoFrame.GetRawFrame());

        // 计算时间延迟
        int delay = CalculateFrameDelay(m_pVideoFrame.GetRawFrame()->pts);
        if (delay > 0)
        {
            SDL_Delay(delay);
        }

        return true;
    }

    return false;
}

// DecodeAudioFrame方法已移除，音频解码由AudioFFmpegPlayer处理

void VideoPlayWorker::RenderFrame(AVFrame* frame)
{
    if (!frame || !m_pSwsCtx)
    {
        return;
    }

    // 转换图像格式
    int ret = sws_scale(m_pSwsCtx, frame->data, frame->linesize, 0, m_videoInfo.m_height, m_pRGBFrame.GetRawFrame()->data, m_pRGBFrame.GetRawFrame()->linesize);

    if (ret <= 0)
    {
        LOG_WARN("Failed to scale video frame");
        return;
    }

    // 发送Qt显示信号（主要使用Qt显示，暂时不使用SDL渲染）
    emit SigFrameDataUpdated(m_pRGBFrame.GetRawFrame()->data[0], m_videoInfo.m_width, m_videoInfo.m_height);

    // 更新当前时间
    if (frame->pts != AV_NOPTS_VALUE)
    {
        AVStream* videoStream = m_pFormatCtx->GetRawContext()->streams[m_videoStreamIndex];
        m_currentTime = frame->pts * av_q2d(videoStream->time_base);
    }

    // 发送帧更新信号
    emit SigFrameUpdated();
}

int VideoPlayWorker::CalculateFrameDelay(int64_t pts)
{
    if (pts == AV_NOPTS_VALUE)
    {
        return static_cast<int>(1000.0 / m_videoInfo.m_frameRate);
    }

    // 计算当前帧的时间戳
    AVStream* videoStream = m_pFormatCtx->GetRawContext()->streams[m_videoStreamIndex];
    double frameTime = pts * av_q2d(videoStream->time_base);

    // 计算已播放时间
    int64_t currentTime = av_gettime();
    double playedTime = (currentTime - m_startTime - m_totalPauseTime) / 1000000.0;

    // 计算需要延迟的时间
    double delay = frameTime - playedTime;

    if (delay > 0)
    {
        return static_cast<int>(delay * 1000); // 转换为毫秒
    }

    return 0;
}
