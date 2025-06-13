#include "AudioResampler.h"
#include <memory>
#include <QDebug>
#include <vector>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include "LogSystem/LogSystem.h"

AudioResampler::AudioResampler()
    : m_lastInLayout(nullptr), m_lastOutLayout(nullptr)
{
    m_lastInFmt.sampleFormat = AV_SAMPLE_FMT_NONE;
    m_lastOutFmt.sampleFormat = AV_SAMPLE_FMT_NONE;
}

void AudioResampler::Resample(const uint8_t** inputData, int inputSamples, ST_ResampleResult& output, ST_ResampleParams& params)
{
    // 验证输入
    if (!inputData || inputSamples <= 0)
    {
        LOG_WARN("Resample() : Invalid input parameters");
        return;
    }

    // 初始化或更新重采样器
    if (!InitializeResampler(params))
    {
        return;
    }

    // 计算输出样本数
    int outSamples = swr_get_out_samples(m_swrCtx.GetRawContext(), inputSamples);
    if (outSamples < 0)
    {
        int64_t delay = swr_get_delay(m_swrCtx.GetRawContext(), params.GetInput().GetSampleRate());
        outSamples = static_cast<int>(av_rescale_rnd(delay + inputSamples, params.GetOutput().GetSampleRate(), params.GetInput().GetSampleRate(), AV_ROUND_UP));
    }

    // 验证输出样本数
    if (outSamples <= 0)
    {
        LOG_WARN("Resample() : Invalid output samples count");
        return;
    }

    // 获取通道数和采样格式
    int outChannels = params.GetOutput().GetChannels();
    if (outChannels <= 0)
    {
        LOG_WARN("Resample() : Invalid channel count");
        return;
    }

    AVSampleFormat outFormat = params.GetOutput().GetSampleFormat().sampleFormat;
    int bytesPerSample = av_get_bytes_per_sample(outFormat);
    if (bytesPerSample <= 0)
    {
        LOG_WARN("Resample() : Invalid sample format");
        return;
    }

    // 计算缓冲区大小，确保16字节对齐
    int linesize;
    int bufSize = av_samples_get_buffer_size(&linesize, outChannels, outSamples, outFormat, 16);
    if (bufSize <= 0)
    {
        LOG_WARN("Resample() : Invalid buffer size calculated");
        return;
    }

    // 分配对齐的内存
    auto alignedBuffer = static_cast<uint8_t*>(av_malloc(bufSize + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!alignedBuffer)
    {
        LOG_WARN("Resample() : Failed to allocate aligned buffer");
        return;
    }

    // 清零整个缓冲区，包括padding区域
    memset(alignedBuffer, 0, bufSize + AV_INPUT_BUFFER_PADDING_SIZE);

    // 执行重采样
    int realOutSamples = swr_convert(m_swrCtx.GetRawContext(), &alignedBuffer, outSamples, inputData, inputSamples);
    if (realOutSamples < 0)
    {
        LOG_WARN("Resample() : Resampling failed");
        av_free(alignedBuffer);
        return;
    }

    // 计算实际使用的缓冲区大小
    int realBufSize = av_samples_get_buffer_size(&linesize, outChannels, realOutSamples, outFormat, 16);
    if (realBufSize > 0)
    {
        // 将对齐的数据复制到输出向量
        std::vector<uint8_t> tempData(alignedBuffer, alignedBuffer + realBufSize);
        output.SetData(std::move(tempData));

        // 填充结果信息
        output.SetSamples(realOutSamples);
        output.SetChannels(outChannels);
        output.SetSampleRate(params.GetOutput().GetSampleRate());
        output.SetSampleFormat(params.GetOutput().GetSampleFormat());
    }

    // 释放对齐的缓冲区
    av_free(alignedBuffer);
}

void AudioResampler::Flush(ST_ResampleResult& output, ST_ResampleParams& params)
{
    if (!m_swrCtx.GetRawContext())
    {
        LOG_WARN("Flush() : No resampler context available");
        return;
    }

    // 估算剩余样本
    int delaySamples = swr_get_delay(m_swrCtx.GetRawContext(), params.GetOutput().GetSampleRate());
    if (delaySamples <= 0)
    {
        std::vector<uint8_t> emptyData;
        output.SetData(std::move(emptyData));
        return;
    }

    int outChannels = params.GetOutput().GetChannels();
    int bufSize = av_samples_get_buffer_size(nullptr, outChannels, delaySamples, m_lastOutFmt.sampleFormat, 1);

    std::vector<uint8_t> tempData(bufSize);
    uint8_t* outBuf = tempData.data();

    // 获取剩余数据
    int realOutSamples = swr_convert(m_swrCtx.GetRawContext(), &outBuf, delaySamples, nullptr, 0);
    if (realOutSamples < 0)
    {
        LOG_WARN("Flush() : Failed to flush resampler");
        return;
    }

    // 调整实际大小
    size_t realBufSize = av_samples_get_buffer_size(nullptr, outChannels, realOutSamples, m_lastOutFmt.sampleFormat, 1);
    tempData.resize(realBufSize);
    output.SetData(std::move(tempData));

    // 填充结果
    output.SetSamples(realOutSamples);
    output.SetChannels(outChannels);
    output.SetSampleRate(m_lastOutRate);
    output.SetSampleFormat(m_lastOutFmt);
}

ST_ResampleParams AudioResampler::GetResampleParams(const QString& format)
{
    ST_ResampleParams params;

    // 设置输出参数（使用默认高质量设置）
    ST_ResampleSimpleData outputParams = GetDefaultOutputParams();
    params.SetOutput(outputParams);

    // 根据不同格式设置输入参数
    if (format.toLower() == "mp3")
    {
        params.GetInput().SetSampleRate(44100);                                   // MP3标准采样率
        params.GetInput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_FLTP)); // MP3解码后的格式

        auto inLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        if (inLayout)
        {
            av_channel_layout_default(inLayout, 2); // MP3通常是立体声
            params.GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout));
        }
    }
    else if (format.toLower() == "wav")
    {
        params.GetInput().SetSampleRate(44100);                                  // WAV标准采样率
        params.GetInput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16)); // WAV通常使用16位整数格式

        auto inLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        if (inLayout)
        {
            av_channel_layout_default(inLayout, 2); // WAV通常是立体声
            params.GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout));
        }
    }
    else if (format.toLower() == "aac")
    {
        params.GetInput().SetSampleRate(48000);                                   // AAC通常使用48kHz
        params.GetInput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_FLTP)); // AAC通常使用平面浮点格式

        auto inLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        if (inLayout)
        {
            av_channel_layout_default(inLayout, 2); // AAC通常是立体声
            params.GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout));
        }
    }
    else if (format.toLower() == "ogg")
    {
        params.GetInput().SetSampleRate(44100);                                   // OGG通常使用44.1kHz
        params.GetInput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_FLTP)); // OGG通常使用平面浮点格式

        auto inLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        if (inLayout)
        {
            av_channel_layout_default(inLayout, 2); // OGG通常是立体声
            params.GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout));
        }
    }
    else if (format.toLower() == "flac")
    {
        params.GetInput().SetSampleRate(96000);                                  // FLAC支持高采样率
        params.GetInput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S32)); // FLAC通常使用32位整数格式

        auto inLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        if (inLayout)
        {
            av_channel_layout_default(inLayout, 2); // FLAC通常是立体声
            params.GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout));
        }
    }
    else
    {
        // 默认参数
        params.GetInput().SetSampleRate(44100);
        params.GetInput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16));

        auto inLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        if (inLayout)
        {
            av_channel_layout_default(inLayout, 2);
            params.GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout));
        }
    }
    params.GetOutput() = params.GetInput();
    return params;
}

