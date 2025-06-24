#include "AudioResampler.h"
#include <memory>
#include <QDebug>
#include <vector>
#include <chrono>
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
    auto startTime = std::chrono::high_resolution_clock::now();
    
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
        auto initStartTime = std::chrono::high_resolution_clock::now();
        if (!InitializeResampler(params))
        {
            LOG_ERROR("Failed to initialize resampler");
            output.SetData(std::vector<uint8_t>());
            return;
        }
        auto initEndTime = std::chrono::high_resolution_clock::now();
        auto initDuration = std::chrono::duration_cast<std::chrono::milliseconds>(initEndTime - initStartTime).count();
        LOG_INFO("Resampler context initialized in " + std::to_string(initDuration) + " ms");
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

    AVSampleFormat outFormat = params.GetOutput().GetSampleFormat().sampleFormat;
    int bytesPerSample = av_get_bytes_per_sample(outFormat);
    if (bytesPerSample <= 0)
    {
        LOG_WARN("Resample() : Invalid sample format: " + std::to_string(outFormat));
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // 计算缓冲区大小，确保对齐
    int linesize;
    int bufSize = av_samples_get_buffer_size(&linesize, outChannels, outSamples, outFormat, 1);
    if (bufSize <= 0)
    {
        LOG_WARN("Resample() : Invalid buffer size calculated: " + std::to_string(bufSize));
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // 分配对齐的内存，增加更多填充
    const int EXTRA_PADDING = 1024;
    auto alignedBuffer = static_cast<uint8_t*>(av_malloc(bufSize + AV_INPUT_BUFFER_PADDING_SIZE + EXTRA_PADDING));
    if (!alignedBuffer)
    {
        LOG_ERROR("Resample() : Failed to allocate aligned buffer");
        output.SetData(std::vector<uint8_t>());
        return;
    }

    // 清零整个缓冲区，包括填充区域
    memset(alignedBuffer, 0, bufSize + AV_INPUT_BUFFER_PADDING_SIZE + EXTRA_PADDING);

    try
    {
        // 执行重采样
        auto resampleStartTime = std::chrono::high_resolution_clock::now();
        int realOutSamples = swr_convert(m_swrCtx.GetRawContext(), &alignedBuffer, outSamples, inputData, inputSamples);
        auto resampleEndTime = std::chrono::high_resolution_clock::now();
        
        auto resampleDuration = std::chrono::duration_cast<std::chrono::microseconds>(resampleEndTime - resampleStartTime).count();
        
        if (realOutSamples < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(realOutSamples, errbuf, sizeof(errbuf));
            LOG_ERROR("Resample Failed with error: " + std::to_string(realOutSamples) + " - " + std::string(errbuf) + " (took " + std::to_string(resampleDuration) + " μs)");
            av_free(alignedBuffer);
            output.SetData(std::vector<uint8_t>());
            return;
        }

        // 只在耗时较长时记录详细信息
        if (resampleDuration > 500) // 大于0.5ms
        {
            LOG_DEBUG("Resampling " + std::to_string(inputSamples) + " samples took " + std::to_string(resampleDuration) + " μs, output: " + std::to_string(realOutSamples) + " samples");
        }

        // 计算实际使用的缓冲区大小
        int realBufSize = av_samples_get_buffer_size(&linesize, outChannels, realOutSamples, outFormat, 1);
        if (realBufSize > 0 && realBufSize <= bufSize)
        {
            // 将重采样后的数据复制到输出向量
            std::vector<uint8_t> tempData(alignedBuffer, alignedBuffer + realBufSize);
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
    } catch (const std::exception& e)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        LOG_ERROR("Exception during resampling after " + std::to_string(duration) + " μs: " + std::string(e.what()));
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
    ST_ResampleSimpleData params;

    // 设置稳定的输出参数
    params.SetSampleRate(44100);                                  // 标准采样率
    params.SetSampleFormat(ST_AVSampleFormat(AV_SAMPLE_FMT_S16)); // 16位整数格式

    auto outLayout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
    if (outLayout)
    {
        av_channel_layout_default(outLayout, 2); // 立体声
        params.SetChannelLayout(ST_AVChannelLayout(outLayout));
    }

    return params;
}

bool AudioResampler::InitializeResampler(ST_ResampleParams& params)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    LOG_INFO("Initializing audio resampler...");
    
    try
    {
        // 验证输入参数
        if (!params.GetInput().GetChannelLayout().GetRawLayout() || !params.GetOutput().GetChannelLayout().GetRawLayout())
        {
            LOG_ERROR("InitializeResampler() : Invalid channel layout");
            return false;
        }

        // 获取通道布局
        uint64_t inChannels = params.GetInput().GetChannelLayout().GetRawLayout()->nb_channels;
        uint64_t outChannels = params.GetOutput().GetChannelLayout().GetRawLayout()->nb_channels;

        // 验证通道数
        if (inChannels == 0 || inChannels > 8 || outChannels == 0 || outChannels > 8)
        {
            LOG_ERROR("InitializeResampler() : Invalid channel count - in: " + std::to_string(inChannels) + ", out: " + std::to_string(outChannels));
            return false;
        }

        // 验证采样率
        if (params.GetInput().GetSampleRate() <= 0 || params.GetInput().GetSampleRate() > 192000 || params.GetOutput().GetSampleRate() <= 0 || params.GetOutput().GetSampleRate() > 192000)
        {
            LOG_ERROR("InitializeResampler() : Invalid sample rate - in: " + std::to_string(params.GetInput().GetSampleRate()) + ", out: " + std::to_string(params.GetOutput().GetSampleRate()));
            return false;
        }

        // 检查是否需要重新初始化
        bool needReinit = !m_swrCtx.GetRawContext() || 
                         inChannels != m_lastInLayout.GetRawLayout()->nb_channels || 
                         outChannels != m_lastOutLayout.GetRawLayout()->nb_channels || 
                         params.GetInput().GetSampleRate() != m_lastInRate || 
                         params.GetOutput().GetSampleRate() != m_lastOutRate || 
                         params.GetInput().GetSampleFormat().sampleFormat != m_lastInFmt.sampleFormat || 
                         params.GetOutput().GetSampleFormat().sampleFormat != m_lastOutFmt.sampleFormat;

        if (needReinit)
        {
            auto contextCreateStartTime = std::chrono::high_resolution_clock::now();
            
            SwrContext* p = m_swrCtx.GetRawContext();
            // 释放旧上下文
            if (p)
            {
                swr_free(&p);
                m_swrCtx.SetRawContext(nullptr);
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
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                LOG_ERROR("InitializeResampler() : Failed to allocate resampler context: " + std::string(errbuf));
                return false;
            }

            if (!p)
            {
                LOG_ERROR("InitializeResampler() : Resampler context is null after allocation");
                return false;
            }

            m_swrCtx.SetRawContext(p);

            // 设置重采样选项，使用高质量但稳定的设置
            av_opt_set_int(m_swrCtx.GetRawContext(), "resampler", SWR_ENGINE_SWR, 0);
            av_opt_set_int(m_swrCtx.GetRawContext(), "filter_size", 32, 0);  // 更高质量的滤波器
            av_opt_set_int(m_swrCtx.GetRawContext(), "phase_shift", 10, 0);  // 更精确的相位偏移
            av_opt_set_double(m_swrCtx.GetRawContext(), "cutoff", 0.98, 0);  // 高质量截止频率
            av_opt_set_int(m_swrCtx.GetRawContext(), "linear_interp", 1, 0); // 启用线性插值

            // 初始化
            ret = m_swrCtx.SwrContextInit();
            if (ret < 0)
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                LOG_ERROR("InitializeResampler() : Failed to initialize resampler context: " + std::string(errbuf));
                swr_free(&p);
                m_swrCtx.SetRawContext(nullptr);
                return false;
            }

            // 保存当前参数
            m_lastInLayout = ST_AVChannelLayout(params.GetInput().GetChannelLayout().GetRawLayout());
            m_lastOutLayout = ST_AVChannelLayout(params.GetOutput().GetChannelLayout().GetRawLayout());
            m_lastInRate = params.GetInput().GetSampleRate();
            m_lastOutRate = params.GetOutput().GetSampleRate();
            m_lastInFmt = params.GetInput().GetSampleFormat();
            m_lastOutFmt = params.GetOutput().GetSampleFormat();

            auto contextCreateEndTime = std::chrono::high_resolution_clock::now();
            auto contextCreateDuration = std::chrono::duration_cast<std::chrono::milliseconds>(contextCreateEndTime - contextCreateStartTime).count();
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            LOG_INFO("Resampler context created and initialized successfully in " + std::to_string(totalDuration) + " ms (context creation: " + std::to_string(contextCreateDuration) + " ms)");
            LOG_INFO("Input: " + std::to_string(m_lastInRate) + "Hz, " + std::to_string(inChannels) + " channels, format: " + std::to_string(m_lastInFmt.sampleFormat));
            LOG_INFO("Output: " + std::to_string(m_lastOutRate) + "Hz, " + std::to_string(outChannels) + " channels, format: " + std::to_string(m_lastOutFmt.sampleFormat));
        }

        return true;
    } 
    catch (const std::exception& e)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        LOG_ERROR("Exception in InitializeResampler after " + std::to_string(duration) + " ms: " + std::string(e.what()));
        return false;
    } 
    catch (...)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        LOG_ERROR("Unknown exception in InitializeResampler after " + std::to_string(duration) + " ms");
        return false;
    }
}
