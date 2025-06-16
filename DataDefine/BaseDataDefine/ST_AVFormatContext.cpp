#pragma once
#include "ST_AVFormatContext.h"
ST_AVFormatContext::~ST_AVFormatContext()
{
    if (m_pFormatCtx)
    {
        avformat_close_input(&m_pFormatCtx);
        m_pFormatCtx = nullptr;
    }
}

ST_AVFormatContext::ST_AVFormatContext(ST_AVFormatContext &&other) noexcept : m_pFormatCtx(other.m_pFormatCtx)
{
    other.m_pFormatCtx = nullptr;
}

ST_AVFormatContext &ST_AVFormatContext::operator=(ST_AVFormatContext &&other) noexcept
{
    if (this != &other)
    {
        if (m_pFormatCtx)
        {
            avformat_close_input(&m_pFormatCtx);
        }
        m_pFormatCtx = other.m_pFormatCtx;
        other.m_pFormatCtx = nullptr;
    }
    return *this;
}

int ST_AVFormatContext::OpenInputFilePath(const char *url, const AVInputFormat *fmt, AVDictionary **options)
{
    return avformat_open_input(&m_pFormatCtx, url, fmt, options);
}

int ST_AVFormatContext::OpenOutputFilePath(const AVOutputFormat *oformat, const char *formatName, const char *filename)
{
    return avformat_alloc_output_context2(&m_pFormatCtx, oformat, formatName, filename);
}

int ST_AVFormatContext::FindBestStream(enum AVMediaType type, int wanted_stream_nb, int related_stream, const AVCodec **decoder_ret, int flags)
{
    return av_find_best_stream(m_pFormatCtx, type, wanted_stream_nb, related_stream, decoder_ret, flags);
}

void ST_AVFormatContext::WriteFileTrailer()
{
    av_write_trailer(m_pFormatCtx);
}

int ST_AVFormatContext::WriteFileHeader(AVDictionary **options)
{
    return avformat_write_header(m_pFormatCtx, options);
}

double ST_AVFormatContext::GetStreamDuration(int streamIndex) const
{
    if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
    {
        return 0.0;
    }
    AVStream *stream = m_pFormatCtx->streams[streamIndex];
    if (stream->duration == AV_NOPTS_VALUE)
    {
        return 0.0;
    }
    return stream->duration * av_q2d(stream->time_base);
}

int ST_AVFormatContext::SeekFrame(int streamIndex, double timestamp)
{
    if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
    {
        return -1;
    }
    AVStream *stream = m_pFormatCtx->streams[streamIndex];
    int64_t ts = static_cast<int64_t>(timestamp / av_q2d(stream->time_base));
    return av_seek_frame(m_pFormatCtx, streamIndex, ts, AVSEEK_FLAG_BACKWARD);
}

double ST_AVFormatContext::GetCurrentTimestamp(int streamIndex) const
{
    if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
    {
        return 0.0;
    }
    AVStream *stream = m_pFormatCtx->streams[streamIndex];
    return av_gettime() * av_q2d(stream->time_base);
}
