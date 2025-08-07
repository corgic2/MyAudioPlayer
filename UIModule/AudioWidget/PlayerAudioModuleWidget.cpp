#include "PlayerAudioModuleWidget.h"

#include <QThread>
#include "CoreServerGlobal.h"
#include "AudioPlayer/AudioPlayerUtils.h"
#include "BasePlayer/MediaPlayerManager.h"
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

void PlayerAudioModuleWidget::LoadWaveWidegt(const QString& inputFilePath)
{
    m_waveformData.clear();
    m_threadID = CoreServerGlobal::Instance().GetThreadPool().CreateDedicatedThread("LoadWaveWidget",[=]()
    {
        QVector<float> waveData;
        if (AudioPlayerUtils::LoadAudioWaveform(inputFilePath, waveData))
        {
            m_currentFilePath = inputFilePath;
            m_waveformData.swap(waveData);
            emit SigThreadExit();
        }
    });
    // 在播放前加载音频数据并生成波形
    connect(this, &PlayerAudioModuleWidget::SigThreadExit, this, [&]()
    {
        m_waveformWidget->SetWaveformData(m_waveformData);
        CoreServerGlobal::Instance().GetThreadPool().StopDedicatedThread(m_threadID);
    }, Qt::QueuedConnection);
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
    // 现在不再直接连接AudioFFmpegPlayer信号
    // 播放状态和进度信息将通过AVBaseWidget传递
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
