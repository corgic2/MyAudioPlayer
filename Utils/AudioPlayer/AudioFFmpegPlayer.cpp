#include "AudioFFmpegPlayer.h"
#include <QFile>
#include "AudioPlayerUtils.h"

#include "AudioResampler.h"
#include "CoreServerGlobal.h"
#include "../BasePlayer/FFmpegPublicUtils.h"
#include "BaseDataDefine/ST_AVCodec.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVCodecParameters.h"
#include "BaseDataDefine/ST_AVFormatContext.h"
#include "BaseDataDefine/ST_AVFrame.h"
#include "DataDefine/ST_AudioDecodeResult.h"
#include "DataDefine/ST_OpenFileResult.h"
#include "DataDefine/ST_ResampleParams.h"
#include "DataDefine/ST_ResampleResult.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "SDL3/SDL.h"
#include "TimeSystem/TimeSystem.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#pragma execution_character_set("utf-8")

// 根据不同设备进行修改，此电脑为USB音频设备
AudioFFmpegPlayer::AudioFFmpegPlayer(QObject* parent)
    : BaseFFmpegPlayer(parent)
{
    // 暂使用默认第一个设备
    m_inputAudioDevices = AudioPlayerUtils::GetInputAudioDevices();
    if (!m_inputAudioDevices.empty())
    {
        m_currentInputDevice = m_inputAudioDevices.first();
    }
}

AudioFFmpegPlayer::~AudioFFmpegPlayer()
{
    StopPlay();
    StopRecording();
}


std::unique_ptr<ST_OpenAudioDevice> AudioFFmpegPlayer::OpenDevice(const QString& devieceFormat, const QString& deviceName, bool bAudio)
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
    openDeviceParam->GetFormatContext().OpenInputFilePath(utf8DeviceName.constData(), openDeviceParam->GetInputFormat().GetRawFormat(), nullptr);
    return openDeviceParam;
}

