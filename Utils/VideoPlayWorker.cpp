#include "VideoPlayWorker.h"
#include <QMutexLocker>
#include <QThread>
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVFormatContext.h"
#include "LogSystem/LogSystem.h"
#include <chrono>

VideoPlayWorker::VideoPlayWorker(QObject* parent)
    : QObject(parent), m_pFormatCtx(nullptr), m_pCodecCtx(nullptr), m_videoStreamIndex(-1), m_pPacket(nullptr), m_pVideoFrame(nullptr), m_pRGBFrame(nullptr), m_pSwsCtx(nullptr), m_pRenderer(nullptr), m_pTexture(nullptr), m_playState(EM_VideoPlayState::Stopped), m_bNeedStop(false), m_startTime(0), m_pauseStartTime(0), m_totalPauseTime(0), m_currentTime(0.0), m_bSeekRequested(false), m_seekTarget(0.0)
{
    // 初始化FFmpeg包
    m_pPacket = av_packet_alloc();
    m_pVideoFrame = av_frame_alloc();
    m_pRGBFrame = av_frame_alloc();
}

VideoPlayWorker::~VideoPlayWorker()
{
    Cleanup();
}

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

    try
    {
        swsCtx = sws_getContext(m_videoInfo.m_width, m_videoInfo.m_height, safeSrcFormat, m_videoInfo.m_width, m_videoInfo.m_height, dstFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
    } catch (...)
    {
        LOG_WARN("Exception occurred while creating swscale context");
        return nullptr;
    }

    if (!swsCtx)
    {
        LOG_WARN("Failed to create swscale context with format: " + std::string(av_get_pix_fmt_name(safeSrcFormat)));

        // 尝试使用YUV420P格式
        if (safeSrcFormat != AV_PIX_FMT_YUV420P)
        {
            LOG_INFO("Trying with YUV420P format...");
            try
            {
                swsCtx = sws_getContext(m_videoInfo.m_width, m_videoInfo.m_height, AV_PIX_FMT_YUV420P, m_videoInfo.m_width, m_videoInfo.m_height, dstFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
            } catch (...)
            {
                LOG_WARN("Exception occurred while creating swscale context with YUV420P");
                return nullptr;
            }
        }

        // 如果还是失败，尝试RGB24格式
        if (!swsCtx && dstFormat != AV_PIX_FMT_RGB24)
        {
            LOG_INFO("Trying with RGB24 output format...");
            try
            {
                swsCtx = sws_getContext(m_videoInfo.m_width, m_videoInfo.m_height, AV_PIX_FMT_YUV420P, m_videoInfo.m_width, m_videoInfo.m_height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
            } catch (...)
            {
                LOG_WARN("Exception occurred while creating swscale context with RGB24");
                return nullptr;
            }
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
    auto startTime = std::chrono::high_resolution_clock::now();
    LOG_INFO("=== Initializing video player for: " + filePath.toStdString() + " ===");
    
    if (filePath.isEmpty())
    {
        LOG_WARN("VideoPlayWorker::InitPlayer() : Invalid file path");
        return false;
    }

    // renderer和texture可以为nullptr，表示仅使用Qt显示
    m_pRenderer = renderer;
    m_pTexture = texture;

    try
    {
        // 创建格式上下文
        auto formatCtxStartTime = std::chrono::high_resolution_clock::now();
        m_pFormatCtx = std::make_unique<ST_AVFormatContext>();
        int ret = m_pFormatCtx->OpenInputFilePath(filePath.toLocal8Bit().constData());
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_ERROR("Failed to open input file: " + std::string(errbuf));
            return false;
        }
        auto formatCtxEndTime = std::chrono::high_resolution_clock::now();
        auto formatCtxDuration = std::chrono::duration_cast<std::chrono::milliseconds>(formatCtxEndTime - formatCtxStartTime).count();
        LOG_INFO("Video format context opened in " + std::to_string(formatCtxDuration) + " ms");

        // 查找视频流
        auto streamFindStartTime = std::chrono::high_resolution_clock::now();
        m_videoStreamIndex = m_pFormatCtx->FindBestStream(AVMEDIA_TYPE_VIDEO);
        if (m_videoStreamIndex < 0)
        {
            LOG_ERROR("No video stream found in file");
            return false;
        }
        auto streamFindEndTime = std::chrono::high_resolution_clock::now();
        auto streamFindDuration = std::chrono::duration_cast<std::chrono::milliseconds>(streamFindEndTime - streamFindStartTime).count();
        LOG_INFO("Video stream found (index: " + std::to_string(m_videoStreamIndex) + ") in " + std::to_string(streamFindDuration) + " ms");

        // 获取视频流信息
        AVFormatContext* rawCtx = m_pFormatCtx->GetRawContext();
        AVStream* videoStream = rawCtx->streams[m_videoStreamIndex];
        AVCodecParameters* codecPar = videoStream->codecpar;

        // 查找解码器
        const AVCodec* decoder = avcodec_find_decoder(codecPar->codec_id);
        if (!decoder)
        {
            LOG_WARN("Decoder not found for codec ID: " + std::to_string(codecPar->codec_id));
            return false;
        }

        // 创建解码器上下文
        auto codecSetupStartTime = std::chrono::high_resolution_clock::now();
        m_pCodecCtx = std::make_unique<ST_AVCodecContext>(decoder);
        if (!m_pCodecCtx->GetRawContext())
        {
            LOG_WARN("Failed to allocate codec context");
            return false;
        }

        // 绑定参数到上下文
        ret = m_pCodecCtx->BindParamToContext(codecPar);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to bind parameters: " + std::string(errbuf));
            return false;
        }

        // 打开解码器
        ret = m_pCodecCtx->OpenCodec(decoder);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to open codec: " + std::string(errbuf));
            return false;
        }
        auto codecSetupEndTime = std::chrono::high_resolution_clock::now();
        auto codecSetupDuration = std::chrono::duration_cast<std::chrono::milliseconds>(codecSetupEndTime - codecSetupStartTime).count();
        LOG_INFO("Video codec initialized in " + std::to_string(codecSetupDuration) + " ms");

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
        if (rawCtx->duration != AV_NOPTS_VALUE)
        {
            m_videoInfo.m_duration = static_cast<double>(rawCtx->duration) / AV_TIME_BASE;
        }
        else
        {
            m_videoInfo.m_duration = 0.0;
        }

        // 计算总帧数
        m_videoInfo.m_totalFrames = static_cast<int64_t>(m_videoInfo.m_duration * m_videoInfo.m_frameRate);

        // 创建安全的图像转换上下文
        auto swsStartTime = std::chrono::high_resolution_clock::now();
        m_pSwsCtx = CreateSafeSwsContext(m_videoInfo.m_pixelFormat, AV_PIX_FMT_RGBA);
        if (!m_pSwsCtx)
        {
            LOG_ERROR("Failed to create swscale context");
            return false;
        }
        auto swsEndTime = std::chrono::high_resolution_clock::now();
        auto swsDuration = std::chrono::duration_cast<std::chrono::milliseconds>(swsEndTime - swsStartTime).count();
        LOG_INFO("Video scaler context created in " + std::to_string(swsDuration) + " ms");

        // 为RGB帧分配缓冲区
        int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_videoInfo.m_width, m_videoInfo.m_height, 1);
        if (bufferSize <= 0)
        {
            LOG_WARN("Invalid buffer size: " + std::to_string(bufferSize));
            return false;
        }

        auto buffer = static_cast<uint8_t*>(av_malloc(bufferSize));
        if (!buffer)
        {
            LOG_WARN("Failed to allocate RGB frame buffer");
            return false;
        }

        ret = av_image_fill_arrays(m_pRGBFrame->data, m_pRGBFrame->linesize, buffer, AV_PIX_FMT_RGBA, m_videoInfo.m_width, m_videoInfo.m_height, 1);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to fill RGB frame arrays: " + std::string(errbuf));
            av_free(buffer);
            return false;
        }

        // 仅在有SDL渲染器和纹理时创建SDL纹理
        if (m_pRenderer && m_pTexture)
        {
            if (!m_pTexture->CreateTexture(m_pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, m_videoInfo.m_width, m_videoInfo.m_height))
            {
                LOG_WARN("Failed to create SDL texture");
                return false;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        LOG_INFO("=== Video player initialized successfully in " + std::to_string(totalDuration) + " ms ===");
        LOG_INFO("Video info - Width: " + std::to_string(m_videoInfo.m_width) + ", Height: " + std::to_string(m_videoInfo.m_height) + ", FPS: " + std::to_string(m_videoInfo.m_frameRate) + ", Duration: " + std::to_string(m_videoInfo.m_duration));

        return true;
    } catch (const std::exception& e)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        LOG_ERROR("Exception in InitPlayer after " + std::to_string(duration) + " ms: " + std::string(e.what()));
        return false;
    }
}

