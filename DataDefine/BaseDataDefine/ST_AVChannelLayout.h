#pragma once

#include "../FFmpegRAII.h"

class ST_AVChannelLayout
{
public:
    ST_AVChannelLayout() = default;

    ST_AVChannelLayout(const ST_AVChannelLayout &other) : m_layout(AVChannelLayoutRAII::copyFrom(other.m_layout.get())) {}

    ST_AVChannelLayout &operator=(const ST_AVChannelLayout &other)
    {
        if (this != &other)
        {
            m_layout = AVChannelLayoutRAII::copyFrom(other.m_layout.get());
        }
        return *this;
    }

    ST_AVChannelLayout(ST_AVChannelLayout &&other) noexcept : m_layout(std::move(other.m_layout)) {}

    ST_AVChannelLayout &operator=(ST_AVChannelLayout &&other) noexcept
    {
        if (this != &other)
        {
            m_layout = std::move(other.m_layout);
        }
        return *this;
    }

    explicit ST_AVChannelLayout(AVChannelLayout *layout) : m_layout(layout) {}

    ~ST_AVChannelLayout() = default;

    AVChannelLayout *GetRawLayout() const { return m_layout.get(); }

    // 创建默认布局
    static ST_AVChannelLayout CreateDefault(int channels)
    {
        return ST_AVChannelLayout(AVChannelLayoutRAII::createDefault(channels).release());
    }

    // 从现有布局复制
    static ST_AVChannelLayout CopyFrom(const AVChannelLayout *layout)
    {
        return ST_AVChannelLayout(AVChannelLayoutRAII::copyFrom(layout).release());
    }

    // 检查是否有效
    bool IsValid() const { return static_cast<bool>(m_layout); }

    // 释放并重置
    void Reset() { m_layout.reset(); }

    // 释放并重置为新布局
    void Reset(AVChannelLayout *layout) { m_layout.reset(layout); }

private:
    AVChannelLayoutRAII m_layout;
};
