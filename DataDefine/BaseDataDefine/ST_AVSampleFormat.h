#pragma once

extern "C" {
#include "libavutil/samplefmt.h"
}
/// <summary>
/// 音频采样格式封装类
/// </summary>
class ST_AVSampleFormat
{
  public:
    ST_AVSampleFormat() = default;

    ST_AVSampleFormat(int format)
    {
        sampleFormat = AVSampleFormat(format);
    }

    ST_AVSampleFormat &operator=(const ST_AVSampleFormat &obj)
    {
        if (this != &obj)
        {
            sampleFormat = obj.sampleFormat;
        }
        return *this;
    }
    AVSampleFormat sampleFormat = AV_SAMPLE_FMT_NONE;
};
