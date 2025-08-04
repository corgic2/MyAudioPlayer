#pragma once

#include "FFmpegRAII.h"
#include "BaseDataDefine/ST_AVChannelLayout.h"
#include "BaseDataDefine/ST_Buffer.h"

// 内存管理工具 - 提供统一的内存管理接口
namespace MemoryManager
{
// 已废弃的函数 - 提供向后兼容的宏定义
#define SafeFreeChannelLayout(layout) FFmpegMemoryPool::safeFreeChannelLayout(layout)
#define SafeFree(ptr) FFmpegMemoryPool::safeFree(ptr)

// 内存池接口
    class MemoryPool
    {
    public:
        static MemoryPool& instance()
        {
            static MemoryPool pool;
            return pool;
        }

        // 分配缓冲区
        ST_Buffer allocateBuffer(size_t size)
        {
            return ST_Buffer::AllocateFromPool(size);
        }

        ST_AudioBuffer allocateAudioBuffer(size_t size, int sample_rate, int channels, int sample_format)
        {
            return ST_AudioBuffer::Create(size, sample_rate, channels, sample_format);
        }

        ST_VideoBuffer allocateVideoBuffer(size_t size, int width, int height, int pixel_format)
        {
            return ST_VideoBuffer::Create(size, width, height, pixel_format);
        }

        // 分配通道布局
        ST_AVChannelLayout allocateChannelLayout()
        {
            return ST_AVChannelLayout::CreateDefault(0);
        }

        // 通用内存分配
        template <typename T>
        std::unique_ptr<T, void(*)(void*)> allocate(size_t count = 1)
        {
            return FFmpegMemoryPool::Instance().allocate<T>(count);
        }

        // 内存使用统计
        size_t getTotalAllocated() const
        {
            return FFmpegMemoryPool::Instance().GetTotalAllocated();
        }

        size_t getCurrentAllocated() const
        {
            return FFmpegMemoryPool::Instance().getCurrentAllocated();
        }

        // 内存安全检查
        static bool isValidBufferSize(size_t size)
        {
            return ST_Buffer::is_valid_size(size);
        }

        static size_t getMaxBufferSize()
        {
            return ST_Buffer::max_size();
        }

    private:
        MemoryPool() = default;
        MemoryPool(const MemoryPool&) = delete;
        MemoryPool& operator=(const MemoryPool&) = delete;
    };

// 工具函数
    namespace Utils
    {
        // 安全分配并初始化缓冲区
        static inline ST_Buffer createSafeBuffer(size_t size, uint8_t fill_value = 0)
        {
            ST_Buffer buffer = ST_Buffer::CreateZeroed(size);
            if (buffer.data() && fill_value != 0)
            {
                std::fill(buffer.data(), buffer.data() + size, fill_value);
            }
            return buffer;
        }

        // 从现有数据创建安全缓冲区
        static inline ST_Buffer copyBuffer(const uint8_t* data, size_t size)
        {
            return ST_Buffer::FromData(data, size);
        }

        // 计算音频缓冲区大小（带安全检查）
        static inline size_t calculateAudioBufferSize(int sample_rate, int channels, int duration_ms, int bytes_per_sample = 2)
        {
            if (sample_rate <= 0 || channels <= 0 || duration_ms <= 0)
            {
                return 0;
            }

            size_t size = static_cast<size_t>(sample_rate) * channels * duration_ms * bytes_per_sample / 1000;
            if (!ST_Buffer::is_valid_size(size))
            {
                return 0;
            }
            return size;
        }

        // 计算视频缓冲区大小（带安全检查）
        static inline size_t calculateVideoBufferSize(int width, int height, int bytes_per_pixel = 3)
        {
            if (width <= 0 || height <= 0)
            {
                return 0;
            }

            size_t size = static_cast<size_t>(width) * height * bytes_per_pixel;
            if (!ST_Buffer::is_valid_size(size))
            {
                return 0;
            }
            return size;
        }

        // 验证缓冲区完整性
        static inline bool validateBuffer(const ST_Buffer& buffer, const char* context = nullptr)
        {
            if (!buffer.data() || buffer.size() == 0)
            {
                if (context)
                {
                    // 可以添加日志记录
                }
                return false;
            }
            return true;
        }
    }

// RAII包装器模板
    template <typename T, typename Deleter = void(*)(void*)>
    class RAIIWrapper
    {
    public:
        using PtrType = std::unique_ptr<T, Deleter>;

        RAIIWrapper() = default;

        explicit RAIIWrapper(T* ptr, Deleter deleter)
            : m_ptr(ptr, deleter)
        {
        }

        T* get() const
        {
            return m_ptr.get();
        }

        T* release()
        {
            return m_ptr.release();
        }

        void reset(T* ptr = nullptr, Deleter deleter = nullptr)
        {
            if (deleter)
            {
                m_ptr.reset(ptr);
            }
            else
            {
                m_ptr.reset(ptr);
            }
        }

        operator bool() const
        {
            return static_cast<bool>(m_ptr);
        }

        T* operator->() const
        {
            return m_ptr.get();
        }

        T& operator*() const
        {
            return *m_ptr;
        }

    private:
        PtrType m_ptr;
    };

// FFmpeg特定包装器
    namespace FFmpeg
    {
        // AVFrame包装器
        using AVFramePtr = std::unique_ptr<AVFrame, void(*)(AVFrame*)>;

        static inline AVFramePtr makeAVFramePtr()
        {
            return AVFramePtr(av_frame_alloc(), av_frame_free);
        }

        // AVPacket包装器
        using AVPacketPtr = std::unique_ptr<AVPacket, void(*)(AVPacket*)>;

        static inline AVPacketPtr makeAVPacketPtr()
        {
            return AVPacketPtr(av_packet_alloc(), av_packet_free);
        }

        // 通道布局包装器
        static inline ST_AVChannelLayout makeChannelLayout(int channels)
        {
            return ST_AVChannelLayout::CreateDefault(channels);
        }

        static inline ST_AVChannelLayout copyChannelLayout(const AVChannelLayout* layout)
        {
            return ST_AVChannelLayout::CopyFrom(layout);
        }
    }
} // namespace MemoryManager
