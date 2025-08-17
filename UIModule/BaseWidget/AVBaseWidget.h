#pragma once

#include <QWidget>
#include "ui_AVBaseWidget.h"
#include "AudioWidget/PlayerAudioModuleWidget.h"
#include "DomainWidget/FilePathIconListWidgetItem.h"
#include "VideoWidget/PlayerVideoModuleWidget.h"
#include "BasePlayer/MediaPlayerManager.h"

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

    /// <summary>
    /// 进度条值改变槽函数
    /// </summary>
    /// <param name="value">进度值</param>
    void SlotProgressBarValueChanged(qint64 value);
    
    /// <summary>
    /// 更新播放进度槽函数
    /// </summary>
    void SlotUpdatePlayProgress();

protected:
    /// <summary>
    /// 窗口关闭事件
    /// </summary>
    /// <param name="event">关闭事件指针</param>
    void closeEvent(QCloseEvent* event) override;
    /// <summary>
    /// Resize事件
    /// </summary>
    /// <param name="event"></param>
    void resizeEvent(QResizeEvent* event) override;
private:
    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectSignals();
    ~AVBaseWidget() override;

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
    /// 启动音视频播放
    /// </summary>
    /// <param name="filePath">播放文件路径</param>
    /// <param name="startPosition">开始位置（秒）</param>
    void StartAVPlay(const QString& filePath, double startPosition = 0.0);

    /// <summary>
    /// 停止音视频播放
    /// </summary>
    void StopAVPlay();

    /// <summary>
    /// 启动音视频录制
    /// </summary>
    /// <param name="filePath">录制文件路径</param>
    void StartAVRecord(const QString& filePath);

    /// <summary>
    /// 停止音视频录制
    /// </summary>
    void StopAVRecord();
    /// <summary>
    /// 显示音视频控件
    /// </summary>
    /// <param name="bAudio"></param>
    void ShowAVWidget(bool bAudio = true);

    /// <summary>
    /// 安全获取播放状态
    /// </summary>
    /// <returns>是否正在播放</returns>
    bool GetIsPlaying() const;

    /// <summary>
    /// 安全获取暂停状态
    /// </summary>
    /// <returns>是否已暂停</returns>
    bool GetIsPaused() const;

    /// <summary>
    /// 安全获取录制状态
    /// </summary>
    /// <returns>是否正在录制</returns>
    bool GetIsRecording() const;

    /// <summary>
    /// 格式化时间显示（已废弃，使用MusicProgressBar内部的时间格式化）
    /// </summary>
    /// <param name="seconds">秒数</param>
    /// <returns>格式化后的时间字符串</returns>
    // QString FormatTime(int seconds) const;

    /// <summary>
    /// 初始化歌单管理
    /// </summary>
    void InitializePlaylistManager();

    /// <summary>
    /// 获取歌单目录路径
    /// </summary>
    QString GetPlaylistDirectory() const;

    /// <summary>
    /// 添加新歌单
    /// </summary>
    void AddNewPlaylist();

    /// <summary>
    /// 删除当前选中的歌单
    /// </summary>
    void DeleteCurrentPlaylist();

    /// <summary>
    /// 加载歌单列表
    /// </summary>
    void LoadPlaylists();

    /// <summary>
    /// 加载指定歌单的内容
    /// </summary>
    /// <param name="playlistPath">歌单JSON文件路径</param>
    void LoadPlaylistContent(const QString& playlistPath);

    /// <summary>
    /// 创建示例歌单
    /// </summary>
    void CreateDefaultPlaylist();

private slots:
    /// <summary>
    /// 歌单被选中槽函数
    /// </summary>
    void SlotPlaylistSelected(const QString& playlistPath);

    /// <summary>
    /// 添加目录按钮点击槽函数
    /// </summary>
    void SlotAddDirectoryClicked();

    /// <summary>
    /// 删除目录按钮点击槽函数
    /// </summary>
    void SlotDeleteDirectoryClicked();

private:
    Ui::AVBaseWidgetClass* ui;
    MediaPlayerManager* m_playerManager{nullptr};           /// 媒体播放管理器（单例）
    PlayerAudioModuleWidget* m_audioPlayerWidget{nullptr}; /// 音频播放器模块控件
    PlayerVideoModuleWidget* m_videoPlayerWidget{nullptr}; /// 视频播放器模块控件
    QString m_currentAVFile;                                /// 当前播放的音视频文件
    QTimer* m_playTimer{nullptr};                           /// 播放定时器
    double m_currentPosition{0.0};                          /// 当前播放位置（秒）
    bool m_isProgressBarUpdating{false};                    /// 进度条更新标志，防止循环更新

};
