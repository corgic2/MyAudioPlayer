#include "AudioFFmpegUtils.h"
#include <QFile>
#include "AudioResampler.h"
#include "FFmpegPublicUtils.h"
#include "../include/SDL3/SDL.h"
#include "BaseDataDefine/ST_AVCodec.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVCodecParameters.h"
#include "DataDefine/ST_AudioDecodeResult.h"
#include "DataDefine/ST_OpenFileResult.h"
#include "DataDefine/ST_ResampleParams.h"
#include "DataDefine/ST_ResampleResult.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#define FMT_NAME "dshow"
#pragma execution_character_set("utf-8")
//根据不同设备进行修改，此电脑为USB音频设备
AudioFFmpegUtils::AudioFFmpegUtils(QObject* parent)
    : QObject(parent)
{
    m_playState.Reset();
}

AudioFFmpegUtils::~AudioFFmpegUtils()
{
    StopAudioPlayback();
    StopAudioRecording();
}

void AudioFFmpegUtils::ResigsterDevice()
{
    avdevice_register_all();
    if (SDL_Init(SDL_INIT_AUDIO))
    {
        LOG_WARN("SDL_Init failed:" + std::string(SDL_GetError()));
    }
}

std::unique_ptr<ST_OpenAudioDevice> AudioFFmpegUtils::OpenDevice(const QString& devieceFormat, const QString& deviceName, bool bAudio)
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
    int ret = openDeviceParam->GetFormatContext().OpenInputFilePath(utf8DeviceName.constData(), openDeviceParam->GetInputFormat().GetRawFormat(), nullptr);
    if (ret < 0)
    {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to open device:" + std::string(errbuf));
        return openDeviceParam;
    }

    return openDeviceParam;
}

