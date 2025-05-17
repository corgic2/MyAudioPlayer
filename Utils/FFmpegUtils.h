#pragma once

#include <QDebug>
#include <QObject>
#include <QString>
#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_stdinc.h"

extern "C" {
#include "../include/libavcodec/avcodec.h"
#include "../include/libavdevice/avdevice.h"
#include "../include/libavformat/avformat.h"
#include "../include/libavutil/avutil.h"
#include "../include/libswresample/swresample.h"
}


class FFmpegUtils : public QObject
{
    Q_OBJECT

  public:
    FFmpegUtils(QObject* parent);
    ~FFmpegUtils() override;

public:
    // 编码或解码 ffmpeg
    // inputFilePath 表示需要修改的文件路径
    // outputFilePath 表示输出的文件路径
    // bEncoder 表示是否编码 true表示编码，false表示解码
    // args 表示ffmpeg的参数
    static void EncoderOrDecoder(const QString& inputFilePath, const QString& outputFilePath, bool bEncoder, const QStringList& args = QStringList());
    // 查看音视频的参数信息 ffprobe
    // inputFilePath 表示需要查看的文件路径
    // args 表示ffprobe的参数
    static QString GetFileInfomation(const QString& inputFilePath, const QStringList& args = QStringList());
    // 注册设备
    static void ResigsterDevice();
    // 开始录音
    static void StartAudioRecording(const QString& outputFilePath, const QString& encoderFormat);
    // 播放音视频文件 ffplay
    // inputFilePath 表示需要播放的文件路径
    // args 表示ffplay的参数
    static void StartAudioPlayback(const QString& inputFilePath, const QStringList& args = QStringList());
    // 显示录音设备的参数S
    static void ShowSpec(AVFormatContext* ctx);

private:
    // 根据输出格式自动选择编码器
    static const AVCodec* FindEncoder(const char* format_name);
    // 根据输入格式自动选择解码器
    static void ConfigureEncoderParams(AVCodecParameters* codecPar, AVCodecContext* enc_ctx);

    // 将 FFmpeg 的 AVSampleFormat 转换为 SDL_AudioFormat
    static SDL_AudioFormat FFmpegToSDLFormat(AVSampleFormat fmt);

};
