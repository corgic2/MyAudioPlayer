#include "ST_OpenFileResult.h"

#include "LogSystem/LogSystem.h"
#include "SDKCommonDefine/SDKCommonDefine.h"

ST_OpenFileResult::~ST_OpenFileResult()
{
    SAFE_DELETE_POINTER_VALUE(m_formatCtx);
    SAFE_DELETE_POINTER_VALUE(m_codec);
    SAFE_DELETE_POINTER_VALUE(m_codecCtx);
    SAFE_DELETE_POINTER_VALUE(m_codecParams);
}

bool ST_OpenFileResult::OpenFilePath(const QString &filePath)
{
    // 打开音频文件
    m_formatCtx = new ST_AVFormatContext;
    int ret = m_formatCtx->OpenInputFilePath(filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0)
    {
        LOG_WARN("Failed to open input file");
        return false;
    }

    ret = avformat_find_stream_info(m_formatCtx->GetRawContext(), nullptr);
    if (ret < 0)
    {
        LOG_WARN("Find stream info failed");
        return false;
    }

    m_audioStreamIdx = m_formatCtx->FindBestStream(AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_audioStreamIdx < 0)
    {
        LOG_WARN("No audio stream found");
        return false;
    }

    // 初始化解码器
    m_codecParams = new ST_AVCodecParameters(m_formatCtx->GetRawContext()->streams[m_audioStreamIdx]->codecpar);
    m_codec = new ST_AVCodec(m_codecParams->GetCodecId());
    m_codecCtx = new ST_AVCodecContext(m_codec->GetRawCodec());

    if (!m_codec->GetRawCodec())
    {
        LOG_WARN("Failed to find codec");
        return false;
    }

    if (m_codecCtx->BindParamToContext(m_codecParams->GetRawParameters()) < 0)
    {
        LOG_WARN("Failed to bind codec parameters");
        return false;
    }

    if (m_codecCtx->OpenCodec(m_codec->GetRawCodec(), nullptr) < 0)
    {
        LOG_WARN("Failed to open codec");
        return false;
    }
    return true;
}