void AudioFFmpegUtils::StartAudioRecording(const QString& outputFilePath, const QString& encoderFormat)
{
    if (m_isRecording.load())
    {
        return;
    }

    // 使用当前选择的输入设备
    m_recordDevice = OpenDevice(FMT_NAME, m_currentInputDevice);
    if (!m_recordDevice || !m_recordDevice->GetFormatContext().GetRawContext())
    {
        LOG_WARN("Failed to open input device");
        return;
    }

    m_isRecording.store(true);
    ST_AVFormatContext outputFormatCtx;
    outputFormatCtx.OpenOutputFilePath(nullptr, encoderFormat.toStdString().c_str(), outputFilePath.toUtf8().constData());
    if (!outputFormatCtx.GetRawContext())
    {
        LOG_WARN("Failed to create output context");
        return;
    }

    // 创建音频流
    AVStream* outStream = avformat_new_stream(outputFormatCtx.GetRawContext(), nullptr);
    if (!outStream)
    {
        LOG_WARN("Failed to create output stream");
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
            LOG_WARN("Could not open output file:" + std::string(errbuf));
            return;
        }
    }

    // 写入文件头
    ret = outputFormatCtx.WriteFileHeader(nullptr);
    if (ret < 0)
    {
        LOG_WARN("Error writing header");
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
            av_packet_rescale_ts(pkt.GetRawPacket(), m_recordDevice->GetFormatContext().GetRawContext()->streams[0]->time_base, outStream->time_base);
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

void AudioFFmpegUtils::StopAudioRecording()
{
    if (!m_isRecording.load())
    {
        return;
    }

    m_isRecording.store(false);
    m_recordDevice.reset();
}

void AudioFFmpegUtils::StartAudioPlayback(const QString& inputFilePath, double startPosition, const QStringList& args)
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
        m_playInfo->UnbindStreamAndDevice();
        m_playInfo.reset();
        SDL_Delay(50); // 等待资源释放
    }

    m_playInfo = std::make_unique<ST_AudioPlayInfo>();
    m_currentFilePath = inputFilePath;

    // 打开文件
    ST_OpenFileResult openFileResult;
    openFileResult.OpenFilePath(inputFilePath);
    if (!openFileResult.m_formatCtx || !openFileResult.m_formatCtx->GetRawContext())
    {
        return;
    }

    // 获取音频时长
    m_duration = static_cast<double>(openFileResult.m_formatCtx->GetRawContext()->duration) / AV_TIME_BASE;

    // 如果指定了起始位置，执行定位
    if (startPosition > 0.0)
    {
        // 确保不会超出音频范围
        if (startPosition > m_duration)
        {
            startPosition = m_duration;
        }

        // 计算目标时间戳并执行定位
        int64_t targetTs = static_cast<int64_t>(startPosition * AV_TIME_BASE);
        int ret = av_seek_frame(openFileResult.m_formatCtx->GetRawContext(), -1, targetTs, AVSEEK_FLAG_BACKWARD);
        if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("Failed to seek audio: " + std::string(errbuf));
            return;
        }

        // 清空解码器缓冲
        avcodec_flush_buffers(openFileResult.m_codecCtx->GetRawContext());
    }

    m_currentPosition = startPosition;

    // 创建重采样器
    AudioResampler resampler;
    QString format = QString::fromStdString(my_sdk::FileSystem::GetExtension(inputFilePath.toStdString()));
    ST_ResampleParams resampleParams = resampler.GetResampleParams(format);

    // 初始化音频规格
    SDL_AudioSpec wantedSpec;
    wantedSpec.freq = resampleParams.GetOutput().GetSampleRate();
    wantedSpec.format = FFmpegPublicUtils::FFmpegToSDLFormat(resampleParams.GetOutput().GetSampleFormat().sampleFormat);
    wantedSpec.channels = resampleParams.GetOutput().GetChannels();

    m_playInfo->InitAudioSpec(true, static_cast<int>(wantedSpec.freq), wantedSpec.format, static_cast<int>(wantedSpec.channels));
    m_playInfo->InitAudioSpec(false, static_cast<int>(wantedSpec.freq), wantedSpec.format, static_cast<int>(wantedSpec.channels));
    m_playInfo->InitAudioStream();

    // 打开音频设备
    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wantedSpec);
    if (deviceId == 0)
    {
        LOG_WARN("Failed to open audio device:" + std::string(SDL_GetError()));
        m_playInfo.reset();
        return;
    }

    m_playInfo->InitAudioDevice(deviceId);

    // 在绑定设备和流之前确保它们都已经正确初始化
    if (!m_playInfo->GetAudioStream().GetRawStream() || !m_playInfo->GetAudioDevice())
    {
        LOG_WARN("Failed to initialize audio stream or device");
        m_playInfo.reset();
        return;
    }

    m_playInfo->BindStreamAndDevice();

    // 开始播放
    m_playInfo->BeginPlayAudio();
    m_playState.SetPlaying(true);
    m_playState.SetPaused(false);
    m_playState.SetStartTime(SDL_GetTicks());
    m_playState.SetCurrentPosition(startPosition);

    // 读取并解码音频数据
    ST_AVPacket pkt;
    while (pkt.ReadPacket(openFileResult.m_formatCtx->GetRawContext()) >= 0)
    {
        if (pkt.GetRawPacket()->stream_index == openFileResult.m_audioStreamIdx)
        {
            ST_AudioDecodeResult decodeResult = FFmpegPublicUtils::DecodeAudioPacket(pkt.GetRawPacket(), openFileResult.m_codecCtx->GetRawContext());
            m_playInfo->PutDataToStream(decodeResult.audioData.data(), decodeResult.audioData.size());
        }
        pkt.UnrefPacket();
    }

    // 刷新重采样器中的剩余数据
    ST_ResampleResult flushResult;
    resampler.Flush(flushResult, resampleParams);
    if (!flushResult.GetData().empty())
    {
        m_playInfo->PutDataToStream(flushResult.GetData().data(), flushResult.GetData().size());
    }

    // 发送进度更新信号
    emit SigProgressChanged(static_cast<qint64>(m_currentPosition), static_cast<qint64>(m_duration));
}

void AudioFFmpegUtils::PauseAudioPlayback()
{
    if (!m_playState.IsPlaying() || !m_playInfo)
    {
        return;
    }

    m_playState.SetPaused(true);
    m_playInfo->PauseAudio();
    m_playState.SetCurrentPosition(m_playState.GetCurrentPosition() + (SDL_GetTicks() - m_playState.GetStartTime()) / 1000.0);
}

