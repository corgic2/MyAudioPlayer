#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "DataDefine/ST_AudioDecodeResult.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_par.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_pixels.h>
#define FMT_NAME "dshow"

/// <summary>
/// 视频解码结果结构体
/// </summary>
struct ST_VideoDecodeResult
{
    /// <summary>
    /// 视频帧数据
    /// </summary>
    std::vector<uint8_t> videoData;

    /// <summary>
    /// 视频宽度
    /// </summary>
    int width;

    /// <summary>
    /// 视频高度
    /// </summary>
    int height;

    /// <summary>
    /// 像素格式
    /// </summary>
    AVPixelFormat format;

    /// <summary>
    /// 时间戳
    /// </summary>
    int64_t timestamp;

    /// <summary>
    /// 构造函数
    /// </summary>
    ST_VideoDecodeResult() : width(0), height(0), format(AV_PIX_FMT_NONE), timestamp(0)
    {
    }
};

/// <summary>
/// FFmpeg公共工具类
/// </summary>
class FFmpegPublicUtils
{
public:
    FFmpegPublicUtils() = default;
    ~FFmpegPublicUtils() = default;

    /// <summary>
    /// 根据输出格式自动选择编码器
    /// </summary>
    /// <param name="formatName">格式名称</param>
    /// <returns>编码器指针</returns>
    static const AVCodec* FindEncoder(const char* formatName);

    /// <summary>
    /// 根据编解码器ID查找解码器
    /// </summary>
    /// <param name="codecId">编解码器ID</param>
    /// <returns>解码器指针</returns>
    static const AVCodec* FindDecoder(AVCodecID codecId);

    /// <summary>
    /// 配置编解码器参数，参数设置对应的接编解码器
    /// </summary>
    /// <param name="codecPar">编解码器参数</param>
    /// <param name="encCtx">编解码器上下文</param>
    static void ConfigureEncoderParams(AVCodecParameters* codecPar, AVCodecContext* encCtx);

    /// <summary>
    /// 将FFmpeg的AVSampleFormat转换为SDL_AudioFormat
    /// </summary>
    /// <param name="fmt">FFmpeg采样格式</param>
    /// <returns>SDL音频格式</returns>
    static SDL_AudioFormat FFmpegToSDLFormat(AVSampleFormat fmt);

    /// <summary>
    /// 将FFmpeg的AVPixelFormat转换为SDL像素格式
    /// </summary>
    /// <param name="fmt">FFmpeg像素格式</param>
    /// <returns>SDL像素格式</returns>
    static SDL_PixelFormat FFmpegToSDLPixelFormat(AVPixelFormat fmt);

    /// <summary>
    /// 解码音频数据包
    /// </summary>
    /// <param name="packet">音频数据包</param>
    /// <param name="codecCtx">解码器上下文</param>
    /// <returns>解码结果</returns>
    static ST_AudioDecodeResult DecodeAudioPacket(const AVPacket* packet, AVCodecContext* codecCtx);

    /// <summary>
    /// 解码视频数据包
    /// </summary>
    /// <param name="packet">视频数据包</param>
    /// <param name="codecCtx">解码器上下文</param>
    /// <returns>解码结果</returns>
    static ST_VideoDecodeResult DecodeVideoPacket(const AVPacket* packet, AVCodecContext* codecCtx);

    /// <summary>
    /// 获取音频帧大小
    /// </summary>
    /// <param name="frame">音频帧</param>
    /// <returns>音频帧大小（字节）</returns>
    static int GetAudioFrameSize(const AVFrame* frame);

    /// <summary>
    /// 获取视频帧大小
    /// </summary>
    /// <param name="frame">视频帧</param>
    /// <returns>视频帧大小（字节）</returns>
    static int GetVideoFrameSize(const AVFrame* frame);

    /// <summary>
    /// 获取视频流信息
    /// </summary>
    /// <param name="formatCtx">格式上下文</param>
    /// <param name="streamIndex">流索引</param>
    /// <param name="width">宽度输出</param>
    /// <param name="height">高度输出</param>
    /// <param name="frameRate">帧率输出</param>
    /// <param name="duration">时长输出</param>
    /// <returns>是否成功获取信息</returns>
    static bool GetVideoStreamInfo(AVFormatContext* formatCtx, int streamIndex, 
                                  int& width, int& height, double& frameRate, double& duration);

    /// <summary>
    /// 创建视频转换上下文
    /// </summary>
    /// <param name="srcWidth">源宽度</param>
    /// <param name="srcHeight">源高度</param>
    /// <param name="srcFormat">源像素格式</param>
    /// <param name="dstWidth">目标宽度</param>
    /// <param name="dstHeight">目标高度</param>
    /// <param name="dstFormat">目标像素格式</param>
    /// <returns>转换上下文指针</returns>
    static struct SwsContext* CreateVideoConverter(int srcWidth, int srcHeight, AVPixelFormat srcFormat,
                                                  int dstWidth, int dstHeight, AVPixelFormat dstFormat);
};
