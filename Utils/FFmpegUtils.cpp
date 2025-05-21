#include "FFmpegUtils.h"
#include <QFile>
#include "FFmpegPublicUtils.h"
#include "../include/SDL3/SDL.h"
#include "libavdevice/avdevice.h"
#define FMT_NAME "dshow"
#define DEVICE_INPUTMICRONAME "audio=麦克风 (2- USB Audio Device)"
#define DEVICE_OUTPUTMICRONAME "audio=扬声器 (2- USB Audio Device)"
#pragma execution_character_set("utf-8")
//根据不同设备进行修改，此电脑为USB音频设备
FFmpegUtils::FFmpegUtils(QObject *parent)
    : QObject(parent)
{
}

FFmpegUtils::~FFmpegUtils()
{}

void FFmpegUtils::EncoderOrDecoder(const QString& inputFilePath, const QString& outputFilePath, bool bEncoder, const QStringList& args)
{
}

QString FFmpegUtils::GetFileInfomation(const QString& inputFilePath, const QStringList& args)
{
    return QString();
}

void FFmpegUtils::ResigsterDevice()
{
    avdevice_register_all();
    if (SDL_Init(SDL_INIT_AUDIO))
    {
        qDebug() << "SDL_Init failed" << SDL_GetError();
    }
}

ST_OpenAudioDevice FFmpegUtils::OpenDevice(const QString& devieceFormat, const QString& deviceName)
{
    ST_OpenAudioDevice openDeviceParam;
    openDeviceParam.m_pInputFormatCtx.FindInputFormat(devieceFormat);
    if (!openDeviceParam.m_pInputFormatCtx.m_pInputFormatCtx)
    {
        qDebug() << "not find InputDevice";
        return openDeviceParam;
    }
    QByteArray utf8DeviceName = QString(deviceName).toUtf8();
    int ret = openDeviceParam.m_pFormatCtx.OpenInputFilePath(utf8DeviceName.constData(), openDeviceParam.m_pInputFormatCtx.m_pInputFormatCtx, nullptr); // 获取输入设备上下文
    if (ret < 0)
    {
        char errbuf[1024] = {0};
        qWarning() << "open Device failed";
        av_strerror(ret, errbuf, sizeof(errbuf));
        return openDeviceParam;
    }
    return openDeviceParam;
}

void FFmpegUtils::StartAudioRecording(const QString& outputFilePath, const QString& encoderFormat)
{
    ST_OpenAudioDevice openAudioDevice = OpenDevice(FMT_NAME, DEVICE_INPUTMICRONAME);

    ST_AVFormatContext outputFormatCtx;
    outputFormatCtx.OpenOutputFilePath(nullptr, encoderFormat.toStdString().c_str(), outputFilePath.toUtf8().constData()); // 输出格式上下文初始化
    if (!outputFormatCtx.m_pFormatCtx)
    {
        qWarning() << "Failed to create output context";
        return;
    }

    // 创建音频流
    AVStream* outStream = avformat_new_stream(outputFormatCtx.m_pFormatCtx, nullptr);
    if (!outStream)
    {
        qWarning() << "Failed to create output stream";
        return;
    }

    // 配置音频编码参数（需与输入设备参数匹配）
    ST_AVCodec codec(FFmpegPublicUtils::FindEncoder(encoderFormat.toStdString().c_str()));
    ST_AVCodecContext encCtx(codec.m_pAvCodec);
    ST_AVCodecParameters codecPar(outStream->codecpar);
    codecPar.m_codecParameters->codec_id = codec.m_pAvCodec->id;
    codecPar.m_codecParameters->codec_type = AVMEDIA_TYPE_AUDIO;
    codecPar.m_codecParameters->ch_layout = AV_CHANNEL_LAYOUT_STEREO; // 立体声
    FFmpegPublicUtils::ConfigureEncoderParams(codecPar.m_codecParameters, encCtx.m_pCodecContext);
    int ret = 0;
    // 打开输出文件
    if (!(outputFormatCtx.m_pFormatCtx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&outputFormatCtx.m_pFormatCtx->pb, outputFilePath.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            char errbuf[1024];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qWarning() << "Could not open output file:" << errbuf;
            return;
        }
    }

    // 写入文件头
    ret = outputFormatCtx.WriteFileHeader(nullptr);
    if (ret < 0)
    {
        qWarning() << "Error writing header";
        return;
    }
    ST_AVPacket pkt;
    int count = 50;
    while (count-- > 0)
    {
        ret = pkt.ReadPacket(openAudioDevice.m_pFormatCtx.m_pFormatCtx);
        if (ret == 0)
        {
            // 直接复用原始数据包（假设输入格式与输出格式匹配）
            av_packet_rescale_ts(pkt.m_pkt, openAudioDevice.m_pFormatCtx.m_pFormatCtx->streams[0]->time_base, outStream->time_base);
            pkt.m_pkt->stream_index = outStream->index;
            av_interleaved_write_frame(outputFormatCtx.m_pFormatCtx, pkt.m_pkt);
            pkt.UnrefPacket();
        }
        else if (ret == AVERROR(EAGAIN))
        {
            continue;
        }
        else
        {
            break;
        }
    }

    // 写入文件尾
    outputFormatCtx.WriteFileTrailer();
}

