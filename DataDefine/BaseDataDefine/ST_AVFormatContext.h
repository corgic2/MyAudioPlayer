#pragma once
#include <QString>

extern "C" {
#include <libavformat/avformat.h>
#include "libavutil/time.h"
}
    /// <summary>
/// 音频格式上下文封装类
/// </summary>
class ST_AVFormatContext
{
  public:
    ST_AVFormatContext() = default;
    ~ST_AVFormatContext();

    // 禁用拷贝操作
    ST_AVFormatContext(const ST_AVFormatContext &) = delete;
    ST_AVFormatContext &operator=(const ST_AVFormatContext &) = delete;

    // 移动操作
    ST_AVFormatContext(ST_AVFormatContext &&other) noexcept;
    ST_AVFormatContext &operator=(ST_AVFormatContext &&other) noexcept;

    /// <summary>
    /// 打开输入文件
    /// </summary>
    /// <param name="url">文件路径</param>
    /// <param name="fmt">输入格式</param>
    /// <param name="options">选项字典</param>
    /// <returns>成功返回0，失败返回负值</returns>
    bool OpenInputFilePath(const char* url, const AVInputFormat* fmt = nullptr, AVDictionary** options = nullptr);

    /// <summary>
    /// 打开输出文件
    /// </summary>
    /// <param name="oformat">输出格式</param>
    /// <param name="formatName">格式名称</param>
    /// <param name="filename">文件名</param>
    /// <returns>成功返回0，失败返回负值</returns>
    bool OpenOutputFilePath(const AVOutputFormat* oformat, const char* formatName, const char* filename);

    /// <summary>
    /// 查找最佳流
    /// </summary>
    /// <returns>成功返回流索引，失败返回负值</returns>
    int FindBestStream(enum AVMediaType type, int wanted_stream_nb = -1, int related_stream = -1, const AVCodec **decoder_ret = nullptr, int flags = 0);

    /// <summary>
    /// 写入文件尾
    /// </summary>
    void WriteFileTrailer();

    /// <summary>
    /// 写入文件头
    /// </summary>
    bool WriteFileHeader(AVDictionary** options = nullptr);

    /// <summary>
    /// 获取流的总时长（秒）
    /// </summary>
    double GetStreamDuration(int streamIndex) const;

    /// <summary>
    /// 跳转到指定时间点
    /// </summary>
    int SeekFrame(int streamIndex, double timestamp);

    /// <summary>
    /// 获取当前时间戳
    /// </summary>
    double GetCurrentTimestamp(unsigned int streamIndex) const;

    /// <summary>
    /// 获取原始格式上下文指针
    /// </summary>
    AVFormatContext *GetRawContext() const
    {
        return m_pFormatCtx;
    }

    /// <summary>
    /// 打开文件
    /// </summary>
    /// <param name="filePath"></param>
    /// <returns></returns>
    bool OpenIOFilePath(const QString& filePath);
    /// <summary>
    /// 跳转帧
    /// </summary>
    /// <param name="stream_index"></param>
    /// <param name="timestamp"></param>
    /// <param name="flags"></param>
    /// <returns></returns>
    bool SeekFrame(int stream_index, int64_t timestamp, int flags);
    /// <summary>
    /// 读取帧
    /// </summary>
    /// <param name="packet"></param>
    /// <returns></returns>
    bool ReadFrame(AVPacket* packet);
    /// <summary>
    ///
    /// </summary>
    /// <param name="packet"></param>
    /// <returns></returns>
    bool WriteFrame(AVPacket* packet);
private:
    AVFormatContext *m_pFormatCtx = nullptr;
};
