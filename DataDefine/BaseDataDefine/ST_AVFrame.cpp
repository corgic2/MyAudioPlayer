#include "ST_AVFrame.h"

ST_AVFrame::ST_AVFrame()
{
    frame = av_frame_alloc();
}

ST_AVFrame::~ST_AVFrame()
{
    // 注意：不要直接释放frame->data[0]，因为这可能是外部提供的缓冲区
    // 只释放AVFrame结构本身，数据内存由外部管理
    av_frame_free(&frame);
}

ST_AVFrame::ST_AVFrame(ST_AVFrame&& other) noexcept
    : frame(other.frame)
{
    other.frame = nullptr;
}

ST_AVFrame& ST_AVFrame::operator=(ST_AVFrame&& other) noexcept
{
    if (this != &other)
    {
        av_frame_free(&frame);
        frame = other.frame;
        other.frame = nullptr;
    }
    return *this;
}

bool ST_AVFrame::GetCodecFrame(AVCodecContext* codeContex)
{
    int ret = avcodec_receive_frame(codeContex, frame);
    if (ret < 0)
    {
        return false;
    }
    return true;
}

AVFrame* ST_AVFrame::GetRawFrame()
{
    return frame;
}
