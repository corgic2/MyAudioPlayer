#pragma once

#include <cstdint>
#include <memory>
#include "../FFmpegRAII.h"

// 内存池管理的缓冲区类 - 解决动态内存分配缺乏池化管理问题
class ST_Buffer
{
public:
    ST_Buffer() = default;

    explicit ST_Buffer(size_t size)
        : m_buffer(size)
    {
    }

    ST_Buffer(const uint8_t* data, size_t size)
        : m_buffer(data, size)
    {
    }

    explicit ST_Buffer(const SafeBuffer& buffer)
        : m_buffer(buffer)
    {
    }

    ST_Buffer(const ST_Buffer& other)
        : m_buffer(other.m_buffer)
    {
    }

    ST_Buffer& operator=(const ST_Buffer& other)
    {
        if (this != &other)
        {
            m_buffer = other.m_buffer;
        }
        return *this;
    }

    ST_Buffer(ST_Buffer&& other) noexcept
        : m_buffer(std::move(other.m_buffer))
    {
    }

    ST_Buffer& operator=(ST_Buffer&& other) noexcept
    {
        if (this != &other)
        {
            m_buffer = std::move(other.m_buffer);
        }
        return *this;
    }

    ~ST_Buffer() = default;

    // 数据访问
    uint8_t* GetRawBuffer()
    {
        return m_buffer.data();
    }

    const uint8_t* GetRawBuffer() const
    {
        return m_buffer.data();
    }

    SafeBuffer& GetBuffer()
    {
        return m_buffer;
    }

    const SafeBuffer& GetBuffer() const
    {
        return m_buffer;
    }

    uint8_t* data()
    {
        return m_buffer.data();
    }

    const uint8_t* data() const
    {
        return m_buffer.data();
    }

    size_t size() const
    {
        return m_buffer.size();
    }

    size_t capacity() const
    {
        return m_buffer.capacity();
    }

    bool empty() const
    {
        return m_buffer.empty();
    }

    // 容量管理（带安全检查）
    bool resize(size_t new_size)
    {
        return m_buffer.resize(new_size);
    }

    bool reserve(size_t new_capacity)
    {
        return m_buffer.reserve(new_capacity);
    }

    // 数据操作
    bool assign(const uint8_t* data, size_t size)
    {
        return m_buffer.assign(data, size);
    }

    bool append(const uint8_t* data, size_t size)
    {
        return m_buffer.append(data, size);
    }

    bool insert(size_t pos, const uint8_t* data, size_t size)
    {
        return m_buffer.insert(pos, data, size);
    }

    void clear()
    {
        m_buffer.clear();
    }

    void shrink_to_fit()
    {
        m_buffer.shrink_to_fit();
    }

    // 安全访问
    uint8_t& operator[](size_t index)
    {
        return m_buffer[index];
    }

    const uint8_t& operator[](size_t index) const
    {
        return m_buffer[index];
    }

    // 工具方法
    static constexpr size_t max_size()
    {
        return SafeBuffer::max_size();
    }

    static bool is_valid_size(size_t size)
    {
        return SafeBuffer::is_valid_size(size);
    }

    // 工厂方法：创建预分配缓冲区
    static ST_Buffer Create(size_t size)
    {
        return ST_Buffer(size);
    }

    // 工厂方法：从数据创建
    static ST_Buffer FromData(const uint8_t* data, size_t size)
    {
        return ST_Buffer(data, size);
    }

    // 工厂方法：创建零初始化缓冲区
    static ST_Buffer CreateZeroed(size_t size)
    {
        ST_Buffer buffer(size);
        if (buffer.data())
        {
            std::fill(buffer.data(), buffer.data() + size, 0);
        }
        return buffer;
    }

    // 内存池分配方法
    static ST_Buffer AllocateFromPool(size_t size)
    {
        return ST_Buffer(FFmpegMemoryPool::Instance().AllocateBuffer(size));
    }

private:
    SafeBuffer m_buffer;
};

