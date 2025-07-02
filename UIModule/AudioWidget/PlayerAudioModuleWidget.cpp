#include "PlayerAudioModuleWidget.h"

#include <QThread>
#include "CoreServerGlobal.h"
#include "SDKCommonDefine/SDKCommonDefine.h"


PlayerAudioModuleWidget::PlayerAudioModuleWidget(QWidget* parent)
    : BaseModuleWidget(parent), ui(new Ui::PlayerAudioModuleWidgetClass())
{
    ui->setupUi(this);

    // 创建波形控件
    m_waveformWidget = new AudioWaveformWidget(this);
    ui->verticalLayout->addWidget(m_waveformWidget);

    // 连接信号槽
    ConnectSignals();
}


PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    SAFE_DELETE_POINTER_VALUE(ui);
}

BaseFFmpegUtils* PlayerAudioModuleWidget::GetFFMpegUtils()
{
    return &m_audioFFmpeg;
}

void PlayerAudioModuleWidget::LoadWaveWidegt(const QString& inputFilePath)
{
    m_waveformData.clear();
    CoreServerGlobal::Instance().GetThreadPool().Submit([&]()
    {
        QVector<float> waveData;
        // 在新线程中加载音频波形数据
        if (m_audioFFmpeg.LoadAudioWaveform(inputFilePath, waveData))
        {
            m_currentFilePath = inputFilePath;
            m_waveformData.swap(waveData);
            emit SigThreadExit();
        }
    });
    // 在播放前加载音频数据并生成波形
    connect(this, &PlayerAudioModuleWidget::SigThreadExit, [&]()
    {
        m_waveformWidget->SetWaveformData(m_waveformData);
    });
}

void PlayerAudioModuleWidget::UpdateWaveformPosition(double currentPos, double duration)
{
    if (m_waveformWidget && duration > 0.0)
    {
        double progress = currentPos / duration;
        // 确保进度值在有效范围内
        progress = std::max(0.0, std::min(1.0, progress));
        m_waveformWidget->SetPlaybackPosition(progress);
    }
}

void PlayerAudioModuleWidget::ConnectSignals()
{
    // 连接音频播放器信号
    connect(&m_audioFFmpeg, &AudioFFmpegUtils::SigPlayStateChanged, this, &PlayerAudioModuleWidget::SigPlayStateChanged);

    connect(&m_audioFFmpeg, &AudioFFmpegUtils::SigProgressChanged, this, &PlayerAudioModuleWidget::SlotProgressChanged);
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