void FFmpegUtils::StartAudioPlayback(const QString& inputFilePath, const QStringList& args)
{
    // 播放音频文件
    ST_AVFormatContext formatCtx;
    int ret = formatCtx.OpenInputFilePath(inputFilePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0)
    {
        qWarning() << "Failed to open input file";
        return;
    }
    int audioStreamIdx = formatCtx.FindBestStream(AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIdx < 0)
    {
        qWarning() << "未找到音频流";
        return;
    }
    // 初始化解码器
    ST_AVCodecParameters codecParams(formatCtx.m_pFormatCtx->streams[audioStreamIdx]->codecpar);
    ST_AVCodec codec(codecParams.GetDeviceId());
    ST_AVCodecContext codeCtx(codec.m_pAvCodec);
    codeCtx.BindParamToContext(codecParams.m_codecParameters);
    codeCtx.OpenCodec(codec.m_pAvCodec, nullptr);
    ST_AudioPlayInfo playInfo;
    playInfo.InitAudioSpec(true, codecParams.m_codecParameters->sample_rate, codecParams.GetSampleFormat(), codecParams.m_codecParameters->ch_layout.nb_channels);
    playInfo.InitAudioSpec(false, playInfo.GetAudioSpec(true).freq, (AVSampleFormat)SDL_AUDIO_S16, playInfo.GetAudioSpec(true).channels);
    playInfo.InitAudioStream();
    playInfo.InitAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK);
    // 打开音频设备
    if (playInfo.GetDeviceId().m_audioDevice == 0)
    {
        qWarning() << "Failed to open audio device";
        return;
    }
    // 绑定流和设备
    playInfo.BindStreamAndDevice();
    playInfo.BeginPlayAudio(); // 开始播放
    ST_AVPacket pkt;
    while (pkt.ReadPacket(formatCtx.m_pFormatCtx) >= 0)
    {
        if (pkt.m_pkt->stream_index == audioStreamIdx)
        {
            pkt.SendPacket(codeCtx.m_pCodecContext);
            // 播放音频数据
            playInfo.PutDataToStream(pkt.m_pkt->data, pkt.m_pkt->size);
        }
        pkt.UnrefPacket();
    }
    // 等待音频播放完成
    while (playInfo.GetDataIsEnd() > 0)
    {
        playInfo.Delay(100);
    }
}
// 从AVFormatContext中获取录音设备的相关参数
void FFmpegUtils::ShowSpec(AVFormatContext* ctx)
{
    if (!ctx)
    {
        qWarning() << "ShowSpec() : AVformatContext ctx is nullptr";
        return;
    }
    // 获取输入流
    AVStream* stream = ctx->streams[0];
    // 获取音频参数
    ST_AVCodecParameters params(stream->codecpar);
    // 声道数
    qDebug() << "channels: " << params.m_codecParameters->ch_layout.nb_channels;
    // 采样率
    qDebug() << "samplerate: " << params.m_codecParameters->sample_rate;
    // 采样格式
    qDebug() << "format: " << params.m_codecParameters->format;
    // 每一个样本的一个声道占用多少个字节
    qDebug() << "per sample bytes: " << params.GetSamplePerRate();
    // 编码ID（可以看出采样格式）
    qDebug() << "codecid: " << params.m_codecParameters->codec_id;
    // 每一个样本的一个声道占用多少位（这个函数需要用到avcodec库）
    qDebug() << "per sample bits: " << params.GetBitPerSample();
}

