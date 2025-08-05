#include "VideoPlayWorker.h"
#include <ThreadPool/ThreadPool.h>
#include "CoreServerGlobal.h"
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
    m_pFormatCtx.reset();

    // 清理帧和缓冲区
    m_pRGBFrame = ST_AVFrame();   // 重置RGB帧
    m_pVideoFrame = ST_AVFrame(); // 重置视频帧
    m_rgbBuffer.clear();
    m_rgbBuffer.shrink_to_fit();

    m_videoInfo = ST_VideoFrameInfo();
    m_currentTime = 0.0;
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_bSeekRequested.store(false);
    m_bNeedStop.store(false);

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
    m_playState.TransitionTo(AVPlayState::Playing);
    m_bNeedStop.store(false);
    m_startTime = av_gettime();
    m_totalPauseTime = 0;
    m_currentTime = 0.0;
    m_bSeekRequested.store(false);
    m_threadId = CoreServerGlobal::Instance().GetThreadPool().CreateDedicatedThread("VideoPlayerThread", [this]()
    {
        PlayLoop();
    });
    LOG_INFO("Play thread started");
}

void VideoPlayWorker::SlotStopPlay()
{
    LOG_INFO("Stop play requested");
    m_playState.TransitionTo(AVPlayState::Stopped);
    m_bNeedStop.store(true);
    CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_threadId);
    SDL_Delay(20);
}

void VideoPlayWorker::SlotPausePlay()
{
    if (m_playState.GetCurrentState() == AVPlayState::Playing)
    {
        m_playState.TransitionTo(AVPlayState::Paused);
        m_pauseStartTime = av_gettime();
        LOG_INFO("Video playback paused");
    }
}