void AudioFFmpegPlayer::StartRecording(const QString& outputFilePath)
{
    QString encoderFormat = QString::fromStdString(my_sdk::FileSystem::GetExtension(outputFilePath.toStdString()));
    LOG_INFO("Starting audio recording, output file: " + outputFilePath.toStdString() + ", encoder format: " + encoderFormat.toStdString());

    if (m_playState.GetCurrentState() == AVPlayState::Recording)
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
    if (!(outputFormatCtx.GetRawContext()->oformat->flags & AVFMT_NOFILE) && !outputFormatCtx.OpenIOFilePath(outputFilePath))
    {
        return;
    }

    // 写入文件头
    ret = outputFormatCtx.WriteFileHeader(nullptr);
    if (ret < 0)
    {
        LOG_ERROR("Failed to write file header");
        return;
    }

    LOG_INFO("Audio recording initialization completed, starting recording");
    m_playState.TransitionTo(AVPlayState::Recording);

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

void AudioFFmpegPlayer::StopRecording()
{
    if (m_playState.GetCurrentState() != AVPlayState::Recording)
    {
        LOG_WARN("Stop recording failed: No recording task in progress");
        return;
    }

    LOG_INFO("Stopping audio recording");
    m_playState.TransitionTo(AVPlayState::Stopped);
    m_recordDevice.reset();
}

void AudioFFmpegPlayer::StartPlay(const QString& inputFilePath, bool bStart, double startPosition, const QStringList& args)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    PlayerStateReSet();

    // 记录播放开始时间和位置
    RecordPlayStartTime(startPosition);

    // 使用时间系统进行整体计时
    TIME_START("AudioPlaybackTotal");
    LOG_INFO("=== Starting audio playback: " + inputFilePath.toStdString() + ", start position: " + std::to_string(startPosition) + " seconds ===");
    m_playInfo = std::make_unique<ST_AudioPlayInfo>();

    // 使用基类的通用文件打开功能
    TIME_START("AudioFileOpen");
    auto openFileResult = OpenMediaFile(inputFilePath);
    if (!openFileResult)
    {
        LOG_ERROR("Failed to open audio file: " + inputFilePath.toStdString());
        return;
    }
    TimeSystem::Instance().StopTimingWithLog("AudioFileOpen", EM_TimingLogLevel::Info);

    LOG_INFO("Audio duration: " + std::to_string(GetDuration()) + " seconds");

    // 确保起始位置在有效范围内
    if (startPosition > GetDuration())
    {
        LOG_WARN("Start position exceeds audio duration, adjusting to end");
        startPosition = GetDuration();
    }

    // 如果指定了起始位置，执行定位
    if (startPosition > 0.0)
    {
        TIME_START("AudioSeek");
        LOG_INFO("Seeking to position: " + std::to_string(startPosition) + " seconds");

        // 确保不会超出音频范围
        if (startPosition > GetDuration())
        {
            startPosition = GetDuration();
        }

        // 使用通用的seek功能
        if (!FFmpegPublicUtils::SeekAudio(openFileResult->m_formatCtx->GetRawContext(), openFileResult->m_codecCtx->GetRawContext(), startPosition))
        {
            return;
        }
        TimeSystem::Instance().StopTimingWithLog("AudioSeek", EM_TimingLogLevel::Info);
    }

    // 初始化音频流索引
    m_audioStreamIdx = openFileResult->m_audioStreamIdx;

    // 创建或重用重采样器
    if (!m_resampler)
    {
        m_resampler = std::make_unique<AudioResampler>();
    }

    // 创建或重用重采样参数
    if (!m_resampleParams)
    {
        m_resampleParams = std::make_unique<ST_ResampleParams>();
    }

    // 重置重采样参数更新标记
    m_bResampleParamsUpdated = false;

    // 获取音频参数
    AVStream* audioStream = openFileResult->m_formatCtx->GetRawContext()->streams[m_audioStreamIdx];
    AVCodecParameters* codecpar = audioStream->codecpar;
    AVCodecContext* codecCtx = openFileResult->m_codecCtx->GetRawContext();

    // 设置实际的输入参数（优先使用解码器上下文的参数）
    int inputSampleRate = (codecCtx->sample_rate > 0) ? codecCtx->sample_rate : codecpar->sample_rate;
    AVSampleFormat inputFormat = (codecCtx->sample_fmt != AV_SAMPLE_FMT_NONE) ? codecCtx->sample_fmt : static_cast<AVSampleFormat>(codecpar->format);

    // 如果格式仍然是NONE，则设置一个默认值
    if (inputFormat == AV_SAMPLE_FMT_NONE)
    {
        LOG_WARN("Audio format is AV_SAMPLE_FMT_NONE, will update from first decoded frame");
        inputFormat = AV_SAMPLE_FMT_FLTP;
    }

    m_resampleParams->GetInput().SetSampleRate(inputSampleRate);
    m_resampleParams->GetInput().SetSampleFormat(ST_AVSampleFormat(inputFormat));

    // 设置输入通道布局
    AVChannelLayoutRAII inLayout;
    if (codecCtx->ch_layout.nb_channels > 0)
    {
        inLayout = AVChannelLayoutRAII::copyFrom(&codecCtx->ch_layout);
    }
    else
    {
        inLayout = AVChannelLayoutRAII::copyFrom(&codecpar->ch_layout);
    }

    if (inLayout)
    {
        m_resampleParams->GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout.release()));
    }

    // 设置标准的输出参数
    m_resampleParams->GetOutput().SetSampleRate(44100);
    m_resampleParams->GetOutput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16));

    // 设置输出通道布局
    auto outLayout = AVChannelLayoutRAII::createDefault(2);
    if (outLayout)
    {
        m_resampleParams->GetOutput().SetChannelLayout(ST_AVChannelLayout(outLayout.release()));
    }

    LOG_INFO("Audio parameters - Input: " + std::to_string(inputSampleRate) + "Hz, " + std::to_string(codecpar->ch_layout.nb_channels) + " channels");
    LOG_INFO("Audio parameters - Output: 44100Hz, 2 channels, S16 format");

    // 优化SDL音频规格配置
    SDL_AudioSpec wantedSpec;
    memset(&wantedSpec, 0, sizeof(wantedSpec));

    // 使用重采样器的输出参数配置SDL
    wantedSpec.freq = m_resampleParams->GetOutput().GetSampleRate();
    wantedSpec.format = FFmpegPublicUtils::FFmpegToSDLFormat(m_resampleParams->GetOutput().GetSampleFormat().sampleFormat);
    wantedSpec.channels = m_resampleParams->GetOutput().GetChannelLayout().GetRawLayout()->nb_channels;

    LOG_INFO("SDL Audio Spec - Freq: " + std::to_string(wantedSpec.freq) + ", Format: " + std::to_string(wantedSpec.format) + ", Channels: " + std::to_string(wantedSpec.channels));

    m_playInfo->InitAudioSpec(true, static_cast<int>(wantedSpec.freq), wantedSpec.format, static_cast<int>(wantedSpec.channels));
    m_playInfo->InitAudioSpec(false, static_cast<int>(wantedSpec.freq), wantedSpec.format, static_cast<int>(wantedSpec.channels));
    m_playInfo->InitAudioStream();

    // 打开音频设备
    TIME_START("SDLAudioInit");
    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wantedSpec);
    if (deviceId == 0)
    {
        LOG_ERROR("Failed to open audio device: " + std::string(SDL_GetError()));
        m_playInfo.reset();
        return;
    }
    TimeSystem::Instance().StopTimingWithLog("SDLAudioInit", EM_TimingLogLevel::Info, EM_TimeUnit::Milliseconds, "SDL audio device opened successfully, Device ID: " + std::to_string(deviceId));

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
    if (bStart)
    {
        m_playInfo->BeginPlayAudio();
        m_playState.TransitionTo(AVPlayState::Playing);
    }
    else
    {
        m_playState.TransitionTo(AVPlayState::Paused);
    }
    LOG_INFO("Audio playback started");
    ProcessAudioData(*openFileResult, *m_resampler, *m_resampleParams, startPosition);
    TimeSystem::Instance().StopTimingWithLog("AudioPlaybackTotal", EM_TimingLogLevel::Info, EM_TimeUnit::Milliseconds, "Audio playback initialization completed");
}

