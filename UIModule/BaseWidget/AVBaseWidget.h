#pragma once

#include <QWidget>
#include "AudioFFmpegUtils.h"
#include "BaseFFmpegUtils.h"
#include "ui_AVBaseWidget.h"
#include "VedioFFmpegUtils.h"
#include "DomainWidget/FilePathIconListWidgetItem.h"

QT_BEGIN_NAMESPACE namespace Ui
{
    class AVBaseWidgetClass;
};

QT_END_NAMESPACE;

class AVBaseWidget : public QWidget
{
    Q_OBJECT;

public:
    AVBaseWidget(QWidget* parent = nullptr);
    /// <summary>
    /// 添加音视频文件到列表
    /// </summary>
    /// <param name="filePaths">音视频文件路径列表</param>
    void AddAVFiles(const QStringList& filePaths);

    /// <summary>
    /// 从列表移除音视频文件
    /// </summary>
    /// <param name="filePath">音视频文件路径</param>
    void RemoveAVFile(const QString& filePath);

    /// <summary>
    /// 清空音视频文件列表
    /// </summary>
    void ClearAVFiles();

    /// <summary>
    /// 获取指定索引的文件信息
    /// </summary>
    /// <param name="index">文件索引</param>
    /// <returns>文件信息</returns>
    FilePathIconListWidgetItem::ST_NodeInfo GetFileInfo(int index) const;

signals:
    /// <summary>
    /// 音视频文件被选中信号
    /// </summary>
    /// <param name="filePath">音视频文件路径</param>
    void SigAVFileSelected(const QString& filePath);

    /// <summary>
    /// 音视频播放完成信号
    /// </summary>
    void SigAVPlayFinished();

    /// <summary>
    /// 音视频录制完成信号
    /// </summary>
    void SigAVRecordFinished();

protected slots:
    /// <summary>
    /// 录制按钮点击槽函数
    /// </summary>
    void SlotBtnRecordClicked();

    /// <summary>
    /// 播放按钮点击槽函数
    /// </summary>
    void SlotBtnPlayClicked();

    /// <summary>
    /// 前进按钮点击槽函数
    /// </summary>
    void SlotBtnForwardClicked();

    /// <summary>
    /// 后退按钮点击槽函数
    /// </summary>
    void SlotBtnBackwardClicked();

    /// <summary>
    /// 下一个按钮点击槽函数
    /// </summary>
    void SlotBtnNextClicked();

    /// <summary>
    /// 上一个按钮点击槽函数
    /// </summary>
    void SlotBtnPreviousClicked();

    /// <summary>
    /// 音视频文件选中槽函数
    /// </summary>
    void SlotAVFileSelected(const QString& filePath);

    /// <summary>
    /// 音视频文件双击槽函数
    /// </summary>
    void SlotAVFileDoubleClicked(const QString& filePath);

    /// <summary>
    /// 音视频播放完成槽函数
    /// </summary>
    void SlotAVPlayFinished();

    /// <summary>
    /// 音视频录制完成槽函数
    /// </summary>
    void SlotAVRecordFinished();

protected:
    /// <summary>
    /// 窗口关闭事件
    /// </summary>
    /// <param name="event">关闭事件指针</param>
    void closeEvent(QCloseEvent* event) override;

private:
    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();
    ~AVBaseWidget();

    /// <summary>
    /// 初始化界面
    /// </summary>
    void InitializeWidget();

    /// <summary>
    /// 获取文件在列表中的索引
    /// </summary>
    /// <param name="filePath">要查找的文件路径</param>
    /// <returns>文件在列表中的索引，如果不存在则返回-1</returns>
    int GetFileIndex(const QString& filePath) const;

    /// <summary>
    /// 启动音视频播放线程
    /// </summary>
    void StartAVPlayThread();

    /// <summary>
    /// 停止音视频播放线程
    /// </summary>
    void StopAVPlayThread();

    /// <summary>
    /// 启动音视频录制线程
    /// </summary>
    /// <param name="filePath">录制文件路径</param>
    void StartAVRecordThread(const QString& filePath);

    /// <summary>
    /// 停止音视频录制线程
    /// </summary>
    void StopAVRecordThread();

private:
    Ui::AVBaseWidgetClass* ui;
    BaseFFmpegUtils* m_ffmpeg = nullptr;
    AudioFFmpegUtils m_audioFFmpeg;
    VedioFFmpegUtils m_vedioFFmpeg;
    QString m_currentAVFile;                     /// 当前播放的音视频文件
    bool m_isRecording = false;                     /// 是否正在录制
    bool m_isPlaying = false;                       /// 是否正在播放
    bool m_isPaused = false;                        /// 是否已暂停
    QTimer* m_playTimer = nullptr;                  /// 播放定时器
    size_t m_playThreadId;                          /// 音视频播放线程ID
    std::atomic<bool> m_playThreadRunning{false};   /// 音视频播放线程运行标志
    size_t m_recordThreadId;                        /// 音视频录制线程ID
    std::atomic<bool> m_recordThreadRunning{false}; /// 音视频录制线程运行标志
    double m_currentPosition{0.0};                  /// 当前播放位置（秒）
};
