#include "PlayerAudioModuleWidget.h"

#include "SDKCommonDefine/SDKCommonDefine.h"


PlayerAudioModuleWidget::PlayerAudioModuleWidget(QWidget* parent)
    : BaseModuleWidegt(parent), ui(new Ui::PlayerAudioModuleWidgetClass())
{
    ui->setupUi(this);
    m_waveformWidget = new AudioWaveformWidget;
    ui->verticalLayout->addWidget(m_waveformWidget);
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
    }
}
