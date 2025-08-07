#include "AudioPlayerUtils.h"

#include "BaseDataDefine/ST_AVFrame.h"
#include "BaseDataDefine/ST_AVInputFormat.h"
#include "BaseDataDefine/ST_AVPacket.h"
#include "BasePlayer/FFmpegPublicUtils.h"
#include "FileSystem/FileSystem.h"
#include "TimeSystem/TimeSystem.h"

AudioPlayerUtils::AudioPlayerUtils(QObject* parent)
    : QObject(parent)
{
}

AudioPlayerUtils::~AudioPlayerUtils()
{
}


void AudioPlayerUtils::ProcessAudioFrame(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample)
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

void AudioPlayerUtils::ProcessFloatSamples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample)
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

void AudioPlayerUtils::ProcessInt16Samples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample)
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

void AudioPlayerUtils::ProcessInt32Samples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample)
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

QStringList AudioPlayerUtils::GetInputAudioDevices()
{
    QStringList devices;
    ST_AVInputFormat inputFormat;
    inputFormat.FindInputFormat(FMT_NAME);
    devices = inputFormat.GetDeviceLists(nullptr, nullptr);
    return devices;
}

bool AudioPlayerUtils::LoadAudioWaveform(const QString& filePath, QVector<float>& waveformData)
{
    TIME_START("AudioWaveformLoading");
    LOG_INFO("=== Starting audio waveform loading for: " + filePath.toStdString() + " ===");

    if (filePath.isEmpty() || !my_sdk::FileSystem::Exists(filePath.toStdString()))
    {
        LOG_WARN("LoadAudioWaveform() : Invalid file path");
        return false;
    }

    AV_TIMER_START("Audio", "WaveformFileOpen");
    ST_OpenFileResult openFileResult;
    openFileResult.OpenFilePath(filePath);

    if (!openFileResult.m_formatCtx || !openFileResult.m_codecCtx)
    {
        LOG_WARN("LoadAudioWaveform() : Failed to open audio file");
        TimeSystem::Instance().StopTimingWithLog("AudioWaveformLoading", EM_TimingLogLevel::Warning, EM_TimeUnit::Milliseconds, "Failed to open waveform file");
        return false;
    }

    // 清空之前的数据
    waveformData.clear();

    // 读取音频数据并计算波形
    ST_AVPacket packet;
    ST_AVFrame frame;
    const int SAMPLES_PER_PIXEL = 1024; // 每个像素点对应的采样数
    float maxSample = 0.0f;
    float currentSum = 0.0f;
    int sampleCount = 0;

    int processedPackets = 0;


    while (packet.ReadPacket(openFileResult.m_formatCtx->GetRawContext()))
    {
        if (packet.GetStreamIndex() == openFileResult.m_audioStreamIdx)
        {
            if (packet.SendPacket(openFileResult.m_codecCtx->GetRawContext()))
            {
                while (frame.GetCodecFrame(openFileResult.m_codecCtx->GetRawContext()))
                {
                    // 处理不同的采样格式
                    AudioPlayerUtils::ProcessAudioFrame(frame.GetRawFrame(), waveformData, SAMPLES_PER_PIXEL, currentSum, sampleCount, maxSample);
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

    LOG_INFO("=== Waveform loading completed, generated " + std::to_string(waveformData.size()) + " data points ===");

    TimeSystem::Instance().StopTimingWithLog("AudioWaveformLoading", EM_TimingLogLevel::Info, EM_TimeUnit::Milliseconds, "Waveform loading completed");
    return !waveformData.isEmpty();
}