#if 0
void FFmpegUtils::ResampleAudio(ST_ResampleAudioData* input, ST_ResampleAudioData* output)
{
    if (nullptr == input || nullptr == output)
    {
        return;
    }

    // 初始化重采样上下文
    swr_alloc_set_opts2(&input->pSwrCtx, &output->pCodecCtx->ch_layout, (AVSampleFormat)output->pCodecCtx->sample_fmt, output->pCodecCtx->sample_rate, &input->pCodecCtx->ch_layout, (AVSampleFormat)input->pCodecCtx->sample_fmt, input->pCodecCtx->sample_rate, 0, nullptr);

    if (!input->pSwrCtx)
    {
        qWarning() << "Failed to allocate SwrContext";
        return;
    }

    if (swr_init(input->pSwrCtx) < 0)
    {
        qWarning() << "Failed to initialize SwrContext";
        return;
    }

    // 重采样
    int ret = swr_convert(input->pSwrCtx, output->pFrame->data, output->pFrame->nb_samples, input->pFrame->data, input->pFrame->nb_samples);

    if (ret < 0)
    {
        qWarning() << "Failed to convert audio";
        return;
    }

    // 设置输出帧的参数
    output->pFrame->format = output->pCodecCtx->sample_fmt;
    output->pFrame->ch_layout = output->pCodecCtx->ch_layout;
    output->pFrame->sample_rate = output->pCodecCtx->sample_rate;
    output->pFrame->nb_samples = input->pFrame->nb_samples * output->pCodecCtx->sample_rate / input->pCodecCtx->sample_rate;
    // 计算输出帧的大小
    int size = av_samples_get_buffer_size(nullptr, output->pCodecCtx->ch_layout.nb_channels, output->pFrame->nb_samples, (AVSampleFormat)output->pCodecCtx->sample_fmt, 1);
    if (size < 0)
    {
        qWarning() << "Failed to get buffer size";
        return;
    }
    // 分配输出帧的内存
    output->pFrame->data[0] = (uint8_t*)av_malloc(size);
    if (!output->pFrame->data[0])
    {
        qWarning() << "Failed to allocate output frame data";
        return;
    }
    // 将重采样后的数据复制到输出帧
    memcpy(output->pFrame->data[0], input->pFrame->data[0], size);
    // 清理资源
    swr_free(&input->pSwrCtx);
    av_frame_free(&input->pFrame);
    av_frame_free(&output->pFrame);
    av_packet_free(&input->pPacket);
    av_packet_free(&output->pPacket);
    avformat_free_context(input->pFormatCtx);
    avformat_free_context(output->pFormatCtx);
    avcodec_free_context(&input->pCodecCtx);
    avcodec_free_context(&output->pCodecCtx);
    SDL_FreeAudioStream(input->pAudioStream);
    SDL_FreeAudioStream(output->pAudioStream);
    SDL_Quit();
}
#endif 