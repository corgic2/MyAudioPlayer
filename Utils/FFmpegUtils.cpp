#include "FFmpegUtils.h"
#include <QFile>
#define FMT_NAME "dshow"
#define DEVICE_INPUTMICRONAME "audio=麦克风 (2- USB Audio Device)"
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

void FFmpegUtils::PlayAudio(const QString& inputFilePath, const QStringList& args)
{
}

void FFmpegUtils::ResigsterDevice()
{
    avdevice_register_all();
}

void FFmpegUtils::StartAudioRecording(const QString& outputFilePath, const QString& encoderFormat)
{
    const AVInputFormat* fmt = av_find_input_format(FMT_NAME);
    if (!fmt)
    {
        qDebug() << "not find InputDevice";
        return;
    }
    AVFormatContext* inputFormatCtx = nullptr;
    QByteArray utf8DeviceName = QString(DEVICE_INPUTMICRONAME).toUtf8(); // 显式转为UTF-8
    int ret = avformat_open_input(&inputFormatCtx, utf8DeviceName.constData(), fmt, nullptr); //获取输入设备上下文
    if (ret < 0)
    {
        char errbuf[1024] = {0};
        qWarning() << "open Device failed";
        av_strerror(ret, errbuf, sizeof(errbuf));
        return;
    }
    showSpec(inputFormatCtx); // 显示设备参数

    AVFormatContext* outputFormatCtx = nullptr;
    avformat_alloc_output_context2(&outputFormatCtx, nullptr, encoderFormat.toStdString().c_str(), outputFilePath.toUtf8().constData()); // 输出格式上下文初始化
    if (!outputFormatCtx)
    {
        qWarning() << "Failed to create output context";
        avformat_close_input(&inputFormatCtx); //输出格式初始化失败则关闭输入设备
        return;
    }

    // 创建音频流
    AVStream* outStream = avformat_new_stream(outputFormatCtx, nullptr); // 由输出格式上下文创建音频流
    if (!outStream)
    {
        qWarning() << "Failed to create output stream";
        avformat_free_context(outputFormatCtx); // 释放输出格式上下文
        avformat_close_input(&inputFormatCtx);
        return;
    }

    // 配置音频编码参数（需与输入设备参数匹配）
    const AVCodec* codec = FindEncoder(encoderFormat.toStdString().c_str());
    AVCodecContext* enc_ctx = avcodec_alloc_context3(codec);
    AVCodecParameters* codecPar = outStream->codecpar;
    codecPar->codec_id = codec->id; // 必须明确指定
    codecPar->codec_type = AVMEDIA_TYPE_AUDIO;
    codecPar->ch_layout = AV_CHANNEL_LAYOUT_STEREO; // 立体声
    ConfigureEncoderParams(codecPar, enc_ctx);

    // 打开输出文件
    if (!(outputFormatCtx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&outputFormatCtx->pb, outputFilePath.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            char errbuf[1024];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qWarning() << "Could not open output file:" << errbuf;
            avformat_free_context(outputFormatCtx);
            avformat_close_input(&inputFormatCtx);
            return;
        }
    }

    // 写入文件头
    ret = avformat_write_header(outputFormatCtx, nullptr);
    if (ret < 0)
    {
        qWarning() << "Error writing header";
        avio_closep(&outputFormatCtx->pb);
        avformat_free_context(outputFormatCtx);
        avformat_close_input(&inputFormatCtx);
        return;
    }

    AVPacket* pkt = av_packet_alloc();
    int count = 50;
    while (count-- > 0)
    {
        ret = av_read_frame(inputFormatCtx, pkt);
        if (ret == 0)
        {
            // 直接复用原始数据包（假设输入格式与输出格式匹配）
            av_packet_rescale_ts(pkt, inputFormatCtx->streams[0]->time_base, outStream->time_base);
            pkt->stream_index = outStream->index;
            av_interleaved_write_frame(outputFormatCtx, pkt);
            av_packet_unref(pkt);
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
    av_write_trailer(outputFormatCtx);

    // 清理资源
    av_packet_free(&pkt);
    if (outputFormatCtx && !(outputFormatCtx->oformat->flags & AVFMT_NOFILE))
    {
        avio_closep(&outputFormatCtx->pb);
    }
    avformat_free_context(outputFormatCtx);
    avformat_close_input(&inputFormatCtx);

}

// 从AVFormatContext中获取录音设备的相关参数
void FFmpegUtils::showSpec(AVFormatContext* ctx)
{
    // 获取输入流
    AVStream* stream = ctx->streams[0];
    // 获取音频参数
    AVCodecParameters* params = stream->codecpar;
    // 声道数
    qDebug() << "channels: " << params->ch_layout.nb_channels;
    // 采样率
    qDebug() << "samplerate: " << params->sample_rate;
    // 采样格式
    qDebug() << "format: " << params->format;
    // 每一个样本的一个声道占用多少个字节
    qDebug() << "per sample bytes: " << av_get_bytes_per_sample((AVSampleFormat)params->format);
    // 编码ID（可以看出采样格式）
    qDebug() << "codecid: " << params->codec_id;
    // 每一个样本的一个声道占用多少位（这个函数需要用到avcodec库）
    qDebug() << "per sample bits: " << av_get_bits_per_sample(params->codec_id);
}

// 根据输出格式自动选择编码器
const AVCodec* FFmpegUtils::FindEncoder(const char* format_name)
{
    if (strcmp(format_name, "wav") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    }
    else if (strcmp(format_name, "mp3") == 0)
    {
        return avcodec_find_encoder(AV_CODEC_ID_MP3);
    }
    // 其他格式处理...
}

void FFmpegUtils::ConfigureEncoderParams(AVCodecParameters* codecPar, AVCodecContext* enc_ctx)
{
    switch (codecPar->codec_id)
    {
        case AV_CODEC_ID_PCM_S16LE: // WAV/PCM
            codecPar->format = AV_SAMPLE_FMT_S16;
            codecPar->sample_rate = 44100;
            codecPar->bit_rate = 1411200; // 44100Hz * 16bit * 2channels
            break;
        case AV_CODEC_ID_MP3: // MP3
            enc_ctx->bit_rate = 192000; // 典型比特率
            enc_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP; // MP3编码器要求的输入格式
            break;
    }
}
