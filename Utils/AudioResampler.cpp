#include "AudioResampler.h"

AudioResampler::AudioResampler()
    : m_lastInLayout{}, m_lastOutLayout{}
{
    m_lastInFmt = AV_SAMPLE_FMT_NONE;
    m_lastOutFmt = AV_SAMPLE_FMT_NONE;
}

void AudioResampler::Resample(const uint8_t** inputData, int inputSamples, ST_ResampleResult& output, const ST_ResampleParams& params)
{
    // 验证输入
    if (!inputData || inputSamples <= 0)
    {
        return;
    }

    // 自动推导声道布局
    uint64_t inLayout = params.input.m_channelLayout.channel->nb_channels;
    if (inLayout == 0)
    {
        inLayout = params.input.m_channels;
    }

    uint64_t outLayout = params.output.m_channelLayout.channel->nb_channels;
    if (outLayout == 0)
    {
        outLayout = params.output.m_channels;
    }

    // 检查是否需要重新初始化
    if (!m_swrCtx.m_swrCtx || inLayout != m_lastInLayout.nb_channels || outLayout != m_lastOutLayout.nb_channels 
        || params.input.m_sampleRate != m_lastInRate || params.output.m_sampleRate != m_lastOutRate || params.input.m_sampleFmt.sampleFormat != m_lastInFmt
        || params.output.m_sampleFmt.sampleFormat != m_lastOutFmt)
    {
        // 释放旧上下文
        if (m_swrCtx.m_swrCtx)
        {
            swr_free(&m_swrCtx.m_swrCtx);
        }

        // 创建新上下文
        swr_alloc_set_opts2(&m_swrCtx.m_swrCtx, params.output.m_channelLayout.channel, params.output.m_sampleFmt.sampleFormat, params.output.m_sampleRate,
            params.input.m_channelLayout.channel, params.input.m_sampleFmt.sampleFormat, params.input.m_sampleRate, 0, nullptr);

        if (!m_swrCtx.m_swrCtx)
        {
            return;
        }

        // 初始化
        int ret = swr_init(m_swrCtx.m_swrCtx);
        if (ret < 0)
        {
            return;
        }

    }
    // 保存当前参数
    m_lastInLayout.nb_channels = inLayout;
    m_lastOutLayout.nb_channels = outLayout;
    m_lastInRate = params.input.m_sampleRate;
    m_lastOutRate = params.output.m_sampleRate;
    m_lastInFmt = params.input.m_sampleFmt.sampleFormat;
    m_lastOutFmt = params.output.m_sampleFmt.sampleFormat;
    // 计算输出样本数
    int outSamples = swr_get_out_samples(m_swrCtx.m_swrCtx, inputSamples);
    if (outSamples < 0)
    {
        outSamples = av_rescale_rnd(swr_get_delay(m_swrCtx.m_swrCtx, params.input.m_sampleRate) + inputSamples,  // NOLINT(clang-diagnostic-shorten-64-to-32)
            params.output.m_sampleRate, params.input.m_sampleRate, AV_ROUND_UP);
    }

    // 分配输出缓冲区
    int outChannels = params.output.m_channels;
    size_t bufSize = av_samples_get_buffer_size(nullptr, outChannels, outSamples, params.output.m_sampleFmt.sampleFormat, 1);

    if (bufSize <= 0)
    {
        return;
    }

    output.data.resize(bufSize);
    uint8_t* outBuf = output.data.data();

    // 执行重采样
    int realOutSamples = swr_convert(m_swrCtx.m_swrCtx, &outBuf, outSamples, inputData, inputSamples);

    if (realOutSamples < 0)
    {
        return;
    }

    // 更新实际输出大小
    size_t realBufSize = av_samples_get_buffer_size(nullptr, outChannels, realOutSamples, params.output.m_sampleFmt.sampleFormat, 1);

    if (realBufSize < bufSize)
    {
        output.data.resize(realBufSize);
    }

    // 填充结果信息
    output.m_samples = realOutSamples;
    output.m_channels = outChannels;
    output.m_sampleRate = params.output.m_sampleRate;
    output.m_sampleFmt.sampleFormat = params.output.m_sampleFmt.sampleFormat;
}

void AudioResampler::Flush(ST_ResampleResult& output, const ST_ResampleParams& params)
{
    if (!m_swrCtx.m_swrCtx)
    {
        return;
    }

    // 估算剩余样本
    int delaySamples = swr_get_delay(m_swrCtx.m_swrCtx, params.output.m_sampleRate);
    if (delaySamples <= 0)
    {
        output.data.clear();
        return;
    }

    av_channel_layout_default(params.output.m_channelLayout.channel,params.output.m_channels);
    int outChannels = params.output.m_channels;
    int bufSize = av_samples_get_buffer_size(nullptr, outChannels, delaySamples, m_lastOutFmt, 1);

    output.data.resize(bufSize);
    uint8_t* outBuf = output.data.data();

    // 获取剩余数据
    int realOutSamples = swr_convert(m_swrCtx.m_swrCtx, &outBuf, delaySamples, nullptr, 0);
    if (realOutSamples < 0)
    {
        return;
    }

    // 调整实际大小
    size_t realBufSize = av_samples_get_buffer_size(nullptr, outChannels, realOutSamples, m_lastOutFmt, 1);

    if (realBufSize < static_cast<size_t>(bufSize))
    {
        output.data.resize(realBufSize);
    }

    // 填充结果
    output.m_samples = realOutSamples;
    output.m_channels = outChannels;
    output.m_sampleRate = m_lastOutRate;
    output.m_sampleFmt.sampleFormat = m_lastOutFmt;
}
