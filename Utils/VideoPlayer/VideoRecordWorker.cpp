#include "VideoRecordWorker.h"
#include <QDebug>
#include <QThread>
#include "AVFileSystem.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"

VideoRecordWorker::VideoRecordWorker(QObject* parent)
    : QObject(parent)
{
    // 注册设备
    avdevice_register_all();
}

VideoRecordWorker::~VideoRecordWorker()
{
    LOG_INFO("VideoRecordWorker destructor called");
    Cleanup();
}

void VideoRecordWorker::SlotStartRecord(const QString& outputPath)
{
    QMutexLocker locker(&m_mutex);
    
    if (outputPath.isEmpty())
    {
        LOG_WARN("VideoRecordWorker::SlotStartRecord() : Invalid output file path");
        emit SigError("无效的输出文件路径");
        return;
    }

    // 设置录制状态为录制中
    m_outputPath = outputPath;
    m_bNeedStop.store(false);

    if (!InitRecorder(outputPath))
    {
        LOG_WARN("VideoRecordWorker::SlotStartRecord() : Failed to initialize recorder");
        emit SigError("录制器初始化失败");
        return;
    }

    // 启动录制循环
    RecordLoop();
}

void VideoRecordWorker::SlotStopRecord()
{
    QMutexLocker locker(&m_mutex);
    
    LOG_INFO("Stop record requested");
    m_bNeedStop.store(true);
    Cleanup();
}

