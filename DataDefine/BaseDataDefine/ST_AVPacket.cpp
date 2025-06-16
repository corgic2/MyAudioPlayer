#pragma once
#include "ST_AVPacket.h"


// ST_AVPacket implementation
ST_AVPacket::ST_AVPacket() : m_pkt(av_packet_alloc())
{
}

ST_AVPacket::~ST_AVPacket()
{
    if (m_pkt)
    {
        av_packet_free(&m_pkt);
    }
}

ST_AVPacket::ST_AVPacket(ST_AVPacket &&other) noexcept : m_pkt(other.m_pkt)
{
    other.m_pkt = nullptr;
}

ST_AVPacket &ST_AVPacket::operator=(ST_AVPacket &&other) noexcept
{
    if (this != &other)
    {
        if (m_pkt)
        {
            av_packet_free(&m_pkt);
        }
        m_pkt = other.m_pkt;
        other.m_pkt = nullptr;
    }
    return *this;
}

int ST_AVPacket::ReadPacket(AVFormatContext *pFormatContext)
{
    return av_read_frame(pFormatContext, m_pkt);
}

int ST_AVPacket::SendPacket(AVCodecContext *pCodecContext)
{
    return avcodec_send_packet(pCodecContext, m_pkt);
}

void ST_AVPacket::UnrefPacket()
{
    av_packet_unref(m_pkt);
}

int ST_AVPacket::CopyPacket(const AVPacket *src)
{
    return av_packet_copy_props(m_pkt, src);
}

void ST_AVPacket::MovePacket(AVPacket *src)
{
    av_packet_move_ref(m_pkt, src);
}

void ST_AVPacket::RescaleTimestamp(const AVRational &srcTimeBase, const AVRational &dstTimeBase)
{
    av_packet_rescale_ts(m_pkt, srcTimeBase, dstTimeBase);
}

int ST_AVPacket::GetPacketSize() const
{
    return m_pkt ? m_pkt->size : 0;
}

int64_t ST_AVPacket::GetTimestamp() const
{
    return m_pkt ? m_pkt->pts : AV_NOPTS_VALUE;
}

int64_t ST_AVPacket::GetDuration() const
{
    return m_pkt ? m_pkt->duration : 0;
}

int ST_AVPacket::GetStreamIndex() const
{
    return m_pkt ? m_pkt->stream_index : -1;
}

void ST_AVPacket::SetTimestamp(int64_t pts)
{
    if (m_pkt)
    {
        m_pkt->pts = pts;
    }
}

void ST_AVPacket::SetDuration(int64_t duration)
{
    if (m_pkt)
    {
        m_pkt->duration = duration;
    }
}

void ST_AVPacket::SetStreamIndex(int index)
{
    if (m_pkt)
    {
        m_pkt->stream_index = index;
    }
}
