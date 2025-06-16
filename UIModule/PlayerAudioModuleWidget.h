#pragma once

#include "CoreServerGlobal.h"
#include "AudioFFmpegUtils.h"
#include "ui_PlayerAudioModuleWidget.h"
#include "CoreWidget/CustomLabel.h"
#include "DomainWidget/FilePathIconListWidgetItem.h"
#include "AudioWaveformWidget.h"


QT_BEGIN_NAMESPACE namespace Ui
{
    class PlayerAudioModuleWidgetClass;
};
QT_END_NAMESPACE

/// <summary>
/// 音频播放器模块控件类
/// </summary>
class PlayerAudioModuleWidget : public QWidget
{
    Q_OBJECT;

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    explicit PlayerAudioModuleWidget(QWidget* parent = nullptr);
    /// <summary>
    /// 析构函数
    /// </summary>
    ~PlayerAudioModuleWidget() override;

    /// <summary>
    /// 添加音频文件到列表
    /// </summary>
    /// <param name="filePaths">音频文件路径列表</param>
    void AddAudioFiles(const QStringList& filePaths);

    /// <summary>
    /// 从列表移除音频文件
    /// </summary>
    /// <param name="filePath">音频文件路径</param>
    void RemoveAudioFile(const QString& filePath);

    /// <summary>
    /// 清空音频文件列表
    /// </summary>
    void ClearAudioFiles();

    /// <summary>
    /// 获取指定索引的文件信息
    /// </summary>
    /// <param name="index">文件索引</param>
    /// <returns>文件信息</returns>
    FilePathIconListWidgetItem::ST_NodeInfo GetFileInfo(int index) const;

    /// <summary>
    /// 保存文件列表到JSON文件
    /// </summary>
    void SaveFileListToJson();

signals:
    /// <summary>
    /// 音频文件被选中信号
    /// </summary>
    /// <param name="filePath">音频文件路径</param>
    void SigAudioFileSelected(const QString& filePath);

    /// <summary>
    /// 音频播放完成信号
    /// </summary>
    void SigAudioPlayFinished();

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
    /// 输入设备改变槽函数
    /// </summary>
    void SlotInputDeviceChanged(int index);

    /// <summary>
    /// 音频文件选中槽函数
    /// </summary>
    void SlotAudioFileSelected(const QString& filePath);

    /// <summary>
    /// 音频文件双击槽函数
    /// </summary>
    void SlotAudioFileDoubleClicked(const QString& filePath);

    /// <summary>
    /// 自动保存槽函数
    /// </summary>
    void SlotAutoSave();

    /// <summary>
    /// 音频播放完成槽函数
    /// </summary>
    void SlotAudioPlayFinished();

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

    /// <summary>
    /// 初始化音频设备
    /// </summary>
    void InitAudioDevices();

    /// <summary>
    /// 初始化界面
    /// </summary>
    void InitializeWidget();

    /// <summary>
    /// 更新播放状态
    /// </summary>
    void UpdatePlayState(bool isPlaying);

    /// <summary>
    /// 获取文件在列表中的索引
    /// </summary>
    /// <param name="filePath">要查找的文件路径</param>
    /// <returns>文件在列表中的索引，如果不存在则返回-1</returns>
    int GetFileIndex(const QString& filePath) const;

    /// <summary>
    /// 从JSON文件加载文件列表
    /// </summary>
    void LoadFileListFromJson();

    /// <summary>
    /// 获取JSON文件路径
    /// </summary>
    /// <returns>JSON文件的完整路径</returns>
    QString GetJsonFilePath() const;

    /// <summary>
    /// 启动音频播放线程
    /// </summary>
    void StartAudioPlayThread();

    /// <summary>
    /// 停止音频播放线程
    /// </summary>
    void StopAudioPlayThread();

private:
    Ui::PlayerAudioModuleWidgetClass* ui;
    AudioFFmpegUtils m_ffmpeg;
    QString m_currentAudioFile;                    /// 当前播放的音频文件
    bool m_isRecording = false;                            /// 是否正在录制
    bool m_isPlaying = false;                      /// 是否正在播放
    bool m_isPaused = false;                               /// 是否已暂停
    QTimer *m_playTimer = nullptr;                   /// 播放定时器
    QString m_jsonFileName = "audiofiles.json";                           /// JSON文件名
    QTimer *m_autoSaveTimer = nullptr;               /// 自动保存定时器
    size_t m_playThreadId;                         /// 音频播放线程ID
    std::atomic<bool> m_playThreadRunning{false};  /// 音频播放线程运行标志
    static const int AUTO_SAVE_INTERVAL = 1800000; // 30分钟 = 30 * 60 * 1000毫秒
    AudioWaveformWidget* m_waveformWidget;         /// 音频波形控件
};
