#pragma once
#include "BaseDataDefine/ST_AVFormatContext.h"
#include "BaseDataDefine/ST_AVInputFormat.h"

/// <summary>
/// 音频设备打开封装类
/// </summary>
class ST_OpenAudioDevice
{
  public:
    ST_OpenAudioDevice() = default;
    ~ST_OpenAudioDevice() = default;

    // 禁用拷贝操作
    ST_OpenAudioDevice(ST_OpenAudioDevice &) = delete;
    ST_OpenAudioDevice(const ST_OpenAudioDevice &) = delete;
    ST_OpenAudioDevice &operator=(ST_OpenAudioDevice &) = delete;
    ST_OpenAudioDevice &operator=(const ST_OpenAudioDevice &) = delete;

    // 移动操作
    ST_OpenAudioDevice(ST_OpenAudioDevice &&other) noexcept;
    ST_OpenAudioDevice &operator=(ST_OpenAudioDevice &&other) noexcept;

    /// <summary>
    /// 获取输入格式
    /// </summary>
    ST_AVInputFormat &GetInputFormat()
    {
        return m_pInputFormatCtx;
    }

    /// <summary>
    /// 获取格式上下文
    /// </summary>
    ST_AVFormatContext &GetFormatContext()
    {
        return m_pFormatCtx;
    }

  private:
    ST_AVInputFormat m_pInputFormatCtx; /// 输入设备格式
    ST_AVFormatContext m_pFormatCtx;    /// 输入格式上下文
};




