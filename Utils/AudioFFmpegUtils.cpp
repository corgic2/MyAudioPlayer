#include "AudioFFmpegUtils.h"
#include <QFile>

#include "AudioResampler.h"
#include "FFmpegPublicUtils.h"
#include "BaseDataDefine/ST_AVCodec.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVCodecParameters.h"
#include "DataDefine/ST_AudioDecodeResult.h"
#include "DataDefine/ST_OpenFileResult.h"
#include "DataDefine/ST_ResampleParams.h"
#include "DataDefine/ST_ResampleResult.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "SDL3/SDL.h"
#define FMT_NAME "dshow"
#pragma execution_character_set("utf-8")
// 根据不同设备进行修改，此电脑为USB音频设备
AudioFFmpegUtils::AudioFFmpegUtils(QObject* parent)
    : BaseFFmpegUtils(parent)
{
    m_playState.Reset();
    // 暂使用默认第一个设备
    m_inputAudioDevices = GetInputAudioDevices();
    if (!m_inputAudioDevices.empty())
    {
        m_currentInputDevice = m_inputAudioDevices.first();
    }
}

AudioFFmpegUtils::~AudioFFmpegUtils()
{
    StopPlay();
    StopRecording();
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

void AudioFFmpegUtils::StartRecording(const QString& outputFilePath)
{
    QString encoderFormat = QString::fromStdString(my_sdk::FileSystem::GetExtension(outputFilePath.toStdString()));
    LOG_INFO("Starting audio recording, output file: " + outputFilePath.toStdString() + ", encoder format: " + encoderFormat.toStdString());

    if (m_isRecording.load())
    {
        LOG_WARN("Recording failed: Another recording task is already in progress");
        return;
    }

    m_recordDevice = OpenDevice(FMT_NAME, m_currentInputDevice);
    if (!m_recordDevice || !m_recordDevice->GetFormatContext().GetRawContext())
    {
        LOG_ERROR("Failed to open input device");
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
        LOG_ERROR("Failed to write file header");
        return;
    }

    LOG_INFO("Audio recording initialization completed, starting recording");
    m_isRecording.store(true);

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

void AudioFFmpegUtils::StopRecording()
{
    if (!m_isRecording.load())
    {
        LOG_WARN("Stop recording failed: No recording task in progress");
        return;
    }

    LOG_INFO("Stopping audio recording");
    m_isRecording.store(false);
    m_recordDevice.reset();
}

void AudioFFmpegUtils::StartPlay(const QString& inputFilePath, double startPosition, const QStringList& args)
{
    LOG_INFO("Starting audio playback: " + inputFilePath.toStdString() + ", start position: " + std::to_string(startPosition) + " seconds");

    // 先停止当前播放并等待资源释放
    if (m_playState.IsPlaying())
    {
        LOG_INFO("Stopping current playback, waiting for resource release");
        StopPlay();
        SDL_Delay(100);
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
        LOG_ERROR("Failed to open audio file: " + inputFilePath.toStdString());
        return;
    }

    LOG_INFO("Audio file opened successfully, total duration: " + std::to_string(m_duration) + " seconds");

    // Get audio duration
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

    // Start playback
    m_playInfo->BeginPlayAudio();
    m_playState.SetPlaying(true);
    m_playState.SetPaused(false);
    m_playState.SetStartTime(SDL_GetTicks());
    m_playState.SetCurrentPosition(startPosition);

    LOG_INFO("Audio playback started");

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

void AudioFFmpegUtils::PausePlay()
{
    if (!m_playState.IsPlaying() || !m_playInfo)
    {
        LOG_WARN("Pause failed: No audio is currently playing");
        return;
    }

    LOG_INFO("Pausing audio playback");
    m_playState.SetPaused(true);
    m_playInfo->PauseAudio();
    m_playState.SetCurrentPosition(m_playState.GetCurrentPosition() + (SDL_GetTicks() - m_playState.GetStartTime()) / 1000.0);
}

void AudioFFmpegUtils::ResumePlay()
{
    if (!m_playState.IsPlaying() || !m_playInfo || !m_playState.IsPaused())
    {
        return;
    }

    m_playState.SetPaused(false);
    m_playState.SetStartTime(SDL_GetTicks());
    m_playInfo->ResumeAudio();
}

void AudioFFmpegUtils::StopPlay()
{
    if (!m_playState.IsPlaying())
    {
        LOG_WARN("Stop failed: No audio is currently playing");
        return;
    }

    LOG_INFO("Stopping audio playback");
    m_playState.Reset();

    if (m_playInfo)
    {
        m_playInfo->StopAudio();
        SDL_Delay(50);
        m_playInfo.reset();
    }

    emit SigPlayStateChanged(false);
}

bool AudioFFmpegUtils::SeekAudio(double seconds)
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
    StartPlay(m_currentFilePath, targetPosition);

    return true;
}

void AudioFFmpegUtils::SeekPlay(double seconds)
{
    if (m_playState.IsPlaying())
    {
        SeekAudio(seconds);
    }
}

void AudioFFmpegUtils::ShowSpec(AVFormatContext* ctx)
{
    if (!ctx)
    {
        LOG_WARN("ShowSpec() : AVFormatContext is nullptr");
        return;
    }

    // Get input stream
    AVStream* stream = ctx->streams[0];
    // Get audio parameters
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
    if (filePath.isEmpty() || !my_sdk::FileSystem::Exists(filePath.toStdString()))
    {
        LOG_WARN("LoadAudioWaveform() : Invalid file path");
        return false;
    }

    ST_OpenFileResult openFileResult;
    openFileResult.OpenFilePath(filePath);

    if (!openFileResult.m_formatCtx || !openFileResult.m_codecCtx)
    {
        LOG_WARN("LoadAudioWaveform() : Failed to open audio file");
        return false;
    }

    // 清空之前的数据
    waveformData.clear();

    // 读取音频数据并计算波形
    ST_AVPacket packet;
    AVFrame* frame = av_frame_alloc();
    if (!frame)
    {
        LOG_WARN("LoadAudioWaveform() : Failed to allocate frame");
        return false;
    }

    const int SAMPLES_PER_PIXEL = 1024; // 每个像素点对应的采样数
    float maxSample = 0.0f;
    float currentSum = 0.0f;
    int sampleCount = 0;

    try
    {
        while (packet.ReadPacket(openFileResult.m_formatCtx->GetRawContext()) >= 0)
        {
            if (packet.GetStreamIndex() == openFileResult.m_audioStreamIdx)
            {
                if (packet.SendPacket(openFileResult.m_codecCtx->GetRawContext()) >= 0)
                {
                    while (avcodec_receive_frame(openFileResult.m_codecCtx->GetRawContext(), frame) >= 0)
                    {
                        // 处理不同的采样格式
                        ProcessAudioFrame(frame, waveformData, SAMPLES_PER_PIXEL, currentSum, sampleCount, maxSample);
                    }
                }
            }
            packet.UnrefPacket();
        }

        // 处理剩余的样本
        if (sampleCount > 0)
        {
            float average = currentSum / sampleCount;
            waveformData.append(average);
            maxSample = std::max(maxSample, average);
        }

        // 归一化波形数据
        if (maxSample > 0.0f)
        {
            for (float& sample : waveformData)
            {
                sample /= maxSample;
            }
        }

        LOG_INFO("LoadAudioWaveform() : Successfully loaded waveform with " + std::to_string(waveformData.size()) + " data points");
    } catch (...)
    {
        LOG_ERROR("LoadAudioWaveform() : Exception occurred during waveform loading");
        av_frame_free(&frame);
        return false;
    }

    av_frame_free(&frame);
    return !waveformData.isEmpty();
}

void AudioFFmpegUtils::ProcessAudioFrame(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample)
{
    if (!frame || frame->nb_samples <= 0)
    {
        return;
    }

    auto format = static_cast<AVSampleFormat>(frame->format);
    int channels = frame->ch_layout.nb_channels;
    int samples = frame->nb_samples;

    // 根据不同的采样格式处理
    switch (format)
    {
        case AV_SAMPLE_FMT_FLT:  // 浮点格式，交错
        case AV_SAMPLE_FMT_FLTP: // 浮点格式，平面
        {
            ProcessFloatSamples(frame, waveformData, samplesPerPixel, currentSum, sampleCount, maxSample);
            break;
        }
        case AV_SAMPLE_FMT_S16:  // 16位整数，交错
        case AV_SAMPLE_FMT_S16P: // 16位整数，平面
        {
            ProcessInt16Samples(frame, waveformData, samplesPerPixel, currentSum, sampleCount, maxSample);
            break;
        }
        case AV_SAMPLE_FMT_S32:  // 32位整数，交错
        case AV_SAMPLE_FMT_S32P: // 32位整数，平面
        {
            ProcessInt32Samples(frame, waveformData, samplesPerPixel, currentSum, sampleCount, maxSample);
            break;
        }
        default:
        {
            LOG_WARN("ProcessAudioFrame() : Unsupported sample format: " + std::to_string(format));
            break;
        }
    }
}

void AudioFFmpegUtils::ProcessFloatSamples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample)
{
    bool isPlanar = av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame->format));
    int channels = frame->ch_layout.nb_channels;

    for (int i = 0; i < frame->nb_samples; i++)
    {
        float sample = 0.0f;

        if (isPlanar)
        {
            // 平面格式：每个通道分别存储
            for (int ch = 0; ch < channels; ch++)
            {
                auto* data = reinterpret_cast<float*>(frame->data[ch]);
                sample += std::abs(data[i]);
            }
            sample /= channels; // 取平均值
        }
        else
        {
            // 交错格式：所有通道交错存储
            auto* data = reinterpret_cast<float*>(frame->data[0]);
            for (int ch = 0; ch < channels; ch++)
            {
                sample += std::abs(data[i * channels + ch]);
            }
            sample /= channels; // 取平均值
        }

        currentSum += sample;
        sampleCount++;

        if (sampleCount >= samplesPerPixel)
        {
            float average = currentSum / sampleCount;
            waveformData.append(average);
            maxSample = std::max(maxSample, average);
            currentSum = 0.0f;
            sampleCount = 0;
        }
    }
}

