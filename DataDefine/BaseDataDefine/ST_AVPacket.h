#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/packet.h"
}
    /// <summary>
/// 音频包封装类
/// </summary>
class ST_AVPacket
{
  public:
    ST_AVPacket();
    ~ST_AVPacket();

    // 禁用拷贝操作
    ST_AVPacket(const ST_AVPacket &) = delete;
    ST_AVPacket &operator=(const ST_AVPacket &) = delete;

    // 移动操作
    ST_AVPacket(ST_AVPacket &&other) noexcept;
    ST_AVPacket &operator=(ST_AVPacket &&other) noexcept;

    /// <summary>
    /// 从格式上下文读取一个数据包
    /// </summary>
    int ReadPacket(AVFormatContext *pFormatContext);

    /// <summary>
    /// 发送数据包到解码器
    /// </summary>
    int SendPacket(AVCodecContext *pCodecContext);

    /// <summary>
    /// 释放数据包引用
    /// </summary>
    void UnrefPacket();

    /// <summary>
    /// 复制数据包
    /// </summary>
    int CopyPacket(const AVPacket *src);

    /// <summary>
    /// 移动数据包
    /// </summary>
    void MovePacket(AVPacket *src);

    /// <summary>
    /// 重置数据包时间戳
    /// </summary>
    void RescaleTimestamp(const AVRational &srcTimeBase, const AVRational &dstTimeBase);

    // Getter方法
    int GetPacketSize() const;
    int64_t GetTimestamp() const;
    int64_t GetDuration() const;
    int GetStreamIndex() const;

    // Setter方法
    void SetTimestamp(int64_t pts);
    void SetDuration(int64_t duration);
    void SetStreamIndex(int index);

    /// <summary>
    /// 获取原始包指针
    /// </summary>
    AVPacket *GetRawPacket() const
    {
        return m_pkt;
    }

  private:
    AVPacket *m_pkt = nullptr;
};
