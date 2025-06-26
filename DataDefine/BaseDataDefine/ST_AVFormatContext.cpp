#pragma once
#include "ST_AVFormatContext.h"

#include "LogSystem/LogSystem.h"
#include "TimeSystem/TimeSystem.h"

ST_AVFormatContext::~ST_AVFormatContext()
{
    if (m_pFormatCtx)
    {
        avformat_close_input(&m_pFormatCtx);
        m_pFormatCtx = nullptr;
    }
}

ST_AVFormatContext::ST_AVFormatContext(ST_AVFormatContext&& other) noexcept
    : m_pFormatCtx(other.m_pFormatCtx)
{
    other.m_pFormatCtx = nullptr;
}

ST_AVFormatContext& ST_AVFormatContext::operator=(ST_AVFormatContext&& other) noexcept
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

bool ST_AVFormatContext::OpenInputFilePath(const char* url, const AVInputFormat* fmt, AVDictionary** options)
{
    int ret = avformat_open_input(&m_pFormatCtx, url, fmt, options);
    if (ret < 0)
    {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to open device:" + std::string(errbuf));
        return false;
    }

    return true;
}

bool ST_AVFormatContext::OpenOutputFilePath(const AVOutputFormat* oformat, const char* formatName, const char* filename)
{
    int ret = avformat_alloc_output_context2(&m_pFormatCtx, oformat, formatName, filename);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to create output context: " + std::string(errbuf));
        return false;
    }

    return true;
}

int ST_AVFormatContext::FindBestStream(enum AVMediaType type, int wanted_stream_nb, int related_stream, const AVCodec** decoder_ret, int flags)
{
    return av_find_best_stream(m_pFormatCtx, type, wanted_stream_nb, related_stream, decoder_ret, flags);
}

void ST_AVFormatContext::WriteFileTrailer()
{
    av_write_trailer(m_pFormatCtx);
}

bool ST_AVFormatContext::WriteFileHeader(AVDictionary** options)
{
    int ret = avformat_write_header(m_pFormatCtx, options);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to write file header: " + std::string(errbuf));
        return false;
    }
    return true;
}

double ST_AVFormatContext::GetStreamDuration(int streamIndex) const
{
    if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
    {
        return 0.0;
    }
    AVStream* stream = m_pFormatCtx->streams[streamIndex];
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
    AVStream* stream = m_pFormatCtx->streams[streamIndex];
    int64_t ts = static_cast<int64_t>(timestamp / av_q2d(stream->time_base));
    return av_seek_frame(m_pFormatCtx, streamIndex, ts, AVSEEK_FLAG_BACKWARD);
}

double ST_AVFormatContext::GetCurrentTimestamp(unsigned int streamIndex) const
{
    if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
    {
        return 0.0;
    }
    AVStream* stream = m_pFormatCtx->streams[streamIndex];
    return av_gettime() * av_q2d(stream->time_base);
}

bool ST_AVFormatContext::OpenIOFilePath(const QString& filePath)
{
    int ret = avio_open(&m_pFormatCtx->pb, filePath.toUtf8().constData(), AVIO_FLAG_WRITE);
    if (ret < 0)
    {
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Could not open output file:" + std::string(errbuf));
        return false;
    }
    return true;
}

bool ST_AVFormatContext::SeekFrame(int stream_index, int64_t timestamp,int flags)
{
    int ret = av_seek_frame(m_pFormatCtx, stream_index, timestamp, flags);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to seek audio: " + std::string(errbuf));
        return false;
    }
    return true;
}

bool ST_AVFormatContext::ReadFrame(AVPacket* packet)
{
    int ret = av_read_frame(m_pFormatCtx, packet);
    if (ret < 0)
    {
        if (ret == AVERROR(EAGAIN))
        {
            TIME_SLEEP_MS(10);
        }
        else
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_strerror(ret, errbuf, sizeof(errbuf));
        }
    }
    return ret;
}

bool ST_AVFormatContext::WriteFrame(AVPacket* packet)
{
    int ret = av_interleaved_write_frame(m_pFormatCtx, packet);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, errbuf, sizeof(errbuf));
        return false;
    }
    return true;
}
