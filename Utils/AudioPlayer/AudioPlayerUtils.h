#pragma once

#include <QObject>
#include "LogSystem/LogSystem.h"


extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
}

/// <summary>
/// 音频播放工具类
/// </summary>
class AudioPlayerUtils  : public QObject
{
    Q_OBJECT

public:
    AudioPlayerUtils(QObject *parent);
    ~AudioPlayerUtils();
    /// <summary>
    /// 注册设备
    /// </summary>
    static void ResigsterDevice();
    /// <summary>
    /// 处理音频帧以生成波形数据
    /// </summary>
    /// <param name="frame">音频帧</param>
    /// <param name="waveformData">波形数据输出</param>
    /// <param name="samplesPerPixel">每像素采样数</param>
    /// <param name="currentSum">当前累计值</param>
    /// <param name="sampleCount">当前采样计数</param>
    /// <param name="maxSample">最大采样值</param>
    static void ProcessAudioFrame(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample);

    /// <summary>
    /// 处理浮点采样格式
    /// </summary>
    static void ProcessFloatSamples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample);

    /// <summary>
    /// 处理16位整数采样格式
    /// </summary>
    static void ProcessInt16Samples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample);

    /// <summary>
    /// 处理32位整数采样格式
    /// </summary>
    static void ProcessInt32Samples(AVFrame* frame, QVector<float>& waveformData, int samplesPerPixel, float& currentSum, int& sampleCount, float& maxSample);

    /// <summary>
    /// 获取所有音频输入设备
    /// </summary>
    /// <returns>设备列表</returns>
    static QStringList GetInputAudioDevices();

};