void VideoPlayWorker::SlotResumePlay()
{
    if (m_playState.GetCurrentState() == AVPlayState::Paused)
    {
        m_playState.TransitionTo(AVPlayState::Playing);

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

    double duration = 0.0;
    duration = m_videoInfo.m_duration;

    if (seconds >= 0.0 && seconds <= duration)
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
    while (m_playState.GetCurrentState() == AVPlayState::Playing && !m_bNeedStop.load())
    {
        // 如果处于暂停状态，等待恢复
        if (m_playState.GetCurrentState() == AVPlayState::Paused)
        {
            SDL_Delay(10); // 10ms延迟避免CPU占用过高
            continue;
        }

        bool frameProcessed = false;

        // 处理跳转请求
        if (m_bSeekRequested.load())
        {
            if (!m_pFormatCtx || !m_pVideoCodecCtx)
            {
                m_bSeekRequested.store(false);
                continue;
            }

            double seekTarget = m_seekTarget.load();
            int64_t timestamp = static_cast<int64_t>(seekTarget * AV_TIME_BASE);

            if (m_pFormatCtx->SeekFrame(-1, timestamp, AVSEEK_FLAG_BACKWARD))
            {
                // 清空解码器缓冲
                if (m_pVideoCodecCtx)
                {
                    m_pVideoCodecCtx->FlushBuffer();
                }
                m_currentTime = seekTarget;
                m_startTime = av_gettime();
                m_totalPauseTime = 0;

                LOG_INFO("Seek completed, time reset to: " + std::to_string(seekTarget) + " seconds");
            }
            m_bSeekRequested.store(false);
        }

        // 读取数据包
        {
            if (!m_pFormatCtx || !m_pVideoCodecCtx)
            {
                SDL_Delay(10);
                continue;
            }

            if (!m_pPacket.ReadPacket(m_pFormatCtx->GetRawContext()))
            {
                // 文件结束
                LOG_INFO("End of file reached");
                break;
            }
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

ST_VideoFrameInfo VideoPlayWorker::GetVideoInfo()
{
    return m_videoInfo;
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

bool VideoPlayWorker::InitPlayer(std::unique_ptr<ST_OpenFileResult> openFileResult, ST_SDL_Renderer* renderer, ST_SDL_Texture* texture)
{
    if (!openFileResult || !openFileResult->m_formatCtx)
    {
        LOG_ERROR("Invalid open file result");
        return false;
    }

    TIME_START("VideoPlayerInit");
    LOG_INFO("=== Initializing video player with pre-opened file ===");

    // renderer和texture可以为nullptr，表示仅使用Qt显示
    m_pRenderer = renderer;
    m_pTexture = texture;
    // 使用传入的已打开格式上下文
    m_pFormatCtx = std::unique_ptr<ST_AVFormatContext>(openFileResult->m_formatCtx);
    openFileResult->m_formatCtx = nullptr; // 转移所有权

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

    // 为RGB帧分配缓冲区（线程安全）
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_videoInfo.m_width, m_videoInfo.m_height, 1);
    if (bufferSize <= 0)
    {
        LOG_WARN("Invalid buffer size: " + std::to_string(bufferSize));
        return false;
    }

    // 确保缓冲区足够大（加锁保护）
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
    if (!m_pVideoCodecCtx || m_bNeedStop.load())
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

    // 转换图像格式（线程安全）
    int ret = sws_scale(m_pSwsCtx, frame->data, frame->linesize, 0, m_videoInfo.m_height, m_pRGBFrame.GetRawFrame()->data, m_pRGBFrame.GetRawFrame()->linesize);

    if (ret <= 0)
    {
        LOG_WARN("Failed to scale video frame");
        return;
    }

    // 发送Qt显示信号（确保RGB缓冲区锁定）
    // 检查缓冲区是否有效
    if (!m_rgbBuffer.empty())
    {
        // 深拷贝RGB帧的data[0]
        int dataSize = m_videoInfo.m_width * m_videoInfo.m_height * 4; // RGBA格式每个像素4字节
        std::vector<uint8_t> frameDataCopy(dataSize);
        std::memcpy(frameDataCopy.data(), m_pRGBFrame.GetRawFrame()->data[0], dataSize);
        emit SigFrameDataUpdated(frameDataCopy, m_videoInfo.m_width, m_videoInfo.m_height);
    }

    // 更新当前时间
    if (frame->pts != AV_NOPTS_VALUE)
    {
        AVStream* videoStream = m_pFormatCtx->GetRawContext()->streams[m_videoStreamIndex];
        m_currentTime = frame->pts * av_q2d(videoStream->time_base);
    }
    else
    {
        // 如果pts无效，根据帧率估算时间
        m_currentTime += (1.0 / m_videoInfo.m_frameRate);
    }

    // 限制当前时间在合理范围内
    if (m_currentTime < 0.0)
    {
        m_currentTime = 0.0;
    }
    if (m_currentTime > m_videoInfo.m_duration)
    {
        m_currentTime = m_videoInfo.m_duration;
    }

    // 发送帧更新信号
    emit SigFrameUpdated();
}

int VideoPlayWorker::CalculateFrameDelay(int64_t pts)
{
    double frameRate = m_videoInfo.m_frameRate;
    if (frameRate <= 0.0)
    {
        frameRate = 25.0;
    }

    if (pts == AV_NOPTS_VALUE)
    {
        return static_cast<int>(1000.0 / frameRate);
    }

    // 计算当前帧的时间戳
    if (!m_pFormatCtx || m_videoStreamIndex < 0)
    {
        return static_cast<int>(1000.0 / frameRate);
    }

    AVStream* videoStream = m_pFormatCtx->GetRawContext()->streams[m_videoStreamIndex];
    double frameTime = pts * av_q2d(videoStream->time_base);

    // 限制frameTime在合理范围内，避免seek后的异常延迟
    if (frameTime < 0.0 || frameTime > m_videoInfo.m_duration + 10.0)
    {
        return static_cast<int>(1000.0 / frameRate);
    }

    // 计算已播放时间
    int64_t currentTime = av_gettime();
    double playedTime = (currentTime - m_startTime - m_totalPauseTime) / 1000000.0;

    // 限制延迟时间在合理范围内，避免过长的等待
    double delay = frameTime - playedTime;
    double maxDelay = 1.0 / frameRate * 2.0; // 最大延迟为2帧时间
    double minDelay = -0.1;                  // 允许少量负延迟来追赶

    if (delay > maxDelay)
    {
        delay = maxDelay;
    }
    else if (delay < minDelay)
    {
        delay = 0; // 如果落后太多，立即显示
    }

    return static_cast<int>(delay * 1000); // 转换为毫秒
}
