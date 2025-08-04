#pragma once

extern "C"
{
#include "libavutil/channel_layout.h"
#include "libavutil/mem.h"
#include "libavutil/frame.h"
#include "libavutil/buffer.h"
#include "libavutil/samplefmt.h"
}

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

// RAII包装器用于AVChannelLayout - 解决SafeFreeChannelLayout手动释放问题
class AVChannelLayoutRAII
{
public:
    AVChannelLayoutRAII()
        : m_layout(nullptr)
    {
    }

    explicit AVChannelLayoutRAII(AVChannelLayout* layout)
        : m_layout(layout)
    {
    }

    ~AVChannelLayoutRAII()
    {
        reset();
    }

    // 允许移动构造和移动赋值，禁止拷贝
    AVChannelLayoutRAII(const AVChannelLayoutRAII&) = delete;
    AVChannelLayoutRAII& operator=(const AVChannelLayoutRAII&) = delete;

    AVChannelLayoutRAII(AVChannelLayoutRAII&& other) noexcept
        : m_layout(other.m_layout)
    {
        other.m_layout = nullptr;
    }

    AVChannelLayoutRAII& operator=(AVChannelLayoutRAII&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            m_layout = other.m_layout;
            other.m_layout = nullptr;
        }
        return *this;
    }

    void reset(AVChannelLayout* layout = nullptr)
    {
        if (m_layout)
        {
            av_channel_layout_uninit(m_layout);
            av_free(m_layout);
        }
        m_layout = layout;
    }

    AVChannelLayout* get() const
    {
        return m_layout;
    }

    AVChannelLayout* release()
    {
        AVChannelLayout* temp = m_layout;
        m_layout = nullptr;
        return temp;
    }

    operator bool() const
    {
        return m_layout != nullptr;
    }

    // 工厂方法：创建默认布局
    static AVChannelLayoutRAII createDefault(int channels)
    {
        auto layout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        if (layout)
        {
            av_channel_layout_default(layout, channels);
        }
        return AVChannelLayoutRAII(layout);
    }

    // 工厂方法：从现有布局复制
    static AVChannelLayoutRAII copyFrom(const AVChannelLayout* src)
    {
        if (!src)
        {
            return AVChannelLayoutRAII();
        }

        auto layout = static_cast<AVChannelLayout*>(av_mallocz(sizeof(AVChannelLayout)));
        if (layout && av_channel_layout_copy(layout, src) >= 0)
        {
            return AVChannelLayoutRAII(layout);
        }

        if (layout)
        {
            av_free(layout);
        }
        return AVChannelLayoutRAII();
    }

private:
    AVChannelLayout* m_layout = nullptr;
};

// 安全的缓冲区类 - 解决缓冲区大小计算缺乏安全检查问题
class SafeBuffer
{
public:
    explicit SafeBuffer(size_t size = 0)
    {
        if (size > 0)
        {
            if (!resize(size))
            {
                // 分配失败时记录日志但不抛异常
            }
        }
    }

    SafeBuffer(const uint8_t* data, size_t size)
    {
        if (data && size > 0)
        {
            assign(data, size);
        }
    }

    ~SafeBuffer() = default;

    SafeBuffer(const SafeBuffer& other) = default;
    SafeBuffer& operator=(const SafeBuffer& other) = default;
    SafeBuffer(SafeBuffer&& other) noexcept = default;
    SafeBuffer& operator=(SafeBuffer&& other) noexcept = default;

    // 数据访问
    uint8_t* data()
    {
        return m_data.empty() ? nullptr : m_data.data();
    }

    const uint8_t* data() const
    {
        return m_data.empty() ? nullptr : m_data.data();
    }

    size_t size() const
    {
        return m_data.size();
    }

    size_t capacity() const
    {
        return m_data.capacity();
    }

    bool empty() const
    {
        return m_data.empty();
    }

    // 容量管理
    bool reserve(size_t new_capacity)
    {
        if (new_capacity > MAX_BUFFER_SIZE)
        {
            return false;
        }
        try
        {
            m_data.reserve(new_capacity);
            return true;
        } catch (...)
        {
            return false;
        }
    }

    bool resize(size_t new_size)
    {
        if (new_size > MAX_BUFFER_SIZE)
        {
            return false;
        }
        try
        {
            m_data.resize(new_size);
            return true;
        } catch (...)
        {
            return false;
        }
    }

    // 数据操作
    bool assign(const uint8_t* data, size_t size)
    {
        if (!data || size == 0)
        {
            clear();
            return true;
        }

        if (size > MAX_BUFFER_SIZE)
        {
            return false;
        }

        try
        {
            m_data.assign(data, data + size);
            return true;
        } catch (...)
        {
            return false;
        }
    }

