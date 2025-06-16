#pragma once

extern "C" {
#include "libavutil/channel_layout.h"
#include "libavutil/mem.h"
}
/// <summary>
/// 音频通道布局封装类
/// </summary>
class ST_AVChannelLayout
{
  public:
    ST_AVChannelLayout() = default;

    ST_AVChannelLayout &operator=(const ST_AVChannelLayout &obj)
    {
        if (this != &obj)
        {
            if (channel)
            {
                av_channel_layout_uninit(channel);
                av_free(channel);
            }
            channel = static_cast<AVChannelLayout *>(av_mallocz(sizeof(AVChannelLayout)));
            if (channel)
            {
                *channel = *obj.channel; // 复制布局
            }
        }
        return *this;
    }

    explicit ST_AVChannelLayout(AVChannelLayout *ptr);
    ~ST_AVChannelLayout() = default;

    /// <summary>
    /// 获取原始通道布局指针
    /// </summary>
    AVChannelLayout *GetRawLayout() const
    {
        return channel;
    }

  private:
    AVChannelLayout *channel = nullptr;
};
