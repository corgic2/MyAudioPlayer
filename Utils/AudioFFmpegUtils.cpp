#include "AudioFFmpegUtils.h"
#include <QFile>
#include <chrono>

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
    auto startTime = std::chrono::high_resolution_clock::now();
    LOG_INFO("=== Starting audio playback: " + inputFilePath.toStdString() + ", start position: " + std::to_string(startPosition) + " seconds ===");

    // 先停止当前播放并等待资源释放
    if (m_playState.IsPlaying())
    {
        auto stopStartTime = std::chrono::high_resolution_clock::now();
        LOG_INFO("Stopping current playback, waiting for resource release");
        StopPlay();
        SDL_Delay(200);
        auto stopEndTime = std::chrono::high_resolution_clock::now();
        auto stopDuration = std::chrono::duration_cast<std::chrono::milliseconds>(stopEndTime - stopStartTime).count();
        LOG_INFO("Current playback stopped in " + std::to_string(stopDuration) + " ms");
    }

    // 确保之前的资源被完全释放
    if (m_playInfo)
    {
        m_playInfo->StopAudio();
        m_playInfo->UnbindStreamAndDevice();
        m_playInfo.reset();
        SDL_Delay(100); // 等待资源释放
    }

    m_playInfo = std::make_unique<ST_AudioPlayInfo>();
    m_currentFilePath = inputFilePath;

    // 打开文件
    auto fileOpenStartTime = std::chrono::high_resolution_clock::now();
    ST_OpenFileResult openFileResult;
    openFileResult.OpenFilePath(inputFilePath);
    if (!openFileResult.m_formatCtx || !openFileResult.m_formatCtx->GetRawContext())
    {
        LOG_ERROR("Failed to open audio file: " + inputFilePath.toStdString());
        return;
    }
    auto fileOpenEndTime = std::chrono::high_resolution_clock::now();
    auto fileOpenDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fileOpenEndTime - fileOpenStartTime).count();
    LOG_INFO("Audio file opened successfully in " + std::to_string(fileOpenDuration) + " ms");

    // Get audio duration
    m_duration = static_cast<double>(openFileResult.m_formatCtx->GetRawContext()->duration) / AV_TIME_BASE;
    LOG_INFO("Audio duration: " + std::to_string(m_duration) + " seconds");

    // 如果指定了起始位置，执行定位
    if (startPosition > 0.0)
    {
        auto seekStartTime = std::chrono::high_resolution_clock::now();
        LOG_INFO("Seeking to position: " + std::to_string(startPosition) + " seconds");
        
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
        
        auto seekEndTime = std::chrono::high_resolution_clock::now();
        auto seekDuration = std::chrono::duration_cast<std::chrono::milliseconds>(seekEndTime - seekStartTime).count();
        LOG_INFO("Audio seek completed in " + std::to_string(seekDuration) + " ms");
    }

    m_currentPosition = startPosition;

    // 创建重采样器并获取实际的音频参数
    auto resamplerInitStartTime = std::chrono::high_resolution_clock::now();
    AudioResampler resampler;
    QString format = QString::fromStdString(my_sdk::FileSystem::GetExtension(inputFilePath.toStdString()));

    // 根据实际文件获取音频参数
    AVStream* audioStream = openFileResult.m_formatCtx->GetRawContext()->streams[openFileResult.m_audioStreamIdx];
    AVCodecParameters* codecpar = audioStream->codecpar;

    ST_ResampleParams resampleParams;

    // 设置实际的输入参数
    resampleParams.GetInput().SetSampleRate(codecpar->sample_rate);
    resampleParams.GetInput().SetSampleFormat(ST_AVSampleFormat(static_cast<AVSampleFormat>(codecpar->format)));

    auto inLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
    if (inLayout)
    {
        av_channel_layout_copy(inLayout, &codecpar->ch_layout);
        resampleParams.GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout));
    }

    // 设置标准的输出参数
    resampleParams.GetOutput().SetSampleRate(44100);                                  // 标准采样率
    resampleParams.GetOutput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16)); // 16位整数格式，兼容性最好

    auto outLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
    if (outLayout)
    {
        av_channel_layout_default(outLayout, 2); // 立体声输出
        resampleParams.GetOutput().SetChannelLayout(ST_AVChannelLayout(outLayout));
    }

    LOG_INFO("Audio parameters - Input: " + std::to_string(codecpar->sample_rate) + "Hz, " + std::to_string(codecpar->ch_layout.nb_channels) + " channels, format: " + std::to_string(codecpar->format));
    LOG_INFO("Audio parameters - Output: 44100Hz, 2 channels, S16 format");

    // 优化SDL音频规格配置
    SDL_AudioSpec wantedSpec;
    memset(&wantedSpec, 0, sizeof(wantedSpec));

    // 使用重采样器的输出参数配置SDL
    wantedSpec.freq = resampleParams.GetOutput().GetSampleRate();
    wantedSpec.format = FFmpegPublicUtils::FFmpegToSDLFormat(resampleParams.GetOutput().GetSampleFormat().sampleFormat);
    wantedSpec.channels = resampleParams.GetOutput().GetChannelLayout().GetRawLayout()->nb_channels;

    LOG_INFO("SDL Audio Spec - Freq: " + std::to_string(wantedSpec.freq) + ", Format: " + std::to_string(wantedSpec.format) + ", Channels: " + std::to_string(wantedSpec.channels));

    m_playInfo->InitAudioSpec(true, static_cast<int>(wantedSpec.freq), wantedSpec.format, static_cast<int>(wantedSpec.channels));
    m_playInfo->InitAudioSpec(false, static_cast<int>(wantedSpec.freq), wantedSpec.format, static_cast<int>(wantedSpec.channels));
    m_playInfo->InitAudioStream();

    // 打开音频设备
    auto sdlInitStartTime = std::chrono::high_resolution_clock::now();
    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wantedSpec);
    if (deviceId == 0)
    {
        LOG_ERROR("Failed to open audio device: " + std::string(SDL_GetError()));
        m_playInfo.reset();
        return;
    }
    auto sdlInitEndTime = std::chrono::high_resolution_clock::now();
    auto sdlInitDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sdlInitEndTime - sdlInitStartTime).count();
    LOG_INFO("SDL audio device opened successfully in " + std::to_string(sdlInitDuration) + " ms, Device ID: " + std::to_string(deviceId));

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

    // 改进音频数据处理流程
    ProcessAudioData(openFileResult, resampler, resampleParams);

    // 发送进度更新信号
    emit SigProgressChanged(static_cast<qint64>(m_currentPosition), static_cast<qint64>(m_duration));

    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    LOG_INFO("=== Audio playback initialization completed in " + std::to_string(totalDuration) + " ms ===");
}

