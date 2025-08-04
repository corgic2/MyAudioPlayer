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
    
    // 禁止拷贝，避免重复释放
    ST_AVFrame(const ST_AVFrame&) = delete;
    ST_AVFrame& operator=(const ST_AVFrame&) = delete;
    
    // 允许移动
    ST_AVFrame(ST_AVFrame&& other) noexcept;
    ST_AVFrame& operator=(ST_AVFrame&& other) noexcept;
    
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
