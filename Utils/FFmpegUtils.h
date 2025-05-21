#pragma once

#include <QDebug>
#include <QObject>
#include <QString>
#include "FFmpegAudioDataDefine.h"
#include "SDL3/SDL_stdinc.h"

extern "C" {
#include "../include/libavformat/avformat.h"
}


class FFmpegUtils : public QObject
{
    Q_OBJECT

public:
    FFmpegUtils(QObject* parent = Q_NULLPTR);
    ~FFmpegUtils() override;
    // 注册设备
    static void ResigsterDevice();

public:
    // 编码或解码 ffmpeg
    // inputFilePath 表示需要修改的文件路径
    // outputFilePath 表示输出的文件路径
    // bEncoder 表示是否编码 true表示编码，false表示解码
    // args 表示ffmpeg的参数
    void EncoderOrDecoder(const QString& inputFilePath, const QString& outputFilePath, bool bEncoder, const QStringList& args = QStringList());
    // 查看音视频的参数信息 ffprobe
    // inputFilePath 表示需要查看的文件路径
    // args 表示ffprobe的参数
    QString GetFileInfomation(const QString& inputFilePath, const QStringList& args = QStringList());

    ST_OpenAudioDevice OpenDevice(const QString& devieceFormat, const QString& deviceName);
    // 开始录音
    void StartAudioRecording(const QString& outputFilePath, const QString& encoderFormat);
    // 播放音视频文件 ffplay
    // inputFilePath 表示需要播放的文件路径
    // args 表示ffplay的参数
    void StartAudioPlayback(const QString& inputFilePath, const QStringList& args = QStringList());
    // 显示录音设备的参数S
    void ShowSpec(AVFormatContext* ctx);
    //void ResampleAudio(ST_ResampleAudioData* input, ST_ResampleAudioData* output);
};
