#include "AudioResampler.h"
#include <QDebug>

AudioResampler::AudioResampler()
    : m_lastInLayout(nullptr), m_lastOutLayout(nullptr)
{
    m_lastInFmt.sampleFormat = AV_SAMPLE_FMT_NONE;
    m_lastOutFmt.sampleFormat = AV_SAMPLE_FMT_NONE;
}

void AudioResampler::Resample(const uint8_t** inputData, int inputSamples, ST_ResampleResult& output, const ST_ResampleParams& params)
{
    // 验证输入
    if (!inputData || inputSamples <= 0)
    {
        qWarning() << "Resample() : Invalid input parameters";
        return;
    }

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
    if (!m_swrCtx.GetRawContext() ||
        inLayout != m_lastInLayout.GetRawLayout()->nb_channels ||
        outLayout != m_lastOutLayout.GetRawLayout()->nb_channels ||
        params.GetInput().GetSampleRate() != m_lastInRate ||
        params.GetOutput().GetSampleRate() != m_lastOutRate ||
        params.GetInput().GetSampleFormat().sampleFormat != m_lastInFmt.sampleFormat ||
        params.GetOutput().GetSampleFormat().sampleFormat != m_lastOutFmt.sampleFormat)
    {
        SwrContext* p = m_swrCtx.GetRawContext();
        // 释放旧上下文
        if (m_swrCtx.GetRawContext())
        {
            swr_free(&p);
        }

        // 创建新上下文
        SwrContext** ctx = &p;
        int ret = swr_alloc_set_opts2(ctx,
                                      params.GetOutput().GetChannelLayout().GetRawLayout(),
                                      params.GetOutput().GetSampleFormat().sampleFormat,
                                      params.GetOutput().GetSampleRate(),
                                      params.GetInput().GetChannelLayout().GetRawLayout(),
                                      params.GetInput().GetSampleFormat().sampleFormat,
                                      params.GetInput().GetSampleRate(),
                                      0, nullptr);

        if (ret < 0)
        {
            qWarning() << "Resample() : Failed to allocate resampler context";
            return;
        }

        // 初始化
        ret = m_swrCtx.SwrContextInit();
        if (ret < 0)
        {
            qWarning() << "Resample() : Failed to initialize resampler context";
            return;
        }
    }

    // 保存当前参数
    m_lastInLayout = ST_AVChannelLayout(params.GetInput().GetChannelLayout().GetRawLayout());
    m_lastOutLayout = ST_AVChannelLayout(params.GetOutput().GetChannelLayout().GetRawLayout());
    m_lastInRate = params.GetInput().GetSampleRate();
    m_lastOutRate = params.GetOutput().GetSampleRate();
    m_lastInFmt = params.GetInput().GetSampleFormat();
    m_lastOutFmt = params.GetOutput().GetSampleFormat();

    // 计算输出样本数
    int outSamples = swr_get_out_samples(m_swrCtx.GetRawContext(), inputSamples);
    if (outSamples < 0)
    {
        outSamples = av_rescale_rnd(
                                    swr_get_delay(m_swrCtx.GetRawContext(), params.GetInput().GetSampleRate()) + inputSamples,
                                    params.GetOutput().GetSampleRate(),
                                    params.GetInput().GetSampleRate(),
                                    AV_ROUND_UP);
    }

    // 分配输出缓冲区
    int outChannels = params.GetOutput().GetChannels();
    size_t bufSize = av_samples_get_buffer_size(nullptr, outChannels, outSamples,
                                                params.GetOutput().GetSampleFormat().sampleFormat, 1);

    if (bufSize <= 0)
    {
        qWarning() << "Resample() : Invalid buffer size calculated";
        return;
    }

    std::vector<uint8_t> tempData(bufSize);
    uint8_t* outBuf = tempData.data();

    // 执行重采样
    int realOutSamples = swr_convert(m_swrCtx.GetRawContext(), &outBuf, outSamples, inputData, inputSamples);
    if (realOutSamples < 0)
    {
        qWarning() << "Resample() : Resampling failed";
        return;
    }

    // 更新实际输出大小
    size_t realBufSize = av_samples_get_buffer_size(nullptr, outChannels, realOutSamples,
                                                    params.GetOutput().GetSampleFormat().sampleFormat, 1);

    tempData.resize(realBufSize);
    output.SetData(std::move(tempData));

    // 填充结果信息
    output.SetSamples(realOutSamples);
    output.SetChannels(outChannels);
    output.SetSampleRate(params.GetOutput().GetSampleRate());
    output.SetSampleFormat(params.GetOutput().GetSampleFormat());
}

void AudioResampler::Flush(ST_ResampleResult& output, const ST_ResampleParams& params)
{
    if (!m_swrCtx.GetRawContext())
    {
        qWarning() << "Flush() : No resampler context available";
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

    av_channel_layout_default(params.GetOutput().GetChannelLayout().GetRawLayout(), params.GetOutput().GetChannels());
    int outChannels = params.GetOutput().GetChannels();
    int bufSize = av_samples_get_buffer_size(nullptr, outChannels, delaySamples, m_lastOutFmt.sampleFormat, 1);

    std::vector<uint8_t> tempData(bufSize);
    uint8_t* outBuf = tempData.data();

    // 获取剩余数据
    int realOutSamples = swr_convert(m_swrCtx.GetRawContext(), &outBuf, delaySamples, nullptr, 0);
    if (realOutSamples < 0)
    {
        qWarning() << "Flush() : Failed to flush resampler";
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