void VideoPlayWorker::Cleanup()
{
    m_bNeedStop = true;

    // 等待播放循环结束
    QThread::msleep(50);

    // 释放FFmpeg资源
    if (m_pSwsCtx)
    {
        sws_freeContext(m_pSwsCtx);
        m_pSwsCtx = nullptr;
    }

    if (m_pRGBFrame)
    {
        if (m_pRGBFrame->data[0])
        {
            av_free(m_pRGBFrame->data[0]);
        }
        av_frame_free(&m_pRGBFrame);
    }

    if (m_pVideoFrame)
    {
        av_frame_free(&m_pVideoFrame);
    }

    if (m_pPacket)
    {
        av_packet_free(&m_pPacket);
    }

    m_pCodecCtx.reset();
    m_pFormatCtx.reset();

    // 清理SDL资源
    if (m_pTexture)
    {
        m_pTexture->DestroyTexture();
    }

    m_pRenderer = nullptr;
    m_pTexture = nullptr;
}

void VideoPlayWorker::SlotStartPlay()
{
    QMutexLocker locker(&m_mutex);

    if (m_playState != EM_VideoPlayState::Stopped)
    {
        return;
    }

    m_playState = EM_VideoPlayState::Playing;
    m_bNeedStop = false;
    m_startTime = av_gettime_relative();
    m_totalPauseTime = 0;
    m_currentTime = 0.0;

    emit SigPlayStateChanged(m_playState);

    // 启动播放循环
    PlayLoop();
}

