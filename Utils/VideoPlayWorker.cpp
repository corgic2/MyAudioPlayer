#include "VideoPlayWorker.h"
#include <QMutexLocker>
#include <QThread>
#include "FFmpegPublicUtils.h"
#include "BaseDataDefine/ST_AVCodec.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVFormatContext.h"
#include "BaseDataDefine/ST_Buffer.h"
#include "DataDefine/ST_ResampleResult.h"
#include "LogSystem/LogSystem.h"
#include "TimeSystem/TimeSystem.h"

VideoPlayWorker::VideoPlayWorker(QObject* parent)
    : QObject(parent), m_pFormatCtx(nullptr), m_pVideoCodecCtx(nullptr), m_pAudioCodecCtx(nullptr), m_videoStreamIndex(-1), m_audioStreamIndex(-1), m_pSwsCtx(nullptr), m_pRenderer(nullptr), m_pTexture(nullptr), m_pAudioPlayInfo(nullptr), m_playState(EM_VideoPlayState::Stopped), m_bNeedStop(false), m_startTime(0), m_pauseStartTime(0), m_totalPauseTime(0), m_currentTime(0.0), m_bSeekRequested(false), m_seekTarget(0.0), m_bHasAudio(false), m_pPlayThread(nullptr), m_bSDLInitialized(false), m_audioDeviceId(0), m_pInputChannelLayout(nullptr), m_pOutputChannelLayout(nullptr), m_frameCount(0), m_lastProcessTime(0), m_bAudioResampleParamsUpdated(false)
{
    InitSDLSystem();
}

VideoPlayWorker::~VideoPlayWorker()
{
    LOG_INFO("VideoPlayWorker destructor called");
    Cleanup();
    CleanupSDLSystem();
}

bool VideoPlayWorker::InitSDLSystem()
{
    if (m_bSDLInitialized.load())
    {
        return true;
    }

    // 初始化SDL音频子系统
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        LOG_WARN("Failed to initialize SDL audio: " + std::string(SDL_GetError()));
        return false;
    }

    m_bSDLInitialized.store(true);
    LOG_INFO("SDL audio system initialized successfully");
    return true;
}

