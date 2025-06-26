#include "VideoRecordWorker.h"
#include <QMutexLocker>
#include <QThread>
#include "FFmpegPublicUtils.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "TimeSystem/TimeSystem.h"

VideoRecordWorker::VideoRecordWorker(QObject* parent)
    : QObject(parent), m_pInputFormatCtx(nullptr), m_pOutputFormatCtx(nullptr), m_pInputCodecCtx(nullptr), m_pOutputCodecCtx(nullptr), m_videoStreamIndex(-1), m_pInputPacket(nullptr), m_pOutputPacket(nullptr), m_bNeedStop(false), m_recordState(EM_VideoRecordState::Stopped)
{
    // 初始化FFmpeg包
    m_pInputPacket = av_packet_alloc();
    m_pOutputPacket = av_packet_alloc();

    // 注册设备
    avdevice_register_all();
}

VideoRecordWorker::~VideoRecordWorker()
{
    Cleanup();
}

void VideoRecordWorker::SlotStartRecord(const QString& outputPath)
{
    QMutexLocker locker(&m_mutex);

    if (m_recordState != EM_VideoRecordState::Stopped)
    {
        LOG_WARN("VideoRecordWorker::SlotStartRecord() : Recording already in progress");
        return;
    }

    m_outputPath = outputPath;

    if (!InitRecorder(outputPath))
    {
        LOG_WARN("VideoRecordWorker::SlotStartRecord() : Failed to initialize recorder");
        emit SigError("录制器初始化失败");
        return;
    }

    m_recordState = EM_VideoRecordState::Recording;
    m_bNeedStop = false;
    emit SigRecordStateChanged(m_recordState);

    // 启动录制循环
    RecordLoop();
}

void VideoRecordWorker::SlotStopRecord()
{
    QMutexLocker locker(&m_mutex);

    m_bNeedStop = true;
    m_recordState = EM_VideoRecordState::Stopped;
    emit SigRecordStateChanged(m_recordState);
}

