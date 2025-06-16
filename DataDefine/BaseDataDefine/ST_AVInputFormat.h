#pragma once
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include "libavutil/time.h"
}
/// <summary>
/// 音频输入格式封装类
/// </summary>
class ST_AVInputFormat
{
  public:
    ST_AVInputFormat() = default;
    ~ST_AVInputFormat() = default;

    // 移动操作
    ST_AVInputFormat(ST_AVInputFormat &&other) noexcept;
    ST_AVInputFormat &operator=(ST_AVInputFormat &&other) noexcept;

    /// <summary>
    /// 查找输入格式
    /// </summary>
    void FindInputFormat(const std::string &deviceFormat);

    /// <summary>
    /// 获取原始输入格式指针
    /// </summary>
    const AVInputFormat *GetRawFormat() const
    {
        return m_pInputFormatCtx;
    }

  private:
    const AVInputFormat *m_pInputFormatCtx = nullptr;
};