void VideoPlayWorker::SlotPausePlay()
{
    QMutexLocker locker(&m_mutex);

    if (m_playState == EM_VideoPlayState::Playing)
    {
        m_playState = EM_VideoPlayState::Paused;
        m_pauseStartTime = av_gettime_relative();
        emit SigPlayStateChanged(m_playState);
    }
}

void VideoPlayWorker::SlotResumePlay()
{
    QMutexLocker locker(&m_mutex);

    if (m_playState == EM_VideoPlayState::Paused)
    {
        m_playState = EM_VideoPlayState::Playing;
        m_totalPauseTime += (av_gettime_relative() - m_pauseStartTime);
        emit SigPlayStateChanged(m_playState);
    }
}

void VideoPlayWorker::SlotStopPlay()
{
    QMutexLocker locker(&m_mutex);

    m_bNeedStop = true;
    m_playState = EM_VideoPlayState::Stopped;
    m_currentTime = 0.0;

    emit SigPlayStateChanged(m_playState);
}

void VideoPlayWorker::SlotSeekPlay(double seconds)
{
    QMutexLocker locker(&m_mutex);

    if (seconds < 0.0)
    {
        seconds = 0.0;
    }
    else if (seconds > m_videoInfo.m_duration)
    {
        seconds = m_videoInfo.m_duration;
    }

    m_seekTarget = seconds;
    m_bSeekRequested = true;
}

