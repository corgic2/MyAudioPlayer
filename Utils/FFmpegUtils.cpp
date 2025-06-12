#include "FFmpegUtils.h"
#include <QFile>
#include "../include/SDL3/SDL.h"
#include "AudioResampler.h"
#include "FFmpegPublicUtils.h"
#define FMT_NAME "dshow"
#pragma execution_character_set("utf-8")
//根据不同设备进行修改，此电脑为USB音频设备
FFmpegUtils::FFmpegUtils(QObject* parent)
    : QObject(parent)
{
    m_playState.Reset();
}

FFmpegUtils::~FFmpegUtils()
{
    StopAudioPlayback();
    StopAudioRecording();
}

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
        qDebug() << "SDL_Init failed:" << SDL_GetError();
    }
}

std::unique_ptr<ST_OpenAudioDevice> FFmpegUtils::OpenDevice(const QString& devieceFormat, const QString& deviceName, bool bAudio)
{
    auto openDeviceParam = std::make_unique<ST_OpenAudioDevice>();
    openDeviceParam->GetInputFormat().FindInputFormat(devieceFormat.toStdString());
    if (!openDeviceParam->GetInputFormat().GetRawFormat())
    {
        qDebug() << "Failed to find input format";
        return openDeviceParam;
    }

    QString type = bAudio ? "audio=" : "video=";
    QByteArray utf8DeviceName = type.toUtf8() + deviceName.toUtf8();
    int ret = openDeviceParam->GetFormatContext().OpenInputFilePath(utf8DeviceName.constData(),
                                                                    openDeviceParam->GetInputFormat().GetRawFormat(),
                                                                    nullptr);
    if (ret < 0)
    {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "Failed to open device:" << errbuf;
        return openDeviceParam;
    }

    return openDeviceParam;
}