// 音频缓冲区特化类
class ST_AudioBuffer : public ST_Buffer
{
public:
    ST_AudioBuffer() = default;

    ST_AudioBuffer(size_t size, int sample_rate, int channels, int sample_format)
        : ST_Buffer(size), m_sample_rate(sample_rate), m_channels(channels), m_sample_format(sample_format)
    {
    }

    ST_AudioBuffer(const uint8_t* data, size_t size, int sample_rate, int channels, int sample_format)
        : ST_Buffer(data, size), m_sample_rate(sample_rate), m_channels(channels), m_sample_format(sample_format)
    {
    }

    explicit ST_AudioBuffer(const SafeBuffer& buffer, int sample_rate, int channels, int sample_format)
        : ST_Buffer(buffer), m_sample_rate(sample_rate), m_channels(channels), m_sample_format(sample_format)
    {
    }

    // 音频参数访问
    int sample_rate() const
    {
        return m_sample_rate;
    }

    int channels() const
    {
        return m_channels;
    }

    int sample_format() const
    {
        return m_sample_format;
    }

    // 计算音频时长（毫秒）
    int duration_ms() const
    {
        if (m_sample_rate <= 0)
        {
            return 0;
        }
        return static_cast<int>((static_cast<double>(size()) / (m_sample_rate * m_channels * 2)) * 1000);
    }

    // 计算样本数
    int sample_count() const
    {
        if (m_channels <= 0)
        {
            return 0;
        }
        return static_cast<int>(size() / (m_channels * 2)); // 假设16bit样本
    }

    // 工厂方法：创建音频缓冲区
    static ST_AudioBuffer Create(size_t size, int sample_rate, int channels, int sample_format)
    {
        return ST_AudioBuffer(size, sample_rate, channels, sample_format);
    }

    // 工厂方法：从数据创建音频缓冲区
    static ST_AudioBuffer FromData(const uint8_t* data, size_t size, int sample_rate, int channels, int sample_format)
    {
        return ST_AudioBuffer(data, size, sample_rate, channels, sample_format);
    }

private:
    int m_sample_rate = 0;
    int m_channels = 0;
    int m_sample_format = 0;
};

// 视频缓冲区特化类
class ST_VideoBuffer : public ST_Buffer
{
public:
    ST_VideoBuffer() = default;

    ST_VideoBuffer(size_t size, int width, int height, int pixel_format)
        : ST_Buffer(size), m_width(width), m_height(height), m_pixel_format(pixel_format)
    {
    }

    ST_VideoBuffer(const uint8_t* data, size_t size, int width, int height, int pixel_format)
        : ST_Buffer(data, size), m_width(width), m_height(height), m_pixel_format(pixel_format)
    {
    }

    explicit ST_VideoBuffer(const SafeBuffer& buffer, int width, int height, int pixel_format)
        : ST_Buffer(buffer), m_width(width), m_height(height), m_pixel_format(pixel_format)
    {
    }

    // 视频参数访问
    int width() const
    {
        return m_width;
    }

    int height() const
    {
        return m_height;
    }

    int pixel_format() const
    {
        return m_pixel_format;
    }

    // 计算帧大小
    int frame_size() const
    {
        return m_width * m_height * 3; // 假设RGB24格式
    }

    // 检查缓冲区是否足够存储一帧
    bool is_frame_complete() const
    {
        return size() >= static_cast<size_t>(frame_size());
    }

    // 工厂方法：创建视频缓冲区
    static ST_VideoBuffer Create(size_t size, int width, int height, int pixel_format)
    {
        return ST_VideoBuffer(size, width, height, pixel_format);
    }

    // 工厂方法：从数据创建视频缓冲区
    static ST_VideoBuffer FromData(const uint8_t* data, size_t size, int width, int height, int pixel_format)
    {
        return ST_VideoBuffer(data, size, width, height, pixel_format);
    }

private:
    int m_width = 0;
    int m_height = 0;
    int m_pixel_format = 0;
};