void AudioFFmpegUtils::ResumeAudioPlayback()
{
    if (!m_playState.IsPlaying() || !m_playInfo || !m_playState.IsPaused())
    {
        return;
    }

    m_playState.SetPaused(false);
    m_playState.SetStartTime(SDL_GetTicks());
    m_playInfo->ResumeAudio();
}

void AudioFFmpegUtils::StopAudioPlayback()
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

bool AudioFFmpegUtils::SeekAudio(int seconds)
{
    if (m_currentFilePath.isEmpty() || !m_playInfo)
    {
        return false;
    }

    // 计算目标位置
    double targetPosition = m_currentPosition + seconds;

    // 确保不会超出音频范围
    if (targetPosition < 0)
    {
        targetPosition = 0;
    }
    else if (targetPosition > m_duration)
    {
        targetPosition = m_duration;
    }

    // 如果位置没有变化，直接返回
    if (qFuzzyCompare(targetPosition, m_currentPosition))
    {
        return false;
    }

    // 暂停当前播放
    bool wasPlaying = m_playState.IsPlaying() && !m_playState.IsPaused();
    if (wasPlaying)
    {
        m_playInfo->PauseAudio();
    }

    // 保存当前设备ID
    SDL_AudioDeviceID currentDeviceId = m_playInfo->GetAudioDevice();

    // 重新初始化播放
    StartAudioPlayback(m_currentFilePath, targetPosition);

    return true;
}

void AudioFFmpegUtils::SeekAudioForward(int seconds)
{
    if (m_playState.IsPlaying())
    {
        SeekAudio(seconds);
    }
}

void AudioFFmpegUtils::SeekAudioBackward(int seconds)
{
    if (m_playState.IsPlaying())
    {
        SeekAudio(-seconds);
    }
}

void AudioFFmpegUtils::ShowSpec(AVFormatContext* ctx)
{
    if (!ctx)
    {
        LOG_WARN("ShowSpec() : AVFormatContext is nullptr");
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

QStringList AudioFFmpegUtils::GetInputAudioDevices()
{
    QStringList devices;
    const AVInputFormat* inputFormat = av_find_input_format(FMT_NAME);
    if (!inputFormat)
    {
        LOG_WARN("Failed to find input format:" + std::string(FMT_NAME));
        return devices;
    }

    AVDeviceInfoList* deviceList = nullptr;
    int ret = avdevice_list_input_sources(inputFormat, nullptr, nullptr, &deviceList);
    if (ret < 0)
    {
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to get input devices:" + std::string(errbuf));
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

void AudioFFmpegUtils::SetInputDevice(const QString& deviceName)
{
    m_currentInputDevice = deviceName;
}

bool AudioFFmpegUtils::LoadAudioWaveform(const QString& filePath, QVector<float>& waveformData)
{
    ST_OpenFileResult openFileResult;
    openFileResult.OpenFilePath(filePath);
    // 读取音频数据并计算波形
    ST_AVPacket packet;
    AVFrame* frame = av_frame_alloc();
    const int SAMPLES_PER_PIXEL = 256; // 每个像素点对应的采样数
    float maxSample = 0;
    float currentSum = 0;
    int sampleCount = 0;

    while (packet.ReadPacket(openFileResult.m_formatCtx->GetRawContext()) >= 0)
    {
        if (packet.GetStreamIndex() == openFileResult.m_audioStreamIdx)
        {
            if (packet.SendPacket(openFileResult.m_codecCtx->GetRawContext()) >= 0)
            {
                while (avcodec_receive_frame(openFileResult.m_codecCtx->GetRawContext(), frame) >= 0)
                {
                    // 处理音频帧数据
                    auto samples = (float*)frame->data[0];
                    for (int i = 0; i < frame->nb_samples; i++)
                    {
                        float sample = std::abs(samples[i]);
                        currentSum += sample;
                        sampleCount++;

                        if (sampleCount >= SAMPLES_PER_PIXEL)
                        {
                            float average = currentSum / sampleCount;
                            waveformData.append(average);
                            maxSample = std::max(maxSample, average);
                            currentSum = 0;
                            sampleCount = 0;
                        }
                    }
                }
            }
        }
        packet.UnrefPacket();
    }

    // 归一化波形数据
    if (maxSample > 0)
    {
        for (float& sample : waveformData)
        {
            sample /= maxSample;
        }
    }
    return true;
}