void AudioFFmpegPlayer::ProcessAudioData(const ST_OpenFileResult& openFileResult, AudioResampler& resampler, ST_ResampleParams& resampleParams, double startSeconds)
{
    TIME_START("AudioDataProcessing");
    LOG_INFO("=== Starting audio data processing from position: " + std::to_string(startSeconds) + " seconds ===");
    
    // 处理音频数据
    ST_AVPacket pkt;
    const int BUFFER_SIZE = 8192;
    
    // 清空之前的缓冲区
    {
        std::lock_guard<std::recursive_mutex> buffer_lock(m_bufferMutex);
        m_audioBuffer.clear();
        m_audioBuffer.reserve(BUFFER_SIZE * 4);
    }
    
    int processedPackets = 0;
    int processedFrames = 0;
    
    // 使用成员变量的重采样参数更新标记
    ST_AVFrame frame;
    FFmpegPublicUtils::SeekAudio(openFileResult.m_formatCtx->GetRawContext(), openFileResult.m_codecCtx->GetRawContext(), startSeconds);
    while (pkt.ReadPacket(openFileResult.m_formatCtx->GetRawContext()))
    {
        if (pkt.GetRawPacket()->stream_index == m_audioStreamIdx)
        {
            // 发送数据包到解码器
            if (!pkt.SendPacket(openFileResult.m_codecCtx->GetRawContext()))
            {
                continue;
            }
            // 接收解码后的帧
            while (frame.GetCodecFrame(openFileResult.m_codecCtx->GetRawContext()))
            {
                processedFrames++;

                // === 修复：在第一次获取解码帧时更新重采样参数 ===
                if (!m_bResampleParamsUpdated)
                {
                    AVFrame* rawFrame = frame.GetRawFrame();

                    // 从实际解码的帧获取正确的音频格式
                    auto actualFormat = static_cast<AVSampleFormat>(rawFrame->format);
                    int actualSampleRate = rawFrame->sample_rate;

                    LOG_INFO("Updating resample params from first decoded frame:");
                    LOG_INFO("  Actual format: " + std::to_string(static_cast<int>(actualFormat)) + " (" + std::string(av_get_sample_fmt_name(actualFormat)) + ")");
                    LOG_INFO("  Actual sample rate: " + std::to_string(actualSampleRate));
                    LOG_INFO("  Actual channels: " + std::to_string(rawFrame->ch_layout.nb_channels));

                    // 更新重采样参数
                    m_resampleParams->GetInput().SetSampleFormat(ST_AVSampleFormat(actualFormat));
                    if (actualSampleRate > 0)
                    {
                        m_resampleParams->GetInput().SetSampleRate(actualSampleRate);
                    }

                    // 使用RAII包装器更新通道布局
                    if (rawFrame->ch_layout.nb_channels > 0)
                    {
                        auto inLayout = AVChannelLayoutRAII::copyFrom(&rawFrame->ch_layout);
                        if (inLayout)
                        {
                            m_resampleParams->GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout.release()));
                        }
                    }

                    m_bResampleParamsUpdated = true;
                }

                // 直接使用AVFrame进行重采样，避免数据格式转换问题
                ST_ResampleResult resampleResult;

                // 准备输入数据指针数组
                const uint8_t* inputDataPtrs[AV_NUM_DATA_POINTERS] = {0};

                // 检查是否为平面格式
                bool isPlanar = av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame.GetRawFrame()->format));
                int channels = frame.GetRawFrame()->ch_layout.nb_channels;

                if (isPlanar)
                {
                    // 平面格式：每个通道分别存储
                    for (int ch = 0; ch < channels && ch < AV_NUM_DATA_POINTERS; ch++)
                    {
                        inputDataPtrs[ch] = frame.GetRawFrame()->data[ch];
                    }
                }
                else
                {
                    // 交错格式：所有通道数据交错存储
                    inputDataPtrs[0] = frame.GetRawFrame()->data[0];
                }

                // 执行重采样
                TIME_START("AudioResample");
                m_resampler->Resample(inputDataPtrs, frame.GetRawFrame()->nb_samples, resampleResult, *m_resampleParams);
                double resampleDuration = TimeSystem::Instance().StopTiming("AudioResample", EM_TimeUnit::Microseconds);

                // 只在耗时较长时记录重采样时间
                if (resampleDuration > 1000) // 大于1ms才记录
                {
                    LOG_DEBUG("Frame resampling took " + std::to_string(resampleDuration) + " μs");
                }

                // 将重采样后的数据添加到缓冲区
                if (!resampleResult.GetData().empty())
                {
                    {
                        std::lock_guard<std::recursive_mutex> buffer_lock(m_bufferMutex);
                        m_audioBuffer.insert(m_audioBuffer.end(), resampleResult.GetData().begin(), resampleResult.GetData().end());

                        // 当缓冲区达到一定大小时才传输
                        if (m_audioBuffer.size() >= BUFFER_SIZE)
                        {
                            m_playInfo->PutDataToStream(m_audioBuffer.data(), static_cast<int>(m_audioBuffer.size()));
                            m_audioBuffer.clear();
                        }
                    }
                }
            }
            processedPackets++;

            // 每处理100个包记录一次进度
            if (processedPackets % 100 == 0 && processedPackets > 0)
            {
                LOG_INFO("Processed " + std::to_string(processedPackets) + " packets, " + std::to_string(processedFrames) + " frames");
            }
        }

        pkt.UnrefPacket();
    }

    // 处理剩余的音频数据
    {
        std::lock_guard<std::recursive_mutex> buffer_lock(m_bufferMutex);
        if (!m_audioBuffer.empty())
        {
            m_playInfo->PutDataToStream(m_audioBuffer.data(), static_cast<int>(m_audioBuffer.size()));
            m_audioBuffer.clear();
        }
    }

    // 刷新重采样器中的剩余数据
    ST_ResampleResult flushResult;
    m_resampler->Flush(flushResult, *m_resampleParams);
    if (!flushResult.GetData().empty())
    {
        std::lock_guard<std::recursive_mutex> buffer_lock(m_bufferMutex);
        m_audioBuffer.insert(m_audioBuffer.end(), flushResult.GetData().begin(), flushResult.GetData().end());
        if (!m_audioBuffer.empty())
        {
            m_playInfo->PutDataToStream(m_audioBuffer.data(), static_cast<int>(m_audioBuffer.size()));
            m_audioBuffer.clear();
        }
    }

    // 清空音频流缓冲区，确保seek时不会有旧数据
    m_playInfo->FlushAudioStream();

    LOG_INFO("=== Audio data processing completed, processed " + std::to_string(processedPackets) + " packets, " + std::to_string(processedFrames) + " frames ===");

    TimeSystem::Instance().StopTimingWithLog("AudioDataProcessing", EM_TimingLogLevel::Info, EM_TimeUnit::Milliseconds, "Audio data processing completed");
}