void VideoPlayWorker::CleanupSDLSystem()
{
    if (m_bSDLInitialized.load())
    {
        LOG_INFO("Cleaning up SDL system");
        SDL_Quit();
        m_bSDLInitialized.store(false);
    }
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

bool VideoPlayWorker::InitAudioSystem()
{
    try
    {
        LOG_INFO("Initializing audio system");

        // 获取音频流
        AVStream* audioStream = m_pFormatCtx->GetRawContext()->streams[m_audioStreamIndex];
        AVCodecParameters* codecpar = audioStream->codecpar;

        // 查找音频解码器
        const AVCodec* audioCodec = avcodec_find_decoder(codecpar->codec_id);
        if (!audioCodec)
        {
            LOG_WARN("Audio codec not found");
            return false;
        }

        // 创建音频解码器上下文
        m_pAudioCodecCtx = std::make_unique<ST_AVCodecContext>(audioCodec);
        if (!m_pAudioCodecCtx->GetRawContext())
        {
            LOG_WARN("Failed to allocate audio codec context");
            return false;
        }

        // 复制编解码器参数
        if (avcodec_parameters_to_context(m_pAudioCodecCtx->GetRawContext(), codecpar) < 0)
        {
            LOG_WARN("Failed to copy audio codec parameters");
            return false;
        }

        // 打开音频解码器
        if (!m_pAudioCodecCtx->OpenCodec(audioCodec, nullptr))
        {
            LOG_WARN("Failed to open audio codec");
            return false;
        }

        // 验证音频参数
        if (codecpar->sample_rate <= 0 || codecpar->sample_rate > 192000)
        {
            LOG_ERROR("Invalid sample rate: " + std::to_string(codecpar->sample_rate));
            return false;
        }

        if (codecpar->ch_layout.nb_channels <= 0 || codecpar->ch_layout.nb_channels > 8)
        {
            LOG_ERROR("Invalid channel count: " + std::to_string(codecpar->ch_layout.nb_channels));
            return false;
        }

        // 安全创建通道布局
        m_pInputChannelLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        m_pOutputChannelLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));

        if (!m_pInputChannelLayout || !m_pOutputChannelLayout)
        {
            LOG_ERROR("Failed to allocate channel layouts");
            SafeFreeChannelLayout(&m_pInputChannelLayout);
            SafeFreeChannelLayout(&m_pOutputChannelLayout);
            return false;
        }

        // === 修复：从解码器上下文获取正确的音频格式 ===
        AVCodecContext* codecCtx = m_pAudioCodecCtx->GetRawContext();
        
        // 设置输入参数（优先使用解码器上下文的参数）
        int inputSampleRate = (codecCtx->sample_rate > 0) ? codecCtx->sample_rate : codecpar->sample_rate;
        AVSampleFormat inputFormat = (codecCtx->sample_fmt != AV_SAMPLE_FMT_NONE) ? codecCtx->sample_fmt : static_cast<AVSampleFormat>(codecpar->format);
        
        // 如果格式仍然是NONE，则设置一个默认值，稍后在处理第一帧时更新
        if (inputFormat == AV_SAMPLE_FMT_NONE)
        {
            LOG_WARN("Audio format is AV_SAMPLE_FMT_NONE, will update from first decoded frame");
            inputFormat = AV_SAMPLE_FMT_FLTP; // 设置一个常见的默认格式
        }
        
        m_resampleParams.GetInput().SetSampleRate(inputSampleRate);
        m_resampleParams.GetInput().SetSampleFormat(ST_AVSampleFormat(inputFormat));

        // 优先使用解码器上下文的通道布局
        if (codecCtx->ch_layout.nb_channels > 0)
        {
            av_channel_layout_copy(m_pInputChannelLayout, &codecCtx->ch_layout);
        }
        else
        {
            av_channel_layout_copy(m_pInputChannelLayout, &codecpar->ch_layout);
        }
        m_resampleParams.GetInput().SetChannelLayout(ST_AVChannelLayout(m_pInputChannelLayout));

        // 设置输出参数
        m_resampleParams.GetOutput().SetSampleRate(44100);
        m_resampleParams.GetOutput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16));

        av_channel_layout_default(m_pOutputChannelLayout, 2);
        m_resampleParams.GetOutput().SetChannelLayout(ST_AVChannelLayout(m_pOutputChannelLayout));

        // 配置SDL音频规格
        SDL_AudioSpec wantedSpec;
        memset(&wantedSpec, 0, sizeof(wantedSpec));
        wantedSpec.freq = 44100;
        wantedSpec.format = SDL_AUDIO_S16;
        wantedSpec.channels = 2;

        LOG_INFO("Opening SDL audio device with spec - Freq: " + std::to_string(wantedSpec.freq) + ", Format: " + std::to_string(wantedSpec.format) + ", Channels: " + std::to_string(wantedSpec.channels));
        LOG_INFO("Audio parameters - Input: " + std::to_string(inputSampleRate) + "Hz, " + std::to_string(codecpar->ch_layout.nb_channels) + " channels, format: " + std::to_string(static_cast<int>(inputFormat)));

        // 创建音频播放信息对象
        m_pAudioPlayInfo = std::make_unique<ST_AudioPlayInfo>();
        m_pAudioPlayInfo->InitAudioSpec(true, static_cast<int>(wantedSpec.freq), wantedSpec.format, static_cast<int>(wantedSpec.channels));
        m_pAudioPlayInfo->InitAudioSpec(false, static_cast<int>(wantedSpec.freq), wantedSpec.format, static_cast<int>(wantedSpec.channels));
        m_pAudioPlayInfo->InitAudioStream();

        // 打开SDL音频设备
        m_audioDeviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wantedSpec);
        if (m_audioDeviceId == 0)
        {
            LOG_WARN("Failed to open audio device: " + std::string(SDL_GetError()));
            return false;
        }

        m_pAudioPlayInfo->InitAudioDevice(m_audioDeviceId);
        m_pAudioPlayInfo->BindStreamAndDevice();
        m_pAudioPlayInfo->BeginPlayAudio();

        // 重置动态更新标志
        m_bAudioResampleParamsUpdated.store(false);

        LOG_INFO("Audio system initialized successfully");
        return true;
    } catch (const std::exception& e)
    {
        LOG_ERROR("Exception in InitAudioSystem: " + std::string(e.what()));
        return false;
    }
}