bool VideoRecordWorker::InitRecorder(const QString& outputPath)
{
    AUTO_TIMER_LOG("VideoRecorderInit", EM_TimingLogLevel::Info);
    LOG_INFO("=== Initializing video recorder for: " + outputPath.toStdString() + " ===");
    
    try
    {
        // 获取默认视频设备
        TIME_START("VideoDeviceFind");
        QString deviceName = GetDefaultVideoDevice();
        if (deviceName.isEmpty())
        {
            LOG_ERROR("No video recording device available");
            return false;
        }
        TimeSystem::Instance().StopTimingWithLog("VideoDeviceFind", EM_TimingLogLevel::Info, EM_TimeUnit::Milliseconds, "Video recording device found: " + deviceName.toStdString());

        // 创建输入格式上下文（录制设备）
        TIME_START("VideoInputCtxInit");
        m_pInputFormatCtx = std::make_unique<ST_AVFormatContext>();

        // 查找输入格式
        const AVInputFormat* inputFormat = av_find_input_format("dshow"); // Windows设备
        if (!inputFormat)
        {
            LOG_WARN("Failed to find dshow input format");
            return false;
        }

        // 打开录制设备
        QString devicePath = QString("video=%1").arg(deviceName);
        int ret = m_pInputFormatCtx->OpenInputFilePath(devicePath.toUtf8().constData(), inputFormat, nullptr);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to open video device: " + std::string(errbuf));
            return false;
        }

        // 查找视频流
        m_videoStreamIndex = m_pInputFormatCtx->FindBestStream(AVMEDIA_TYPE_VIDEO);
        if (m_videoStreamIndex < 0)
        {
            LOG_WARN("No video stream found in input device");
            return false;
        }

        // 获取输入视频流信息
        AVFormatContext* inputCtx = m_pInputFormatCtx->GetRawContext();
        AVStream* inputStream = inputCtx->streams[m_videoStreamIndex];
        AVCodecParameters* inputCodecPar = inputStream->codecpar;

        TimeSystem::Instance().StopTimingWithLog("VideoInputCtxInit", EM_TimingLogLevel::Info);

        // 创建输出格式上下文
        TIME_START("VideoOutputCtxInit");
        m_pOutputFormatCtx = std::make_unique<ST_AVFormatContext>();
        QString format = QString::fromStdString(my_sdk::FileSystem::GetExtension(outputPath.toStdString()));
        ret = m_pOutputFormatCtx->OpenOutputFilePath(nullptr, format.toStdString().c_str(), outputPath.toUtf8().constData());
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to create output context: " + std::string(errbuf));
            return false;
        }

        // 创建输出视频流
        AVFormatContext* outputCtx = m_pOutputFormatCtx->GetRawContext();
        AVStream* outputStream = avformat_new_stream(outputCtx, nullptr);
        if (!outputStream)
        {
            LOG_WARN("Failed to create output stream");
            return false;
        }

        // 查找编码器
        const AVCodec* encoder = FFmpegPublicUtils::FindEncoder(format.toStdString().c_str());
        if (!encoder)
        {
            // 使用默认H.264编码器
            encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
            if (!encoder)
            {
                LOG_WARN("No suitable video encoder found");
                return false;
            }
        }

        // 创建编码器上下文
        m_pOutputCodecCtx = std::make_unique<ST_AVCodecContext>(encoder);
        if (!m_pOutputCodecCtx->GetRawContext())
        {
            LOG_WARN("Failed to allocate encoder context");
            return false;
        }

        // 配置编码器参数
        AVCodecContext* encoderCtx = m_pOutputCodecCtx->GetRawContext();
        encoderCtx->width = inputCodecPar->width;
        encoderCtx->height = inputCodecPar->height;
        encoderCtx->time_base = {1, 25}; // 25fps
        encoderCtx->framerate = {25, 1};
        encoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        encoderCtx->bit_rate = 400000; // 400kbps

        // 如果输出格式需要全局头部
        if (outputCtx->oformat->flags & AVFMT_GLOBALHEADER)
        {
            encoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        // 打开编码器
        ret = m_pOutputCodecCtx->OpenCodec(encoder);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to open encoder: " + std::string(errbuf));
            return false;
        }

        // 复制编码器参数到流
        ret = avcodec_parameters_from_context(outputStream->codecpar, encoderCtx);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to copy encoder parameters: " + std::string(errbuf));
            return false;
        }

        outputStream->time_base = encoderCtx->time_base;

        // 打开输出文件
        if (!(outputCtx->oformat->flags & AVFMT_NOFILE))
        {
            ret = avio_open(&outputCtx->pb, outputPath.toUtf8().constData(), AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                LOG_WARN("Could not open output file: " + std::string(errbuf));
                return false;
            }
        }

        // 写入文件头
        ret = m_pOutputFormatCtx->WriteFileHeader(nullptr);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to write file header: " + std::string(errbuf));
            return false;
        }

        TimeSystem::Instance().StopTimingWithLog("VideoOutputCtxInit", EM_TimingLogLevel::Info);

        LOG_INFO("=== Video recorder initialized successfully ===");
        return true;
    } catch (const std::exception& e)
    {
        LOG_ERROR("Exception in InitRecorder: " + std::string(e.what()));
        return false;
    }
}

void VideoRecordWorker::Cleanup()
{
    m_bNeedStop = true;

    // 等待录制循环结束
    TIME_SLEEP_MS(100);

    // 释放FFmpeg资源
    if (m_pOutputPacket)
    {
        av_packet_free(&m_pOutputPacket);
    }

    if (m_pInputPacket)
    {
        av_packet_free(&m_pInputPacket);
    }

    m_pOutputCodecCtx.reset();
    m_pInputCodecCtx.reset();

    if (m_pOutputFormatCtx && m_pOutputFormatCtx->GetRawContext())
    {
        // 写入文件尾
        m_pOutputFormatCtx->WriteFileTrailer();

        // 关闭输出文件
        AVFormatContext* outputCtx = m_pOutputFormatCtx->GetRawContext();
        if (outputCtx->pb && !(outputCtx->oformat->flags & AVFMT_NOFILE))
        {
            avio_closep(&outputCtx->pb);
        }
    }

    m_pOutputFormatCtx.reset();
    m_pInputFormatCtx.reset();
}

