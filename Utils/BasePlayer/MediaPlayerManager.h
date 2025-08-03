#pragma once

#include <memory>
#include <mutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include "../AudioPlayer/AudioFFmpegPlayer.h"
#include "../VideoPlayer/VideoFFmpegPlayer.h"
#include <atomic>
#include <chrono>

/// <summary>
/// 媒体类型枚举
/// </summary>
enum class EM_MediaType
{
    /// <summary>
    /// 音频
    /// </summary>
    Audio,

    /// <summary>
    /// 视频（仅视频，无音频）
    /// </summary>
    Video,

    /// <summary>
    /// 视频含音频（需同时播放音频和视频）
    /// </summary>
    VideoWithAudio,

    /// <summary>
    /// 未知
    /// </summary>
    Unknown
};

/// <summary>
/// 媒体播放管理器（单例模式）
/// 确保同一时间只有一个音视频播放对象在工作
/// </summary>
class MediaPlayerManager : public QObject
{
    Q_OBJECT

public:
    /// <summary>
    /// 获取单例实例
    /// </summary>
    /// <returns>单例实例指针</returns>
    static MediaPlayerManager* Instance();

    /// <summary>
    /// 析构函数
    /// </summary>
    ~MediaPlayerManager() override;

    /// <summary>
    /// 播放媒体文件
    /// </summary>
    /// <param name="filePath">文件路径</param>
    /// <param name="startPosition">开始位置（秒）</param>
    /// <param name="args">播放参数</param>
    /// <returns>是否成功开始播放</returns>
    bool PlayMedia(const QString& filePath, double startPosition = 0.0, const QStringList& args = QStringList());

    /// <summary>
    /// 暂停播放
    /// </summary>
    void PausePlay();

    /// <summary>
    /// 恢复播放
    /// </summary>
    void ResumePlay();

    /// <summary>
    /// 停止播放
    /// </summary>
    void StopPlay();

    /// <summary>
    /// 跳转播放位置
    /// </summary>
    /// <param name="seconds">目标时间（秒）</param>
    void SeekPlay(double seconds);

    /// <summary>
    /// 开始录制
    /// </summary>
    /// <param name="outputPath">输出文件路径</param>
    /// <param name="mediaType">录制媒体类型</param>
    void StartRecording(const QString& outputPath, EM_MediaType mediaType = EM_MediaType::Audio);

    /// <summary>
    /// 停止录制
    /// </summary>
    void StopRecording();

    /// <summary>
    /// 获取是否正在播放
    /// </summary>
    /// <returns>是否正在播放</returns>
    bool IsPlaying() const;

    /// <summary>
    /// 获取是否已暂停
    /// </summary>
    /// <returns>是否已暂停</returns>
    bool IsPaused() const;

    /// <summary>
    /// 获取是否正在录制
    /// </summary>
    /// <returns>是否正在录制</returns>
    bool IsRecording() const;

    /// <summary>
    /// 获取当前播放位置
    /// </summary>
    /// <returns>当前播放位置（秒）</returns>
    double GetCurrentPosition() const;

    /// <summary>
    /// 获取总时长
    /// </summary>
    /// <returns>总时长（秒）</returns>
    double GetDuration() const;

    /// <summary>
    /// 获取当前媒体类型
    /// </summary>
    /// <returns>当前媒体类型</returns>
    EM_MediaType GetCurrentMediaType() const;

    /// <summary>
    /// 获取当前文件路径
    /// </summary>
    /// <returns>当前文件路径</returns>
    QString GetCurrentFilePath() const;

    /// <summary>
    /// 设置视频显示控件
    /// </summary>
    /// <param name="videoWidget">视频显示控件</param>
    void SetVideoDisplayWidget(PlayerVideoModuleWidget* videoWidget);

    /// <summary>
    /// 加载音频波形数据
    /// </summary>
    /// <param name="filePath">音频文件路径</param>
    /// <param name="waveformData">输出的波形数据</param>
    /// <returns>是否成功加载波形数据</returns>
    bool LoadAudioWaveform(const QString& filePath, QVector<float>& waveformData);

