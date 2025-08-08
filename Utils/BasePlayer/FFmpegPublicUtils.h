#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "DataDefine/ST_AudioDecodeResult.h"
#include "DataDefine/ST_OpenFileResult.h"
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
#include "libavdevice/avdevice.h"
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
    /// 注册设备
    /// </summary>
    static void ResigsterDevice();
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

    /// <summary>
    /// 验证文件路径有效性
    /// </summary>
    /// <param name="filePath">文件路径</param>
    /// <returns>是否有效</returns>
    static bool ValidateFilePath(const QString& filePath);

    /// <summary>
    /// 获取音频流信息
    /// </summary>
    /// <param name="formatCtx">格式上下文</param>
    /// <param name="streamIndex">流索引</param>
    /// <param name="sampleRate">采样率输出</param>
    /// <param name="channels">通道数输出</param>
    /// <param name="format">音频格式输出</param>
    /// <param name="duration">时长输出</param>
    /// <returns>是否成功获取信息</returns>
    static bool GetAudioStreamInfo(AVFormatContext* formatCtx, int streamIndex,
                                  int& sampleRate, int& channels, AVSampleFormat& format, double& duration);

    /// <summary>
    /// 执行音频seek操作
    /// </summary>
    /// <param name="formatCtx">格式上下文</param>
    /// <param name="codecCtx">解码器上下文</param>
    /// <param name="seconds">跳转到的时间（秒）</param>
    /// <returns>是否成功</returns>
    static bool SeekAudio(AVFormatContext* formatCtx, AVCodecContext* codecCtx, double seconds);

    /// <summary>
    /// 执行视频seek操作
    /// </summary>
    /// <param name="formatCtx">格式上下文</param>
    /// <param name="codecCtx">解码器上下文</param>
    /// <param name="seconds">跳转到的时间（秒）</param>
    /// <returns>是否成功</returns>
    static bool SeekVideo(AVFormatContext* formatCtx, AVCodecContext* codecCtx, double seconds);

    /// <summary>
    /// 通用的文件时长获取
    /// </summary>
    /// <param name="formatCtx">格式上下文</param>
    /// <returns>时长（秒）</returns>
    static double GetFileDuration(AVFormatContext* formatCtx);

    /// <summary>
    /// 获取音视频文件详细信息
    /// </summary>
    /// <param name="filePath">文件路径</param>
    /// <param name="fileName">文件名输出</param>
    /// <param name="fileSize">文件大小输出（字节）</param>
    /// <param name="duration">时长输出（秒）</param>
    /// <param name="format">格式输出</param>
    /// <param name="bitrate">比特率输出（bps）</param>
    /// <param name="width">视频宽度输出</param>
    /// <param name="height">视频高度输出</param>
    /// <param name="sampleRate">音频采样率输出</param>
    /// <param name="channels">音频声道数输出</param>
    /// <returns>是否成功获取信息</returns>
    static bool GetMediaFileInfo(const QString& filePath, QString& fileName, qint64& fileSize, 
                                double& duration, QString& format, int& bitrate, 
                                int& width, int& height, int& sampleRate, int& channels);
};