void VideoRecordWorker::RecordLoop()
{
    AUTO_TIMER_LOG("VideoRecordLoop", EM_TimingLogLevel::Info);
    LOG_INFO("=== Video recording loop started ===");

    int frameCount = 0;
    const int maxFrames = 1500; // 录制约60秒（25fps）
    auto lastLogTime = TimeSystem::Instance().GetCurrentTimePoint();

    while (!m_bNeedStop && frameCount < maxFrames)
    {
        // 每录制50帧记录一次进度
        if (frameCount % 50 == 0 && frameCount > 0)
        {
            auto currentTime = TimeSystem::Instance().GetCurrentTimePoint();
            double duration = TimeSystem::Instance().CalculateTimeDifference(lastLogTime, currentTime, EM_TimeUnit::Milliseconds);
            double fps = frameCount > 0 ? (50.0 * 1000.0 / duration) : 0.0;
            LOG_INFO("Recorded " + std::to_string(frameCount) + " frames, current FPS: " + std::to_string(fps) + " (last 50 frames in " + std::to_string(duration) + " ms)");
            lastLogTime = currentTime;
        }

        // 读取输入数据包
        auto readStartTime = TimeSystem::Instance().GetCurrentTimePoint();
        int ret = av_read_frame(m_pInputFormatCtx->GetRawContext(), m_pInputPacket);
        auto readEndTime = TimeSystem::Instance().GetCurrentTimePoint();
        
        double readDuration = TimeSystem::Instance().CalculateTimeDifference(readStartTime, readEndTime, EM_TimeUnit::Microseconds);
        
        if (ret < 0)
        {
            if (ret == AVERROR(EAGAIN))
            {
                TIME_SLEEP_MS(10);
                continue;
            }
            else
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                LOG_WARN("Error reading input frame after " + std::to_string(readDuration) + " μs: " + std::string(errbuf));
                break;
            }
        }

        // 只在读取耗时较长时记录
        if (readDuration > 1000) // 大于1ms
        {
            LOG_DEBUG("Frame read took " + std::to_string(readDuration) + " μs");
        }

        // 只处理视频流的数据包
        if (m_pInputPacket->stream_index == m_videoStreamIndex)
        {
            auto writeStartTime = TimeSystem::Instance().GetCurrentTimePoint();
            
            // 重新计算时间戳
            AVStream* inputStream = m_pInputFormatCtx->GetRawContext()->streams[m_videoStreamIndex];
            AVStream* outputStream = m_pOutputFormatCtx->GetRawContext()->streams[0];

            av_packet_rescale_ts(m_pInputPacket, inputStream->time_base, outputStream->time_base);
            m_pInputPacket->stream_index = 0;

            // 写入输出文件
            ret = av_interleaved_write_frame(m_pOutputFormatCtx->GetRawContext(), m_pInputPacket);
            auto writeEndTime = TimeSystem::Instance().GetCurrentTimePoint();
            double writeDuration = TimeSystem::Instance().CalculateTimeDifference(writeStartTime, writeEndTime, EM_TimeUnit::Microseconds);
            
            if (ret < 0)
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                LOG_WARN("Error writing output frame after " + std::to_string(writeDuration) + " μs: " + std::string(errbuf));
            }
            else
            {
                frameCount++;
                // 只在写入耗时较长时记录
                if (writeDuration > 5000) // 大于5ms
                {
                    LOG_DEBUG("Frame write took " + std::to_string(writeDuration) + " μs");
                }
            }
        }

        av_packet_unref(m_pInputPacket);
        TIME_SLEEP_MS(40); // 约25fps
    }

    LOG_INFO("=== Video recording loop finished, recorded " + std::to_string(frameCount) + " frames ===");

    // 录制完成
    m_recordState = EM_VideoRecordState::Stopped;
    emit SigRecordStateChanged(m_recordState);
}

QString VideoRecordWorker::GetDefaultVideoDevice()
{
    // 获取默认视频录制设备
    const AVInputFormat* inputFormat = av_find_input_format("dshow");
    if (!inputFormat)
    {
        LOG_WARN("Failed to find dshow input format");
        return QString();
    }

    AVDeviceInfoList* deviceList = nullptr;
    int ret = avdevice_list_input_sources(inputFormat, nullptr, nullptr, &deviceList);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to get input devices: " + std::string(errbuf));
        return QString();
    }

    QString deviceName;
    for (int i = 0; i < deviceList->nb_devices; i++)
    {
        if (*deviceList->devices[i]->media_types == AVMEDIA_TYPE_VIDEO)
        {
            deviceName = QString::fromUtf8(deviceList->devices[i]->device_description);
            break;
        }
    }

    avdevice_free_list_devices(&deviceList);
    return deviceName;
}
