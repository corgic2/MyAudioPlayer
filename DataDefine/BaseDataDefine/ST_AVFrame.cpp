#include "ST_AVFrame.h"

ST_AVFrame::ST_AVFrame()
{
    frame = av_frame_alloc();
}

ST_AVFrame::~ST_AVFrame()
{
    if (frame->data[0])
    {
        av_free(frame->data[0]);
    }

    av_frame_free(&frame);
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