void AudioFFmpegUtils::ProcessInt16Samples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample)
{
    bool isPlanar = av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame->format));
    int channels = frame->ch_layout.nb_channels;
    const float scale = 1.0f / 32768.0f; // 16位整数的归一化因子

    for (int i = 0; i < frame->nb_samples; i++)
    {
        float sample = 0.0f;

        if (isPlanar)
        {
            // 平面格式
            for (int ch = 0; ch < channels; ch++)
            {
                auto* data = reinterpret_cast<int16_t*>(frame->data[ch]);
                sample += std::abs(data[i]) * scale;
            }
            sample /= channels;
        }
        else
        {
            // 交错格式
            auto* data = reinterpret_cast<int16_t*>(frame->data[0]);
            for (int ch = 0; ch < channels; ch++)
            {
                sample += std::abs(data[i * channels + ch]) * scale;
            }
            sample /= channels;
        }

        currentSum += sample;
        sampleCount++;

        if (sampleCount >= samplesPerPixel)
        {
            float average = currentSum / sampleCount;
            waveformData.append(average);
            maxSample = std::max(maxSample, average);
            currentSum = 0.0f;
            sampleCount = 0;
        }
    }
}

void AudioFFmpegUtils::ProcessInt32Samples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample)
{
    bool isPlanar = av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame->format));
    int channels = frame->ch_layout.nb_channels;
    const float scale = 1.0f / 2147483648.0f; // 32位整数的归一化因子

    for (int i = 0; i < frame->nb_samples; i++)
    {
        float sample = 0.0f;

        if (isPlanar)
        {
            // 平面格式
            for (int ch = 0; ch < channels; ch++)
            {
                auto* data = reinterpret_cast<int32_t*>(frame->data[ch]);
                sample += std::abs(data[i]) * scale;
            }
            sample /= channels;
        }
        else
        {
            // 交错格式
            auto* data = reinterpret_cast<int32_t*>(frame->data[0]);
            for (int ch = 0; ch < channels; ch++)
            {
                sample += std::abs(data[i * channels + ch]) * scale;
            }
            sample /= channels;
        }

        currentSum += sample;
        sampleCount++;

        if (sampleCount >= samplesPerPixel)
        {
            float average = currentSum / sampleCount;
            waveformData.append(average);
            maxSample = std::max(maxSample, average);
            currentSum = 0.0f;
            sampleCount = 0;
        }
    }
}

double AudioFFmpegUtils::GetCurrentPosition() const
{
    return m_currentPosition;
}

double AudioFFmpegUtils::GetDuration() const
{
    return m_duration;
}

bool AudioFFmpegUtils::IsPlaying()
{
    return m_playState.IsPlaying();
}

bool AudioFFmpegUtils::IsPaused()
{
    return m_playState.IsPaused();
}

bool AudioFFmpegUtils::IsRecording()
{
    return m_isRecording.load();
}