void AudioFFmpegPlayer::PlayerStateReSet()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 确保之前的资源被完全释放
    if (m_playInfo)
    {
        m_playInfo->StopAudio();
        m_playInfo->UnbindStreamAndDevice();
        m_playInfo.reset();
        SDL_Delay(10); // 等待资源释放
    }

    // 清理缓存的重采样资源
    m_resampler.reset();
    m_resampleParams.reset();
    m_audioStreamIdx = -1;
    m_bResampleParamsUpdated = false;

    // 清空音频缓冲区
    {
        std::lock_guard<std::recursive_mutex> buffer_lock(m_bufferMutex);
        m_audioBuffer.clear();
        m_audioBuffer.shrink_to_fit();
    }
}

void AudioFFmpegPlayer::PausePlay()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_playState.GetCurrentState() != AVPlayState::Playing || !m_playInfo)
    {
        LOG_WARN("Pause failed: No audio is currently playing");
        return;
    }

    LOG_INFO("Pausing audio playback");

    // 保存当前播放位置
    double currentPos = GetCurrentPosition();
    m_playState.TransitionTo(AVPlayState::Paused);
    m_playInfo->PauseAudio();
}

void AudioFFmpegPlayer::ResumePlay()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_playState.GetCurrentState() != AVPlayState::Paused || !m_playInfo)
    {
        return;
    }

    m_playState.TransitionTo(AVPlayState::Playing);
    m_playInfo->ResumeAudio();
}

