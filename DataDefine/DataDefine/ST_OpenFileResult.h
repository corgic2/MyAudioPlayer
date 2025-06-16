#pragma once
#include <QString>
#include "BaseDataDefine/ST_AVCodec.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVCodecParameters.h"
#include "BaseDataDefine/ST_AVFormatContext.h"
class ST_OpenFileResult
{
  public:
    ST_OpenFileResult() = default;
    ~ST_OpenFileResult();

  public:
    bool OpenFilePath(const QString &filePath);
    ST_AVFormatContext *m_formatCtx = nullptr;
    ST_AVCodecParameters *m_codecParams = nullptr;
    ST_AVCodec *m_codec = nullptr;
    ST_AVCodecContext *m_codecCtx = nullptr;
    int m_audioStreamIdx = -1;
};



