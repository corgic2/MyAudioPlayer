#include "AudioResampler.h"
#include <memory>
#include <QDebug>
#include <vector>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include "LogSystem/LogSystem.h"
#include "TimeSystem/TimeSystem.h"

AudioResampler::AudioResampler()
    : m_lastInLayout(nullptr), m_lastOutLayout(nullptr)
{
    m_lastInFmt.sampleFormat = AV_SAMPLE_FMT_NONE;
    m_lastOutFmt.sampleFormat = AV_SAMPLE_FMT_NONE;
}

void AudioResampler::Resample(const uint8_t** inputData, int inputSamples, ST_ResampleResult& output, ST_ResampleParams& params)
{
    AUTO_TIMER_LOG("AudioResampler", EM_TimingLogLevel::Debug);
    
    // 验证输入
    if (!inputData || inputSamples <= 0)
    {
        LOG_ERROR("Resampling failed: Invalid input parameters");
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // 验证重采样器上下文
    if (!m_swrCtx.GetRawContext())
    {
        TIME_START("ResamplerInit");
        if (!InitializeResampler(params))
        {
            LOG_ERROR("Failed to initialize resampler");
            output.SetData(std::vector<uint8_t>());
            return;
        }
        TimeSystem::Instance().StopTimingWithLog("ResamplerInit", EM_TimingLogLevel::Info, EM_TimeUnit::Milliseconds, "Resampler context initialized");
    }

    // 验证输入数据
    if (!inputData[0])
    {
        LOG_ERROR("Resampling failed: Input data is null");
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // 计算输出样本数，使用更安全的计算方式
    int64_t delay = swr_get_delay(m_swrCtx.GetRawContext(), params.GetInput().GetSampleRate());
    int outSamples = static_cast<int>(av_rescale_rnd(delay + inputSamples, params.GetOutput().GetSampleRate(), params.GetInput().GetSampleRate(), AV_ROUND_UP));

    // 确保输出样本数合理，添加更多安全检查
    outSamples = std::max(outSamples, inputSamples);
    outSamples = std::max(outSamples, 1024);  // 最小缓冲区
    outSamples = std::min(outSamples, 65536); // 最大缓冲区限制

    // 验证输出样本数
    if (outSamples <= 0)
    {
        LOG_WARN("Resample() : Invalid output samples count: " + std::to_string(outSamples));
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // Get channels count and sample format
    int outChannels = params.GetOutput().GetChannelLayout().GetRawLayout()->nb_channels;
    if (outChannels <= 0 || outChannels > 8)
    {
        LOG_WARN("Resample() : Invalid channel count: " + std::to_string(outChannels));
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // 计算输出缓冲区大小
    int bufSize = av_samples_get_buffer_size(nullptr, outChannels, outSamples, params.GetOutput().GetSampleFormat().sampleFormat, 1);
    if (bufSize <= 0)
    {
        LOG_WARN("Resample() : Invalid buffer size: " + std::to_string(bufSize));
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // 使用对齐分配以提高性能和避免访问冲突
    uint8_t* alignedBuffer = static_cast<uint8_t*>(av_malloc(bufSize + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!alignedBuffer)
    {
        LOG_ERROR("Resample() : Failed to allocate aligned buffer");
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // 清零padding区域
    memset(alignedBuffer + bufSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    try
    {
        uint8_t* outBuf = alignedBuffer;

        // 执行重采样
        TIME_START("SwrConvert");
        int realOutSamples = swr_convert(m_swrCtx.GetRawContext(), &outBuf, outSamples, inputData, inputSamples);
        double convertDuration = TimeSystem::Instance().StopTiming("SwrConvert", EM_TimeUnit::Microseconds);

        if (realOutSamples < 0)
        {
            char error_buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(realOutSamples, error_buffer, AV_ERROR_MAX_STRING_SIZE);
            LOG_ERROR("Resample conversion failed: " + std::string(error_buffer));
            output.SetData(std::vector<uint8_t>());
        }
        else if (realOutSamples == 0)
        {
            LOG_DEBUG("Resample() : No output samples generated");
            output.SetData(std::vector<uint8_t>());
        }
        else
        {
            // 只在转换耗时较长时记录
            if (convertDuration > 500) // 大于0.5ms
            {
                LOG_DEBUG("Resampling conversion took " + std::to_string(convertDuration) + " μs");
            }

            // 计算实际输出缓冲区大小
            size_t realBufSize = av_samples_get_buffer_size(nullptr, outChannels, realOutSamples, params.GetOutput().GetSampleFormat().sampleFormat, 1);

            if (realBufSize > 0 && realBufSize <= static_cast<size_t>(bufSize))
            {
                // 创建输出向量并复制数据
                std::vector<uint8_t> tempData(realBufSize);
                memcpy(tempData.data(), alignedBuffer, realBufSize);
                output.SetData(std::move(tempData));

                // 填充结果信息
                output.SetSamples(realOutSamples);
                output.SetChannels(outChannels);
                output.SetSampleRate(params.GetOutput().GetSampleRate());
                output.SetSampleFormat(params.GetOutput().GetSampleFormat());
            }
            else
            {
                LOG_WARN("Invalid real buffer size: " + std::to_string(realBufSize));
                output.SetData(std::vector<uint8_t>());
            }
        }
    } catch (const std::exception& e)
    {
        LOG_ERROR("Exception during resampling: " + std::string(e.what()));
        output.SetData(std::vector<uint8_t>());
    } catch (...)
    {
        LOG_ERROR("Unknown exception during resampling");
        output.SetData(std::vector<uint8_t>());
    }

    // 释放对齐的缓冲区
    av_free(alignedBuffer);
}

void AudioResampler::Flush(ST_ResampleResult& output, ST_ResampleParams& params)
{
    TIME_START("ResamplerFlush");
    
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

    TimeSystem::Instance().StopTimingWithLog("ResamplerFlush", EM_TimingLogLevel::Debug, EM_TimeUnit::Microseconds, "Resampler flushed");
}

ST_ResampleParams AudioResampler::GetResampleParams(const QString& format)
{
    ST_ResampleParams params;

    // 注意：这个方法现在主要用作默认参数，实际参数应该从文件中获取
    // 设置默认输出参数
    ST_ResampleSimpleData outputParams = GetDefaultOutputParams();
    params.SetOutput(outputParams);

    // 设置默认输入参数（会被实际文件参数覆盖）
    params.GetInput().SetSampleRate(44100);
    params.GetInput().SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16));

    auto inLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
    if (inLayout)
    {
        av_channel_layout_default(inLayout, 2);
        params.GetInput().SetChannelLayout(ST_AVChannelLayout(inLayout));
    }

    return params;
}

ST_ResampleSimpleData AudioResampler::GetDefaultOutputParams() const
{
    ST_ResampleSimpleData output;
    output.SetSampleRate(44100);
    output.SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16));

    auto outLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
    if (outLayout)
    {
        av_channel_layout_default(outLayout, 2);
        output.SetChannelLayout(ST_AVChannelLayout(outLayout));
    }

    return output;
}

bool AudioResampler::InitializeResampler(ST_ResampleParams& params)
{
    TIME_START("ResamplerContextInit");
    
    // 检查参数是否有变化
    bool needReinit = false;
    if (m_lastInRate != params.GetInput().GetSampleRate() ||
        m_lastOutRate != params.GetOutput().GetSampleRate() ||
        m_lastInFmt.sampleFormat != params.GetInput().GetSampleFormat().sampleFormat ||
        m_lastOutFmt.sampleFormat != params.GetOutput().GetSampleFormat().sampleFormat)
    {
        needReinit = true;
    }

    // 如果上下文已存在且需要重新初始化，则创建新的上下文
    if (m_swrCtx.GetRawContext() && needReinit)
    {
        LOG_INFO("Parameters changed, reinitializing resampler");
        // ST_SwrContext的析构函数会自动释放资源，创建新实例
        m_swrCtx = ST_SwrContext();
    }

    if (!m_swrCtx.GetRawContext())
    {
        // 分配新的重采样上下文
        SwrContext* newCtx = swr_alloc();
        if (!newCtx)
        {
            LOG_ERROR("Failed to allocate resampler context");
            return false;
        }

        m_swrCtx.SetRawContext(newCtx);

        // 设置输入参数
        av_opt_set_chlayout(m_swrCtx.GetRawContext(), "in_chlayout", params.GetInput().GetChannelLayout().GetRawLayout(), 0);
        av_opt_set_int(m_swrCtx.GetRawContext(), "in_sample_rate", params.GetInput().GetSampleRate(), 0);
        av_opt_set_sample_fmt(m_swrCtx.GetRawContext(), "in_sample_fmt", params.GetInput().GetSampleFormat().sampleFormat, 0);

        // 设置输出参数
        av_opt_set_chlayout(m_swrCtx.GetRawContext(), "out_chlayout", params.GetOutput().GetChannelLayout().GetRawLayout(), 0);
        av_opt_set_int(m_swrCtx.GetRawContext(), "out_sample_rate", params.GetOutput().GetSampleRate(), 0);
        av_opt_set_sample_fmt(m_swrCtx.GetRawContext(), "out_sample_fmt", params.GetOutput().GetSampleFormat().sampleFormat, 0);

        // 初始化重采样上下文
        if (m_swrCtx.SwrContextInit() < 0)
        {
            LOG_ERROR("Failed to initialize resampler context");
            // ST_SwrContext析构函数会自动释放资源
            m_swrCtx = ST_SwrContext();
            return false;
        }

        // 保存当前参数
        m_lastInRate = params.GetInput().GetSampleRate();
        m_lastOutRate = params.GetOutput().GetSampleRate();
        m_lastInFmt = params.GetInput().GetSampleFormat();
        m_lastOutFmt = params.GetOutput().GetSampleFormat();
        m_lastInLayout = params.GetInput().GetChannelLayout();
        m_lastOutLayout = params.GetOutput().GetChannelLayout();
    }

    TimeSystem::Instance().StopTimingWithLog("ResamplerContextInit", EM_TimingLogLevel::Debug, EM_TimeUnit::Microseconds, "Resampler context initialized");
    return true;
}