void AudioFFmpegUtils::ProcessAudioData(ST_OpenFileResult& openFileResult, AudioResampler& resampler, ST_ResampleParams& resampleParams)
{
    auto processStartTime = std::chrono::high_resolution_clock::now();
    LOG_INFO("=== Starting audio data processing ===");
    
    ST_AVPacket pkt;
    const int BUFFER_SIZE = 8192;
    std::vector<uint8_t> audioBuffer;
    audioBuffer.reserve(BUFFER_SIZE * 4);
    
    int processedPackets = 0;
    int processedFrames = 0;
    auto lastLogTime = processStartTime;
    
    AVFrame* frame = av_frame_alloc();
    if (!frame)
    {
        LOG_ERROR("Failed to allocate frame for audio processing");
        return;
    }

    try
    {
        while (pkt.ReadPacket(openFileResult.m_formatCtx->GetRawContext()) >= 0)
        {
            if (pkt.GetRawPacket()->stream_index == openFileResult.m_audioStreamIdx)
            {
                // 每处理100个包记录一次进度
                if (processedPackets % 100 == 0 && processedPackets > 0)
                {
                    auto currentTime = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastLogTime).count();
                    LOG_INFO("Processed " + std::to_string(processedPackets) + " packets, " + std::to_string(processedFrames) + " frames in last " + std::to_string(duration) + " ms");
                    lastLogTime = currentTime;
                }

                // 发送数据包到解码器
                int ret = avcodec_send_packet(openFileResult.m_codecCtx->GetRawContext(), pkt.GetRawPacket());
                if (ret < 0)
                {
                    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_strerror(ret, errbuf, sizeof(errbuf));
                    LOG_WARN("Failed to send packet to decoder: " + std::string(errbuf));
                    pkt.UnrefPacket();
                    continue;
                }

                // 接收解码后的帧
                while ((ret = avcodec_receive_frame(openFileResult.m_codecCtx->GetRawContext(), frame)) >= 0)
                {
                    processedFrames++;
                    
                    // 直接使用AVFrame进行重采样，避免数据格式转换问题
                    ST_ResampleResult resampleResult;

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
                    auto resampleStartTime = std::chrono::high_resolution_clock::now();
                    resampler.Resample(inputDataPtrs, frame->nb_samples, resampleResult, resampleParams);
                    auto resampleEndTime = std::chrono::high_resolution_clock::now();
                    
                    // 只在耗时较长时记录重采样时间
                    auto resampleDuration = std::chrono::duration_cast<std::chrono::microseconds>(resampleEndTime - resampleStartTime).count();
                    if (resampleDuration > 1000) // 大于1ms才记录
                    {
                        LOG_DEBUG("Frame resampling took " + std::to_string(resampleDuration) + " μs");
                    }

                    // 将重采样后的数据添加到缓冲区
                    if (!resampleResult.GetData().empty())
                    {
                        audioBuffer.insert(audioBuffer.end(), resampleResult.GetData().begin(), resampleResult.GetData().end());

                        // 当缓冲区达到一定大小时才传输
                        if (audioBuffer.size() >= BUFFER_SIZE)
                        {
                            m_playInfo->PutDataToStream(audioBuffer.data(), static_cast<int>(audioBuffer.size()));
                            audioBuffer.clear();

                            // 定期检查是否需要停止
                            if (!m_playState.IsPlaying())
                            {
                                break;
                            }
                        }
                    }

                    // 更新播放位置
                    if (frame->pts != AV_NOPTS_VALUE)
                    {
                        AVStream* audioStream = openFileResult.m_formatCtx->GetRawContext()->streams[openFileResult.m_audioStreamIdx];
                        double timeStamp = static_cast<double>(frame->pts) * av_q2d(audioStream->time_base);
                        m_currentPosition = timeStamp;

                        // 定期发送进度更新信号
                        if (processedPackets % 25 == 0)
                        {
                            emit SigProgressChanged(static_cast<qint64>(m_currentPosition), static_cast<qint64>(m_duration));
                        }
                    }
                }

                if (ret == AVERROR(EAGAIN))
                {
                    // 需要更多输入数据
                    pkt.UnrefPacket();
                    continue;
                }
                else if (ret == AVERROR_EOF)
                {
                    // 解码器已到达文件末尾
                    break;
                }
                else if (ret < 0)
                {
                    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_strerror(ret, errbuf, sizeof(errbuf));
                    LOG_WARN("Error receiving frame from decoder: " + std::string(errbuf));
                }
                
                processedPackets++;
            }

            pkt.UnrefPacket();

            // 定期检查是否需要停止
            if (processedPackets % 50 == 0 && !m_playState.IsPlaying())
            {
                break;
            }
        }

        // 处理剩余的音频数据
        if (!audioBuffer.empty())
        {
            m_playInfo->PutDataToStream(audioBuffer.data(), static_cast<int>(audioBuffer.size()));
        }

        // 刷新重采样器中的剩余数据
        ST_ResampleResult flushResult;
        resampler.Flush(flushResult, resampleParams);
        if (!flushResult.GetData().empty())
        {
            m_playInfo->PutDataToStream(flushResult.GetData().data(), static_cast<int>(flushResult.GetData().size()));
        }

        auto processEndTime = std::chrono::high_resolution_clock::now();
        auto totalProcessDuration = std::chrono::duration_cast<std::chrono::milliseconds>(processEndTime - processStartTime).count();
        LOG_INFO("=== Audio data processing completed in " + std::to_string(totalProcessDuration) + " ms, processed " + std::to_string(processedPackets) + " packets, " + std::to_string(processedFrames) + " frames ===");
    } 
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in audio processing: " + std::string(e.what()));
    } 
    catch (...)
    {
        LOG_ERROR("Unknown exception in audio processing");
    }

    av_frame_free(&frame);
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
    auto startTime = std::chrono::high_resolution_clock::now();
    LOG_INFO("=== Starting audio waveform loading for: " + filePath.toStdString() + " ===");
    
    if (filePath.isEmpty() || !my_sdk::FileSystem::Exists(filePath.toStdString()))
    {
        LOG_WARN("LoadAudioWaveform() : Invalid file path");
        return false;
    }

    auto fileOpenStartTime = std::chrono::high_resolution_clock::now();
    ST_OpenFileResult openFileResult;
    openFileResult.OpenFilePath(filePath);

    if (!openFileResult.m_formatCtx || !openFileResult.m_codecCtx)
    {
        LOG_WARN("LoadAudioWaveform() : Failed to open audio file");
        return false;
    }
    auto fileOpenEndTime = std::chrono::high_resolution_clock::now();
    auto fileOpenDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fileOpenEndTime - fileOpenStartTime).count();
    LOG_INFO("Waveform file opened in " + std::to_string(fileOpenDuration) + " ms");

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
    
    int processedPackets = 0;

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
                processedPackets++;
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

        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        LOG_INFO("=== Waveform loading completed in " + std::to_string(totalDuration) + " ms, generated " + std::to_string(waveformData.size()) + " data points ===");
    } 
    catch (...)
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
