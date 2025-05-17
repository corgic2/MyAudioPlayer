#include "FFmpegUtils.h"
#include "../include/SDL3/SDL.h"
#include <QFile>
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

void FFmpegUtils::StartAudioRecording(const QString& outputFilePath, const QString& encoderFormat)
{
    const AVInputFormat* fmt = av_find_input_format(FMT_NAME);
    if (!fmt)
    {
        qDebug() << "not find InputDevice";
        return;
    }
    AVFormatContext* inputFormatCtx = nullptr;
    QByteArray utf8DeviceName = QString(DEVICE_INPUTMICRONAME).toUtf8();
    int ret = avformat_open_input(&inputFormatCtx, utf8DeviceName.constData(), fmt, nullptr); //获取输入设备上下文
    if (ret < 0)
    {
        char errbuf[1024] = {0};
        qWarning() << "open Device failed";
        av_strerror(ret, errbuf, sizeof(errbuf));
        return;
    }
    ShowSpec(inputFormatCtx); // 显示设备参数

    AVFormatContext* outputFormatCtx = nullptr;
    avformat_alloc_output_context2(&outputFormatCtx, nullptr, encoderFormat.toStdString().c_str(), outputFilePath.toUtf8().constData()); // 输出格式上下文初始化
    if (!outputFormatCtx)
    {
        qWarning() << "Failed to create output context";
        avformat_close_input(&inputFormatCtx); //输出格式初始化失败则关闭输入设备
        return;
    }

    // 创建音频流
    AVStream* outStream = avformat_new_stream(outputFormatCtx, nullptr);
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
    codecPar->codec_id = codec->id;
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

void FFmpegUtils::StartAudioPlayback(const QString& inputFilePath, const QStringList& args)
{
    // 播放音频文件
    AVFormatContext* formatCtx = nullptr;
    int ret = avformat_open_input(&formatCtx, inputFilePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0)
    {
        qWarning() << "Failed to open input file";
        return;
    }
    int audioStreamIdx = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIdx < 0)
    {
        qWarning() << "未找到音频流";
        avformat_close_input(&formatCtx);
        return;
    }
    // 初始化解码器
    const AVCodecParameters* codecParams = formatCtx->streams[audioStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecParams);
    avcodec_open2(codecCtx, codec, nullptr);
    // 输入格式（解码后的数据格式）
    SDL_AudioSpec srcSpec;
    SDL_zero(srcSpec);
    srcSpec.freq = codecParams->sample_rate; // 采样率
    srcSpec.format = FFmpegToSDLFormat((AVSampleFormat)codecParams->format); // 格式转换（见下文）
    srcSpec.channels = codecParams->ch_layout.nb_channels; // 声道数

    // 输出格式（设备支持的格式）
    SDL_AudioSpec dstSpec;
    SDL_zero(dstSpec);
    dstSpec.freq = srcSpec.freq; // 保持采样率一致
    dstSpec.format = SDL_AUDIO_S16; // SDL 常用格式（16位整型）
    dstSpec.channels = srcSpec.channels; // 声道数（需与设备匹配）

    // 创建音频流（自动处理重采样和格式转换）
    SDL_AudioStream* audioStreamSDL = SDL_CreateAudioStream(&srcSpec, &dstSpec);
    if (!audioStreamSDL)
    {
        qWarning() << "创建音频流失败:" << SDL_GetError();
        return;
    }

    // 打开音频设备
    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &dstSpec);
    if (audioDevice == 0)
    {
        qWarning() << "Failed to open audio device";
        avformat_close_input(&formatCtx);
        return;
    }
    // 播放音频
    SDL_BindAudioStream(audioDevice, audioStreamSDL); // 关键：绑定流和设备
    SDL_ResumeAudioDevice(audioDevice); // 开始播放
    AVPacket* pkt = av_packet_alloc();
    while (av_read_frame(formatCtx, pkt) >= 0)
    {
        if (pkt->stream_index == audioStreamIdx)
        {
            avcodec_send_packet(codecCtx, pkt);
            // 播放音频数据
            SDL_PutAudioStreamData(audioStreamSDL, pkt->data, pkt->size);
        }
        av_packet_unref(pkt);
    }
    // 等待音频播放完成
    while (SDL_GetAudioStreamQueued(audioStreamSDL) > 0)
    {
        SDL_Delay(100);
    }
    // 清理资源
    av_packet_free(&pkt);
    SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyAudioStream(audioStreamSDL);
    SDL_Quit();
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
}
// 从AVFormatContext中获取录音设备的相关参数
void FFmpegUtils::ShowSpec(AVFormatContext* ctx)
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

SDL_AudioFormat FFmpegUtils::FFmpegToSDLFormat(AVSampleFormat fmt)
{
    switch (fmt)
    {
        case AV_SAMPLE_FMT_U8:
            return SDL_AUDIO_U8;
        case AV_SAMPLE_FMT_S16:
            return SDL_AUDIO_S16;
        case AV_SAMPLE_FMT_S32:
            return SDL_AUDIO_S32;
        case AV_SAMPLE_FMT_FLT:
            return SDL_AUDIO_F32;
        case AV_SAMPLE_FMT_DBL:
            return SDL_AUDIO_S32LE;
        default:
            return SDL_AUDIO_S16; // 默认兼容格式
    }
}