bool VideoRecordWorker::InitRecorder(const QString& outputPath)
{
    LOG_INFO("Initializing video recorder for output: " + outputPath.toStdString());

    // 初始化输入设备（摄像头）
    QString deviceName = GetDefaultVideoDevice();
    if (deviceName.isEmpty())
    {
        LOG_WARN("No video input device found");
        return false;
    }

    LOG_INFO("Using video device: " + deviceName.toStdString());

    // 分配输入格式上下文
    m_pInputFormatCtx = std::make_unique<ST_AVFormatContext>();

    // 查找输入格式
    const AVInputFormat* inputFormat = av_find_input_format("dshow");
    if (!inputFormat)
    {
        LOG_WARN("Failed to find dshow input format");
        return false;
    }

    // 准备设备字符串 
    QString deviceString = QString("video=%1").arg(deviceName);

    // 打开输入设备
    if (!m_pInputFormatCtx->OpenInputFilePath(deviceString.toLocal8Bit().constData(), inputFormat))
    {
        LOG_WARN("Failed to open video input device");
        return false;
    }

    // 查找视频流
    m_videoStreamIndex = m_pInputFormatCtx->FindBestStream(AVMEDIA_TYPE_VIDEO);
    if (m_videoStreamIndex < 0)
    {
        LOG_WARN("No video stream found in input device");
        return false;
    }

    // 获取视频流参数
    AVStream* inputStream = m_pInputFormatCtx->GetRawContext()->streams[m_videoStreamIndex];
    AVCodecParameters* codecPar = inputStream->codecpar;

    // 查找输入解码器
    const AVCodec* inputCodec = avcodec_find_decoder(codecPar->codec_id);
    if (!inputCodec)
    {
        LOG_WARN("Input codec not found");
        return false;
    }

    // 创建输入解码器上下文
    m_pInputCodecCtx = std::make_unique<ST_AVCodecContext>(inputCodec);
    if (!m_pInputCodecCtx->BindParamToContext(codecPar))
    {
        LOG_WARN("Failed to copy input codec parameters");
        return false;
    }

    if (!m_pInputCodecCtx->OpenCodec(inputCodec))
    {
        LOG_WARN("Failed to open input codec");
        return false;
    }

    // 分配输出格式上下文
    m_pOutputFormatCtx = std::make_unique<ST_AVFormatContext>();
    QString extension = QString::fromStdString(my_sdk::FileSystem::GetExtension(outputPath.toStdString()));

    if (!m_pOutputFormatCtx->OpenOutputFilePath(nullptr, extension.toLocal8Bit().constData(), outputPath.toLocal8Bit().constData()))
    {
        LOG_WARN("Failed to create output format context");
        return false;
    }

    // 添加视频流到输出
    AVStream* outputStream = avformat_new_stream(m_pOutputFormatCtx->GetRawContext(), nullptr);
    if (!outputStream)
    {
        LOG_WARN("Failed to create output stream");
        return false;
    }

    // 查找输出编码器
    const AVCodec* outputCodec = nullptr;
    if (extension.toLower() == "mp4")
    {
        outputCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    else if (extension.toLower() == "avi")
    {
        outputCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    }
    else
    {
        outputCodec = avcodec_find_encoder(AV_CODEC_ID_H264); // 默认使用H264
    }

    if (!outputCodec)
    {
        LOG_WARN("Output codec not found");
        return false;
    }

    // 创建输出编码器上下文
    m_pOutputCodecCtx = std::make_unique<ST_AVCodecContext>(outputCodec);

    // 配置输出编码器参数
    AVCodecContext* outputCtx = m_pOutputCodecCtx->GetRawContext();
    outputCtx->width = codecPar->width;
    outputCtx->height = codecPar->height;
    outputCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    outputCtx->time_base = {1, 25}; // 25fps
    outputCtx->framerate = {25, 1};
    outputCtx->bit_rate = 400000;
    outputCtx->gop_size = 12;
    outputCtx->max_b_frames = 1;

    // 对于某些格式，需要设置全局头标志
    if (m_pOutputFormatCtx->GetRawContext()->oformat->flags & AVFMT_GLOBALHEADER)
    {
        outputCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // 打开输出编码器
    if (!m_pOutputCodecCtx->OpenCodec(outputCodec))
    {
        LOG_WARN("Failed to open output codec");
        return false;
    }

    // 将编码器参数复制到流
    if (avcodec_parameters_from_context(outputStream->codecpar, outputCtx) < 0)
    {
        LOG_WARN("Failed to copy output codec parameters to stream");
        return false;
    }

    // 写入输出文件头
    if (!m_pOutputFormatCtx->OpenIOFilePath(outputPath))
    {
        LOG_WARN("Failed to open output file");
        return false;
    }

    if (m_pOutputFormatCtx->WriteFileHeader(nullptr) < 0)
    {
        LOG_WARN("Failed to write output file header");
        return false;
    }

    LOG_INFO("Video recorder initialized successfully");
    return true;
}

void VideoRecordWorker::Cleanup()
{
    LOG_INFO("Cleaning up video recorder resources");

    // 写入文件尾
    if (m_pOutputFormatCtx && m_pOutputFormatCtx->GetRawContext())
    {
        m_pOutputFormatCtx->WriteFileTrailer();
    }

    // 重置所有资源
    m_pInputFormatCtx.reset();
    m_pOutputFormatCtx.reset();
    m_pInputCodecCtx.reset();
    m_pOutputCodecCtx.reset();

    LOG_INFO("Video recorder cleanup completed");
}

void VideoRecordWorker::RecordLoop()
{
    LOG_INFO("Video recording loop started");

    const int MAX_CONSECUTIVE_ERRORS = 10;
    int consecutiveErrors = 0;

    while (!m_bNeedStop.load())
    {
        // 读取输入包
        if (!m_pInputPacket.ReadPacket(m_pInputFormatCtx->GetRawContext()))
        {
            // 可能是设备断开或结束
            LOG_INFO("Failed to read packet from input device");
            break;
        }

        // 检查是否是视频流的包
        if (m_pInputPacket.GetStreamIndex() != m_videoStreamIndex)
        {
            m_pInputPacket.UnrefPacket();
            continue;
        }

        // 这里可以添加解码和重新编码的逻辑
        // 当前简化实现：直接复制包（适用于相同编码格式）

        // 调整时间戳
        AVStream* inputStream = m_pInputFormatCtx->GetRawContext()->streams[m_videoStreamIndex];
        AVStream* outputStream = m_pOutputFormatCtx->GetRawContext()->streams[0];

        av_packet_rescale_ts(m_pInputPacket.GetRawPacket(), inputStream->time_base, outputStream->time_base);
        m_pInputPacket.GetRawPacket()->stream_index = 0;

        // 写入输出包
        if (av_interleaved_write_frame(m_pOutputFormatCtx->GetRawContext(), m_pInputPacket.GetRawPacket()) < 0)
        {
            LOG_WARN("Failed to write output packet");
            consecutiveErrors++;
            if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
            {
                LOG_WARN("Too many consecutive write errors, stopping recording");
                break;
            }
        }
        else
        {
            consecutiveErrors = 0; // 重置错误计数
        }

        m_pInputPacket.UnrefPacket();

        // 短暂延迟避免CPU占用过高
        if (m_bNeedStop.load())
        {
            break;
        }
    }

    LOG_INFO("Video recording loop ended");
}

QString VideoRecordWorker::GetDefaultVideoDevice()
{
    // 这里应该实现设备枚举，当前返回默认设备名
    // 实际实现中可以调用系统API获取可用的摄像头设备列表
    return "0"; // 默认第一个摄像头设备
}
