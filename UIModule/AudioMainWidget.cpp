#include "AudioMainWidget.h"
#include "PlayerAudioModuleWidget.h"
AudioMainWidget::AudioMainWidget(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::AudioMainWidgetClass())
{
    ui->setupUi(this);
}

AudioMainWidget::~AudioMainWidget()
{
    delete ui;
}