void VideoPlayWorker::CleanupAudioSystem()
{
    LOG_INFO("Cleaning up audio system");

    // 停止音频播放
    if (m_pAudioPlayInfo)
    {
        m_pAudioPlayInfo->StopAudio();
        m_pAudioPlayInfo->UnbindStreamAndDevice();
        m_pAudioPlayInfo.reset();
    }

    // 关闭音频设备
    if (m_audioDeviceId != 0)
    {
        SDL_CloseAudioDevice(m_audioDeviceId);
        m_audioDeviceId = 0;
    }

    // 释放通道布局内存
    SafeFreeChannelLayout(&m_pInputChannelLayout);
    SafeFreeChannelLayout(&m_pOutputChannelLayout);

    // 重置解码器
    m_pAudioCodecCtx.reset();

    LOG_INFO("Audio system cleanup completed");
}

void VideoPlayWorker::SafeStopPlayThread()
{
    // 设置停止标志
    m_bNeedStop.store(true);

    // 唤醒可能等待的线程
    m_threadWaitCondition.wakeAll();

    // 等待线程结束
    if (m_pPlayThread && m_pPlayThread->isRunning())
    {
        LOG_INFO("Waiting for play thread to finish...");
        if (!m_pPlayThread->wait(200)) // 等待3秒
        {
            LOG_WARN("Play thread did not finish gracefully, forcing termination");
            m_pPlayThread->terminate();
            m_pPlayThread->wait(200); // 再等待1秒
        }
    }

    m_pPlayThread.reset();
    LOG_INFO("Play thread stopped safely");
}

void VideoPlayWorker::Cleanup()
{
    LOG_INFO("Cleaning up video player resources");

    // 安全停止播放线程
    SafeStopPlayThread();

    // 清理音频系统
    CleanupAudioSystem();

    // 清理视频相关资源
    if (m_pSwsCtx)
    {
        sws_freeContext(m_pSwsCtx);
        m_pSwsCtx = nullptr;
    }

    m_pVideoCodecCtx.reset();
    m_pFormatCtx.reset();

    // 清理缓冲区
    m_rgbBuffer.clear();
    m_rgbBuffer.shrink_to_fit();

    LOG_INFO("Video player cleanup completed");
}

void VideoPlayWorker::SlotStartPlay()
{
    QMutexLocker locker(&m_mutex);

    if (m_playState.load() == EM_VideoPlayState::Playing)
    {
        return;
    }

    // 确保之前的线程已经停止
    SafeStopPlayThread();

    m_bNeedStop.store(false);
    m_startTime = av_gettime();
    m_totalPauseTime = 0;
    m_currentTime = 0.0;
    m_frameCount.store(0);
    m_lastProcessTime.store(av_gettime());
    
    // 重置音频重采样参数更新标志
    m_bAudioResampleParamsUpdated.store(false);

    m_playState.store(EM_VideoPlayState::Playing);
    emit SigPlayStateChanged(m_playState.load());

    // 创建新的播放线程
    m_pPlayThread = std::make_unique<QThread>();

    // 将播放循环移到线程中
    connect(m_pPlayThread.get(), &QThread::started, [this]()
    {
        PlayLoop();
    });

    connect(m_pPlayThread.get(), &QThread::finished, [this]()
    {
        LOG_INFO("Play thread finished");
    });

    m_pPlayThread->start();
    LOG_INFO("Play thread started");
}

