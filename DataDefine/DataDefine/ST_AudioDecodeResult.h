#pragma once

#include <vector>
#include "BaseDataDefine/ST_AVSampleFormat.h"

/// <summary>
/// 音频解码结果结构体
/// </summary>
struct ST_AudioDecodeResult
{
    std::vector<uint8_t> audioData; /// 解码后的音频数据
    int samplesCount = 0;           /// 样本数量
    int channels = 0;               /// 通道数
    int sampleRate = 0;             /// 采样率
    AVSampleFormat format;          /// 采样格式
};


