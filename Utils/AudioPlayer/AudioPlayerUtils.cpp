#include "AudioPlayerUtils.h"

#include "BaseDataDefine/ST_AVInputFormat.h"
#include "BasePlayer/FFmpegPublicUtils.h"


AudioPlayerUtils::AudioPlayerUtils(QObject *parent)
    : QObject(parent)
{
    
}

AudioPlayerUtils::~AudioPlayerUtils()
{
    
}

void AudioPlayerUtils::ResigsterDevice()
{
    avdevice_register_all();
    if (SDL_Init(SDL_INIT_AUDIO))
    {
        LOG_WARN("SDL_Init failed:" + std::string(SDL_GetError()));
    }
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