void VideoPlayWorker::SlotStopPlay()
{
    LOG_INFO("Stop play requested");
    SafeStopPlayThread();
    m_playState.store(EM_VideoPlayState::Stopped);
    emit SigPlayStateChanged(m_playState.load());
}

void VideoPlayWorker::PlayLoop()
{
    LOG_INFO("Video playback loop started");

    const int MAX_CONSECUTIVE_ERRORS = 10;
    int consecutiveErrors = 0;

    try
    {
        while (!m_bNeedStop.load() && m_playState.load() != EM_VideoPlayState::Stopped)
        {
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

            // 暂停处理
            if (m_playState.load() == EM_VideoPlayState::Paused)
            {
                if (m_pauseStartTime == 0)
                {
                    m_pauseStartTime = av_gettime();
                }

                // 暂停时使用条件等待，避免忙等待
                QMutexLocker locker(&m_mutex);
                m_threadWaitCondition.wait(&m_mutex, 50); // 最多等待50ms
                continue;
            }
            else if (m_pauseStartTime != 0)
            {
                // 从暂停状态恢复
                m_totalPauseTime += av_gettime() - m_pauseStartTime;
                m_pauseStartTime = 0;
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
                    emit SigFrameUpdated();
                    consecutiveErrors = 0; // 重置错误计数
                }
            }
            // 处理音频包
            else if (m_bHasAudio && m_pPacket.GetStreamIndex() == m_audioStreamIndex)
            {
                if (DecodeAudioFrame())
                {
                    frameProcessed = true;
                    consecutiveErrors = 0; // 重置错误计数
                }
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

            // 更新播放进度（减少频率）
            m_frameCount.fetch_add(1);
            int64_t currentTime = av_gettime();
            if (currentTime - m_lastProcessTime.load() > 100000) // 每100ms更新一次
            {
                emit SigPlayProgressUpdated(m_currentTime, m_videoInfo.m_duration);
                m_lastProcessTime.store(currentTime);
            }

            // 检查是否需要停止
            if (m_bNeedStop.load())
            {
                break;
            }
        }
    } catch (const std::exception& e)
    {
        LOG_ERROR("Exception in PlayLoop: " + std::string(e.what()));
    } catch (...)
    {
        LOG_ERROR("Unknown exception in PlayLoop");
    }

    LOG_INFO("Video playback loop ended, processed " + std::to_string(m_frameCount.load()) + " frames");
}