    /// <summary>
    /// 获取播放器性能统计信息
    /// </summary>
    /// <returns>性能统计信息</returns>
    QString GetPerformanceStats() const;

    /// <summary>
    /// 重置性能统计
    /// </summary>
    void ResetPerformanceStats();

signals:
    /// <summary>
    /// 播放状态改变信号
    /// </summary>
    /// <param name="isPlaying">是否正在播放</param>
    void SigPlayStateChanged(bool isPlaying);

    /// <summary>
    /// 录制状态改变信号
    /// </summary>
    /// <param name="isRecording">是否正在录制</param>
    void SigRecordStateChanged(bool isRecording);

    /// <summary>
    /// 播放进度改变信号
    /// </summary>
    /// <param name="position">当前位置（秒）</param>
    /// <param name="duration">总时长（秒）</param>
    void SigProgressChanged(qint64 position, qint64 duration);

    /// <summary>
    /// 视频帧更新信号
    /// </summary>
    /// <param name="frameData">帧数据</param>
    /// <param name="width">帧宽度</param>
    /// <param name="height">帧高度</param>
    void SigFrameUpdated(const uint8_t* frameData, int width, int height);

    /// <summary>
    /// 错误信号
    /// </summary>
    /// <param name="errorMsg">错误信息</param>
    void SigError(const QString& errorMsg);

private:
    /// <summary>
    /// 私有构造函数（单例模式）
    /// </summary>
    /// <param name="parent">父对象</param>
    explicit MediaPlayerManager(QObject* parent = nullptr);

    /// <summary>
    /// 禁用拷贝构造函数
    /// </summary>
    MediaPlayerManager(const MediaPlayerManager&) = delete;

    /// <summary>
    /// 禁用赋值操作符
    /// </summary>
    MediaPlayerManager& operator=(const MediaPlayerManager&) = delete;

    /// <summary>
    /// 检测媒体文件类型
    /// </summary>
    /// <param name="filePath">文件路径</param>
    /// <returns>媒体类型</returns>
    EM_MediaType DetectMediaType(const QString& filePath);

    /// <summary>
    /// 检测视频文件是否包含音频轨道
    /// </summary>
    /// <param name="filePath">视频文件路径</param>
    /// <returns>视频媒体类型（Video或VideoWithAudio）</returns>
    EM_MediaType DetectVideoWithAudio(const QString& filePath);

    /// <summary>
    /// 停止当前播放器
    /// </summary>
    void StopCurrentPlayer();

    /// <summary>
    /// 连接信号槽
    /// </summary>
    void ConnectPlayerSignals();

private:
    /// <summary>
    /// 单例实例
    /// </summary>
    static MediaPlayerManager* s_instance;

    /// <summary>
    /// 单例锁
    /// </summary>
    static std::mutex s_mutex;

    /// <summary>
    /// 音频播放器
    /// </summary>
    std::unique_ptr<AudioFFmpegPlayer> m_audioPlayer{nullptr};

    /// <summary>
    /// 视频播放器
    /// </summary>
    std::unique_ptr<VideoFFmpegPlayer> m_videoPlayer{nullptr};

    /// <summary>
    /// 当前活动的媒体类型
    /// </summary>
    EM_MediaType m_currentMediaType{EM_MediaType::Unknown};

    /// <summary>
    /// 当前文件路径
    /// </summary>
    QString m_currentFilePath;

    /// <summary>
    /// 播放次数统计
    /// </summary>
    std::atomic<size_t> m_playCount{0};

    /// <summary>
    /// 错误次数统计
    /// </summary>
    std::atomic<size_t> m_errorCount{0};

    /// <summary>
    /// 总播放时长（秒）
    /// </summary>
    std::atomic<double> m_totalPlayTime{0.0};

    /// <summary>
    /// 上次播放开始时间
    /// </summary>
    std::chrono::steady_clock::time_point m_lastPlayStartTime;

    /// <summary>
    /// 是否正在进行音视频同步播放
    /// </summary>
    std::atomic<bool> m_isSyncPlaying{false};
}; 