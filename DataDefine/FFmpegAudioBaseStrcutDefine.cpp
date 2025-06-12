#include "FFmpegAudioBaseStrcutDefine.h"

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

int ST_AVFormatContext::OpenInputFilePath(const char* url, const AVInputFormat* fmt, AVDictionary** options)
{
    return avformat_open_input(&m_pFormatCtx, url, fmt, options);
}

int ST_AVFormatContext::OpenOutputFilePath(const AVOutputFormat* oformat, const char* formatName, const char* filename)
{
    return avformat_alloc_output_context2(&m_pFormatCtx, oformat, formatName, filename);
}

int ST_AVFormatContext::FindBestStream(enum AVMediaType type, int wanted_stream_nb, 
                                     int related_stream, const AVCodec** decoder_ret, int flags)
{
    return av_find_best_stream(m_pFormatCtx, type, wanted_stream_nb, related_stream, decoder_ret, flags);
}

void ST_AVFormatContext::WriteFileTrailer()
{
    av_write_trailer(m_pFormatCtx);
}

int ST_AVFormatContext::WriteFileHeader(AVDictionary** options)
{
    return avformat_write_header(m_pFormatCtx, options);
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

double ST_AVFormatContext::GetCurrentTimestamp(int streamIndex) const
{
    if (!m_pFormatCtx || streamIndex < 0 || streamIndex >= m_pFormatCtx->nb_streams)
    {
        return 0.0;
    }
    AVStream* stream = m_pFormatCtx->streams[streamIndex];
    return av_gettime() * av_q2d(stream->time_base);
}

// ST_AVPacket implementation
ST_AVPacket::ST_AVPacket()
    : m_pkt(av_packet_alloc())
{
}

ST_AVPacket::~ST_AVPacket()
{
    if (m_pkt)
    {
        av_packet_free(&m_pkt);
    }
}

ST_AVPacket::ST_AVPacket(ST_AVPacket&& other) noexcept
    : m_pkt(other.m_pkt)
{
    other.m_pkt = nullptr;
}

ST_AVPacket& ST_AVPacket::operator=(ST_AVPacket&& other) noexcept
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

int ST_AVPacket::ReadPacket(AVFormatContext* pFormatContext)
{
    return av_read_frame(pFormatContext, m_pkt);
}

int ST_AVPacket::SendPacket(AVCodecContext* pCodecContext)
{
    return avcodec_send_packet(pCodecContext, m_pkt);
}

void ST_AVPacket::UnrefPacket()
{
    av_packet_unref(m_pkt);
}

int ST_AVPacket::CopyPacket(const AVPacket* src)
{
    return av_packet_copy_props(m_pkt, src);
}

void ST_AVPacket::MovePacket(AVPacket* src)
{
    av_packet_move_ref(m_pkt, src);
}

void ST_AVPacket::RescaleTimestamp(const AVRational& srcTimeBase, const AVRational& dstTimeBase)
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

// ST_AVInputFormat implementation
ST_AVInputFormat::ST_AVInputFormat(ST_AVInputFormat&& other) noexcept
    : m_pInputFormatCtx(other.m_pInputFormatCtx)
{
    other.m_pInputFormatCtx = nullptr;
}

ST_AVInputFormat& ST_AVInputFormat::operator=(ST_AVInputFormat&& other) noexcept
{
    if (this != &other)
    {
        m_pInputFormatCtx = other.m_pInputFormatCtx;
        other.m_pInputFormatCtx = nullptr;
    }
    return *this;
}

void ST_AVInputFormat::FindInputFormat(const std::string& deviceFormat)
{
    m_pInputFormatCtx = av_find_input_format(deviceFormat.c_str());
}

// ST_AVCodecContext implementation
ST_AVCodecContext::ST_AVCodecContext(const AVCodec* codec)
    : m_pCodecContext(avcodec_alloc_context3(codec))
{
}

ST_AVCodecContext::~ST_AVCodecContext()
{
    if (m_pCodecContext)
    {
        avcodec_free_context(&m_pCodecContext);
    }
}

int ST_AVCodecContext::BindParamToContext(const AVCodecParameters* parameters)
{
    return avcodec_parameters_to_context(m_pCodecContext, parameters);
}

int ST_AVCodecContext::OpenCodec(const AVCodec* codec, AVDictionary** options)
{
    return avcodec_open2(m_pCodecContext, codec, options);
}

// ST_SDLAudioDeviceID implementation
ST_SDLAudioDeviceID::ST_SDLAudioDeviceID(SDL_AudioDeviceID deviceId, const SDL_AudioSpec* spec)
    : m_audioDevice(SDL_OpenAudioDevice(deviceId, spec))
{
}

ST_SDLAudioDeviceID::~ST_SDLAudioDeviceID()
{
    if (m_audioDevice)
    {
        SDL_CloseAudioDevice(m_audioDevice);
    }
}

ST_SDLAudioDeviceID::ST_SDLAudioDeviceID(ST_SDLAudioDeviceID&& other) noexcept
    : m_audioDevice(other.m_audioDevice)
{
    other.m_audioDevice = 0;
}

ST_SDLAudioDeviceID& ST_SDLAudioDeviceID::operator=(ST_SDLAudioDeviceID&& other) noexcept
{
    if (this != &other)
    {
        if (m_audioDevice)
        {
            SDL_CloseAudioDevice(m_audioDevice);
        }
        m_audioDevice = other.m_audioDevice;
        other.m_audioDevice = 0;
    }
    return *this;
}

// ST_SDLAudioStream implementation
ST_SDLAudioStream::ST_SDLAudioStream(SDL_AudioSpec* srcSpec, SDL_AudioSpec* dstSpec)
    : m_audioStreamSdl(SDL_CreateAudioStream(srcSpec, dstSpec))
{
}

ST_SDLAudioStream::~ST_SDLAudioStream()
{
    if (m_audioStreamSdl)
    {
        SDL_DestroyAudioStream(m_audioStreamSdl);
    }
}

ST_SDLAudioStream::ST_SDLAudioStream(ST_SDLAudioStream&& other) noexcept
    : m_audioStreamSdl(other.m_audioStreamSdl)
{
    other.m_audioStreamSdl = nullptr;
}

ST_SDLAudioStream& ST_SDLAudioStream::operator=(ST_SDLAudioStream&& other) noexcept
{
    if (this != &other)
    {
        if (m_audioStreamSdl)
        {
            SDL_DestroyAudioStream(m_audioStreamSdl);
        }
        m_audioStreamSdl = other.m_audioStreamSdl;
        other.m_audioStreamSdl = nullptr;
    }
    return *this;
}

// ST_AVCodecParameters implementation
ST_AVCodecParameters::ST_AVCodecParameters(AVCodecParameters* obj)
    : m_codecParameters(obj)
{
}

ST_AVCodecParameters::~ST_AVCodecParameters()
{
    m_codecParameters = nullptr;
}

AVCodecID ST_AVCodecParameters::GetCodecId() const
{
    return m_codecParameters ? m_codecParameters->codec_id : AV_CODEC_ID_NONE;
}

int ST_AVCodecParameters::GetSamplePerRate() const
{
    return m_codecParameters ? av_get_bytes_per_sample(GetSampleFormat()) : 0;
}

AVSampleFormat ST_AVCodecParameters::GetSampleFormat() const
{
    return m_codecParameters ? static_cast<AVSampleFormat>(m_codecParameters->format) : AV_SAMPLE_FMT_NONE;
}

int ST_AVCodecParameters::GetBitPerSample() const
{
    return m_codecParameters ? av_get_bits_per_sample(m_codecParameters->codec_id) : 0;
}

// ST_AVCodec implementation
ST_AVCodec::ST_AVCodec(enum AVCodecID id)
    : m_pAvCodec(avcodec_find_decoder(id))
{
}

ST_AVCodec::ST_AVCodec(const AVCodec* obj)
    : m_pAvCodec(obj)
{
}

// ST_SwrContext implementation
ST_SwrContext::~ST_SwrContext()
{
    if (m_swrCtx)
    {
        swr_free(&m_swrCtx);
    }
}

int ST_SwrContext::SwrContextInit()
{
    return m_swrCtx ? swr_init(m_swrCtx) : -1;
}

// ST_AVChannelLayout implementation
ST_AVChannelLayout::ST_AVChannelLayout(AVChannelLayout* ptr)
    : channel(ptr)
{
}

// ST_AudioPlayState implementation
void ST_AudioPlayState::Reset()
{
    m_isPlaying = false;
    m_isPaused = false;
    m_isRecording = false;
    m_currentPosition = 0.0;
    m_duration = 0.0;
    m_startTime = 0;
} 