void FFmpegUtils::StartAudioRecording(const QString& outputFilePath, const QString& encoderFormat)
{
    if (m_playState.IsRecording())
    {
        return;
    }

    // 使用当前选择的输入设备
    m_recordDevice = OpenDevice(FMT_NAME, m_currentInputDevice);
    if (!m_recordDevice || !m_recordDevice->GetFormatContext().GetRawContext())
    {
        qWarning() << "Failed to open input device";
        return;
    }

    m_playState.SetRecording(true);
    ST_AVFormatContext outputFormatCtx;
    outputFormatCtx.OpenOutputFilePath(nullptr, encoderFormat.toStdString().c_str(), outputFilePath.toUtf8().constData());
    if (!outputFormatCtx.GetRawContext())
    {
        qWarning() << "Failed to create output context";
        return;
    }

    // 创建音频流
    AVStream* outStream = avformat_new_stream(outputFormatCtx.GetRawContext(), nullptr);
    if (!outStream)
    {
        qWarning() << "Failed to create output stream";
        return;
    }

    // 配置音频编码参数
    ST_AVCodec codec(FFmpegPublicUtils::FindEncoder(encoderFormat.toStdString().c_str()));
    ST_AVCodecContext encCtx(codec.GetRawCodec());
    ST_AVCodecParameters codecPar(outStream->codecpar);
    codecPar.GetRawParameters()->codec_id = codec.GetRawCodec()->id;
    codecPar.GetRawParameters()->codec_type = AVMEDIA_TYPE_AUDIO;
    codecPar.GetRawParameters()->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    FFmpegPublicUtils::ConfigureEncoderParams(codecPar.GetRawParameters(), encCtx.GetRawContext());

    int ret = 0;
    // 打开输出文件
    if (!(outputFormatCtx.GetRawContext()->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&outputFormatCtx.GetRawContext()->pb, outputFilePath.toUtf8().constData(), AVIO_FLAG_WRITE);
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
        ret = pkt.ReadPacket(m_recordDevice->GetFormatContext().GetRawContext());
        if (ret == 0)
        {
            // 直接复用原始数据包
            av_packet_rescale_ts(pkt.GetRawPacket(),
                                 m_recordDevice->GetFormatContext().GetRawContext()->streams[0]->time_base,
                                 outStream->time_base);
            pkt.GetRawPacket()->stream_index = outStream->index;
            av_interleaved_write_frame(outputFormatCtx.GetRawContext(), pkt.GetRawPacket());
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

void FFmpegUtils::StopAudioRecording()
{
    if (!m_playState.IsRecording())
    {
        return;
    }

    m_playState.SetRecording(false);
    m_recordDevice.reset();
}

void FFmpegUtils::StartAudioPlayback(const QString& inputFilePath, const QStringList& args)
{
    
    // 先停止当前播放并等待资源释放
    if (m_playState.IsPlaying())
    {
        StopAudioPlayback();
        SDL_Delay(100); // 给予系统一些时间来清理资源
    }

    // 确保之前的资源被完全释放
    if (m_playInfo)
    {
        m_playInfo->StopAudio();
        m_playInfo.reset();
        SDL_Delay(50); // 等待资源释放
    }

    m_playInfo = std::make_unique<ST_AudioPlayInfo>();

    // 打开音频文件
    ST_AVFormatContext formatCtx;
    int ret = formatCtx.OpenInputFilePath(inputFilePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0)
    {
        qWarning() << "Failed to open input file";
        m_playInfo.reset();
        return;
    }

    ret = avformat_find_stream_info(formatCtx.GetRawContext(), nullptr);
    if (ret < 0)
    {
        qWarning() << "Find stream info failed";
        return;
    }

    int audioStreamIdx = formatCtx.FindBestStream(AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIdx < 0)
    {
        qWarning() << "No audio stream found";
        m_playInfo.reset();
        return;
    }

    // 初始化解码器
    ST_AVCodecParameters codecParams(formatCtx.GetRawContext()->streams[audioStreamIdx]->codecpar);
    ST_AVCodec codec(codecParams.GetCodecId());
    ST_AVCodecContext codeCtx(codec.GetRawCodec());
    
    if (!codec.GetRawCodec())
    {
        qWarning() << "Failed to find codec";
        m_playInfo.reset();
        return;
    }

    if (codeCtx.BindParamToContext(codecParams.GetRawParameters()) < 0)
    {
        qWarning() << "Failed to bind codec parameters";
        m_playInfo.reset();
        return;
    }

    if (codeCtx.OpenCodec(codec.GetRawCodec(), nullptr) < 0)
    {
        qWarning() << "Failed to open codec";
        m_playInfo.reset();
        return;
    }

    // 初始化音频规格
    SDL_AudioSpec wantedSpec = {};
    wantedSpec.freq = codecParams.GetRawParameters()->sample_rate;
    wantedSpec.format = FFmpegPublicUtils::FFmpegToSDLFormat(codecParams.GetSampleFormat());
    wantedSpec.channels = codecParams.GetRawParameters()->ch_layout.nb_channels;

    m_playInfo->InitAudioSpec(true, wantedSpec.freq, wantedSpec.format, wantedSpec.channels);
    m_playInfo->InitAudioSpec(false, wantedSpec.freq, SDL_AUDIO_S16, wantedSpec.channels);
    m_playInfo->InitAudioStream();

    // 打开音频设备
    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wantedSpec);
    if (deviceId == 0)
    {
        qWarning() << "Failed to open audio device:" << SDL_GetError();
        m_playInfo.reset();
        return;
    }

    m_playInfo->InitAudioDevice(deviceId);
    m_playInfo->BindStreamAndDevice();
    m_playInfo->BeginPlayAudio();

    m_playState.SetPlaying(true);
    m_playState.SetPaused(false);
    m_playState.SetStartTime(SDL_GetTicks());
    m_playState.SetCurrentPosition(0.0);
    ST_AVPacket pkt;
    while (pkt.ReadPacket(formatCtx.GetRawContext()) >= 0)
    {
        if (pkt.GetRawPacket()->stream_index == audioStreamIdx)
        {
            // 解码音频数据包
            ST_AudioDecodeResult decodeResult = FFmpegPublicUtils::DecodeAudioPacket(pkt.GetRawPacket(), codeCtx.GetRawContext());
            
            // 如果解码成功且有数据，则播放
            if (!decodeResult.audioData.empty())
            {
                m_playInfo->PutDataToStream(decodeResult.audioData.data(), decodeResult.audioData.size());
            }
        }
        pkt.UnrefPacket();
    }
    // 等待音频播放完成
    while (m_playInfo->GetDataIsEnd() > 0)
    {
        m_playInfo->Delay(100);
    }
    emit SigPlayStateChanged(true);
}

void FFmpegUtils::PauseAudioPlayback()
{
    if (!m_playState.IsPlaying() || !m_playInfo)
    {
        return;
    }

    m_playState.SetPaused(true);
    m_playInfo->PauseAudio();
    m_playState.SetCurrentPosition(m_playState.GetCurrentPosition() + (SDL_GetTicks() - m_playState.GetStartTime()) / 1000.0);
}

void FFmpegUtils::ResumeAudioPlayback()
{
    if (!m_playState.IsPlaying() || !m_playInfo || !m_playState.IsPaused())
    {
        return;
    }

    m_playState.SetPaused(false);
    m_playState.SetStartTime(SDL_GetTicks());
    m_playInfo->ResumeAudio();
}

void FFmpegUtils::StopAudioPlayback()
{
    if (!m_playState.IsPlaying())
    {
        return;
    }

    // 先停止播放状态
    m_playState.Reset();

    // 停止音频播放并释放资源
    if (m_playInfo)
    {
        // 先停止音频播放
        m_playInfo->StopAudio();

        // 给系统一些时间来处理停止操作
        SDL_Delay(50);

        // 最后释放资源
        m_playInfo.reset();
    }

    emit SigPlayStateChanged(false);
}

void FFmpegUtils::SeekAudioForward(int seconds)
{
    if (!m_playState.IsPlaying() || !m_playInfo)
    {
        return;
    }

    m_playInfo->SeekAudio(seconds);
    m_playState.SetCurrentPosition(m_playInfo->GetCurrentPosition());
}

void FFmpegUtils::SeekAudioBackward(int seconds)
{
    if (!m_playState.IsPlaying() || !m_playInfo)
    {
        return;
    }

    m_playInfo->SeekAudio(-seconds);
    m_playState.SetCurrentPosition(m_playInfo->GetCurrentPosition());
}

void FFmpegUtils::ShowSpec(AVFormatContext* ctx)
{
    if (!ctx)
    {
        qWarning() << "ShowSpec() : AVFormatContext is nullptr";
        return;
    }

    // 获取输入流
    AVStream* stream = ctx->streams[0];
    // 获取音频参数
    ST_AVCodecParameters params(stream->codecpar);

    // 输出音频参数信息
    qDebug() << "Audio Specifications:";
    qDebug() << "Channels:" << params.GetRawParameters()->ch_layout.nb_channels;
    qDebug() << "Sample Rate:" << params.GetRawParameters()->sample_rate;
    qDebug() << "Format:" << params.GetRawParameters()->format;
    qDebug() << "Bytes per Sample:" << params.GetSamplePerRate();
    qDebug() << "Codec ID:" << params.GetRawParameters()->codec_id;
    qDebug() << "Bits per Sample:" << params.GetBitPerSample();
}

void FFmpegUtils::ResampleAudio(const uint8_t* input, size_t inputSize, ST_ResampleResult& output, const ST_ResampleParams& params)
{
    AudioResampler resampler;
    ST_ResampleResult tmp;

    // 计算输入样本数
    int inChannels = params.GetInput().GetChannels();
    if (params.GetInput().GetChannelLayout().GetRawLayout())
    {
        inChannels = params.GetInput().GetChannels();
    }

    // 获取单个格式字节数
    int bytesPerSample = av_get_bytes_per_sample(params.GetInput().GetSampleFormat().sampleFormat);
    int inputSamples = inputSize / (inChannels * bytesPerSample);

    // 准备输入指针数组
    const uint8_t* in_data[AV_NUM_DATA_POINTERS] = {input};

    // 执行重采样
    resampler.Resample(in_data, inputSamples, output, params);

    // 刷新剩余数据
    resampler.Flush(tmp, params);
    if (!tmp.GetData().empty())
    {
        std::vector<uint8_t> newData = output.GetData();
        newData.insert(newData.end(), tmp.GetData().begin(), tmp.GetData().end());
        output.SetData(std::move(newData));
        output.SetSamples(output.GetSamples() + tmp.GetSamples());
    }
}

QStringList FFmpegUtils::GetInputAudioDevices()
{
    QStringList devices;
    const AVInputFormat* inputFormat = av_find_input_format(FMT_NAME);
    if (!inputFormat)
    {
        qWarning() << "Failed to find input format:" << FMT_NAME;
        return devices;
    }

    AVDeviceInfoList* deviceList = nullptr;
    int ret = avdevice_list_input_sources(inputFormat, nullptr, nullptr, &deviceList);
    if (ret < 0)
    {
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "Failed to get input devices:" << errbuf;
        return devices;
    }

    for (int i = 0; i < deviceList->nb_devices; i++)
    {
        if (*deviceList->devices[i]->media_types == AVMEDIA_TYPE_AUDIO)
        {
            devices.append(QString::fromUtf8(deviceList->devices[i]->device_description));
        }
    }

    avdevice_free_list_devices(&deviceList);
    return devices;
}

void FFmpegUtils::SetInputDevice(const QString& deviceName)
{
    m_currentInputDevice = deviceName;
}
