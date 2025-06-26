#pragma once

extern "C"
{
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}

class ST_AVFrame
{
public:
    ST_AVFrame();
    ~ST_AVFrame();
    /// <summary>
    /// 获取帧
    /// </summary>
    /// <param name="codeContex"></param>
    /// <returns></returns>
    bool GetCodecFrame(AVCodecContext* codeContex);

    AVFrame* GetRawFrame();
private:
    AVFrame* frame = nullptr;
};