void VideoPlayWorker::PlayLoop()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    LOG_INFO("=== Starting video playback loop ===");
    
    int frameCount = 0;
    auto lastLogTime = startTime;
    
    while (!m_bNeedStop && m_playState != EM_VideoPlayState::Stopped)
    {
        // 每播放100帧记录一次进度
        if (frameCount % 100 == 0 && frameCount > 0)
        {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastLogTime).count();
            double fps = frameCount > 0 ? (100.0 * 1000.0 / duration) : 0.0;
            LOG_INFO("Played " + std::to_string(frameCount) + " frames, current FPS: " + std::to_string(fps) + " (last 100 frames in " + std::to_string(duration) + " ms)");
            lastLogTime = currentTime;
        }

        // 检查跳转请求
        if (m_bSeekRequested)
        {
            auto seekStartTime = std::chrono::high_resolution_clock::now();
            double seekTime = m_seekTarget.load();
            LOG_INFO("Processing seek request to " + std::to_string(seekTime) + " seconds");

            if (m_pFormatCtx->SeekFrame(m_videoStreamIndex, seekTime) >= 0)
            {
                avcodec_flush_buffers(m_pCodecCtx->GetRawContext());
                m_currentTime = seekTime;
                m_startTime = av_gettime_relative() - static_cast<int64_t>(seekTime * 1000000);
                m_totalPauseTime = 0;
                
                auto seekEndTime = std::chrono::high_resolution_clock::now();
                auto seekDuration = std::chrono::duration_cast<std::chrono::milliseconds>(seekEndTime - seekStartTime).count();
                LOG_INFO("Video seek completed in " + std::to_string(seekDuration) + " ms");
            }
            else
            {
                LOG_WARN("Video seek failed for position " + std::to_string(seekTime));
            }

            m_bSeekRequested = false;
        }

        // 如果暂停，等待
        if (m_playState == EM_VideoPlayState::Paused)
        {
            QThread::msleep(10);
            continue;
        }

        // 解码并渲染帧
        if (!DecodeVideoFrame())
        {
            break;
        }
        
        frameCount++;
        QThread::msleep(1);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    LOG_INFO("=== Video playback loop ended after " + std::to_string(totalDuration) + " ms, played " + std::to_string(frameCount) + " frames ===");
    
    // 播放结束
    m_playState = EM_VideoPlayState::Stopped;
    emit SigPlayStateChanged(m_playState);
}

bool VideoPlayWorker::DecodeVideoFrame()
{
    AVFormatContext* rawCtx = m_pFormatCtx->GetRawContext();
    AVCodecContext* codecCtx = m_pCodecCtx->GetRawContext();

    while (true)
    {
        // 读取数据包
        int ret = av_read_frame(rawCtx, m_pPacket);
        if (ret < 0)
        {
            if (ret == AVERROR_EOF)
            {
                LOG_INFO("Reached end of file");
            }
            else
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                LOG_WARN("Error reading frame: " + std::string(errbuf));
            }
            return false;
        }

        // 只处理视频流的数据包
        if (m_pPacket->stream_index != m_videoStreamIndex)
        {
            av_packet_unref(m_pPacket);
            continue;
        }

        // 发送数据包到解码器
        ret = avcodec_send_packet(codecCtx, m_pPacket);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Error sending packet to decoder: " + std::string(errbuf));
            av_packet_unref(m_pPacket);
            continue;
        }

        // 接收解码后的帧
        while (ret >= 0)
        {
            ret = avcodec_receive_frame(codecCtx, m_pVideoFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            else if (ret < 0)
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                LOG_WARN("Error receiving frame from decoder: " + std::string(errbuf));
                break;
            }

            // 计算帧的显示时间
            int64_t pts = m_pVideoFrame->pts;
            if (pts != AV_NOPTS_VALUE)
            {
                AVStream* stream = rawCtx->streams[m_videoStreamIndex];
                double frameTime = pts * av_q2d(stream->time_base);

                // 计算延迟时间
                int delay = CalculateFrameDelay(pts);
                if (delay > 0)
                {
                    QThread::msleep(delay);
                }

                m_currentTime = frameTime;
                emit SigPlayProgressUpdated(m_currentTime, m_videoInfo.m_duration);
            }

            // 渲染帧
            RenderFrame(m_pVideoFrame);

            av_packet_unref(m_pPacket);
            return true;
        }

        av_packet_unref(m_pPacket);
    }

    return false;
}