void VideoPlayWorker::ProcessAudioFrame(AVFrame* frame)
{
    if (!frame || !m_pAudioPlayInfo)
    {
        return;
    }

    // 验证音频帧
    if (frame->nb_samples <= 0 || frame->nb_samples > 8192)
    {
        LOG_WARN("Invalid sample count: " + std::to_string(frame->nb_samples));
        return;
    }

    if (frame->ch_layout.nb_channels <= 0 || frame->ch_layout.nb_channels > 8)
    {
        LOG_WARN("Invalid channel count: " + std::to_string(frame->ch_layout.nb_channels));
        return;
    }

    try
    {
        // === 修复：在第一次获取解码帧时更新重采样参数 ===
        if (!m_bAudioResampleParamsUpdated.load())
        {
            // 从实际解码的帧获取正确的音频格式
            AVSampleFormat actualFormat = static_cast<AVSampleFormat>(frame->format);
            int actualSampleRate = frame->sample_rate;
            
            LOG_INFO("Updating video audio resample params from first decoded frame:");
            LOG_INFO("  Actual format: " + std::to_string(static_cast<int>(actualFormat)) + " (" + std::string(av_get_sample_fmt_name(actualFormat)) + ")");
            LOG_INFO("  Actual sample rate: " + std::to_string(actualSampleRate));
            LOG_INFO("  Actual channels: " + std::to_string(frame->ch_layout.nb_channels));
            
            // 更新重采样参数
            m_resampleParams.GetInput().SetSampleFormat(ST_AVSampleFormat(actualFormat));
            if (actualSampleRate > 0)
            {
                m_resampleParams.GetInput().SetSampleRate(actualSampleRate);
            }
            
            // 更新通道布局
            if (frame->ch_layout.nb_channels > 0)
            {
                // 安全释放之前的输入通道布局
                SafeFreeChannelLayout(&m_pInputChannelLayout);
                
                // 重新分配并复制通道布局
                m_pInputChannelLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
                if (m_pInputChannelLayout)
                {
                    av_channel_layout_copy(m_pInputChannelLayout, &frame->ch_layout);
                    m_resampleParams.GetInput().SetChannelLayout(ST_AVChannelLayout(m_pInputChannelLayout));
                }
            }
            
            m_bAudioResampleParamsUpdated.store(true);
        }

        // 准备输入数据指针数组
        const uint8_t* inputDataPtrs[AV_NUM_DATA_POINTERS] = {0};

        // 检查是否为平面格式
        bool isPlanar = av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame->format));
        int channels = frame->ch_layout.nb_channels;

        if (isPlanar)
        {
            // 平面格式：每个通道分别存储
            for (int ch = 0; ch < channels && ch < AV_NUM_DATA_POINTERS; ch++)
            {
                inputDataPtrs[ch] = frame->data[ch];
            }
        }
        else
        {
            // 交错格式：所有通道数据交错存储
            inputDataPtrs[0] = frame->data[0];
        }

        // 执行重采样
        ST_ResampleResult resampleResult;
        m_audioResampler.Resample(inputDataPtrs, frame->nb_samples, resampleResult, m_resampleParams);

        // 将重采样后的数据发送到音频设备
        if (!resampleResult.GetData().empty())
        {
            int dataSize = static_cast<int>(resampleResult.GetData().size());

            // 检查音频缓冲区是否过满，避免累积延迟
            // 这里可以添加缓冲区管理逻辑
            m_pAudioPlayInfo->PutDataToStream(resampleResult.GetData().data(), dataSize);
        }
        else
        {
            // 如果重采样失败，记录但不中断播放
            static int resampleFailCount = 0;
            if (++resampleFailCount % 100 == 0) // 每100次失败记录一次
            {
                LOG_WARN("Video audio resample failed " + std::to_string(resampleFailCount) + " times");
            }
        }
    } catch (const std::exception& e)
    {
        LOG_ERROR("Exception in ProcessAudioFrame: " + std::string(e.what()));
    } catch (...)
    {
        LOG_ERROR("Unknown exception in ProcessAudioFrame");
    }
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
    AV_AUTO_TIMER("VideoPlayer", "InitPlayer");
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

        // 如果有音频流，初始化音频解码器
        if (m_bHasAudio)
        {
            if (!InitAudioSystem())
            {
                LOG_WARN("Failed to initialize audio system, continuing without audio");
                m_bHasAudio = false;
            }
            else
            {
                LOG_INFO("Audio system initialized successfully");
            }
        }

        LOG_INFO("=== Video player initialized successfully ===");
        LOG_INFO("Video info - Width: " + std::to_string(m_videoInfo.m_width) + ", Height: " + std::to_string(m_videoInfo.m_height) + ", FPS: " + std::to_string(m_videoInfo.m_frameRate) + ", Duration: " + std::to_string(m_videoInfo.m_duration));

        return true;
    } catch (const std::exception& e)
    {
        LOG_ERROR("Exception in InitPlayer: " + std::string(e.what()));
        return false;
    }
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

bool VideoPlayWorker::DecodeAudioFrame()
{
    if (!m_pAudioCodecCtx || !m_bHasAudio)
    {
        return false;
    }

    // 发送数据包到解码器
    if (!m_pPacket.SendPacket(m_pAudioCodecCtx->GetRawContext()))
    {
        return false;
    }

    // 接收解码后的音频帧
    while (m_pAudioFrame.GetCodecFrame(m_pAudioCodecCtx->GetRawContext()))
    {
        ProcessAudioFrame(m_pAudioFrame.GetRawFrame());
    }

    return true;
}

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
