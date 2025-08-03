#pragma once

#include <atomic>
#include <memory>
#include <QMutex>
#include <QObject>
#include <QString>
#include "VideoPlayWorker.h"
#include "BaseDataDefine/ST_AVCodecContext.h"
#include "BaseDataDefine/ST_AVFormatContext.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
}

/// <summary>
/// 视频录制工作线程
/// </summary>
class VideoRecordWorker : public QObject
{
    Q_OBJECT public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit VideoRecordWorker(QObject* parent = nullptr);

    /// <summary>
    /// 析构函数
    /// </summary>
    ~VideoRecordWorker() override;

public slots:
    /// <summary>
    /// 开始录制
    /// </summary>
    /// <param name="outputPath">输出文件路径</param>
    void SlotStartRecord(const QString& outputPath);

    /// <summary>
    /// 停止录制
    /// </summary>
    void SlotStopRecord();

signals:
    /// <summary>
    /// 错误信号
    /// </summary>
    /// <param name="errorMsg">错误信息</param>
    void SigError(const QString& errorMsg);

private:
    /// <summary>
    /// 初始化录制器
    /// </summary>
    /// <param name="outputPath">输出文件路径</param>
    /// <returns>是否初始化成功</returns>
    bool InitRecorder(const QString& outputPath);

    /// <summary>
    /// 清理资源
    /// </summary>
    void Cleanup();

    /// <summary>
    /// 录制循环
    /// </summary>
    void RecordLoop();

    /// <summary>
    /// 获取默认视频录制设备
    /// </summary>
    /// <returns>设备名称</returns>
    QString GetDefaultVideoDevice();

private:
    /// <summary>
    /// 输入格式上下文
    /// </summary>
    std::unique_ptr<ST_AVFormatContext> m_pInputFormatCtx{nullptr};

    /// <summary>
    /// 输出格式上下文
    /// </summary>
    std::unique_ptr<ST_AVFormatContext> m_pOutputFormatCtx{nullptr};

    /// <summary>
    /// 输入解码器上下文
    /// </summary>
    std::unique_ptr<ST_AVCodecContext> m_pInputCodecCtx{nullptr};

    /// <summary>
    /// 输出编码器上下文
    /// </summary>
    std::unique_ptr<ST_AVCodecContext> m_pOutputCodecCtx{nullptr};

    /// <summary>
    /// 视频流索引
    /// </summary>
    int m_videoStreamIndex{-1};

    /// <summary>
    /// 输入数据包
    /// </summary>
    ST_AVPacket m_pInputPacket;

    /// <summary>
    /// 输出数据包
    /// </summary>
    ST_AVPacket m_pOutputPacket;

    /// <summary>
    /// 是否需要停止
    /// </summary>
    std::atomic<bool> m_bNeedStop{false};
    /// <summary>
    /// 线程安全锁
    /// </summary>
    QMutex m_mutex;

    /// <summary>
    /// 输出文件路径
    /// </summary>
    QString m_outputPath;
};