void VideoPlayWorker::RenderFrame(AVFrame* frame)
{
    if (!frame || !m_pSwsCtx)
    {
        return;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try
    {
        // 转换图像格式到RGBA
        auto scaleStartTime = std::chrono::high_resolution_clock::now();
        int result = sws_scale(m_pSwsCtx, frame->data, frame->linesize, 0, m_videoInfo.m_height, m_pRGBFrame->data, m_pRGBFrame->linesize);
        auto scaleEndTime = std::chrono::high_resolution_clock::now();
        
        if (result <= 0)
        {
            auto scaleDuration = std::chrono::duration_cast<std::chrono::microseconds>(scaleEndTime - scaleStartTime).count();
            LOG_WARN("Failed to scale frame, result: " + std::to_string(result) + " (took " + std::to_string(scaleDuration) + " μs)");
            return;
        }

        // 只在缩放耗时较长时记录
        auto scaleDuration = std::chrono::duration_cast<std::chrono::microseconds>(scaleEndTime - scaleStartTime).count();
        if (scaleDuration > 5000) // 大于5ms
        {
            LOG_DEBUG("Frame scaling took " + std::to_string(scaleDuration) + " μs");
        }

        // 计算RGBA数据大小
        int dataSize = m_videoInfo.m_width * m_videoInfo.m_height * 4; // RGBA = 4 bytes per pixel

        // 确保缓冲区大小足够
        if (m_rgbBuffer.size() != dataSize)
        {
            m_rgbBuffer.resize(dataSize);
        }

        // 复制RGBA数据到缓冲区
        memcpy(m_rgbBuffer.data(), m_pRGBFrame->data[0], dataSize);

        // 发送帧数据给Qt显示
        emit SigFrameDataUpdated(m_rgbBuffer.data(), m_videoInfo.m_width, m_videoInfo.m_height);

        // 仅在有SDL纹理时更新SDL显示
        if (m_pTexture && m_pTexture->GetTexture())
        {
            void* pixels = nullptr;
            int pitch = 0;

            if (m_pTexture->LockTexture(nullptr, &pixels, &pitch))
            {
                uint8_t* src = m_pRGBFrame->data[0];
                auto dst = static_cast<uint8_t*>(pixels);

                for (int y = 0; y < m_videoInfo.m_height; y++)
                {
                    memcpy(dst, src, m_videoInfo.m_width * 4);
                    src += m_pRGBFrame->linesize[0];
                    dst += pitch;
                }

                m_pTexture->UnlockTexture();
            }
        }

        emit SigFrameUpdated();

        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        
        // 只在渲染耗时较长时记录
        if (totalDuration > 10000) // 大于10ms
        {
            LOG_DEBUG("Frame rendering took " + std::to_string(totalDuration) + " μs");
        }
    } catch (const std::exception& e)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        LOG_ERROR("Exception in RenderFrame after " + std::to_string(duration) + " μs: " + std::string(e.what()));
    }
}

int VideoPlayWorker::CalculateFrameDelay(int64_t pts)
{
    if (m_videoInfo.m_frameRate <= 0.0)
    {
        return 0;
    }

    // 计算每帧的理想时间间隔（微秒）
    int64_t frameInterval = static_cast<int64_t>(1000000.0 / m_videoInfo.m_frameRate);

    // 计算已经过去的时间（不包括暂停时间）
    int64_t currentRealTime = av_gettime_relative();
    int64_t elapsedTime = currentRealTime - m_startTime - m_totalPauseTime;

    // 计算期望的播放时间
    int64_t expectedTime = static_cast<int64_t>(m_currentTime * 1000000);

    // 计算需要延迟的时间
    int64_t delay = expectedTime - elapsedTime;

    // 限制延迟时间范围
    if (delay < 0)
    {
        return 0; // 已经落后，不需要延迟
    }
    else if (delay > frameInterval * 2)
    {
        return static_cast<int>(frameInterval / 1000); // 限制最大延迟
    }

    return static_cast<int>(delay / 1000); // 转换为毫秒
}