void AudioFFmpegPlayer::StopPlay()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    PlayerStateReSet();
    m_playState.TransitionTo(AVPlayState::Stopped);
}

bool AudioFFmpegPlayer::SeekAudio(double seconds)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (GetCurrentFilePath().isEmpty() || !m_playInfo)
    {
        return false;
    }

    // 验证seek位置
    if (seconds < 0.0 || seconds > GetDuration())
    {
        LOG_WARN("Invalid seek position: " + std::to_string(seconds));
        return false;
    }

    LOG_INFO("Seeking audio to position: " + std::to_string(seconds) + " seconds");

    // 暂停播放
    bool wasPlaying = m_playState.GetCurrentState() == AVPlayState::Playing;
    if (wasPlaying)
    {
        m_playInfo->PauseAudio();
    }

    // 重新打开文件以获取新的文件上下文
    auto openFileResult = OpenMediaFile(GetCurrentFilePath());
    if (!openFileResult)
    {
        LOG_WARN("Failed to reopen file for seek operation");
        return false;
    }

    // 清空音频缓冲区和设备缓冲区
    m_playInfo->FlushAudioStream();
    m_playInfo->ClearAudioDeviceBuffer();
    {
        std::lock_guard<std::recursive_mutex> buffer_lock(m_bufferMutex);
        m_audioBuffer.clear();
        m_audioBuffer.shrink_to_fit();
    }

    // 更新音频流索引
    m_audioStreamIdx = openFileResult->m_audioStreamIdx;

    // 重置重采样参数更新标记
    m_bResampleParamsUpdated = false;

    // 更新播放起始时间
    RecordPlayStartTime(seconds);

    // 如果正在播放，重新启动数据读取
    if (wasPlaying)
    {
        // 重新处理音频数据，从seek位置开始
        ProcessAudioData(*openFileResult, *m_resampler, *m_resampleParams, seconds);
        m_playInfo->ResumeAudio();
    }

    LOG_INFO("Audio seek completed successfully to position: " + std::to_string(seconds) + " seconds");
    return true;
}

void AudioFFmpegPlayer::SeekPlay(double seconds)
{
    if (m_playState.GetCurrentState() == AVPlayState::Playing)
    {
        SeekAudio(seconds);
    }
}

void AudioFFmpegPlayer::SetInputDevice(const QString& deviceName)
{
    m_currentInputDevice = deviceName;
}

double AudioFFmpegPlayer::GetCurrentPosition()
{
    // 使用基类的计算方法
    return CalculateCurrentPosition();
}

void AudioFFmpegPlayer::ResetPlayerState()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    LOG_INFO("Resetting AudioFFmpegPlayer player state");

    // 强制停止播放和录制
    ForceStop();

    // 清理音频播放资源
    PlayerStateReSet();

    // 清理录制设备
    m_recordDevice.reset();

    // 调用基类的重置方法
    BaseFFmpegPlayer::ResetPlayerState();

    LOG_INFO("AudioFFmpegPlayer player state reset completed");
}

void AudioFFmpegPlayer::ForceStop()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    LOG_INFO("Force stopping AudioFFmpegPlayer");

    // 强制停止播放
    if (m_playInfo)
    {
        m_playInfo->StopAudio();
        m_playInfo->UnbindStreamAndDevice();
    }

    // 强制停止录制
    if (m_recordDevice)
    {
        m_recordDevice.reset();
    }

    // 调用基类的强制停止
    BaseFFmpegPlayer::ForceStop();

    // 重置播放状态
    m_playState.Reset();

    LOG_INFO("AudioFFmpegPlayer force stop completed");
}