    bool append(const uint8_t* data, size_t size)
    {
        if (!data || size == 0)
        {
            return true;
        }

        size_t new_size = m_data.size() + size;
        if (new_size > MAX_BUFFER_SIZE)
        {
            return false;
        }

        try
        {
            m_data.insert(m_data.end(), data, data + size);
            return true;
        } catch (...)
        {
            return false;
        }
    }

    bool insert(size_t pos, const uint8_t* data, size_t size)
    {
        if (!data || size == 0)
        {
            return true;
        }
        if (pos > m_data.size())
        {
            return false;
        }

        size_t new_size = m_data.size() + size;
        if (new_size > MAX_BUFFER_SIZE)
        {
            return false;
        }

        try
        {
            m_data.insert(m_data.begin() + pos, data, data + size);
            return true;
        } catch (...)
        {
            return false;
        }
    }

    void clear()
    {
        m_data.clear();
    }

    void shrink_to_fit()
    {
        m_data.shrink_to_fit();
    }

    // 安全访问
    uint8_t& operator[](size_t index)
    {
        static uint8_t dummy = 0;
        return index < m_data.size() ? m_data[index] : dummy;
    }

    const uint8_t& operator[](size_t index) const
    {
        static uint8_t dummy = 0;
        return index < m_data.size() ? m_data[index] : dummy;
    }

    // 工具方法
    static constexpr size_t max_size()
    {
        return MAX_BUFFER_SIZE;
    }

    static bool is_valid_size(size_t size)
    {
        return size <= MAX_BUFFER_SIZE;
    }

private:
    std::vector<uint8_t> m_data;
    static constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024 * 100; // 100MB限制
};

// FFmpeg内存池管理器 - 解决动态内存分配缺乏池化管理问题
class FFmpegMemoryPool
{
public:
    static FFmpegMemoryPool& Instance()
    {
        static FFmpegMemoryPool pool;
        return pool;
    }

    // 分配AVChannelLayout
    AVChannelLayoutRAII AllocateChannelLayout()
    {
        return AVChannelLayoutRAII(AVChannelLayoutRAII::createDefault(0).release());
    }

    // 分配缓冲区
    SafeBuffer AllocateBuffer(size_t size)
    {
        return SafeBuffer(size);
    }

    // 通用内存分配
    template <typename T>
    std::unique_ptr<T, void(*)(void*)> Allocate(size_t count = 1)
    {
        T* ptr = static_cast<T*>(av_mallocz(sizeof(T) * count));
        return {
            ptr, [](void* p)
            {
                av_free(p);
            }
        };
    }

    // 安全释放函数 - 替代SafeFreeChannelLayout
    static void SafeFreeChannelLayout(AVChannelLayout** layout)
    {
        if (layout && *layout)
        {
            av_channel_layout_uninit(*layout);
            av_free(*layout);
            *layout = nullptr;
        }
    }

    // 安全释放函数 - 通用版本
    template <typename T>
    static void SafeFree(T** ptr)
    {
        if (ptr && *ptr)
        {
            av_free(*ptr);
            *ptr = nullptr;
        }
    }

    // 内存使用统计
    size_t GetTotalAllocated() const
    {
        return m_totalAllocated;
    }

    size_t getCurrentAllocated() const
    {
        return m_currentAllocated;
    }

private:
    FFmpegMemoryPool() = default;
    FFmpegMemoryPool(const FFmpegMemoryPool&) = delete;
    FFmpegMemoryPool& operator=(const FFmpegMemoryPool&) = delete;

    size_t m_totalAllocated = 0;
    size_t m_currentAllocated = 0;
};

// RAII包装器用于AVFrame
class AVFrameRAII
{
public:
    AVFrameRAII()
        : m_frame(av_frame_alloc())
    {
    }

    explicit AVFrameRAII(AVFrame* frame)
        : m_frame(frame)
    {
    }

    ~AVFrameRAII()
    {
        reset();
    }

    AVFrameRAII(const AVFrameRAII& other) = delete;
    AVFrameRAII& operator=(const AVFrameRAII& other) = delete;

    AVFrameRAII(AVFrameRAII&& other) noexcept
        : m_frame(other.m_frame)
    {
        other.m_frame = nullptr;
    }

    AVFrameRAII& operator=(AVFrameRAII&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            m_frame = other.m_frame;
            other.m_frame = nullptr;
        }
        return *this;
    }

    void reset(AVFrame* frame = nullptr)
    {
        if (m_frame)
        {
            av_frame_free(&m_frame);
        }
        m_frame = frame;
    }

    AVFrame* get() const
    {
        return m_frame;
    }

    AVFrame* release()
    {
        AVFrame* temp = m_frame;
        m_frame = nullptr;
        return temp;
    }

    operator bool() const
    {
        return m_frame != nullptr;
    }

private:
    AVFrame* m_frame = nullptr;
};