ST_ResampleSimpleData AudioResampler::GetDefaultOutputParams() const
{
    ST_ResampleSimpleData params;

    // 设置标准高质量输出参数
    params.SetSampleRate(48000);                                  // 标准采样率
    params.SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16)); // 使用S16格式，确保更好的兼容性和质量

    auto outLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
    if (outLayout)
    {
        av_channel_layout_default(outLayout, 2); // 默认立体声
        params.SetChannelLayout(ST_AVChannelLayout(outLayout));
    }

    return params;
}

bool AudioResampler::InitializeResampler(ST_ResampleParams& params)
{
    // 自动推导声道布局
    uint64_t inLayout = params.GetInput().GetChannelLayout().GetRawLayout()->nb_channels;
    if (inLayout == 0)
    {
        inLayout = params.GetInput().GetChannels();
    }

    uint64_t outLayout = params.GetOutput().GetChannelLayout().GetRawLayout()->nb_channels;
    if (outLayout == 0)
    {
        outLayout = params.GetOutput().GetChannels();
    }

    // 检查是否需要重新初始化
    if (!m_swrCtx.GetRawContext() || inLayout != m_lastInLayout.GetRawLayout()->nb_channels || outLayout != m_lastOutLayout.GetRawLayout()->nb_channels || params.GetInput().GetSampleRate() != m_lastInRate || params.GetOutput().GetSampleRate() != m_lastOutRate || params.GetInput().GetSampleFormat().sampleFormat != m_lastInFmt.sampleFormat || params.GetOutput().GetSampleFormat().sampleFormat != m_lastOutFmt.sampleFormat)
    {
        SwrContext* p = m_swrCtx.GetRawContext();
        // 释放旧上下文
        if (m_swrCtx.GetRawContext())
        {
            swr_free(&p);
        }

        // 创建新上下文
        SwrContext** ctx = &p;
        int ret = swr_alloc_set_opts2(ctx, params.GetOutput().GetChannelLayout().GetRawLayout(), params.GetOutput().GetSampleFormat().sampleFormat, params.GetOutput().GetSampleRate(), params.GetInput().GetChannelLayout().GetRawLayout(), params.GetInput().GetSampleFormat().sampleFormat, params.GetInput().GetSampleRate(), 0, nullptr);

        if (ret < 0)
        {
            LOG_WARN("InitializeResampler() : Failed to allocate resampler context");
            return false;
        }
        m_swrCtx.SetRawContext(p);
        // 初始化
        ret = m_swrCtx.SwrContextInit();
        if (ret < 0)
        {
            LOG_WARN("InitializeResampler() : Failed to initialize resampler context");
            return false;
        }

        // 保存当前参数
        m_lastInLayout = ST_AVChannelLayout(params.GetInput().GetChannelLayout().GetRawLayout());
        m_lastOutLayout = ST_AVChannelLayout(params.GetOutput().GetChannelLayout().GetRawLayout());
        m_lastInRate = params.GetInput().GetSampleRate();
        m_lastOutRate = params.GetOutput().GetSampleRate();
        m_lastInFmt = params.GetInput().GetSampleFormat();
        m_lastOutFmt = params.GetOutput().GetSampleFormat();
    }

    return true;
}
