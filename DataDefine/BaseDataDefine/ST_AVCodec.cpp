#pragma once
#include "ST_AVCodec.h"

// ST_AVCodec implementation
ST_AVCodec::ST_AVCodec(enum AVCodecID id) : m_pAvCodec(avcodec_find_decoder(id))
{
}

ST_AVCodec::ST_AVCodec(const AVCodec *obj) : m_pAvCodec(obj)
{
}