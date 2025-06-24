#include "PlayerAudioModuleWidget.h"

#include "SDKCommonDefine/SDKCommonDefine.h"


PlayerAudioModuleWidget::PlayerAudioModuleWidget(QWidget* parent)
    : BaseModuleWidegt(parent), ui(new Ui::PlayerAudioModuleWidgetClass())
{
    ui->setupUi(this);

    // 创建波形控件
    m_waveformWidget = new AudioWaveformWidget(this);
    ui->verticalLayout->addWidget(m_waveformWidget);

    // 连接信号槽
    ConnectSignals();

    // 初始化播放进度定时器
    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(100); // 100ms更新一次
    connect(m_progressTimer, &QTimer::timeout, this, &PlayerAudioModuleWidget::SlotUpdateProgress);
}


PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    SAFE_DELETE_POINTER_VALUE(ui)
}

BaseFFmpegUtils* PlayerAudioModuleWidget::GetFFMpegUtils()
{
    return &m_audioFFmpeg;
}

void PlayerAudioModuleWidget::LoadWaveWidegt(const QString& inputFilePath)
{
    // 在播放前加载音频数据并生成波形
    QVector<float> waveformData;
    if (m_audioFFmpeg.LoadAudioWaveform(inputFilePath, waveformData))
    {
        m_waveformWidget->SetWaveformData(waveformData);
        m_currentFilePath = inputFilePath;
    }
}

void PlayerAudioModuleWidget::StartPlay(const QString& filePath, double startPosition)
{
    if (filePath != m_currentFilePath)
    {
        LoadWaveWidegt(filePath);
    }

    m_audioFFmpeg.StartPlay(filePath, startPosition);
    m_progressTimer->start();
}

void PlayerAudioModuleWidget::PausePlay()
{
    m_audioFFmpeg.PausePlay();
    m_progressTimer->stop();
}

void PlayerAudioModuleWidget::ResumePlay()
{
    m_audioFFmpeg.ResumePlay();
    m_progressTimer->start();
}

void PlayerAudioModuleWidget::StopPlay()
{
    m_audioFFmpeg.StopPlay();
    m_progressTimer->stop();
    m_waveformWidget->SetPlaybackPosition(0.0);
}

void PlayerAudioModuleWidget::SeekTo(double position)
{
    double duration = m_audioFFmpeg.GetDuration();
    if (duration > 0)
    {
        double targetTime = position * duration;
        m_audioFFmpeg.StartPlay(m_currentFilePath, targetTime);
        m_waveformWidget->SetPlaybackPosition(position);
    }
}

void PlayerAudioModuleWidget::ConnectSignals()
{
    // 连接音频播放器信号
    connect(&m_audioFFmpeg, &AudioFFmpegUtils::SigPlayStateChanged, this, &PlayerAudioModuleWidget::SigPlayStateChanged);

    connect(&m_audioFFmpeg, &AudioFFmpegUtils::SigProgressChanged, this, &PlayerAudioModuleWidget::SlotProgressChanged);

    // 连接波形控件信号
    connect(m_waveformWidget, &AudioWaveformWidget::SigSeekPosition, this, &PlayerAudioModuleWidget::SeekTo);
}

void PlayerAudioModuleWidget::SlotProgressChanged(qint64 position, qint64 duration)
{
    m_currentPosition = position;
    m_totalDuration = duration;

    if (duration > 0)
    {
        double progress = static_cast<double>(position) / duration;
        m_waveformWidget->SetPlaybackPosition(progress);
    }

    emit SigProgressChanged(position, duration);
}

void PlayerAudioModuleWidget::SlotUpdateProgress()
{
    if (m_audioFFmpeg.IsPlaying() && !m_audioFFmpeg.IsPaused())
    {
        // 定期更新播放进度
        double currentPos = m_audioFFmpeg.GetCurrentPosition();
        double duration = m_audioFFmpeg.GetDuration();

        if (duration > 0)
        {
            double progress = currentPos / duration;
            m_waveformWidget->SetPlaybackPosition(progress);
            emit SigProgressChanged(static_cast<qint64>(currentPos), static_cast<qint64>(duration));
        }
    }
}
